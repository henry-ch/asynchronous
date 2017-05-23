#include <iostream>


#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_fill.hpp>

using namespace std;

namespace
{
struct Servant : boost::asynchronous::trackable_servant<>
{
    // optional, ctor is simple enough not to be posted
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               // threadpool with 3 threads and a simple lockfree_queue
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<>>>(6))
        // for testing purpose
        , m_promise(new std::promise<void>)
        , m_data(10000,1)
    {
    }
    // called when task done, in our thread
    void on_callback()
    {
        std::cout << "Callback in our (safe) single-thread scheduler" << std::endl;
        std::cout << "The following values should all be 42." << std::endl;
        auto it = m_data.begin();
        std::cout << "m_data[0]= " << *it << std::endl;
        std::advance(it,100);
        std::cout << "m_data[100]= " << *it << std::endl;
        std::advance(it,900);
        std::cout << "m_data[1000]= " << *it << std::endl;
        std::advance(it,8999);
        std::cout << "m_data[9999]= " << *it << std::endl;
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
               [this](){
                        return boost::asynchronous::parallel_fill(this->m_data.begin(), this->m_data.end(), 42, 1500);
                      },// work
               // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
               [this](boost::asynchronous::expected<void> /*res*/){
                            this->on_callback();
               }// callback functor.
        );
        return fu;
    }
private:
// for testing
std::shared_ptr<std::promise<void> > m_promise;
std::vector<int> m_data;
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

void example_fill()
{
    std::cout << "example_fill" << std::endl;
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
    std::cout << "end example_fill" << std::endl << std::endl;
}
