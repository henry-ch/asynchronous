
// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <iostream>

#include <boost/thread/future.hpp>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/guarded_deque.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/notification/local_subscription.hpp>
#include <boost/asynchronous/helpers/recursive_future_get.hpp>
#include <boost/asynchronous/notification/notification_proxy.hpp>

#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

namespace
{
    typedef boost::asynchronous::any_loggable servant_job;
    typedef std::map<std::string, std::list<boost::asynchronous::diagnostic_item> > diag_type;

// main thread id
boost::thread::id main_thread_id;
struct some_event
{
    int data = 0;
};

// pure subscriber
struct Servant : boost::asynchronous::trackable_servant<servant_job, servant_job>
{

    template <class Threadpool>
    Servant(boost::asynchronous::any_weak_scheduler<servant_job> scheduler, Threadpool p)
        : boost::asynchronous::trackable_servant<servant_job, servant_job>(scheduler,p)
    {
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
    }
    std::future<int> wait_for_some_event()
    {
        std::shared_ptr<std::promise<int> > p(new std::promise<int>);
        auto fu = p->get_future();
        boost::thread::id threadid = boost::this_thread::get_id();

        auto cb = [p = std::move(p), threadid](some_event const& e)
        {
            BOOST_CHECK_MESSAGE(threadid == boost::this_thread::get_id(), "notification callback in wrong thread.");
            p->set_value(42); 
        };
        this->subscribe(std::move(cb),"ServantSubscribe",0);

        return fu;
    }
    void force_unsubscribe()
    {
        unsubscribe<some_event>();
    }
    
};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant, servant_job>
{
public:
    template <class Scheduler, class Threadpool>
    ServantProxy(Scheduler s, Threadpool p):
        boost::asynchronous::servant_proxy<ServantProxy,Servant, servant_job>(s,p)
    {}

    BOOST_ASYNC_SERVANT_POST_CTOR_LOG("ServantProxy::ctor", 1);
    BOOST_ASYNC_SERVANT_POST_DTOR_LOG("ServantProxy::dtor", 1);

    BOOST_ASYNC_FUTURE_MEMBER_LOG(wait_for_some_event,"wait_for_some_event",1)
    BOOST_ASYNC_FUTURE_MEMBER_LOG(force_unsubscribe,"force_unsubscribe",1)

};

// pure publisher
struct Servant2 : boost::asynchronous::trackable_servant<servant_job, servant_job>
{

    template <class Threadpool>
    Servant2(boost::asynchronous::any_weak_scheduler<servant_job> scheduler, Threadpool p)
        : boost::asynchronous::trackable_servant<servant_job, servant_job>(scheduler, p)
    {
    }
    ~Servant2()
    {
        BOOST_CHECK_MESSAGE(main_thread_id != boost::this_thread::get_id(), "servant dtor not posted.");
    }
    void trigger_some_event()
    {
        this->publish(some_event{ 42 });
    }
    void trigger_some_event_in_threadpool()
    {
        this->post_callback(
            []() {
                auto wsched = boost::asynchronous::get_thread_scheduler<servant_job>();
                auto sched = wsched.lock();
                if (sched.is_valid())
                {
                    sched.publish(some_event{ 42 });
                }
            },
            [](auto res) {},
            "trigger_some_event_in_threadpool",0
        );
    }
    std::future<int> wait_for_some_event()
    {
        std::shared_ptr<std::promise<int> > p(new std::promise<int>);
        auto fu = p->get_future();
        boost::thread::id threadid = boost::this_thread::get_id();

        auto cb = [p = std::move(p), threadid](some_event const& e)
            {
                BOOST_CHECK_MESSAGE(threadid == boost::this_thread::get_id(), "notification callback in wrong thread.");
                p->set_value(42);
            };
        this->subscribe(std::move(cb), "Servant2Subscribe", 0);

        return fu;
    }

};
class ServantProxy2 : public boost::asynchronous::servant_proxy<ServantProxy2, Servant2, servant_job>
{
public:
    template <class Scheduler, class Threadpool>
    ServantProxy2(Scheduler s, Threadpool p) :
        boost::asynchronous::servant_proxy<ServantProxy2, Servant2, servant_job>(s, p)
    {}
    BOOST_ASYNC_SERVANT_POST_CTOR_LOG("ServantProxy2::ctor", 1);
    BOOST_ASYNC_SERVANT_POST_DTOR_LOG("ServantProxy2::dtor", 1);


