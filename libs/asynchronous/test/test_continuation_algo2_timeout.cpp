
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
        BOOST_CHECK_MESSAGE(contains_id(tpids.begin(),tpids.end(),boost::this_thread::get_id()),"sub_task executed in the wrong thread");
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"sub_task in main thread.");
        boost::this_thread::sleep(boost::posix_time::milliseconds(m_sleep_time));
        return 1;
    }
    int m_sleep_time;
};

struct Servant : boost::asynchronous::trackable_servant<>
{
    // optional, ctor is simple enough not to be posted
    typedef int simple_ctor;
    // optional, dtor is simple enough not to be waited for (no complicated dependency to other servants' schedulers)
    typedef int simple_dtor;

    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               // threadpool and a simple lockfree_queue
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::multiqueue_threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<>>>(6))
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
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        tpids = tp.thread_ids();
        // start long tasks in threadpool
        std::vector<boost::future<int> > fus;
        boost::future<int> fu1 = boost::asynchronous::post_future(get_worker(),sub_task(10));
        fus.emplace_back(std::move(fu1));
        boost::future<int> fu2 = boost::asynchronous::post_future(get_worker(),sub_task(3000));
        fus.emplace_back(std::move(fu2));
        boost::future<int> fu3 = boost::asynchronous::post_future(get_worker(),sub_task(3000));
        fus.emplace_back(std::move(fu3));

        boost::asynchronous::create_continuation_timeout(
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
                    boost::chrono::milliseconds(1500),
                    // future results of recursive tasks
                    std::move(fus));
        return fu;
    }
private:
// for testing
std::shared_ptr<boost::promise<long> > m_promise;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    // caller will get a future
#ifndef _MSC_VER
    BOOST_ASYNC_FUTURE_MEMBER(calc_algo)
#else
    BOOST_ASYNC_FUTURE_MEMBER_1(calc_algo)
#endif
};

}

BOOST_AUTO_TEST_CASE( test_continuation_algo2_timeout )
{
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                              boost::asynchronous::lockfree_queue<>>>();
        {
            ServantProxy proxy(scheduler);
            boost::shared_future<boost::shared_future<long> > fu = proxy.calc_algo();
            boost::shared_future<long> resfu = fu.get();
            long res = resfu.get();
            BOOST_CHECK_MESSAGE(1 == res,"we didn't get the expected number of subtasks");
        }
    }
}

