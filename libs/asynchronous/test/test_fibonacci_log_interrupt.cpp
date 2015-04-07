#include <iostream>
#include <tuple>
#include <utility>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/detail/any_continuation.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/continuation_task.hpp>

#include "test_common.hpp"
#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;
namespace
{
typedef boost::asynchronous::any_loggable<boost::chrono::high_resolution_clock> servant_job;
typedef std::map<std::string,std::list<boost::asynchronous::diagnostic_item<boost::chrono::high_resolution_clock> > > diag_type;

// main thread id
boost::thread::id main_thread_id;
std::vector<boost::thread::id> tpids;

long serial_fib( long n )
{
    if( n<2 )
        return n;
    else
    {
        // check if interrupted
        boost::this_thread::interruption_point();
        return serial_fib(n-1)+serial_fib(n-2);
    }
}

struct fib_task : public boost::asynchronous::continuation_task<long>
{
    // task name is its fibonacci number
    fib_task(long n,long cutoff)
        : boost::asynchronous::continuation_task<long>(std::to_string(n))
        , n_(n),cutoff_(cutoff){}


    void operator()()const
    {
        if (!contains_id(tpids.begin(),tpids.end(),boost::this_thread::get_id()))
        {
            BOOST_FAIL("fib_task runs in wrong thread");
        }
        boost::asynchronous::continuation_result<long> task_res = this_task_result();
        if (n_<cutoff_)
        {
            task_res.set_value(serial_fib(n_));
        }
        else
        {
            boost::asynchronous::create_callback_continuation_job<servant_job>(
                        [task_res](std::tuple<boost::asynchronous::expected<long>,boost::asynchronous::expected<long> > res)
                        {
                            long r = std::get<0>(res).get() + std::get<1>(res).get();
                            task_res.set_value(r);
                        },
                        fib_task(n_-1,cutoff_),
                        fib_task(n_-2,cutoff_));
        }
    }
    long n_;
    long cutoff_;
};

struct Servant : boost::asynchronous::trackable_servant<servant_job,servant_job>
{
    // optional, ctor is simple enough not to be posted
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<servant_job> scheduler)
        : boost::asynchronous::trackable_servant<servant_job,servant_job>(scheduler,
                                               // threadpool with 4 threads and a simple lockfree_queue queue
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::multiqueue_threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<servant_job>>>(4))
        // for testing purpose
        , m_promise(new boost::promise<long>)
    {
    }
    // call to this is posted and executes in our (safe) single-thread scheduler
    std::pair<boost::shared_future<long>, boost::asynchronous::any_interruptible> calc_fibonacci(long n,long cutoff)
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant calc_fibonacci not posted.");
        // for testing purpose
        boost::shared_future<long> fu = m_promise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<servant_job> tp =get_worker();
        tpids = tp.thread_ids();
        // start long tasks in threadpool (first lambda) and callback in our thread
        boost::asynchronous::any_interruptible interruptible =
        interruptible_post_callback(
                [n,cutoff]()
                {
                    BOOST_CHECK_MESSAGE(contains_id(tpids.begin(),tpids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::top_level_callback_continuation_job<long,servant_job>(fib_task(n,cutoff));
                 }// work
               ,
               // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
               [this](boost::asynchronous::expected<long> res){
                            BOOST_CHECK_MESSAGE(!contains_id(tpids.begin(),tpids.end(),boost::this_thread::get_id()),"fibonacci callback executed in the wrong thread(pool)");
                            BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                            BOOST_CHECK_MESSAGE(!res.has_value(),"callback should not get a value.");
                            BOOST_CHECK_MESSAGE(res.has_exception(),"callback should get an exception.");
                            bool has_abort_exception=false;
                            try{res.get();}
                            catch(boost::asynchronous::task_aborted_exception& )
                            {
                                has_abort_exception= true;
                            }
                            BOOST_CHECK_MESSAGE(has_abort_exception,"callback got a wrong exception.");
               },// callback functor.
               "calc_fibonacci"
        );
        return std::make_pair(fu,interruptible);
    }
    // threadpool diagnostics
    diag_type get_diagnostics() const
    {
        return get_worker().get_diagnostics().totals();
    }
private:
// for testing
boost::shared_ptr<boost::promise<long> > m_promise;
};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant,servant_job>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant,servant_job>(s)
    {}
    // caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER_LOG(calc_fibonacci,"proxy::calc_fibonacci")
    BOOST_ASYNC_FUTURE_MEMBER_LOG(get_diagnostics,"proxy::get_diagnostics")
};
}

BOOST_AUTO_TEST_CASE( test_fibonacci_100_100_log_interrupt )
{
    long fibo_val = 100;
    long cutoff = 100;
    main_thread_id = boost::this_thread::get_id();

    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                                boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::lockfree_queue<servant_job>>>();

        {
            ServantProxy proxy(scheduler);
            boost::shared_future<std::pair<boost::shared_future<long>, boost::asynchronous::any_interruptible>  > fu = proxy.calc_fibonacci(fibo_val,cutoff);
            std::pair<boost::shared_future<long>, boost::asynchronous::any_interruptible>  resfu = fu.get();
            // ok we decide it takes too long, interrupt
            boost::this_thread::sleep(boost::posix_time::milliseconds(100));
            resfu.second.interrupt();
            boost::this_thread::sleep(boost::posix_time::milliseconds(300));
            BOOST_CHECK_MESSAGE(!resfu.first.has_value(),"fibonacci should not be ready");
            //check if we found our 2 interrupted fibonacci tasks
            boost::shared_future<diag_type> fu_diag = proxy.get_diagnostics();
            diag_type diag = fu_diag.get();
            bool fib_99_called=false;
            bool fib_98_called=false;
            for (auto mit = diag.begin(); mit != diag.end() ; ++mit)
            {
                if ((*mit).first == "calc_fibonacci")
                {
                    continue;
                }

                if ((*mit).first == "99")
                {
                    fib_99_called = true;
                    for (auto jit = (*mit).second.begin(); jit != (*mit).second.end();++jit)
                    {
                        //TODO seems to have a bug in is_interrupted
//                        BOOST_CHECK_MESSAGE(boost::chrono::nanoseconds((*jit).get_finished_time() - (*jit).get_started_time()).count() / 1000 >= 100000,
//                                            "fibonacci 99 task should have been interrupted.");
                        //BOOST_CHECK_MESSAGE((*jit).is_interrupted(),"fibonacci 99 task should have been interrupted.");
                    }
                }

                if ((*mit).first == "98")
                {
                    fib_98_called = true;
                    for (auto jit = (*mit).second.begin(); jit != (*mit).second.end();++jit)
                    {
                        //TODO seems to have a bug in is_interrupted
//                        BOOST_CHECK_MESSAGE(boost::chrono::nanoseconds((*jit).get_finished_time() - (*jit).get_started_time()).count() / 1000 >= 100000,
//                                            "fibonacci 98 task should have been interrupted.");
                        //BOOST_CHECK_MESSAGE((*jit).is_interrupted(),"fibonacci 98 task should have been interrupted.");
                    }
                }
            }
            BOOST_CHECK_MESSAGE(fib_98_called,"fibonacci 98 task should have been called.");
            BOOST_CHECK_MESSAGE(fib_99_called,"fibonacci 99 task should have been called.");
        }
    }
}



