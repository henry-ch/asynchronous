#include <iostream>
#include <boost/enable_shared_from_this.hpp>
#include <boost/chrono/chrono.hpp>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/threadsafe_list.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/diagnostics/any_loggable.hpp>

using namespace std;

namespace
{
typedef boost::asynchronous::any_loggable<boost::chrono::high_resolution_clock> servant_job;
typedef std::map<std::string,
                 std::list<boost::asynchronous::diagnostic_item<boost::chrono::high_resolution_clock> > > diag_type;

// we log our scheduler and our threadpool scheduler (both use servant_job)
struct Servant : boost::asynchronous::trackable_servant<servant_job,servant_job>
{
    // optional, ctor is simple enough not to be posted
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<servant_job> scheduler)
        : boost::asynchronous::trackable_servant<servant_job,servant_job>(scheduler,
                                               // threadpool with 3 threads and a simple threadsafe_list queue
                                               // Furthermore, it logs posted tasks
                                               boost::asynchronous::create_shared_scheduler_proxy(
                                                   new boost::asynchronous::threadpool_scheduler<
                                                           boost::asynchronous::threadsafe_list< servant_job > >(3)))
        , m_promise(new boost::promise<int>)
    {
    }

    // called when task done, in our thread
    void on_callback(int res)
    {
        std::cout << "Callback in our (safe) single-thread scheduler" << std::endl;
        // inform test caller
        m_promise->set_value(res);
    }
    // call to this is posted and executes in our (safe) single-thread scheduler
    boost::shared_future<int> start_async_work()
    {
        // for testing purpose
        boost::shared_future<int> fu = m_promise->get_future();
        // start long tasks in threadpool (first lambda) and callback in our thread
        post_callback(
               // task posted to threadpool
               [](){
                        std::cout << "Long Work" << std::endl;
                        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
                        return 42;}//always...
                ,
               // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
               [this](boost::future<int> res){
                            this->on_callback(res.get());
               },// callback functor.
               // the task / callback name for logging
               "int_async_work"
        );
        return fu;
    }
    boost::asynchronous::any_interruptible start_interruptible_async_work()
    {
        // start long tasks
        boost::asynchronous::any_interruptible interruptible =
        interruptible_post_callback(
               // task posted to threadpool
               [](){
                        std::cout << "Long Work" << std::endl;
                        boost::this_thread::sleep(boost::posix_time::milliseconds(2000));}
                ,
               // callback functor.
               [this](boost::future<void> ){
                    std::cout << "Callback, has a good chance not to be called." << std::endl;
                },
               // the task / callback name for logging
               "void_async_work"
        );
        return interruptible;
    }
    // we happily provide a way for the outside world to know what our threadpool did.
    // get_worker is provided by trackable_servant and gives the proxy of our threadpool
    diag_type get_diagnostics() const
    {
        return get_worker().get_diagnostics();
    }

private:
// for testing
boost::shared_ptr<boost::promise<int> > m_promise;
};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant,servant_job>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant,servant_job>(s)
    {}
    // the _LOG macros do the same as the others, but take an extra argument, the logged task name
    BOOST_ASYNC_FUTURE_MEMBER_LOG(start_async_work,"proxy::start_async_work")
    BOOST_ASYNC_FUTURE_MEMBER_LOG(get_diagnostics,"proxy::get_diagnostics")
    BOOST_ASYNC_FUTURE_MEMBER_LOG(start_interruptible_async_work,"proxy::start_interruptible_async_work")
};

}

void example_log()
{
    cout << "start example_log" << endl;
    {
        // a single-threaded world, where Servant will live.
        // this scheduler supports logging (servant_job)
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::single_thread_scheduler<
                                    boost::asynchronous::threadsafe_list<servant_job> >);
        {
            ServantProxy proxy(scheduler);
            // result of BOOST_ASYNC_FUTURE_MEMBER_LOG is a shared_future,
            // so we have a shared_future of a shared_future(result of start_async_work)
            boost::shared_future<boost::shared_future<int> > fu = proxy.start_async_work();
            boost::shared_future<int> resfu = fu.get();
            int res = resfu.get();
            std::cout << "res==42? " << std::boolalpha << (res == 42) << std::endl;

            // post a task, and interrupt it immediately
            boost::shared_future<boost::asynchronous::any_interruptible> interruptible_fu =
                    proxy.start_interruptible_async_work();
            boost::asynchronous::any_interruptible interruptible = interruptible_fu.get();
            interruptible.interrupt();

            // wait a bit for tasks to be interrupted
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

            // let's ask the sthreadpool scheduler what it did.
            // only void_async_work will be reported as interrupted
            boost::shared_future<diag_type> fu_diag = proxy.get_diagnostics();
            diag_type diag = fu_diag.get();
            std::cout << "Display of threadpool jobs" << std::endl;
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

            // let's ask the single-threaded scheduler what it did.
            diag_type single_thread_sched_diag = scheduler.get_diagnostics();
            // we expect 4 jobs: proxy::start_async_work, proxy::get_diagnostics and
            // int_async_work (the callback part of post_callback), and proxy::start_interruptible_async_work
            // no job will be interrupted as we stop the threadpool worker task, not the single-threaded one.
            std::cout << "Display of scheduler jobs" << std::endl;
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
    std::cout << "end example_log \n" << std::endl;
}



