#include <iostream>
#include <tuple>
#include <utility>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/threadsafe_list.hpp>
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
        // it is inefficient to test it at each call but we just want to show how it's done
        // we left checks for interrput only every few calls as exercise to the reader...
        if (n%10==0)
        {
            boost::this_thread::interruption_point();
        }
        return serial_fib(n-1)+serial_fib(n-2);
    }
}

struct fib_task : public boost::asynchronous::continuation_task<long>
{
    fib_task(long n,long cutoff):n_(n),cutoff_(cutoff)
    {

    }
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
            boost::asynchronous::create_continuation(
                        [task_res](std::tuple<boost::future<long>,boost::future<long> > res)
                        {
                            BOOST_CHECK_MESSAGE(std::get<0>(res).has_exception(),"Fib subtask 99 got no exception");
                            BOOST_CHECK_MESSAGE(std::get<1>(res).has_exception(),"Fib subtask 98 got no exception");
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

struct Servant : boost::asynchronous::trackable_servant<>
{
    // optional, ctor is simple enough not to be posted
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               // threadpool with 4 threads and a simple threadsafe_list queue
                                               boost::asynchronous::create_shared_scheduler_proxy(
                                                   new boost::asynchronous::multiqueue_threadpool_scheduler<
                                                           boost::asynchronous::threadsafe_list<> >(4)))
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
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        tpids = tp.thread_ids();
        // start long tasks in threadpool (first lambda) and callback in our thread
        boost::asynchronous::any_interruptible interruptible =
        interruptible_post_callback(
                [n,cutoff]()
                {
                    BOOST_CHECK_MESSAGE(contains_id(tpids.begin(),tpids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::top_level_continuation<long>(fib_task(n,cutoff));
                 }// work
               ,
               // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
               [this](boost::future<long> res){
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
               }// callback functor.
        );
        return std::make_pair(fu,interruptible);
    }
private:
// for testing
boost::shared_ptr<boost::promise<long> > m_promise;
};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    // caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER(calc_fibonacci)
};
}

BOOST_AUTO_TEST_CASE( test_fibonacci_100_100_interrupt )
{
    long fibo_val = 100;
    long cutoff = 100;
    main_thread_id = boost::this_thread::get_id();

    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::threadsafe_list<> >);
        {
            ServantProxy proxy(scheduler);
            boost::shared_future<std::pair<boost::shared_future<long>, boost::asynchronous::any_interruptible>  > fu = proxy.calc_fibonacci(fibo_val,cutoff);
            std::pair<boost::shared_future<long>, boost::asynchronous::any_interruptible>  resfu = fu.get();
            // ok we decide it takes too long, interrupt
            boost::this_thread::sleep(boost::posix_time::milliseconds(30));
            resfu.second.interrupt();
            boost::this_thread::sleep(boost::posix_time::milliseconds(30));
            BOOST_CHECK_MESSAGE(!resfu.first.has_value(),"fibonacci should not be ready");
        }
    }
}


