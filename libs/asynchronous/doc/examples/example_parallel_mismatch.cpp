#include <iostream>


#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_mismatch.hpp>

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
        , m_promise(new boost::promise<void>)
        , m_data(10000,1)
    {
        m_data2.resize(10000, 1);
        int value = 2;
        for (int x = 2000; x < 10000; ++x) {
            if (x % 2000 == 1999) ++value;
            m_data2[x] = value;
        }
    }
    // called when task done, in our thread
    template <typename Iterator>
    void on_callback(Iterator first, Iterator second)
    {
        std::cout << "Callback in our (safe) single-thread scheduler" << std::endl;
        std::cout << "First mismatch is at index " << std::distance(m_data.begin(), first) << " (second list gives index " << std::distance(m_data2.begin(), second) << ")" << std::endl;
        std::cout << "Value in the first list: " << *first << std::endl;
        std::cout << "Value in the second list: " << *second << std::endl;
        // inform test caller
        m_promise->set_value();
    }
    // call to this is posted and executes in our (safe) single-thread scheduler
    boost::future<void> start_async_work()
    {
        // for testing purpose
        auto fu = m_promise->get_future();
        // start long tasks in threadpool (first lambda) and callback in our thread
        // we know the data will be alive until the end so we can use "this"
        post_callback(
               [this](){
                        return boost::asynchronous::parallel_mismatch(this->m_data.begin(),this->m_data.end(),this->m_data2.begin(),1500);
                      },// work
               // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
               [this](boost::asynchronous::expected<std::pair<decltype(this->m_data.begin()), decltype(this->m_data2.begin())>> res){
                   auto pair = res.get();
                   this->on_callback(pair.first, pair.second);
               }// callback functor.
        );
        return fu;
    }
private:
// for testing
std::shared_ptr<boost::promise<void> > m_promise;
std::vector<int> m_data;
std::vector<int> m_data2;
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

void example_mismatch()
{
    std::cout << "example_mismatch" << std::endl;
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
    std::cout << "end example_mismatch \n" << std::endl;
}

