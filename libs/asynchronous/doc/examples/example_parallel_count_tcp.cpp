#include <iostream>


#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/tcp/tcp_server_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/any_serializable.hpp>
#include <boost/asynchronous/scheduler/composite_threadpool_scheduler.hpp>

#include "dummy_parallel_count_task.hpp"

using namespace std;

namespace
{

// std::vector until lazy_range has a proper location
std::vector<int> mkdata() {
    std::vector<int> result;
    for (int i = 0; i < 1000; ++i) {
        result.push_back(i);
    }
    return result;
}

struct Servant : boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,boost::asynchronous::any_serializable>
{
    // optional, ctor is simple enough not to be posted
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,boost::asynchronous::any_serializable>(scheduler)
        // for testing purpose
        , m_promise(new std::promise<void>)
    {
        // let's build our worker pool step by step.
        // we need a pool to execute jobs ourselves
        auto pool = boost::asynchronous::make_shared_scheduler_proxy<
                    boost::asynchronous::threadpool_scheduler<
                            boost::asynchronous::lockfree_queue<boost::asynchronous::any_serializable>>>(1);
        // We need a worker pool
        // possibly for us, and we want to share it with the tcp pool for its serialization work
        boost::asynchronous::any_shared_scheduler_proxy<> workers = boost::asynchronous::make_shared_scheduler_proxy<
            boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(1);
        // we use a tcp pool using the 3 worker threads we just built
        auto tcp_server= boost::asynchronous::make_shared_scheduler_proxy<
                    boost::asynchronous::tcp_server_scheduler<
                            boost::asynchronous::lockfree_queue<boost::asynchronous::any_serializable>,
                            boost::asynchronous::any_callable,true>>
                                (workers,"localhost",12345);
        // we need a composite for stealing
        m_composite = boost::asynchronous::make_shared_scheduler_proxy<
                boost::asynchronous::composite_threadpool_scheduler<boost::asynchronous::any_serializable>>
                          (pool,tcp_server);
        // and this will be the worker pool for post_callback
        set_worker(pool);
    }
    // called when task done, in our thread
    void on_callback(long const& result)
    {
        std::cout << "Callback in our (safe) single-thread scheduler" << std::endl;
        std::cout << "result = " << result << std::endl;
        // inform test caller
        m_promise->set_value();
    }
    // call to this is posted and executes in our (safe) single-thread scheduler
    std::future<void> start_async_work()
    {
        // for testing purpose
        auto fu = m_promise->get_future();
        // start long tasks in threadpool (first lambda) and callback in our thread
        // we know the data will be alive until the end so we can use "this"
        post_callback(
               dummy_parallel_count_task(),
               // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
               [this](boost::asynchronous::expected<long> res){
                            this->on_callback(res.get());
               }// callback functor.
        );
        return fu;
    }
private:
// for testing
std::shared_ptr<std::promise<void> > m_promise;
// attribute to keep composite alive
boost::asynchronous::any_shared_scheduler_proxy<boost::asynchronous::any_serializable> m_composite;
};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    // caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work)
};

}

void example_count_tcp()
{
    std::cout << "example_count_tcp" << std::endl;
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                                boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::lockfree_queue<>>>();
        {
            ServantProxy proxy(scheduler);
            auto fu = proxy.start_async_work();
            auto resfu = fu.get();
            resfu.get();
        }
    }
    std::cout << "end example_count_tcp \n" << std::endl;
}


