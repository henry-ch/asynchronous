#include <iostream>
#include <boost/enable_shared_from_this.hpp>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/trackable_servant.hpp>

using namespace std;

namespace
{
struct Servant : boost::asynchronous::trackable_servant<>
{
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               // threadpool with 3 threads and a simple lockfree_queue queue
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::threadpool_scheduler<
                                                    boost::asynchronous::lockfree_queue<>>>(3))
    {
    }
    // call to this is posted and executes in our (safe) single-thread scheduler
    void start_async_work()
    {
        // start long interruptible tasks
        boost::asynchronous::any_interruptible interruptible =
        interruptible_post_callback(
                // interruptible task
               [](){
                    std::cout << "Long Work" << std::endl;
                    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));}
               ,
               // callback functor.
               [](boost::asynchronous::expected<void> ){std::cout << "Callback will most likely not be called" << std::endl;}
        );
        // let the task start (not sure but likely)
        // if it had no time to start, well, then it will never.
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        // actually, we changed our mind and want to interrupt the task
        interruptible.interrupt();
    }
private:
};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work)
};

}

void example_interrupt()
{
    cout << "start example_interrupt" << endl;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                                boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::lockfree_queue<>>>();
        {
            ServantProxy proxy(scheduler);
            boost::future<void> fu = proxy.start_async_work();
            fu.get();
        }
    }
    std::cout << "end example_interrupt \n" << std::endl;
}



