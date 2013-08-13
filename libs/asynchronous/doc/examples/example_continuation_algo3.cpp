#include <iostream>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/threadsafe_list.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/continuation_task.hpp>

#include <boost/asynchronous/servant_proxy.h>
#include <boost/asynchronous/trackable_servant.hpp>

using namespace std;

namespace
{

// dummy task started by the main algorithm
// could be a lambda too of course
struct sub_sub_task
{
    int operator()()const
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        return 1;
    }
};

// our first sub algo task. Needs to inherit continuation_task<value type returned by this task>
struct sub_task : public boost::asynchronous::continuation_task<long>
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
        std::vector<boost::future<int> > fus;
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        // let's say we just found a subtask
        boost::future<int> fu1 = boost::asynchronous::post_future(locked_scheduler,sub_sub_task());
        fus.emplace_back(std::move(fu1));
        // simulate more algo work
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        // let's say we just found a subtask
        boost::future<int> fu2 = boost::asynchronous::post_future(locked_scheduler,sub_sub_task());
        fus.emplace_back(std::move(fu2));
        // simulate algo work
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        // let's say we just found a subtask
        boost::future<int> fu3 = boost::asynchronous::post_future(locked_scheduler,sub_sub_task());
        fus.emplace_back(std::move(fu3));

        // our algo is now done, wrap all and return
        boost::asynchronous::create_continuation<long>(
                    // called when subtasks are done, set our result
                    [task_res](std::vector<boost::future<int>>&& res)
                    {
                        try
                        {
                            long r = res[0].get() + res[1].get() + res[2].get();
                            task_res.set_value(r);
                        }
                        catch(std::exception& e)
                        {
                            task_res.set_exception(boost::copy_exception(e));
                        }
                    },
                    // future results of recursive tasks
                    std::move(fus));
    }
};

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
        // we need 2 bigger subtasks
        // prepare return
        boost::asynchronous::create_continuation<long>(
                    // called when subtasks are done, set our result
                    [task_res](std::tuple<boost::future<long>,boost::future<long> >&& res)
                    {
                        try
                        {
                            long r = std::get<0>(res).get() + std::get<1>(res).get();
                            task_res.set_value(r);
                        }
                        catch(std::exception& e)
                        {
                            task_res.set_exception(boost::copy_exception(e));
                        }
                    },
                    // future results of recursive tasks
                    sub_task(),sub_task());
    }
};

struct Servant : boost::asynchronous::trackable_servant<>
{
    // optional, ctor is simple enough not to be posted
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               // threadpool and a simple threadsafe_list queue
                                               boost::asynchronous::create_shared_scheduler_proxy(
                                                   new boost::asynchronous::multiqueue_threadpool_scheduler<
                                                           boost::asynchronous::threadsafe_list<> >(6)))
        // for testing purpose
        , m_promise(new boost::promise<long>)
    {
    }
    // called when task done, in our thread
    void on_callback(long res)
    {
        // inform test caller
        m_promise->set_value(res);
    }
    // call to this is posted and executes in our (safe) single-thread scheduler
    boost::shared_future<long> calc_algo()
    {
        // for testing purpose
        boost::shared_future<long> fu = m_promise->get_future();
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
               [this](boost::future<long> res){
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
boost::shared_ptr<boost::promise<long> > m_promise;
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

void example_continuation_algo3()
{
    std::cout << "example_continuation_algo3" << std::endl;
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::threadsafe_list<> >);
        {
            ServantProxy proxy(scheduler);
            boost::shared_future<boost::shared_future<long> > fu = proxy.calc_algo();
            boost::shared_future<long> resfu = fu.get();
            long res = resfu.get();
            std::cout << "res= " << res << std::endl; //expect 6 (6 subtasks)
        }
    }
    std::cout << "end example_continuation_algo3 \n" << std::endl;
}



