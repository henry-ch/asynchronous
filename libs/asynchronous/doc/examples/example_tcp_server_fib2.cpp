#include <iostream>


#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/queue/guarded_deque.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/tcp/tcp_server_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/any_serializable.hpp>
#include <boost/asynchronous/scheduler/composite_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/tcp/simple_tcp_client.hpp>
#include <boost/asynchronous/extensions/asio/asio_scheduler.hpp>

#include "dummy_tcp_task.hpp"
#include "serializable_fib_task.hpp"

using namespace std;

namespace
{
typename std::chrono::high_resolution_clock::time_point start;
typename std::chrono::high_resolution_clock::time_point stop;
struct Servant : boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,boost::asynchronous::any_serializable>
{
    // optional, ctor is simple enough not to be posted
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler,
            boost::asynchronous::any_shared_scheduler_proxy<boost::asynchronous::any_serializable> worker)
        : boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,boost::asynchronous::any_serializable>
          (scheduler,worker)
        // for testing purpose
        , m_promise(new boost::promise<long>)
    {
    }
    // called when task done, in our thread
    void on_callback(long res)
    {
        stop = std::chrono::high_resolution_clock::now();
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
std::shared_ptr<boost::promise<long> > m_promise;
};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s, boost::asynchronous::any_shared_scheduler_proxy<boost::asynchronous::any_serializable> worker):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s,worker)
    {}
    // caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER(calc_fibonacci)
};

}

void example_post_tcp_fib2(std::string const& server_address,std::string const& server_port,
                           std::string const& own_server_address, long own_server_port, int threads, long fibo_val,long cutoff)
{
//    std::cout << "fibonacci single-threaded" << std::endl;
//    start = std::chrono::high_resolution_clock::now();
//    long sres = tcp_example::serial_fib(fibo_val);
//    stop = std::chrono::high_resolution_clock::now();
//    std::cout << "sres= " << sres << std::endl;
//    std::cout << "fibonacci single-threaded single took in us:"
//              <<  (std::chrono::nanoseconds(stop - start).count() / 1000) <<"\n" <<std::endl;
    {
        std::cout << "fibonacci parallel TCP 2" << std::endl;
        // create pools
        // we need a pool where the tasks execute
        auto pool = boost::asynchronous::make_shared_scheduler_proxy<
                    boost::asynchronous::threadpool_scheduler<
                            boost::asynchronous::guarded_deque<boost::asynchronous::any_serializable>>>(threads);
        // a client will steal jobs in this pool
        auto cscheduler = boost::asynchronous::make_shared_scheduler_proxy<
                            boost::asynchronous::asio_scheduler<
                                boost::asynchronous::default_find_position<boost::asynchronous::sequential_push_policy >>>();
        // jobs we will support
        std::function<void(std::string const&,boost::asynchronous::tcp::server_reponse,
                           std::function<void(boost::asynchronous::tcp::client_request const&)>)> executor=
        [](std::string const& task_name,boost::asynchronous::tcp::server_reponse resp,
           std::function<void(boost::asynchronous::tcp::client_request const&)> when_done)
        {
            std::cout << "got task: " << task_name
                      << " task: " << resp.m_task
                      << " m_task_id: " << resp.m_task_id
                      << std::endl;
            if (task_name=="serializable_sub_fib_task")
            {
                tcp_example::fib_task fib(0,0);
                boost::asynchronous::tcp::deserialize_and_call_callback_continuation_task(fib,resp,when_done);
            }
            else if (task_name=="serializable_fib_task")
            {
                tcp_example::serializable_fib_task fib(0,0);
                boost::asynchronous::tcp::deserialize_and_call_top_level_callback_continuation_task(fib,resp,when_done);
            }
            // else whatever functor we support
            else
            {
                std::cout << "unknown task! Sorry, don't know: " << task_name << std::endl;
                throw boost::asynchronous::tcp::transport_exception("unknown task");
            }
        };
// g++ in uncooperative, clang no
#if defined(__clang__)
        boost::asynchronous::tcp::simple_tcp_client_proxy_ext<boost::asynchronous::tcp::queue_size_check_policy<>>
                    client_proxy(
                        cscheduler,pool,server_address,server_port,executor,
                        0/*ms between calls to server*/,
                        0 /* number of jobs we try to keep in queue */);
#else
        typename boost::asynchronous::tcp::get_correct_simple_tcp_client_proxy<boost::asynchronous::tcp::queue_size_check_policy<>>::type
                   client_proxy(
                       cscheduler,pool,server_address,server_port,executor,
                       0/*ms between calls to server*/,
                       0 /* number of jobs we try to keep in queue */);
#endif

        // we need a server
        // we use a tcp pool using 1 worker
        auto server_pool = boost::asynchronous::make_shared_scheduler_proxy<
                    boost::asynchronous::threadpool_scheduler<
                            boost::asynchronous::lockfree_queue<>>>(1);
        auto tcp_server= boost::asynchronous::make_shared_scheduler_proxy<
                    boost::asynchronous::tcp_server_scheduler<
                            boost::asynchronous::lockfree_queue<boost::asynchronous::any_serializable>,
                            boost::asynchronous::any_callable,true>>
                                (server_pool,own_server_address,(unsigned int)own_server_port);
        // we need a composite for stealing
        auto composite = boost::asynchronous::make_shared_scheduler_proxy<
                boost::asynchronous::composite_threadpool_scheduler<boost::asynchronous::any_serializable>>
                          (pool,tcp_server);

        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                                boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::lockfree_queue<>>>();
        {
            ServantProxy proxy(scheduler,pool);
            start = std::chrono::high_resolution_clock::now();
            // result of BOOST_ASYNC_FUTURE_MEMBER is a shared_future,
            // so we have a shared_future of a shared_future(result of start_async_work)
            boost::shared_future<boost::shared_future<long> > fu = proxy.calc_fibonacci(fibo_val,cutoff);
            boost::shared_future<long> resfu = fu.get();
            long res = resfu.get();
            std::cout << "res= " << res << std::endl;
            std::cout << "fibonacci parallel TCP 2 took in us:"
                      <<  (std::chrono::nanoseconds(stop - start).count() / 1000) <<"\n" <<std::endl;
        }
    }
    std::cout << "end example_post_tcp_fib2 \n" << std::endl;
}


