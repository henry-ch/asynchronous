#include <iostream>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/queue/any_queue_container.hpp>
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
typedef boost::asynchronous::any_loggable servant_job;
typedef std::map<std::string,std::list<boost::asynchronous::diagnostic_item> > diag_type;


struct Servant : boost::asynchronous::trackable_servant<servant_job,servant_job>
{
    typedef int requires_weak_scheduler;
    Servant(boost::asynchronous::any_weak_scheduler<servant_job> scheduler)
        : boost::asynchronous::trackable_servant<servant_job,servant_job>(scheduler,
                                               // a threadpool with 1 thread using a lockfree queue of capacity 100
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue< servant_job >>>(1))
        , m_promise(new std::promise<int>)
    {
    }

    std::future<int> start_async_work()
    {
        // for demonstration purpose
        auto fu = m_promise->get_future();
        // start long tasks
        post_callback(
               [](){return 42;}// work
                ,
               [this](boost::asynchronous::expected<int> res){
                   this->m_promise->set_value(res.get());
               },// callback functor.
               // task name for logging
               "int_async_work",
               // priority of the task itself (won't be used by threadpool_scheduler)
               2,
               // priority of the callback
               2
        );
        return fu;
    }
    // threadpool diagnostics
    diag_type get_diagnostics() const
    {
        return get_worker().get_diagnostics().totals();
    }

private:
// for demonstration purpose
std::shared_ptr<std::promise<int> > m_promise;
};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant,servant_job>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant,servant_job>(s)
    {}
    // we give ctor and dtor a task name
    BOOST_ASYNC_SERVANT_POST_CTOR_LOG("Servant ctor",3)
    BOOST_ASYNC_SERVANT_POST_DTOR_LOG("Servant dtor",4)
    // member name, task name and priority
    BOOST_ASYNC_FUTURE_MEMBER_LOG(start_async_work,"proxy::start_async_work",1)
    BOOST_ASYNC_FUTURE_MEMBER_LOG(get_diagnostics,"proxy::get_diagnostics",1)
};

}

void example_queue_container_log()
{
    std::cout << "example_queue_container_log" << std::endl;
    {
        // a scheduler with 1 threadsafe list, and 3 lockfree queues as work input queue
        // the queue indicates the job name (loggable job)
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                                boost::asynchronous::single_thread_scheduler<
                                        boost::asynchronous::any_queue_container<servant_job>>>
                                (boost::asynchronous::any_queue_container_config<boost::asynchronous::lockfree_queue<servant_job> >(1),
                                 boost::asynchronous::any_queue_container_config<boost::asynchronous::lockfree_queue<servant_job> >(3)
                                 );
        {
            ServantProxy proxy(scheduler);

            auto fu = proxy.start_async_work();
            auto resfu = fu.get();
            int res = resfu.get();
            std::cout << "res==42? " << std::boolalpha << (res == 42) << std::endl;

            // wait a bit for tasks to be finished
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

            auto fu_diag = proxy.get_diagnostics();
            diag_type diag = fu_diag.get();
            std::cout << "Display of worker jobs" << std::endl;
            for (auto mit = diag.begin(); mit != diag.end() ; ++mit)
            {
                std::cout << "job type: " << (*mit).first << std::endl;
                for (auto jit = (*mit).second.begin(); jit != (*mit).second.end();++jit)
                {
                    std::cout << "job waited in us: " << std::chrono::nanoseconds((*jit).get_started_time() - (*jit).get_posted_time()).count() / 1000 << std::endl;
                    std::cout << "job lasted in us: " << std::chrono::nanoseconds((*jit).get_finished_time() - (*jit).get_started_time()).count() / 1000 << std::endl;
                    std::cout << "job interrupted? "  << std::boolalpha << (*jit).is_interrupted() << std::endl;
                }
            }
        }
        // wait a bit for tasks to be finished
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

        diag_type single_thread_sched_diag = scheduler.get_diagnostics().totals();
        std::cout << "Display of scheduler jobs" << std::endl;
        for (auto mit = single_thread_sched_diag.begin(); mit != single_thread_sched_diag.end() ; ++mit)
        {
            std::cout << "job type: " << (*mit).first << std::endl;
            for (auto jit = (*mit).second.begin(); jit != (*mit).second.end();++jit)
            {
                std::cout << "job waited in us: " << std::chrono::nanoseconds((*jit).get_started_time() - (*jit).get_posted_time()).count() / 1000 << std::endl;
                std::cout << "job lasted in us: " << std::chrono::nanoseconds((*jit).get_finished_time() - (*jit).get_started_time()).count() / 1000 << std::endl;
                std::cout << "job interrupted? "  << std::boolalpha << (*jit).is_interrupted() << std::endl;
            }
        }

    }
    std::cout << "example_queue_container_log end" << std::endl;
}




