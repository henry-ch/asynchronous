#include <iostream>


#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/tcp/tcp_server_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/any_serializable.hpp>
#include "dummy_tcp_task.hpp"
#include "serializable_fib_task.hpp"

using namespace std;

namespace
{
struct Servant : boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,boost::asynchronous::any_serializable>
{
    // optional, ctor is simple enough not to be posted
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,boost::asynchronous::any_serializable>(scheduler)
        // for testing purpose
        , m_promise(new std::promise<long>)
    {
        // let's build our pool step by step. First we need a worker pool
        // possibly for us, and we want to share it with the tcp pool for its serialization work
        boost::asynchronous::any_shared_scheduler_proxy<> workers = boost::asynchronous::make_shared_scheduler_proxy<
            boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(3);
        // we use a tcp pool using the 3 worker threads we just built
        auto pool= boost::asynchronous::make_shared_scheduler_proxy<
                    boost::asynchronous::tcp_server_scheduler<
                            boost::asynchronous::lockfree_queue<boost::asynchronous::any_serializable>>>
                                (workers,"localhost",12345);
        // and this will be the worker pool for post_callback
        set_worker(pool);

    }
    // called when task done, in our thread
    void on_callback(long res)
    {
        // inform test caller
        m_promise->set_value(res);
    }
    // call to this is posted and executes in our (safe) single-thread scheduler
    std::future<long> calc_fibonacci(long n,long cutoff)
    {
        // for testing purpose
        auto fu = m_promise->get_future();
        // start long tasks in threadpool (first lambda) and callback in our thread
        post_callback(
                    tcp_example::serializable_fib_task(n,cutoff)
               ,
               // callback with fibonacci result.
               [this](boost::asynchronous::expected<long> res){
                            this->on_callback(res.get());
               }// callback functor.
        );
        return fu;
    }
private:
// for testing
std::shared_ptr<std::promise<long> > m_promise;
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

void example_post_tcp_fib(long fibo_val,long cutoff)
{
    std::cout << "fibonacci single-threaded" << std::endl;
    typename std::chrono::high_resolution_clock::time_point start;
    typename std::chrono::high_resolution_clock::time_point stop;
    start = std::chrono::high_resolution_clock::now();
    long sres = tcp_example::serial_fib(fibo_val);
    stop = std::chrono::high_resolution_clock::now();
    std::cout << "sres= " << sres << std::endl;
    std::cout << "fibonacci single-threaded single took in us:"
              <<  (std::chrono::nanoseconds(stop - start).count() / 1000) <<"\n" <<std::endl;
    {
        std::cout << "fibonacci parallel TCP" << std::endl;
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                                boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::lockfree_queue<>>>();
        {
            ServantProxy proxy(scheduler);
            start = std::chrono::high_resolution_clock::now();
            auto fu = proxy.calc_fibonacci(fibo_val,cutoff);
            auto resfu = fu.get();
            long res = resfu.get();
            stop = std::chrono::high_resolution_clock::now();
            std::cout << "res= " << res << std::endl;
            std::cout << "fibonacci parallel TCP took in us:"
                      <<  (std::chrono::nanoseconds(stop - start).count() / 1000) <<"\n" <<std::endl;
        }
    }
    std::cout << "end example_post_tcp_fib \n" << std::endl;
}