    BOOST_ASYNC_FUTURE_MEMBER_LOG(trigger_some_event,"trigger_some_event",1)
    BOOST_ASYNC_FUTURE_MEMBER_LOG(trigger_some_event_in_threadpool,"trigger_some_event_in_threadpool",1)
    BOOST_ASYNC_FUTURE_MEMBER_LOG(wait_for_some_event,"wait_for_some_event",1)

};

}


BOOST_AUTO_TEST_CASE( test_full_notification_log )
{
    auto scheduler1 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::guarded_deque<servant_job>>>();
    auto scheduler2 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::guarded_deque<servant_job>>>();
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
                                                                            boost::asynchronous::guarded_deque<servant_job>>>(2);

    auto scheduler_notify = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::guarded_deque<servant_job>>>();
    auto notification_ptr = std::make_shared<boost::asynchronous::subscription::notification_proxy<servant_job>>
                                (scheduler_notify,pool);


    boost::asynchronous::subscription::register_scheduler_to_notification(scheduler1.get_weak_scheduler(), notification_ptr);
    boost::asynchronous::subscription::register_scheduler_to_notification(scheduler2.get_weak_scheduler(), notification_ptr);

    std::shared_ptr<ServantProxy> proxy = std::make_shared<ServantProxy>(scheduler1,pool);
    ServantProxy2 proxy2(scheduler2, pool);

    try
    {

        auto res_fu = proxy->wait_for_some_event().get();
        proxy2.trigger_some_event().get();
        
        auto res = boost::asynchronous::recursive_future_get(std::move(res_fu));
        BOOST_CHECK_MESSAGE(res == 42, "invalid result");

        proxy->force_unsubscribe().get();


        // servant gone, check for removal
        auto wsched2 = scheduler2.get_weak_scheduler();
        auto sched2 = wsched2.lock();

        if (sched2.is_valid())
        {
            std::shared_ptr<std::promise<void> > p(new std::promise<void>);
            auto fu = p->get_future();

            auto fu2= boost::asynchronous::post_future(sched2,
                [p = std::move(p)]() mutable
                {
                    p->set_value();
                    BOOST_CHECK_MESSAGE(boost::asynchronous::subscription::local_subscription_store_<some_event>.m_scheduler_subscribers.empty(), "scheduler subscribers not removed");
                }, "check_local_removed", 0);

            fu.get();
            fu2.get();

            diag_type diag = scheduler2.get_diagnostics().totals();
            BOOST_CHECK_MESSAGE(!diag.empty(), "servant should have diagnostics.");
            for (auto mit = diag.begin(); mit != diag.end(); ++mit)
            {
                BOOST_CHECK_MESSAGE(!(*mit).first.empty(), "no job should have an empty name.");
                for (auto jit = (*mit).second.begin(); jit != (*mit).second.end(); ++jit)
                {
                    BOOST_CHECK_MESSAGE(std::chrono::nanoseconds((*jit).get_finished_time() - (*jit).get_started_time()).count() >= 0, "task finished before it started.");
                    BOOST_CHECK_MESSAGE(!(*jit).is_interrupted(), "no task should have been interrupted.");
                    BOOST_CHECK_MESSAGE(!(*jit).is_failed(), "no task should have failed.");
                }
            }
        }
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }

    diag_type diag = scheduler1.get_diagnostics().totals();
    BOOST_CHECK_MESSAGE(!diag.empty(), "servant should have diagnostics.");
    for (auto mit = diag.begin(); mit != diag.end(); ++mit)
    {
        BOOST_CHECK_MESSAGE(!(*mit).first.empty(), "no job should have an empty name.");
        for (auto jit = (*mit).second.begin(); jit != (*mit).second.end(); ++jit)
        {
            BOOST_CHECK_MESSAGE(std::chrono::nanoseconds((*jit).get_finished_time() - (*jit).get_started_time()).count() >= 0, "task finished before it started.");
            BOOST_CHECK_MESSAGE(!(*jit).is_interrupted(), "no task should have been interrupted.");
            BOOST_CHECK_MESSAGE(!(*jit).is_failed(), "no task should have failed.");
        }
    }
}

