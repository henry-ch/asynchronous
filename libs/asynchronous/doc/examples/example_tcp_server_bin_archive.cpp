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
#include "bin_archive_types.hpp"
#include "dummy_tcp_task.hpp"

using namespace std;

namespace
{
struct Servant : boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,boost::asynchronous::any_bin_serializable>
{
    // optional, ctor is simple enough not to be posted
    //typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,boost::asynchronous::any_bin_serializable>(scheduler)
        // for testing purpose
        , m_promise(new boost::promise<int>)
        , m_total(0)
        , m_tasks_done(0)
    {
        // let's build our pool step by step. First we need a worker pool
        // possibly for us, and we want to share it with the tcp pool for its serialization work
        boost::asynchronous::any_shared_scheduler_proxy<> workers = boost::asynchronous::create_shared_scheduler_proxy(
            new boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<> >(3));
        // we use a tcp pool using the 3 worker threads we just built
        auto pool= boost::asynchronous::create_shared_scheduler_proxy(
                    new boost::asynchronous::tcp_server_scheduler<
                            boost::asynchronous::lockfree_queue<boost::asynchronous::any_bin_serializable> >
                                (workers,"localhost",12345));
        // and this will be the worker pool for post_callback
        set_worker(pool);

    }
    // called when task done, in our thread
    void on_callback(int res)
    {
        std::cout << "Callback in our (safe) single-thread scheduler with result: " << res << std::endl;
        ++m_tasks_done;
        m_total += res;
        if (m_tasks_done==10) // 10 tasks started
        {
            // inform test caller
            m_promise->set_value(m_total);
        }
    }
    // call to this is posted and executes in our (safe) single-thread scheduler
    boost::shared_future<int> start_async_work()
    {
        std::cout << "start_async_work()" << std::endl;
        // for testing purpose
        boost::shared_future<int> fu = m_promise->get_future();
        // start long tasks in threadpool (first lambda) and callback in our thread
        for (int i =0 ;i < 10 ; ++i)
        {
            std::cout << "call post_callback with i: " << i << std::endl;
            post_callback(
                        dummy_tcp_task(i)
                        //std::move(s)
                    ,
                   // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
                   [this](boost::asynchronous::expected<int> res){
                                try{
                                    this->on_callback(res.get());
                                }
                                catch(std::exception& e)
                                {
                                    std::cout << "got exception: " << e.what() << std::endl;
                                    this->on_callback(0);
                                }
                   }// callback functor.
            );
        }
        return fu;
    }
private:
// for testing
boost::shared_ptr<boost::promise<int> > m_promise;
int m_total;
unsigned int m_tasks_done;//will count until 10, then we are done (we start 10 tasks)
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

void example_post_tcp_bin_archive()
{
    std::cout << "example_post_tcp" << std::endl;
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::lockfree_queue<> >);
        {
            ServantProxy proxy(scheduler);
            // result of BOOST_ASYNC_FUTURE_MEMBER is a shared_future,
            // so we have a shared_future of a shared_future(result of start_async_work)
            boost::shared_future<boost::shared_future<int> > fu = proxy.start_async_work();
            boost::shared_future<int> resfu = fu.get();
            int res = resfu.get();
            std::cout << "res==45? " << std::boolalpha << (res == 45) << std::endl;// 1+2..9.
        }
    }
    std::cout << "end example_post_tcp \n" << std::endl;
}




