
#include <iostream>
#include <boost/enable_shared_from_this.hpp>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/tcp/tcp_server_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/any_serializable.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/algorithm/parallel_invoke.hpp>

using namespace std;

namespace
{
struct my_exception : virtual boost::exception, virtual std::exception
{
};
struct Servant : boost::asynchronous::trackable_servant<>
{
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               boost::asynchronous::create_shared_scheduler_proxy(
                                                   new boost::asynchronous::multiqueue_threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<> >(6)))
        // for testing purpose
        , m_promise(new boost::promise<void>)
    {
    }
    // called when task done, in our thread
    void on_callback()
    {
        std::cout << "Callback in our (safe) single-thread scheduler" << std::endl;
        m_promise->set_value();
    }
    // call to this is posted and executes in our (safe) single-thread scheduler
    boost::shared_future<void> start_async_work()
    {
        std::cout << "start_async_work()" << std::endl;
        // for testing purpose
        boost::shared_future<void> fu = m_promise->get_future();
        post_callback(
                   []()
                   {
                         return boost::asynchronous::parallel_invoke<boost::asynchronous::any_callable>(
                                     boost::asynchronous::to_continuation_task([](){throw my_exception();}),
                                     boost::asynchronous::to_continuation_task([](){return 42.0;})
                                );
                   },// work
                   // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
                   [this](boost::future<std::tuple<boost::future<void>,boost::future<double>>> res)
                   {
                        try
                        {
                            auto t = res.get();
                            std::cout << "got result: " << (std::get<1>(t)).get() << std::endl;
                            std::cout << "got exception?: " << (std::get<0>(t)).has_exception() << std::endl;
                            this->on_callback();
                        }
                        catch(std::exception& e)
                        {
                            std::cout << "got exception: " << e.what() << std::endl;
                        }
                   }// callback functor.
            );
        return fu;
    }
private:
// for testing
boost::shared_ptr<boost::promise<void> > m_promise;
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

void example_parallel_invoke()
{
    std::cout << "example_parallel_invoke" << std::endl;
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::lockfree_queue<> >);
        {
            ServantProxy proxy(scheduler);
            // result of BOOST_ASYNC_FUTURE_MEMBER is a shared_future,
            // so we have a shared_future of a shared_future(result of start_async_work)
            boost::shared_future<boost::shared_future<void> > fu = proxy.start_async_work();
            boost::shared_future<void> resfu = fu.get();
            resfu.get();
        }
    }
    std::cout << "end example_parallel_invoke \n" << std::endl;
}




