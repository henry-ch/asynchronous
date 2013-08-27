
#include <iostream>
#include <boost/enable_shared_from_this.hpp>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/io_threadpool_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.h>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/trackable_servant.hpp>

using namespace std;

namespace
{
struct Servant : boost::asynchronous::trackable_servant<>
{
    // optional, ctor is simple enough not to be posted
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               // io pool with 2 threads and up to 4 threads
                                               boost::asynchronous::create_shared_scheduler_proxy(
                                                   new boost::asynchronous::io_threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<> >(2,4,10)))
        // for testing purpose
        , m_promise(new boost::promise<int>)
        , m_counter(0)
        , m_current(0)
    {
    }
    // called when task done, in our thread
    void on_callback(int res)
    {
        std::cout << "Callback in our (safe) single-thread scheduler" << std::endl;
        // inform test caller
        m_promise->set_value(res);
    }
    // call to this is posted and executes in our (safe) single-thread scheduler
    boost::shared_future<int> start_async_work(int cpt)
    {
        // for testing purpose
        m_counter = cpt;
        m_current=0;
        boost::shared_future<int> fu = m_promise->get_future();
        // start long tasks in threadpool (first lambda) and callback in our thread
        for (int i = 0; i< m_counter ; ++i)
        {
            post_callback(
                   [](){
                            std::cout << "Long Work" << std::endl;
                            boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
                            return 42; //always...
                        }// work
                    ,
                   // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
                   [this](boost::future<int> res){
                                ++this->m_current;
                                if (this->m_current == this->m_counter)
                                    this->on_callback(res.get());
                   }// callback functor.
            );
        }
        return fu;
    }
private:
// for testing
boost::shared_ptr<boost::promise<int> > m_promise;
int m_counter;
int m_current;
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

void example_io_pool_2()
{
    std::cout << "example_io_pool_2" << std::endl;
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::single_thread_scheduler<
                                    boost::asynchronous::lockfree_queue<> >(10));
        {
            ServantProxy proxy(scheduler);
            // result of BOOST_ASYNC_FUTURE_MEMBER is a shared_future,
            // so we have a shared_future of a shared_future(result of start_async_work)
            boost::shared_future<boost::shared_future<int> > fu = proxy.start_async_work(2);
            boost::shared_future<int> resfu = fu.get();
            int res = resfu.get();
            std::cout << "res==42? " << std::boolalpha << (res == 42) << std::endl;// of course 42.
        }
    }
    std::cout << "end example_io_pool_2 \n" << std::endl;
}


void example_io_pool_4()
{
    std::cout << "example_io_pool_4" << std::endl;
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::single_thread_scheduler<
                                    boost::asynchronous::lockfree_queue<> >(10));
        {
            ServantProxy proxy(scheduler);
            // result of BOOST_ASYNC_FUTURE_MEMBER is a shared_future,
            // so we have a shared_future of a shared_future(result of start_async_work)
            boost::shared_future<boost::shared_future<int> > fu = proxy.start_async_work(4);
            boost::shared_future<int> resfu = fu.get();
            int res = resfu.get();
            std::cout << "res==42? " << std::boolalpha << (res == 42) << std::endl;// of course 42.
        }
    }
    std::cout << "end example_io_pool_4 \n" << std::endl;
}

void example_io_pool_5()
{
    std::cout << "example_io_pool_5" << std::endl;
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::single_thread_scheduler<
                                    boost::asynchronous::lockfree_queue<> >(10));
        {
            ServantProxy proxy(scheduler);
            // result of BOOST_ASYNC_FUTURE_MEMBER is a shared_future,
            // so we have a shared_future of a shared_future(result of start_async_work)
            boost::shared_future<boost::shared_future<int> > fu = proxy.start_async_work(5);
            boost::shared_future<int> resfu = fu.get();
            int res = resfu.get();
            std::cout << "res==42? " << std::boolalpha << (res == 42) << std::endl;// of course 42.
        }
    }
    std::cout << "end example_io_pool_5 \n" << std::endl;
}
void example_io_pool_9()
{
    std::cout << "example_io_pool_9" << std::endl;
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::single_thread_scheduler<
                                    boost::asynchronous::lockfree_queue<> >(10));
        {
            ServantProxy proxy(scheduler);
            // result of BOOST_ASYNC_FUTURE_MEMBER is a shared_future,
            // so we have a shared_future of a shared_future(result of start_async_work)
            boost::shared_future<boost::shared_future<int> > fu = proxy.start_async_work(9);
            boost::shared_future<int> resfu = fu.get();
            int res = resfu.get();
            std::cout << "res==42? " << std::boolalpha << (res == 42) << std::endl;// of course 42.
        }
    }
    std::cout << "end example_io_pool_9 \n" << std::endl;
}

