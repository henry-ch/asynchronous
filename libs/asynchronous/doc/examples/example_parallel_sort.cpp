#include <iostream>
#include <random>
#include <boost/enable_shared_from_this.hpp>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_sort.hpp>

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
                                                           boost::asynchronous::lockfree_queue<> >(6)))
        // for testing purpose
        , m_promise(new boost::promise<void>)
    {
        // build a test vector
        generate();
        // copy the data for later comparison
        m_copied_data = m_data;
    }
    // called when task done, in our thread
    void on_callback()
    {
        std::cout << "Callback in our (safe) single-thread scheduler" << std::endl;
        std::sort(m_copied_data.begin(),m_copied_data.end(),std::less<int>());
        std::cout << "parallel_sort and sort gave the same result? " << std::boolalpha << (m_data == m_copied_data) << std::endl;
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
                        return boost::asynchronous::parallel_sort(this->m_data.begin(),this->m_data.end(),std::less<int>(),1500);
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
// helper, generate vector
void generate()
{
    m_data = std::vector<int>(10000,1);
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<> dis(0, 1000);
    std::generate(m_data.begin(), m_data.end(), std::bind(dis, std::ref(mt)));
}
boost::shared_ptr<boost::promise<void> > m_promise;

std::vector<int> m_data;
std::vector<int> m_copied_data;
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

void example_sort()
{
    std::cout << "example_sort" << std::endl;
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::lockfree_queue<> >);
        {
            ServantProxy proxy(scheduler);
            boost::shared_future<boost::shared_future<void> > fu = proxy.start_async_work();
            boost::shared_future<void> resfu = fu.get();
            resfu.get();
        }
    }
    std::cout << "end example_sort \n" << std::endl;
}


