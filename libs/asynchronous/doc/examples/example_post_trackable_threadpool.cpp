#include <iostream>
#include <boost/enable_shared_from_this.hpp>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/threadsafe_list.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
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
                                               // threadpool with 3 threads and a simple threadsafe_list queue
                                               boost::asynchronous::create_shared_scheduler_proxy(
                                                   new boost::asynchronous::threadpool_scheduler<
                                                           boost::asynchronous::threadsafe_list<> >(3)))
        // for testing purpose
        , m_promise(new boost::promise<int>)
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
    boost::shared_future<int> start_async_work()
    {
        // for testing purpose
        boost::shared_future<int> fu = m_promise->get_future();
        // start long tasks in threadpool (first lambda) and callback in our thread
        post_callback(
               [](){
                        std::cout << "Long Work" << std::endl;
                        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
                        return 42; //always...
                    }// work
                ,
               // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
               [this](boost::future<int> res){
                            this->on_callback(res.get());
               }// callback functor.
        );
        return fu;
    }
private:
// for testing
boost::shared_ptr<boost::promise<int> > m_promise;
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

void example_post_trackable_alive()
{
    std::cout << "example_post_trackable_alive" << std::endl;
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::threadsafe_list<> >);
        {
            ServantProxy proxy(scheduler);
            // result of BOOST_ASYNC_FUTURE_MEMBER is a shared_future,
            // so we have a shared_future of a shared_future(result of start_async_work)
            boost::shared_future<boost::shared_future<int> > fu = proxy.start_async_work();
            boost::shared_future<int> resfu = fu.get();
            int res = resfu.get();
            std::cout << "res==42? " << std::boolalpha << (res == 42) << std::endl;// of course 42.
        }
    }
    std::cout << "end example_post_trackable_alive \n" << std::endl;
}

void example_post_trackable_not_alive()
{
    cout << "start example_post_trackable_not_alive" << endl;
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::threadsafe_list<> >);
        {
            ServantProxy proxy(scheduler);
            // call task but not callback, should not crash...
            boost::shared_future<boost::shared_future<int> > fu = proxy.start_async_work();
            // call was posted, let's destroy servant before callback
            // Actually we don't know if callback will be called before destruction, it's luck
            // but in any case, we don't want the callback to be called if Servant has been destroyed.
            fu.get();
        }
    }
}

