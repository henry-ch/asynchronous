#include <iostream>
#include <future>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/continuation_task.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>

using namespace std;

namespace
{

// dummy task started by the main algorithm
// could be a lambda too of course
struct sub_task
{
    int operator()()const
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        return 1;
    }
};

// our main algo task. Needs to inherit continuation_task<value type returned by this task>
struct main_task : public boost::asynchronous::continuation_task<long>
{
    void operator()()const
    {
        // the result of this task
        boost::asynchronous::continuation_result<long> task_res = this_task_result();

        // we start calculation, then while doing this we see new tasks which can be posted and done concurrently to us
        // when all are done, we will set the result
        // to post tasks, we need a scheduler
        boost::asynchronous::any_weak_scheduler<> weak_scheduler = boost::asynchronous::get_thread_scheduler<>();
        boost::asynchronous::any_shared_scheduler<> locked_scheduler = weak_scheduler.lock();
        if (!locked_scheduler.is_valid())
            // ok, we are shutting down, ok give up
            return;
        // simulate algo work
        std::vector<std::future<int> > fus;
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        // let's say we just found a subtask
        std::future<int> fu1 = boost::asynchronous::post_future(locked_scheduler,sub_task());
        fus.emplace_back(std::move(fu1));
        // simulate more algo work
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        // let's say we just found a subtask
        std::future<int> fu2 = boost::asynchronous::post_future(locked_scheduler,sub_task());
        fus.emplace_back(std::move(fu2));
        // simulate algo work
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        // let's say we just found a subtask
        std::future<int> fu3 = boost::asynchronous::post_future(locked_scheduler,sub_task());
        fus.emplace_back(std::move(fu3));

        // our algo is now done, wrap all and return
        boost::asynchronous::create_continuation(
                    // called when subtasks are done, set our result
                    [task_res](std::vector<std::future<int>> res)
                    {
                        try
                        {
                            long r = res[0].get() + res[1].get() + res[2].get();
                            task_res.set_value(r);
                        }
                        catch(std::exception& e)
                        {
                            task_res.set_exception(std::make_exception_ptr(e));
                        }
                    },
                    // future results of recursive tasks
                    std::move(fus));
    }
};

struct Servant : boost::asynchronous::trackable_servant<>
{
    // optional, ctor is simple enough not to be posted
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               // threadpool and a simple lockfree_queue queue
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::multiqueue_threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<>>>(6))
        // for testing purpose
        , m_promise(new std::promise<long>)
    {
    }
    // called when task done, in our thread
    void on_callback(long res)
    {
        // inform test caller
        m_promise->set_value(res);
    }
    // call to this is posted and executes in our (safe) single-thread scheduler
    std::shared_future<long> calc_algo()
    {
        // for testing purpose
        std::shared_future<long> fu = m_promise->get_future();
        // start long tasks in threadpool (first lambda) and callback in our thread
        post_callback(
                []()
                {
                     // a top-level continuation is the first one in a recursive serie.
                     // Its result will be passed to callback
                     return boost::asynchronous::top_level_continuation<long>(main_task());
                 }// work
               ,
               // callback with result.
               [this](boost::asynchronous::expected<long> res){
                    try
                    {
                        this->on_callback(res.get());
                    }
                    catch(std::exception& e)
                    {
                        // do something useful
                        this->on_callback(-1);
                    }
               }// callback functor.
        );
        return fu;
    }
private:
// for testing
std::shared_ptr<std::promise<long> > m_promise;
};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    // caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER(calc_algo)
};

}

void example_continuation_algo2()
{
    std::cout << "example_continuation_algo2" << std::endl;
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                                 boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::lockfree_queue<>>>();
        {
            ServantProxy proxy(scheduler);
            auto fu = proxy.calc_algo();
            auto resfu = fu.get();
            long res = resfu.get();
            std::cout << "res= " << res << std::endl; //expect 3 (3 subtasks)
        }
    }
    std::cout << "end example_continuation_algo2 \n" << std::endl;
}