BOOST_AUTO_TEST_CASE(test_full_notification2_log)
{
    auto scheduler1 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<servant_job>>>();
    auto scheduler2 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<servant_job>>>();
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
        boost::asynchronous::guarded_deque<servant_job>>>(2);

    auto scheduler_notify = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<servant_job>>>();
    auto notification_ptr = std::make_shared<boost::asynchronous::subscription::notification_proxy<servant_job>>
        (scheduler_notify, pool);

    boost::asynchronous::subscription::register_scheduler_to_notification(scheduler2.get_weak_scheduler(), notification_ptr);
    boost::asynchronous::subscription::register_scheduler_to_notification(scheduler1.get_weak_scheduler(), notification_ptr);


    ServantProxy proxy(scheduler1, pool);
    ServantProxy2 proxy2(scheduler2, pool);

    try
    {
        auto res_fu = proxy.wait_for_some_event().get();
        proxy2.trigger_some_event().get();

        auto res = boost::asynchronous::recursive_future_get(std::move(res_fu));
        BOOST_CHECK_MESSAGE(res == 42, "invalid result");
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_full_notification_pub_and_sub_log)
{
    auto scheduler1 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<servant_job>>>();
    auto scheduler2 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<servant_job>>>();

    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
        boost::asynchronous::guarded_deque<servant_job>>>(2);

    auto scheduler_notify = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<servant_job>>>();
    auto notification_ptr = std::make_shared<boost::asynchronous::subscription::notification_proxy<servant_job>>
        (scheduler_notify, pool);


    boost::asynchronous::subscription::register_scheduler_to_notification(scheduler1.get_weak_scheduler(), notification_ptr);
    boost::asynchronous::subscription::register_scheduler_to_notification(scheduler2.get_weak_scheduler(), notification_ptr);

    ServantProxy proxy(scheduler1, pool);
    ServantProxy2 proxy2(scheduler2, pool);

    try
    {

        auto res_fu = proxy.wait_for_some_event().get();
        auto res_fu2 = proxy2.wait_for_some_event().get();
        proxy2.trigger_some_event().get();

        auto res = boost::asynchronous::recursive_future_get(std::move(res_fu));
        BOOST_CHECK_MESSAGE(res == 42, "invalid result");
        auto res2 = boost::asynchronous::recursive_future_get(std::move(res_fu2));
        BOOST_CHECK_MESSAGE(res2 == 42, "invalid result");

    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_full_notification_multiple_subs_log)
{
    auto scheduler1 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<servant_job>>>();
    auto scheduler2 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<servant_job>>>();
    auto scheduler3 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<servant_job>>>();

    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
        boost::asynchronous::guarded_deque<servant_job>>>(2);

    auto scheduler_notify = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<servant_job>>>();
    auto notification_ptr = std::make_shared<boost::asynchronous::subscription::notification_proxy<servant_job>>
        (scheduler_notify, pool);


    boost::asynchronous::subscription::register_scheduler_to_notification(scheduler1.get_weak_scheduler(), notification_ptr);
    boost::asynchronous::subscription::register_scheduler_to_notification(scheduler2.get_weak_scheduler(), notification_ptr);
    boost::asynchronous::subscription::register_scheduler_to_notification(scheduler3.get_weak_scheduler(), notification_ptr);

    ServantProxy proxy(scheduler1, pool);
    ServantProxy2 proxy2(scheduler2, pool);
    ServantProxy proxy3(scheduler3, pool);

    try
    {

        auto res_fu = proxy.wait_for_some_event().get();
        auto res_fu3 = proxy3.wait_for_some_event().get();
        proxy2.trigger_some_event().get();

        auto res = boost::asynchronous::recursive_future_get(std::move(res_fu));
        BOOST_CHECK_MESSAGE(res == 42, "invalid result");
        auto res3 = boost::asynchronous::recursive_future_get(std::move(res_fu3));
        BOOST_CHECK_MESSAGE(res3 == 42, "invalid result");

    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_full_notification_threadpool_log)
{
    auto scheduler1 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<servant_job>>>();
    auto scheduler2 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<servant_job>>>();
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
        boost::asynchronous::guarded_deque<servant_job>>>(2);

    auto scheduler_notify = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<servant_job>>>();
    auto notification_ptr = std::make_shared<boost::asynchronous::subscription::notification_proxy<servant_job>>
        (scheduler_notify, pool);


    boost::asynchronous::subscription::register_scheduler_to_notification(scheduler1.get_weak_scheduler(), notification_ptr);
    boost::asynchronous::subscription::register_scheduler_to_notification(scheduler2.get_weak_scheduler(), notification_ptr);
    boost::asynchronous::subscription::register_scheduler_to_notification(pool.get_weak_scheduler(), notification_ptr);

    ServantProxy proxy(scheduler1, pool);
    ServantProxy2 proxy2(scheduler2, pool);

    try
    {

        auto res_fu = proxy.wait_for_some_event().get();
        proxy2.trigger_some_event_in_threadpool().get();

        auto res = boost::asynchronous::recursive_future_get(std::move(res_fu));
        BOOST_CHECK_MESSAGE(res == 42, "invalid result");

    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_full_notification_multiple_notification_buses_log)
{
    auto scheduler1 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<servant_job>>>();
    auto scheduler1bis = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<servant_job>>>();
    auto scheduler2 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<servant_job>>>();
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
        boost::asynchronous::guarded_deque<servant_job>>>(2);

    auto scheduler_notify = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<servant_job>>>();
    auto notification_ptr = std::make_shared<boost::asynchronous::subscription::notification_proxy<servant_job>>
        (scheduler_notify, pool);
    auto notification2_ptr = std::make_shared<boost::asynchronous::subscription::notification_proxy<servant_job>>
        (scheduler_notify, pool);


    boost::asynchronous::subscription::register_scheduler_to_notification(scheduler1.get_weak_scheduler(), notification_ptr);
    boost::asynchronous::subscription::register_scheduler_to_notification(scheduler2.get_weak_scheduler(), notification_ptr);
    boost::asynchronous::subscription::register_scheduler_to_notification(scheduler1bis.get_weak_scheduler(), notification2_ptr);
    boost::asynchronous::subscription::register_scheduler_to_notification(scheduler2.get_weak_scheduler(), notification2_ptr);

    std::shared_ptr<ServantProxy> proxy = std::make_shared<ServantProxy>(scheduler1, pool);
    std::shared_ptr<ServantProxy> another_sub_proxy = std::make_shared<ServantProxy>(scheduler1bis, pool);
    ServantProxy2 proxy2(scheduler2, pool);

    try
    {

        auto res_fu = proxy->wait_for_some_event().get();
        auto res_fu2 = another_sub_proxy->wait_for_some_event().get();
        proxy2.trigger_some_event().get();

        auto res = boost::asynchronous::recursive_future_get(std::move(res_fu));
        auto res2 = boost::asynchronous::recursive_future_get(std::move(res_fu2));
        BOOST_CHECK_MESSAGE(res == 42, "invalid result");
        BOOST_CHECK_MESSAGE(res2 == 42, "invalid result");

        proxy->force_unsubscribe().get();
        another_sub_proxy->force_unsubscribe().get();

        // servant gone, check for removal
        auto wsched = scheduler2.get_weak_scheduler();
        auto sched = wsched.lock();
        std::shared_ptr<std::promise<void> > p(new std::promise<void>);
        auto fu = p->get_future();
        if (sched.is_valid())
        {
            boost::asynchronous::post_future(sched,
                [p = std::move(p)]() mutable
                {
                    p->set_value();
                    BOOST_CHECK_MESSAGE(boost::asynchronous::subscription::local_subscription_store_<some_event>.m_scheduler_subscribers.empty(), "scheduler subscribers not removed");
                }, "check_local_removed", 0);
        }
        fu.get();

    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}
