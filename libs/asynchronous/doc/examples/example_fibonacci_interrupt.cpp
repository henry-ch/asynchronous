#include <iostream>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/threadsafe_list.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/continuation_task.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>

using namespace std;

namespace
{
// a simnple, single-threaded fibonacci function used for cutoff
long serial_fib( long n ) {

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

// our recursive fibonacci tasks. Needs to inherit continuation_task<value type returned by this task>
struct fib_task : public boost::asynchronous::continuation_task<long>
{
    fib_task(long n,long cutoff):n_(n),cutoff_(cutoff){}
    void operator()()const
    {
        // the result of this task, will be either set directly if < cutoff, otherwise when taks is ready
        boost::asynchronous::continuation_result<long> task_res = this_task_result();
        if (n_<cutoff_)
        {
            // n < cutoff => execute ourselves
            task_res.set_value(serial_fib(n_));
        }
        else
        {
            // n> cutoff, create 2 new tasks and when both are done, set our result (res(task1) + res(task2))
            boost::asynchronous::create_continuation(
                        // called when subtasks are done, set our result
                        [task_res](std::tuple<boost::future<long>,boost::future<long> > res)
                        {
                            if (!std::get<0>(res).has_value() || !std::get<1>(res).has_value())
                            {
                                // oh we got interrupted and have no value, give up
                                return;
                            }
                            long r = std::get<0>(res).get() + std::get<1>(res).get();
                            task_res.set_value(r);
                        },
                        // recursive tasks
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
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler, int threads)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               // threadpool and a simple threadsafe_list queue
                                               boost::asynchronous::create_shared_scheduler_proxy(
                                                   new boost::asynchronous::multiqueue_threadpool_scheduler<
                                                           boost::asynchronous::threadsafe_list<> >(threads)))
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
    std::pair<boost::shared_future<long>, boost::asynchronous::any_interruptible> calc_fibonacci(long n,long cutoff)
    {
        // for testing purpose
        boost::shared_future<long> fu = m_promise->get_future();
        // start long tasks in threadpool (first lambda) and callback in our thread
        boost::asynchronous::any_interruptible interruptible =
        interruptible_post_callback(
                [n,cutoff]()
                {
                     // a top-level continuation is the first one in a recursive serie.
                     // Its result will be passed to callback
                     return boost::asynchronous::top_level_continuation<long>(fib_task(n,cutoff));
                 }// work
               ,
               // callback with fibonacci result.
               [this](boost::asynchronous::expected<long> res){
                            std::cout << "called CB of interruptible_post_callback" << std::endl;
                            std::cout << "future has value: " << std::boolalpha << res.has_value() << std::endl; //false
                            std::cout << "future has exception: " << std::boolalpha << res.has_exception() << std::endl; //true, task_aborted_exception
                            if (res.has_value())
                            {
                                std::cout << "calling on_callback of interruptible_post_callback" << std::endl;
                                this->on_callback(res.get());
                            }
                            else
                            {
                                std::cout << "Oops no nalue, give up" << std::endl;
                            }
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
    ServantProxy(Scheduler s, int threads):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s,threads)
    {}
    // caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER(calc_fibonacci)
};

}

void example_fibonacci_interrupt(long fibo_val,long cutoff, int threads)
{
    std::cout << "example_fibonacci_interrupt" << std::endl;
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::threadsafe_list<> >);
        {
            ServantProxy proxy(scheduler,threads);
            boost::shared_future<std::pair<boost::shared_future<long>, boost::asynchronous::any_interruptible>  > fu = proxy.calc_fibonacci(fibo_val,cutoff);
            std::pair<boost::shared_future<long>, boost::asynchronous::any_interruptible>  resfu = fu.get();
            // ok we decide it takes too long, interrupt
            boost::this_thread::sleep(boost::posix_time::milliseconds(30));
            resfu.second.interrupt();
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
            // as we interrupt we might have no result
            bool has_value = resfu.first.has_value();
            std::cout << "do we have a value? " << std::boolalpha << has_value << std::endl;
            if (has_value)
            {
                std::cout << "value: " << resfu.first.get() << std::endl;
            }
        }
    }
    std::cout << "end example_fibonacci_interrupt \n" << std::endl;
}

