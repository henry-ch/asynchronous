#include <iostream>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/threadsafe_list.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/queue/any_queue_container.hpp>
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
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               // a threadpool with 1 thread using a threadsafe list
                                               boost::asynchronous::create_shared_scheduler_proxy(
                                                   new boost::asynchronous::threadpool_scheduler<
                                                            boost::asynchronous::threadsafe_list<> >(1)))
        , m_promise(new boost::promise<int>)
    {
    }
    boost::shared_future<int> start_async_work()
    {
        // for demonstration purpose
        boost::shared_future<int> fu = m_promise->get_future();
        // start long tasks
        post_callback(
               [](){return 42;}// work
                ,
               [this](boost::future<int> res){
                            this->m_promise->set_value(res.get());
               }// callback functor.
               // task name for logging, not used here
               ,"",
               // priority of the task itself (won't be used by threadpool_scheduler)
               2,
               // priority of the callback
               2
        );
        return fu;
    }
private:
// for demonstration purpose
boost::shared_ptr<boost::promise<int> > m_promise;
};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    // we give ctor and dtor a low priority
    BOOST_ASYNC_SERVANT_POST_CTOR(3)
    BOOST_ASYNC_SERVANT_POST_DTOR(4)
    // start_async_work gets the highest priority. No particular reason, we feel like it.
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work,1)
};

}

void example_queue_container()
{
    {
        // a scheduler with 1 threadsafe list, and 3 lockfree queues as work input queue
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::single_thread_scheduler<
                                        boost::asynchronous::any_queue_container<> >
                        (boost::asynchronous::any_queue_container_config<boost::asynchronous::threadsafe_list<> >(1),
                         boost::asynchronous::any_queue_container_config<boost::asynchronous::lockfree_queue<> >(3,100)
                         ));
        {
            ServantProxy proxy(scheduler);

            boost::shared_future<boost::shared_future<int> > fu = proxy.start_async_work();
            boost::shared_future<int> resfu = fu.get();
            int res = resfu.get();
            std::cout << "res==42? " << std::boolalpha << (res == 42) << std::endl;
        }
    }
}


