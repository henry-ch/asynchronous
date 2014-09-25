
#include <iostream>
#include <tuple>
#include <utility>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/threadsafe_list.hpp>
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
    int operator()()const
    {
        BOOST_CHECK_MESSAGE(contains_id(tpids.begin(),tpids.end(),boost::this_thread::get_id()),"sub_task executed in the wrong thread");
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"sub_task in main thread.");
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        return 1;
    }
};

// our main algo task. Needs to inherit continuation_task<value type returned by this task>
struct main_task : public boost::asynchronous::continuation_task<long>
{
    main_task(): boost::asynchronous::continuation_task<long>("main_task"){}
    main_task(main_task const&) = default;
    main_task(main_task&&) = default;


    void operator()()const
    {
        BOOST_CHECK_MESSAGE(contains_id(tpids.begin(),tpids.end(),boost::this_thread::get_id()),"main_task executed in the wrong thread");
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"main_task in main thread.");

        // the result of this task
        boost::asynchronous::continuation_result<long> task_res = this_task_result();

        // we start calculation, then while doing this we see new tasks which can be posted and done concurrently to us
        // when all are done, we will set the result
        // to post tasks, we need a scheduler
        boost::asynchronous::any_weak_scheduler<servant_job> weak_scheduler = boost::asynchronous::get_thread_scheduler<servant_job>();
        boost::asynchronous::any_shared_scheduler<servant_job> locked_scheduler = weak_scheduler.lock();
        if (!locked_scheduler.is_valid())
            // ok, we are shutting down, ok give up
            return;
        // simulate algo work
        std::vector<boost::future<int> > fus;
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        // let's say we just found a subtask
        boost::future<int> fu1 = boost::asynchronous::post_future(locked_scheduler,sub_task(),"sub_task_1");
        fus.emplace_back(std::move(fu1));
        // simulate more algo work
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        // let's say we just found a subtask
        boost::future<int> fu2 = boost::asynchronous::post_future(locked_scheduler,sub_task(),"sub_task_2");
        fus.emplace_back(std::move(fu2));
        // simulate algo work
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        // let's say we just found a subtask
        boost::future<int> fu3 = boost::asynchronous::post_future(locked_scheduler,sub_task(),"sub_task_3");
        fus.emplace_back(std::move(fu3));

        // our algo is now done, wrap all and return
        boost::asynchronous::create_continuation_job<servant_job>(
                    // called when subtasks are done, set our result
                    [task_res](std::vector<boost::future<int>> res)
                    {
                        long r = res[0].get() + res[1].get() + res[2].get();
                        task_res.set_value(r);
                    },
                    // future results of recursive tasks
                    std::move(fus));
    }
};

struct Servant : boost::asynchronous::trackable_servant<servant_job,servant_job>
{
    // optional, ctor is simple enough not to be posted
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<servant_job> scheduler)
        : boost::asynchronous::trackable_servant<servant_job,servant_job>(scheduler,
                                               // threadpool and a simple threadsafe_list queue
                                               boost::asynchronous::create_shared_scheduler_proxy(
                                                   new boost::asynchronous::multiqueue_threadpool_scheduler<
                                                           boost::asynchronous::threadsafe_list<servant_job> >(6)))
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
        // start long tasks in threadpool (first lambda) and callback in our thread
        post_callback(
                []()
                {
                     // a top-level continuation is the first one in a recursive serie.
                     // Its result will be passed to callback
                     return boost::asynchronous::top_level_continuation_job<long,servant_job>(main_task());
                 }// work
               ,
               // callback with result.
               [this](boost::asynchronous::expected<long> res){
                            BOOST_CHECK_MESSAGE(!contains_id(tpids.begin(),tpids.end(),boost::this_thread::get_id()),"algo callback executed in the wrong thread(pool)");
                            BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                            BOOST_CHECK_MESSAGE(res.has_value(),"callback has a blocking future.");
                            this->on_callback(res.get());
               },// callback functor.
               "calc_algo"
        );
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
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant,servant_job>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant,servant_job>(s)
    {}
    // caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER_LOG(calc_algo,"proxy::calc_algo")
    BOOST_ASYNC_FUTURE_MEMBER_LOG(get_diagnostics,"proxy::get_diagnostics")
};

}

BOOST_AUTO_TEST_CASE( test_continuation_algo_log2 )
{
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::threadsafe_list<servant_job> >);
        {
            ServantProxy proxy(scheduler);
            boost::shared_future<boost::shared_future<long> > fu = proxy.calc_algo();
            boost::shared_future<long> resfu = fu.get();
            long res = resfu.get();
            BOOST_CHECK_MESSAGE(3 == res,"we didn't get the expected number of subtasks");

            // check if we found all tasks
            boost::shared_future<diag_type> fu_diag = proxy.get_diagnostics();
            diag_type diag = fu_diag.get();

            bool found_calc_algo=false;
            bool found_main_task=false;
            bool found_sub_task1=false;
            bool found_sub_task2=false;
            bool found_sub_task3=false;

            for (auto mit = diag.begin(); mit != diag.end() ; ++mit)
            {
                if ((*mit).first == "calc_algo")
                {
                    found_calc_algo = true;
                }
                if ((*mit).first == "main_task")
                {
                    found_main_task = true;
                }
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
            BOOST_CHECK_MESSAGE(found_calc_algo,"calc_algo not called.");
            BOOST_CHECK_MESSAGE(found_main_task,"main_task not called.");
            BOOST_CHECK_MESSAGE(found_sub_task1,"sub_task1 not called.");
            BOOST_CHECK_MESSAGE(found_sub_task2,"sub_task2 not called.");
            BOOST_CHECK_MESSAGE(found_sub_task3,"sub_task3 not called.");

        }
    }
}



