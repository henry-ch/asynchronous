#include <iostream>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/threadsafe_list.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/queue/lockfree_spsc_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/stealing_multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/composite_threadpool_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
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
        : boost::asynchronous::trackable_servant<>(scheduler)
        // for testing purpose
        , m_promise(new std::promise<int>)
    {
        // create a composite threadpool made of:
        // a multiqueue_threadpool_scheduler, 1 thread, with a lockfree_queue.
        // This scheduler does not steal from other schedulers, but will lend its queues for stealing
        auto tp = boost::asynchronous::make_shared_scheduler_proxy<
                    boost::asynchronous::multiqueue_threadpool_scheduler<boost::asynchronous::lockfree_queue<>>> (1);
        // a stealing_multiqueue_threadpool_scheduler, 3 threads, each with a threadsafe_list
        // this scheduler will steal from other schedulers if it can. In this case it will manage only with tp, not tp3
        auto tp2 = boost::asynchronous::make_shared_scheduler_proxy<
                    boost::asynchronous::stealing_multiqueue_threadpool_scheduler<boost::asynchronous::threadsafe_list<>>> (3);
        // a multiqueue_threadpool_scheduler, 4 threads, each with a lockfree_spsc_queue
        // this works because there will be no stealing as the queue can't, and only this single-thread scheduler will
        // be the producer
        auto tp3 = boost::asynchronous::make_shared_scheduler_proxy<
                    boost::asynchronous::multiqueue_threadpool_scheduler<boost::asynchronous::lockfree_spsc_queue<>>> (4);
        auto tp_worker =
                boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::composite_threadpool_scheduler<>> (tp,tp2,tp3);

        // use it as worker
        set_worker(tp_worker);
    }
    // called when task done, in our thread
    void on_callback(int res)
    {
        std::cout << "Callback in our (safe) single-thread scheduler" << std::endl;
        // inform test caller
        m_promise->set_value(res);
    }
    // call to this is posted and executes in our (safe) single-thread scheduler
    std::future<int> start_async_work()
    {
        // for testing purpose
        std::future<int> fu = m_promise->get_future();
        // start long tasks in threadpool (first lambda) and callback in our thread
        post_callback(
               [](){
                        std::cout << "Long Work tp1 in thread:" << boost::this_thread::get_id() << std::endl;
                        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
                        return 42; //always...
                    }// work
                ,
               [this](boost::asynchronous::expected<int>){},// callback functor.
               "",1,0
        );
        post_callback(
               [](){
                        // chances are good that the id will be different from previous work even with the same pool priority
                        // because it will be stolen from pool 2 (stealing_multiqueue_threadpool_scheduler)
                        std::cout << "Long Work tp1 in thread:" << boost::this_thread::get_id() << std::endl;
                        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
                        return 42; //always...
                    }// work
                ,
               // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
               [this](boost::asynchronous::expected<int> res){
                            this->on_callback(res.get());
               },// callback functor.
               "",1,0
        );
        return std::move(fu);
    }
private:
// for testing
std::shared_ptr<std::promise<int> > m_promise;
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

void example_composite_threadpool()
{
    std::cout << "example_composite_threadpool" << std::endl;
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                                boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::lockfree_queue<>>>();
        {
            ServantProxy proxy(scheduler);
            // result of BOOST_ASYNC_FUTURE_MEMBER is a future,
            // so we have a future holding a future(result of start_async_work)
            std::future<std::future<int> > fu = proxy.start_async_work();
            std::future<int> resfu = fu.get();
            int res = resfu.get();
            std::cout << "res==42? " << std::boolalpha << (res == 42) << std::endl;// of course 42.
        }
        std::cout << "example_composite_threadpool, destroyed servant \n" << std::endl;
    }
    std::cout << "end example_composite_threadpool \n" << std::endl;
}



