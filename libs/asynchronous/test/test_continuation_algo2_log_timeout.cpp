
#include <iostream>
#include <tuple>
#include <utility>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/detail/any_continuation.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/continuation_task.hpp>

#include "test_common.hpp"
#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;
namespace
{
typedef boost::asynchronous::any_loggable<boost::chrono::high_resolution_clock> servant_job;
typedef std::map<std::string,std::list<boost::asynchronous::diagnostic_item<boost::chrono::high_resolution_clock> > > diag_type;

// main thread id
boost::thread::id main_thread_id;
std::vector<boost::thread::id> tpids;

// dummy task started by the main algorithm
// could be a lambda too of course
struct sub_task
{
    sub_task(int sleep_time)
        : m_sleep_time(sleep_time)
    {
    }

    int operator()()const
    {
        //std::cout << "sub_task in thread:" << boost::this_thread::get_id() << std::endl;
        BOOST_CHECK_MESSAGE(contains_id(tpids.begin(),tpids.end(),boost::this_thread::get_id()),"sub_task executed in the wrong thread");
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"sub_task in main thread.");
        boost::this_thread::sleep(boost::posix_time::milliseconds(m_sleep_time));
        return 1;
    }
    int m_sleep_time;
};
struct Servant : boost::asynchronous::trackable_servant<servant_job,servant_job>
{
    // optional, ctor is simple enough not to be posted
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<servant_job> scheduler)
        : boost::asynchronous::trackable_servant<servant_job,servant_job>(scheduler,
                                               // threadpool and a simple threadsafe_list queue
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::multiqueue_threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<servant_job>>>(6))
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
        boost::asynchronous::any_shared_scheduler_proxy<servant_job> tp =get_worker();
        tpids = tp.thread_ids();
        // start long tasks in threadpool
        std::vector<boost::future<int> > fus;
        boost::future<int> fu1 = boost::asynchronous::post_future(get_worker(),sub_task(100),"sub_task_1");
        fus.emplace_back(std::move(fu1));
        boost::future<int> fu2 = boost::asynchronous::post_future(get_worker(),sub_task(2000),"sub_task_2");
        fus.emplace_back(std::move(fu2));
        boost::future<int> fu3 = boost::asynchronous::post_future(get_worker(),sub_task(2000),"sub_task_3");
        fus.emplace_back(std::move(fu3));

        boost::asynchronous::create_continuation_job_timeout<servant_job>(
                    // called when subtasks are done, set our result
                    [this](std::vector<boost::future<int>> res)
                    {
                        BOOST_CHECK_MESSAGE(!contains_id(tpids.begin(),tpids.end(),boost::this_thread::get_id()),"algo callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");

                        BOOST_CHECK_MESSAGE(res[0].has_value(),"first task should be finished");
                        BOOST_CHECK_MESSAGE(!res[1].has_value(),"second task should not be finished");
                        BOOST_CHECK_MESSAGE(!res[2].has_value(),"third task should not be finished");
                        long r = 0;
                        if ( res[0].has_value())
                            r += res[0].get();
                        if ( res[1].has_value())
                            r += res[1].get();
                        if ( res[2].has_value())
                            r += res[2].get();
                        this->on_callback(r);
                    },
                    // timeout
                    boost::chrono::milliseconds(1000),
                    // future results of recursive tasks
                    std::move(fus));
        return fu;
    }
    // threadpool diagnostics
    diag_type get_diagnostics() const
    {
        return get_worker().get_diagnostics();
    }
private:
// for testing
boost::shared_ptr<boost::promise<long> > m_promise;
};
// we give 4s for destruction timeout as we're going to wait 2s for tasks to complete
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant,servant_job,4000>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant,servant_job,4000>(s)
    {}
    // caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER_LOG(calc_algo,"proxy::calc_algo")
    BOOST_ASYNC_FUTURE_MEMBER_LOG(get_diagnostics,"proxy::get_diagnostics")
};

}

BOOST_AUTO_TEST_CASE( test_continuation_algo2_log_timeout )
{
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<servant_job>>>();
        {
            ServantProxy proxy(scheduler);
            boost::shared_future<boost::shared_future<long> > fu = proxy.calc_algo();
            boost::shared_future<long> resfu = fu.get();
            long res = resfu.get();
            BOOST_CHECK_MESSAGE(1 == res,"we didn't get the expected number of subtasks");

            // check if we found all tasks
            boost::shared_future<diag_type> fu_diag = proxy.get_diagnostics();
            diag_type diag = fu_diag.get();

            bool found_sub_task1=false;
            bool found_sub_task2=false;
            bool found_sub_task3=false;

            for (auto mit = diag.begin(); mit != diag.end() ; ++mit)
            {
                if ((*mit).first == "sub_task_1")
                {
                    found_sub_task1 = true;
                }
                if ((*mit).first == "sub_task_2")
                {
                    found_sub_task2 = true;
                }
                if ((*mit).first == "sub_task_3")
                {
                    found_sub_task3 = true;
                }
                for (auto jit = (*mit).second.begin(); jit != (*mit).second.end();++jit)
                {
                    BOOST_CHECK_MESSAGE(!(*jit).is_interrupted(),"no task should have been interrupted.");
                }
            }
            // only this task is sure to be finished
            BOOST_CHECK_MESSAGE(found_sub_task1,"sub_task1 not called.");
            // this ones are not as we gave up
            BOOST_CHECK_MESSAGE(!found_sub_task2,"sub_task2 finished.");
            BOOST_CHECK_MESSAGE(!found_sub_task3,"sub_task3 finished.");

        }
    }
}


