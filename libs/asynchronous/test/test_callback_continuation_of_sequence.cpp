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
#include <boost/asynchronous/any_continuation_task.hpp>
#include <boost/asynchronous/algorithm/parallel_reduce.hpp>

#include "test_common.hpp"
#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;
namespace
{
// main thread id
boost::thread::id main_thread_id;
std::vector<boost::thread::id> tpids;

struct sub_task : public boost::asynchronous::continuation_task<long>
{
    sub_task(long val):m_val(val){}
    void operator()()
    {
        boost::asynchronous::continuation_result<long> task_res = this_task_result();
        task_res.set_value(m_val);
    }
    long m_val;
};
struct sub_task2 : public boost::asynchronous::continuation_task<long>
{
    sub_task2(long val):m_val(val){}
    void operator()()
    {
        boost::asynchronous::continuation_result<long> task_res = this_task_result();
        task_res.set_value(m_val);
    }
    long m_val;
};
struct sub_task3 : public boost::asynchronous::continuation_task<long>
{
    sub_task3(long val):m_val(val){}
    void operator()()
    {
        boost::asynchronous::continuation_result<long> task_res = this_task_result();
        task_res.set_value(m_val);
    }
    long m_val;
};
struct main_task : public boost::asynchronous::continuation_task<long>
{
    void operator()()
    {
        boost::asynchronous::continuation_result<long> task_res = this_task_result();
        std::vector<sub_task> subs;
        subs.push_back(sub_task(15));
        subs.push_back(sub_task(22));
        subs.push_back(sub_task(5));

        boost::asynchronous::create_callback_continuation(
                        [task_res](std::vector<boost::asynchronous::expected<long>> res)
                        {
                            long r = res[0].get() + res[1].get() + res[2].get();
                            task_res.set_value(r);
                        },
                        std::move(subs));
    }
};
struct main_task2 : public boost::asynchronous::continuation_task<long>
{
    void operator()()
    {
        boost::asynchronous::continuation_result<long> task_res = this_task_result();
        std::vector<boost::asynchronous::any_continuation_task<long>> subs;
        subs.push_back(sub_task(15));
        subs.push_back(sub_task2(22));
        subs.push_back(sub_task3(5));

        boost::asynchronous::create_callback_continuation(
                        [task_res](std::vector<boost::asynchronous::expected<long>> res)
                        {
                            long r = res[0].get() + res[1].get() + res[2].get();
                            task_res.set_value(r);
                        },
                        std::move(subs));
    }
};
struct main_task3 : public boost::asynchronous::continuation_task<long>
{
    void operator()()
    {
        boost::asynchronous::continuation_result<long> task_res = this_task_result();
        std::vector<boost::asynchronous::detail::callback_continuation<long>> subs;
        std::vector<long> data1(10000,1);
        std::vector<long> data2(10000,1);
        std::vector<long> data3(10000,1);
        subs.push_back(boost::asynchronous::parallel_reduce(std::move(data1),
                                                            [](long const& a, long const& b)
                                                            {
                                                              return a + b;
                                                            },1000));
        subs.push_back(boost::asynchronous::parallel_reduce(std::move(data2),
                                                            [](long const& a, long const& b)
                                                            {
                                                              return a + b;
                                                            },1000));
        subs.push_back(boost::asynchronous::parallel_reduce(std::move(data3),
                                                            [](long const& a, long const& b)
                                                            {
                                                              return a + b;
                                                            },1000));

        boost::asynchronous::create_callback_continuation(
                        [task_res](std::vector<boost::asynchronous::expected<long>> res)
                        {
                            long r = res[0].get() + res[1].get() + res[2].get();
                            task_res.set_value(r);
                        },
                        std::move(subs));
    }
};
struct main_task4 : public boost::asynchronous::continuation_task<long>
{
    void operator()()
    {
        boost::asynchronous::continuation_result<long> task_res = this_task_result();
        std::vector<boost::asynchronous::any_continuation_task<int>> subs;
        subs.push_back(boost::asynchronous::make_lambda_continuation_wrapper([](){return 15;}));
        subs.push_back(boost::asynchronous::make_lambda_continuation_wrapper([](){return 22;}));
        subs.push_back(boost::asynchronous::make_lambda_continuation_wrapper([](){return 5;}));

        boost::asynchronous::create_callback_continuation(
                        [task_res](std::vector<boost::asynchronous::expected<int>> res)
                        {
                            long r = res[0].get() + res[1].get() + res[2].get();
                            task_res.set_value(r);
                        },
                        std::move(subs));
    }
};

struct Servant : boost::asynchronous::trackable_servant<>
{
    // optional, ctor is simple enough not to be posted
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               // threadpool with 4 threads and a simple lockfree_queue queue
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::multiqueue_threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<>>>(4))
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
    boost::future<long> do_it()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant do_it not posted.");
        // for testing purpose
        boost::future<long> fu = m_promise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        tpids = tp.thread_ids();
        // start long tasks in threadpool (first lambda) and callback in our thread
        post_callback(
                []()
                {
                    BOOST_CHECK_MESSAGE(contains_id(tpids.begin(),tpids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::top_level_callback_continuation<long>(main_task());
                 }// work
               ,
               // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
               [this](boost::asynchronous::expected<long> res){
                            BOOST_CHECK_MESSAGE(!contains_id(tpids.begin(),tpids.end(),boost::this_thread::get_id()),"do_it callback executed in the wrong thread(pool)");
                            BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                            BOOST_CHECK_MESSAGE(res.has_value(),"callback has a blocking future.");
                            this->on_callback(res.get());
               }// callback functor.
        );
        return fu;
    }
    boost::future<long> do_it2()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant do_it not posted.");
        // for testing purpose
        boost::future<long> fu = m_promise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        tpids = tp.thread_ids();
        // start long tasks in threadpool (first lambda) and callback in our thread
        post_callback(
                []()
                {
                    BOOST_CHECK_MESSAGE(contains_id(tpids.begin(),tpids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::top_level_callback_continuation<long>(main_task2());
                 }// work
               ,
               // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
               [this](boost::asynchronous::expected<long> res){
                            BOOST_CHECK_MESSAGE(!contains_id(tpids.begin(),tpids.end(),boost::this_thread::get_id()),"do_it callback executed in the wrong thread(pool)");
                            BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                            BOOST_CHECK_MESSAGE(res.has_value(),"callback has a blocking future.");
                            this->on_callback(res.get());
               }// callback functor.
        );
        return fu;
    }
    boost::future<long> do_it3()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant do_it not posted.");
        // for testing purpose
        boost::future<long> fu = m_promise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        tpids = tp.thread_ids();
        // start long tasks in threadpool (first lambda) and callback in our thread
        post_callback(
                []()
                {
                    BOOST_CHECK_MESSAGE(contains_id(tpids.begin(),tpids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::top_level_callback_continuation<long>(main_task3());
                 }// work
               ,
               // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
               [this](boost::asynchronous::expected<long> res){
                            BOOST_CHECK_MESSAGE(!contains_id(tpids.begin(),tpids.end(),boost::this_thread::get_id()),"do_it callback executed in the wrong thread(pool)");
                            BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                            BOOST_CHECK_MESSAGE(res.has_value(),"callback has a blocking future.");
                            this->on_callback(res.get());
               }// callback functor.
        );
        return fu;
    }
    boost::future<long> do_it4()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant do_it not posted.");
        // for testing purpose
        boost::future<long> fu = m_promise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        tpids = tp.thread_ids();
        // start long tasks in threadpool (first lambda) and callback in our thread
        post_callback(
                []()
                {
                    BOOST_CHECK_MESSAGE(contains_id(tpids.begin(),tpids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::top_level_callback_continuation<long>(main_task4());
                 }// work
               ,
               // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
               [this](boost::asynchronous::expected<long> res){
                            BOOST_CHECK_MESSAGE(!contains_id(tpids.begin(),tpids.end(),boost::this_thread::get_id()),"do_it callback executed in the wrong thread(pool)");
                            BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                            BOOST_CHECK_MESSAGE(res.has_value(),"callback has a blocking future.");
                            this->on_callback(res.get());
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
#ifndef _MSC_VER
	// caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER_1(do_it)
    BOOST_ASYNC_FUTURE_MEMBER_1(do_it2)
    BOOST_ASYNC_FUTURE_MEMBER_1(do_it3)
    BOOST_ASYNC_FUTURE_MEMBER_1(do_it4)
#else
    // caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER_1(do_it)
    BOOST_ASYNC_FUTURE_MEMBER_1(do_it2)
    BOOST_ASYNC_FUTURE_MEMBER_1(do_it3)
    BOOST_ASYNC_FUTURE_MEMBER_1(do_it4)
#endif
};
}

BOOST_AUTO_TEST_CASE( test_callback_continuation_of_sequence )
{
    main_thread_id = boost::this_thread::get_id();
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        {
            ServantProxy proxy(scheduler);
            boost::future<boost::future<long> > fu = proxy.do_it();
            boost::future<long> resfu = fu.get();
            long res = resfu.get();
            BOOST_CHECK_MESSAGE(res == 42,"test_callback_continuation_of_sequence has wrong value");
        }
    }
}


BOOST_AUTO_TEST_CASE( test_callback_continuation_of_sequence_any_continuation_task )
{
    main_thread_id = boost::this_thread::get_id();
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        {
            ServantProxy proxy(scheduler);
            boost::future<boost::future<long> > fu = proxy.do_it2();
            boost::future<long> resfu = fu.get();
            long res = resfu.get();
            BOOST_CHECK_MESSAGE(res == 42,"test_callback_continuation_of_sequence_any_continuation_task has wrong value");
        }
    }
}

BOOST_AUTO_TEST_CASE( test_callback_continuation_of_sequence_continuation_task )
{
    main_thread_id = boost::this_thread::get_id();
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        {
            ServantProxy proxy(scheduler);
            boost::future<boost::future<long> > fu = proxy.do_it3();
            boost::future<long> resfu = fu.get();
            long res = resfu.get();
            BOOST_CHECK_MESSAGE(res == 30000,"test_callback_continuation_of_sequence_continuation_task has wrong value");
        }
    }
}

BOOST_AUTO_TEST_CASE( test_callback_continuation_of_sequence_any_continuation_task_lambda )
{
    main_thread_id = boost::this_thread::get_id();
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        {
            ServantProxy proxy(scheduler);
            boost::future<boost::future<long> > fu = proxy.do_it4();
            boost::future<long> resfu = fu.get();
            long res = resfu.get();
            BOOST_CHECK_MESSAGE(res == 42,"test_callback_continuation_of_sequence_any_continuation_task_lambda has wrong value");
        }
    }
}

