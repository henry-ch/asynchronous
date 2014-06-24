#include <iostream>
#include <boost/enable_shared_from_this.hpp>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/tcp/tcp_server_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/diagnostics/any_loggable_serializable.hpp>
#include "dummy_tcp_task.hpp"

using namespace std;

namespace
{
typedef boost::asynchronous::any_loggable<boost::chrono::high_resolution_clock> log_servant_job;
typedef std::map<std::string,std::list<boost::asynchronous::diagnostic_item<boost::chrono::high_resolution_clock> > > diag_type;

// notice how the worker pool has a different job type
struct Servant : boost::asynchronous::trackable_servant<log_servant_job,boost::asynchronous::any_loggable_serializable<>>
{
    // optional, ctor is simple enough not to be posted
    //typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<log_servant_job> scheduler)
        : boost::asynchronous::trackable_servant<log_servant_job,boost::asynchronous::any_loggable_serializable<>>(scheduler)
        // for testing purpose
        , m_promise(new boost::promise<int>)
        , m_total(0)
        , m_tasks_done(0)
    {
        // let's build our pool step by step. First we need a worker pool
        // possibly for us, and we want to share it with the tcp pool for its serialization work
        boost::asynchronous::any_shared_scheduler_proxy<log_servant_job> workers = boost::asynchronous::create_shared_scheduler_proxy(
            new boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<log_servant_job> >(3));
        // we use a tcp pool using the 3 worker threads we just built
        // our server will listen on "localhost" port 12345
        boost::asynchronous::any_shared_scheduler_proxy<boost::asynchronous::any_loggable_serializable<>> pool=
                boost::asynchronous::create_shared_scheduler_proxy(
                    new boost::asynchronous::tcp_server_scheduler<
                            boost::asynchronous::lockfree_queue<boost::asynchronous::any_loggable_serializable<>>,log_servant_job >
                                (workers,"localhost",12345));
        // and this will be the worker pool for post_callback
        set_worker(pool);

    }
    // called when task done, in our thread
    void on_callback(int res)
    {
        std::cout << "Callback in our (safe) single-thread scheduler with result: " << res << std::endl;
        ++m_tasks_done;
        m_total += res;
        if (m_tasks_done==10) // 10 tasks started
        {
            // inform test caller
            m_promise->set_value(m_total);
        }
    }
    // call to this is posted and executes in our (safe) single-thread scheduler
    boost::shared_future<int> start_async_work()
    {
        std::cout << "start_async_work()" << std::endl;
        // for testing purpose
        boost::shared_future<int> fu = m_promise->get_future();
        // start long tasks in threadpool (first lambda) and callback in our thread
        for (int i =0 ;i < 10 ; ++i)
        {
            std::cout << "call post_callback with i: " << i << std::endl;
            post_callback(
                   dummy_tcp_task(i),
                   // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
                   [this](boost::future<int> res){
                                try{
                                    this->on_callback(res.get());
                                }
                                catch(std::exception& e)
                                {
                                    std::cout << "got exception: " << e.what() << std::endl;
                                    this->on_callback(0);
                                }
                   },// callback functor.
                    // task name for logging
                    "int_async_work"
            );
        }
        return fu;
    }
    // threadpool diagnostics
    diag_type get_diagnostics() const
    {
        return get_worker().get_diagnostics();
    }
private:
// for testing
boost::shared_ptr<boost::promise<int> > m_promise;
int m_total;
unsigned int m_tasks_done;//will count until 10, then we are done (we start 10 tasks)
};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant,log_servant_job>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant,log_servant_job>(s)
    {}
    // caller will get a future
    // we give ctor and dtor a task name
    BOOST_ASYNC_SERVANT_POST_CTOR_LOG("Servant ctor",3)
    BOOST_ASYNC_SERVANT_POST_DTOR_LOG("Servant dtor",4)
    // member name, task name and priority
    BOOST_ASYNC_FUTURE_MEMBER_LOG(start_async_work,"proxy::start_async_work",1)
    BOOST_ASYNC_FUTURE_MEMBER_LOG(get_diagnostics,"proxy::get_diagnostics",1)
};

}

void example_post_tcp_log()
{
    std::cout << "example_post_tcp_log" << std::endl;
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::single_thread_scheduler<
                                        boost::asynchronous::lockfree_queue<log_servant_job> >(10));
        {
            ServantProxy proxy(scheduler);
            // result of BOOST_ASYNC_FUTURE_MEMBER is a shared_future,
            // so we have a shared_future of a shared_future(result of start_async_work)
            boost::shared_future<boost::shared_future<int> > fu = proxy.start_async_work();
            boost::shared_future<int> resfu = fu.get();
            int res = resfu.get();
            std::cout << "res==45? " << std::boolalpha << (res == 45) << std::endl;// 1+2..9.

            // logs
            boost::shared_future<diag_type> fu_diag = proxy.get_diagnostics();
            diag_type diag = fu_diag.get();
            std::cout << "Display of worker jobs" << std::endl;
            for (auto mit = diag.begin(); mit != diag.end() ; ++mit)
            {
                std::cout << "job type: " << (*mit).first << std::endl;
                for (auto jit = (*mit).second.begin(); jit != (*mit).second.end();++jit)
                {
                    std::cout << "job waited in us: " << boost::chrono::nanoseconds((*jit).get_started_time() - (*jit).get_posted_time()).count() / 1000 << std::endl;
                    std::cout << "job lasted in us: " << boost::chrono::nanoseconds((*jit).get_finished_time() - (*jit).get_started_time()).count() / 1000 << std::endl;
                    std::cout << "job interrupted? "  << std::boolalpha << (*jit).is_interrupted() << std::endl;
                }
            }
            std::cout << "Display of servant jobs" << std::endl;
            diag_type single_thread_sched_diag = scheduler.get_diagnostics();
            for (auto mit = single_thread_sched_diag.begin(); mit != single_thread_sched_diag.end() ; ++mit)
            {
                std::cout << "job type: " << (*mit).first << std::endl;
                for (auto jit = (*mit).second.begin(); jit != (*mit).second.end();++jit)
                {
                    std::cout << "job waited in us: " << boost::chrono::nanoseconds((*jit).get_started_time() - (*jit).get_posted_time()).count() / 1000 << std::endl;
                    std::cout << "job lasted in us: " << boost::chrono::nanoseconds((*jit).get_finished_time() - (*jit).get_started_time()).count() / 1000 << std::endl;
                    std::cout << "job interrupted? "  << std::boolalpha << (*jit).is_interrupted() << std::endl;
                }
            }
        }
    }
    std::cout << "end example_post_tcp_log \n" << std::endl;
}

void example_tcp_post_future_log()
{
//    std::cout << "example_tcp_post_future_log" << std::endl;
//    {
//        boost::asynchronous::any_shared_scheduler_proxy<> workers = boost::asynchronous::create_shared_scheduler_proxy(
//            new boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<> >(3));
//        // we use a tcp pool using the 3 worker threads we just built
//        auto pool= boost::asynchronous::create_shared_scheduler_proxy(
//                    new boost::asynchronous::tcp_server_scheduler<
//                            boost::asynchronous::lockfree_queue<boost::asynchronous::any_loggable_serializable<>> >
//                                (workers,"localhost",12345));
//        boost::shared_future<int> fui = boost::asynchronous::post_future(pool, dummy_tcp_task(42));
//        int res = fui.get();
//        std::cout << "res==42? " << std::boolalpha << (res == 42) << std::endl;// of course 42.
//    }
//    std::cout << "end example_tcp_post_future_log \n" << std::endl;
}



