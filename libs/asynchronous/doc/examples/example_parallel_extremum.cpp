#include <iostream>


#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_extremum.hpp>
#include <boost/asynchronous/helpers/lazy_irange.hpp>

using namespace std;

namespace
{
    
struct identity {
    int operator()(int x) {
        return x;
    }
};
    
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
        , m_promise(new boost::promise<void>)
        , m_data(100)
    {
    }
    // called when task done, in our thread
    void on_callback(int result)
    {
        std::cout << "Callback in our (safe) single-thread scheduler" << std::endl;
        std::cout << "result = " << result << std::endl;
        // inform test caller
        m_promise->set_value();
    }
    // call to this is posted and executes in our (safe) single-thread scheduler
    boost::shared_future<void> start_async_work()
    {
        // for testing purpose
        boost::shared_future<void> fu = m_promise->get_future();
        // start long tasks in threadpool (first lambda) and callback in our thread
        // we know the data will be alive until the end so we can use "this"
        post_callback(
               [this](){
                        return boost::asynchronous::parallel_extremum(this->m_data.begin(),this->m_data.end(), [](int a,int b){return a>b;}, 20);
                      }// work
                ,
               // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
               [this](boost::asynchronous::expected<int> res){
                            this->on_callback(res.get());
               }// callback functor.
        );
        return fu;
    }
private:
// for testing
std::shared_ptr<boost::promise<void> > m_promise;
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

void example_extremum()
{
    std::cout << "example_extremum" << std::endl;
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                                boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::lockfree_queue<>>>();
        {
            ServantProxy proxy(scheduler);
            boost::shared_future<boost::shared_future<void> > fu = proxy.start_async_work();
            boost::shared_future<void> resfu = fu.get();
            resfu.get();
        }
    }
    std::cout << "end example_extremum \n" << std::endl;
}

