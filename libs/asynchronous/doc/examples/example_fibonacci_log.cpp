#include <iostream>
#include <tuple>
#include <utility>

#include <boost/lexical_cast.hpp>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/threadsafe_list.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/detail/any_continuation.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/continuation_task.hpp>

namespace
{
typedef boost::asynchronous::any_loggable<boost::chrono::high_resolution_clock> servant_job;
typedef std::map<std::string,std::list<boost::asynchronous::diagnostic_item<boost::chrono::high_resolution_clock> > > diag_type;

long serial_fib( long n ) {
    if( n<2 )
        return n;
    else
        return serial_fib(n-1)+serial_fib(n-2);
}

struct fib_task : public boost::asynchronous::continuation_task<long>
{
    // give each task a name for logging
    fib_task(long n,long cutoff)
        : boost::asynchronous::continuation_task<long>("fib_task_" + boost::lexical_cast<std::string>(n))
        ,n_(n),cutoff_(cutoff){}
    void operator()()const
    {
        boost::asynchronous::continuation_result<long> task_res = this_task_result();
        if (n_<cutoff_)
        {
            task_res.set_value(serial_fib(n_));
        }
        else
        {
           boost::asynchronous::create_continuation_job<servant_job>(
                        [task_res](std::tuple<boost::future<long>,boost::future<long> > res)
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
    Servant(boost::asynchronous::any_weak_scheduler<servant_job> scheduler, int threads)
        : boost::asynchronous::trackable_servant<servant_job,servant_job>(scheduler,
                                               // threadpool and a simple threadsafe_list queue
                                               boost::asynchronous::create_shared_scheduler_proxy(
                                                   new boost::asynchronous::multiqueue_threadpool_scheduler<
                                                           boost::asynchronous::threadsafe_list<servant_job> >(threads)))
        // for testing purpose
        , m_promise(new boost::promise<long>)
    {
    }
    // called when task done, in our thread
    void on_callback(long res)
    {
        //std::cout << "Callback in single-thread scheduler with value:" << res << std::endl;
        // inform test caller
        m_promise->set_value(res);
    }
    // call to this is posted and executes in our (safe) single-thread scheduler
    boost::shared_future<long> calc_fibonacci(long n,long cutoff)
    {
        // for testing purpose
        boost::shared_future<long> fu = m_promise->get_future();
        // start long tasks in threadpool (first lambda) and callback in our thread
        post_callback(
                [n,cutoff]()
                {
                     return boost::asynchronous::top_level_continuation_log<long,servant_job>(fib_task(n,cutoff));
                 }// work
               ,
               // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
               [this](boost::asynchronous::expected<long> res){
                            this->on_callback(res.get());
               },// callback functor.
               "calc_fibonacci"
        );
        return fu;
    }
    // threadpool diagnostics
    diag_type get_diagnostics() const
    {
        return get_worker().get_diagnostics();
    }
private:
// for testing
boost::shared_ptr<boost::promise<long> > m_promise;
};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant,servant_job>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s, int threads):
        boost::asynchronous::servant_proxy<ServantProxy,Servant,servant_job>(s,threads)
    {}
    // caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER_LOG(calc_fibonacci,"proxy::calc_fibonacci")
    BOOST_ASYNC_FUTURE_MEMBER_LOG(get_diagnostics,"proxy::get_diagnostics")
};
}

void example_fibonacci_log(long fibo_val,long cutoff, int threads)
{
    std::cout << "example_fibonacci_log single" << std::endl;
    typename boost::chrono::high_resolution_clock::time_point start;
    typename boost::chrono::high_resolution_clock::time_point stop;
    start = boost::chrono::high_resolution_clock::now();
    long sres = serial_fib(fibo_val);
    stop = boost::chrono::high_resolution_clock::now();
    std::cout << "sres= " << sres << std::endl;
    std::cout << "example_fibonacci_log single took in us:"
              <<  (boost::chrono::nanoseconds(stop - start).count() / 1000) <<"\n" <<std::endl;

    std::cout << "example_fibonacci_log parallel" << std::endl;
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::threadsafe_list<servant_job> >);
        {
            ServantProxy proxy(scheduler, threads);
            start = boost::chrono::high_resolution_clock::now();
            boost::shared_future<boost::shared_future<long> > fu = proxy.calc_fibonacci(fibo_val,cutoff);
            boost::shared_future<long> resfu = fu.get();
            long res = resfu.get();
            stop = boost::chrono::high_resolution_clock::now();
            std::cout << "res= " << res << std::endl;
            std::cout << "example_fibonacci_log parallel took in us:"
                      <<  (boost::chrono::nanoseconds(stop - start).count() / 1000) <<"\n" <<std::endl;

            // now display run time of tasks
            boost::shared_future<diag_type> fu_diag = proxy.get_diagnostics();
            diag_type diag = fu_diag.get();
            std::cout << "Display of worker jobs" << std::endl;
            for (auto mit = diag.begin(); mit != diag.end() ; ++mit)
            {
                std::cout << "job type: " << (*mit).first << std::endl;
                for (auto jit = (*mit).second.begin(); jit != (*mit).second.end();++jit)
                {
                    std::cout << "job waited in us: " << boost::chrono::nanoseconds((*jit).get_started_time() - (*jit).get_posted_time()).count() / 1000 << std::endl;
                    std::cout << "job lasted in us: " << boost::chrono::nanoseconds((*jit).get_finished_time() - (*jit).get_started_time()).count() / 1000 << std::endl;
                    std::cout << "job interrupted? "  << std::boolalpha << (*jit).is_interrupted() << std::endl;
                }
            }
        }
    }
    std::cout << "end example_fibonacci_log \n" << std::endl;
}

