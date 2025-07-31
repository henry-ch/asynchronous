
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
#include <boost/thread/futures/wait_for_all.hpp>

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
// main thread id
boost::thread::id main_thread_id;
struct some_event
{
    int data = 0;
};

// helper to make sure a subscribe has completed
void wait_for_subscribe(auto& proxy)
{
    auto p = std::make_shared< std::promise<void>>();
    auto fu = p->get_future();
    proxy.post([p = std::move(p)]()mutable {p->set_value(); });
    fu.get();
}

// pure subscriber
struct Servant : boost::asynchronous::trackable_servant<>
{

    template <class Threadpool>
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler, Threadpool p)
        : boost::asynchronous::trackable_servant<>(scheduler,p)
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

        auto cb = [p = std::move(p), threadid, this](some_event const& e)
        {
            ++cb_called_;
            BOOST_CHECK_MESSAGE(threadid == boost::this_thread::get_id(), "notification callback in wrong thread.");
            p->set_value(42); 
        };
        token_ = this->subscribe(std::move(cb));

        return fu;
    }

    std::future <int> wait_for_some_event_single_shot()
    {
        std::shared_ptr<std::promise<int> > p(new std::promise<int>);
        auto fu = p->get_future();
        boost::thread::id threadid = boost::this_thread::get_id();

        auto cb = [p = std::move(p), threadid, this](some_event const& e) 
            {
                ++cb_called_;
                BOOST_CHECK_MESSAGE(threadid == boost::this_thread::get_id(), "notification callback in wrong thread.");
                p->set_value(42);
                // false => do not keep me subscribed
                return false;
            };
        token_ = this->subscribe(std::move(cb));

        return fu;
    }

    std::future<int> wait_for_some_event_self_unsubscribe()
    {
        std::shared_ptr<std::promise<int> > p(new std::promise<int>);
        auto fu = p->get_future();
        boost::thread::id threadid = boost::this_thread::get_id();

        auto token = std::make_shared< boost::asynchronous::subscription_token>();
        auto cb = [p = std::move(p), threadid, token, this](some_event const& e)mutable
            {
                ++cb_called_;
                BOOST_CHECK_MESSAGE(threadid == boost::this_thread::get_id(), "notification callback in wrong thread.");
                unsubscribe<some_event>(*token);
                p->set_value(42);
            };
        *token = this->subscribe(std::move(cb));

        return fu;
    }

    std::pair<std::future<int>, std::future<int>> wait_for_some_event_two_subscribe()
    {
        std::shared_ptr<std::promise<int> > p(new std::promise<int>);
        auto fu = p->get_future();
        std::shared_ptr<std::promise<int> > p2(new std::promise<int>);
        auto fu2 = p2->get_future();

        boost::thread::id threadid = boost::this_thread::get_id();

        auto cb2 = [p2 = std::move(p2), threadid, this](auto const& e)
            {
                ++cb_called_;
                BOOST_CHECK_MESSAGE(threadid == boost::this_thread::get_id(), "notification callback in wrong thread.");
                p2->set_value(42);
            };
        token2_ = this->subscribe<some_event>(std::move(cb2));

        auto cb = [p = std::move(p), threadid, this](some_event const& e)
            {
                ++cb_called_;
                BOOST_CHECK_MESSAGE(threadid == boost::this_thread::get_id(), "notification callback in wrong thread.");
                p->set_value(42);
            };
        token_ = this->subscribe(std::move(cb));

        return std::make_pair(std::move(fu),std::move(fu2));
    }

    void force_unsubscribe()
    {
        unsubscribe<some_event>(token_);
        unsubscribe<some_event>(token2_);
    }

    int cb_called()const
    {
        return cb_called_;
    }

    int cb_called_ = 0;
    boost::asynchronous::subscription_token token_;
    boost::asynchronous::subscription_token token2_;

};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler, class Threadpool>
    ServantProxy(Scheduler s, Threadpool p):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s,p)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(wait_for_some_event)
    BOOST_ASYNC_FUTURE_MEMBER(wait_for_some_event_single_shot)
    BOOST_ASYNC_FUTURE_MEMBER(wait_for_some_event_two_subscribe)
    BOOST_ASYNC_FUTURE_MEMBER(wait_for_some_event_self_unsubscribe)
    BOOST_ASYNC_FUTURE_MEMBER(force_unsubscribe)
    BOOST_ASYNC_FUTURE_MEMBER(cb_called)
};

// pure publisher
struct Servant2 : boost::asynchronous::trackable_servant<>
{

    template <class Threadpool>
    Servant2(boost::asynchronous::any_weak_scheduler<> scheduler, Threadpool p)
        : boost::asynchronous::trackable_servant<>(scheduler, p)
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
                auto wsched = boost::asynchronous::get_thread_scheduler<>();
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
        this->subscribe(std::move(cb));

        return fu;
    }

};
class ServantProxy2 : public boost::asynchronous::servant_proxy<ServantProxy2, Servant2>
{
public:
    template <class Scheduler, class Threadpool>
    ServantProxy2(Scheduler s, Threadpool p) :
        boost::asynchronous::servant_proxy<ServantProxy2, Servant2>(s, p)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(trigger_some_event)
    BOOST_ASYNC_FUTURE_MEMBER(trigger_some_event_in_threadpool)
    BOOST_ASYNC_FUTURE_MEMBER(wait_for_some_event)

};

}


BOOST_AUTO_TEST_CASE( test_full_notification )
{
    auto scheduler1 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::guarded_deque<>>>();
    auto scheduler2 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::guarded_deque<>>>();
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
                                                                            boost::asynchronous::guarded_deque<>>>(2);

    auto scheduler_notify = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::guarded_deque<>>>();
    auto notification_ptr = std::make_shared<boost::asynchronous::subscription::notification_proxy<>>
                                (scheduler_notify,pool);

    std::vector<std::future<void>> notification_futures;
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler1.get_weak_scheduler(), notification_ptr));
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler2.get_weak_scheduler(), notification_ptr));
    boost::wait_for_all(notification_futures.begin(), notification_futures.end());


    std::shared_ptr<ServantProxy> proxy = std::make_shared<ServantProxy>(scheduler1,pool);
    ServantProxy2 proxy2(scheduler2, pool);

    try
    {

        auto res_fu = proxy->wait_for_some_event().get();
        wait_for_subscribe(proxy2);
        proxy2.trigger_some_event().get();
        
        auto res = boost::asynchronous::recursive_future_get(std::move(res_fu));
        BOOST_CHECK_MESSAGE(res == 42, "invalid result");

        proxy->force_unsubscribe().get();

        // servant gone, check for removal
        wait_for_subscribe(proxy2);
        auto wsched = scheduler2.get_weak_scheduler();
        auto sched = wsched.lock();
        std::shared_ptr<std::promise<void> > p(new std::promise<void>);
        auto fu = p->get_future();
        if (sched.is_valid())
        {
            sched.post([p = std::move(p)]() mutable
                {
                    p->set_value();
                    BOOST_CHECK_MESSAGE(boost::asynchronous::subscription::get_local_subscription_store_<some_event>().m_scheduler_subscribers.empty(), "scheduler subscribers not removed");
                });
        }
        fu.get();

        BOOST_CHECK_MESSAGE(proxy->cb_called().get() == 1, "got wrong number of events");
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_full_notification2)
{
    auto scheduler1 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto scheduler2 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
        boost::asynchronous::guarded_deque<>>>(2);

    auto scheduler_notify = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto notification_ptr = std::make_shared<boost::asynchronous::subscription::notification_proxy<>>
        (scheduler_notify, pool);

    std::vector<std::future<void>> notification_futures;
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler2.get_weak_scheduler(), notification_ptr));
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler1.get_weak_scheduler(), notification_ptr));
    boost::wait_for_all(notification_futures.begin(), notification_futures.end());


    ServantProxy proxy(scheduler1, pool);
    ServantProxy2 proxy2(scheduler2, pool);

    try
    {
        auto res_fu = proxy.wait_for_some_event().get();
        wait_for_subscribe(proxy2);
        proxy2.trigger_some_event().get();

        auto res = boost::asynchronous::recursive_future_get(std::move(res_fu));
        BOOST_CHECK_MESSAGE(res == 42, "invalid result");
        BOOST_CHECK_MESSAGE(proxy.cb_called().get() == 1, "got wrong number of events");
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_full_notification_two_subscribe)
{
    auto scheduler1 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto scheduler2 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
        boost::asynchronous::guarded_deque<>>>(2);

    auto scheduler_notify = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto notification_ptr = std::make_shared<boost::asynchronous::subscription::notification_proxy<>>
        (scheduler_notify, pool);

    std::vector<std::future<void>> notification_futures;
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler1.get_weak_scheduler(), notification_ptr, true));
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler2.get_weak_scheduler(), notification_ptr, true));
    boost::wait_for_all(notification_futures.begin(), notification_futures.end());


    std::shared_ptr<ServantProxy> proxy = std::make_shared<ServantProxy>(scheduler1, pool);
    ServantProxy2 proxy2(scheduler2, pool);

    try
    {

        auto res_fu = proxy->wait_for_some_event_two_subscribe().get();
        proxy2.trigger_some_event().get();

        auto res = boost::asynchronous::recursive_future_get(std::move(res_fu.first));
        auto res2 = boost::asynchronous::recursive_future_get(std::move(res_fu.second));
        BOOST_CHECK_MESSAGE(res == 42, "invalid result");
        BOOST_CHECK_MESSAGE(res2 == 42, "invalid result2");

        proxy->force_unsubscribe().get();

        // servant gone, check for removal
        auto wsched = scheduler2.get_weak_scheduler();
        auto sched = wsched.lock();
        std::shared_ptr<std::promise<void> > p(new std::promise<void>);
        auto fu = p->get_future();
        if (sched.is_valid())
        {
            sched.post([p = std::move(p)]() mutable
                {
                    p->set_value();
                    BOOST_CHECK_MESSAGE(boost::asynchronous::subscription::get_local_subscription_store_<some_event>().m_scheduler_subscribers.empty(), "scheduler subscribers not removed");
                });
        }
        fu.get();
        BOOST_CHECK_MESSAGE(proxy->cb_called().get() == 2, "got wrong number of events");
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_full_notification_pub_and_sub)
{
    auto scheduler1 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto scheduler2 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();

    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
        boost::asynchronous::guarded_deque<>>>(2);

    auto scheduler_notify = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto notification_ptr = std::make_shared<boost::asynchronous::subscription::notification_proxy<>>
        (scheduler_notify, pool);


    std::vector<std::future<void>> notification_futures;
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler1.get_weak_scheduler(), notification_ptr));
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler2.get_weak_scheduler(), notification_ptr));
    boost::wait_for_all(notification_futures.begin(), notification_futures.end());


    ServantProxy proxy(scheduler1, pool);
    ServantProxy2 proxy2(scheduler2, pool);

    try
    {

        auto res_fu = proxy.wait_for_some_event().get();
        auto res_fu2 = proxy2.wait_for_some_event().get();
        wait_for_subscribe(proxy);
        wait_for_subscribe(proxy2);
        proxy2.trigger_some_event().get();

        auto res = boost::asynchronous::recursive_future_get(std::move(res_fu));
        BOOST_CHECK_MESSAGE(res == 42, "invalid result");
        auto res2 = boost::asynchronous::recursive_future_get(std::move(res_fu2));
        BOOST_CHECK_MESSAGE(res2 == 42, "invalid result");
        BOOST_CHECK_MESSAGE(proxy.cb_called().get() == 1, "got wrong number of events");
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_full_notification_multiple_subs)
{
    auto scheduler1 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto scheduler2 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto scheduler3 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();

    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
        boost::asynchronous::guarded_deque<>>>(2);

    auto scheduler_notify = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto notification_ptr = std::make_shared<boost::asynchronous::subscription::notification_proxy<>>
        (scheduler_notify, pool);


    std::vector<std::future<void>> notification_futures;
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler1.get_weak_scheduler(), notification_ptr, true));
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler2.get_weak_scheduler(), notification_ptr, true));
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler3.get_weak_scheduler(), notification_ptr, true));
    boost::wait_for_all(notification_futures.begin(), notification_futures.end());

    ServantProxy proxy(scheduler1, pool);
    ServantProxy2 proxy2(scheduler2, pool);
    ServantProxy proxy3(scheduler3, pool);

    try
    {

        auto res_fu = proxy.wait_for_some_event().get();
        auto res_fu3 = proxy3.wait_for_some_event().get();;
        proxy2.trigger_some_event().get();

        auto res = boost::asynchronous::recursive_future_get(std::move(res_fu));
        BOOST_CHECK_MESSAGE(res == 42, "invalid result");
        auto res3 = boost::asynchronous::recursive_future_get(std::move(res_fu3));
        BOOST_CHECK_MESSAGE(res3 == 42, "invalid result");
        BOOST_CHECK_MESSAGE(proxy.cb_called().get() == 1, "got wrong number of events");
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_full_notification_threadpool)
{
    auto scheduler1 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto scheduler2 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
        boost::asynchronous::guarded_deque<>>>(4);

    auto scheduler_notify = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto notification_ptr = std::make_shared<boost::asynchronous::subscription::notification_proxy<>>
        (scheduler_notify, pool);


    std::vector<std::future<void>> notification_futures;
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler1.get_weak_scheduler(), notification_ptr, true));
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler2.get_weak_scheduler(), notification_ptr, true));
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(pool.get_weak_scheduler(), notification_ptr, true));
    boost::wait_for_all(notification_futures.begin(), notification_futures.end());

    ServantProxy proxy(scheduler1, pool);
    ServantProxy2 proxy2(scheduler2, pool);

    try
    {

        auto res_fu = proxy.wait_for_some_event().get();
        proxy2.trigger_some_event_in_threadpool().get();

        auto res = boost::asynchronous::recursive_future_get(std::move(res_fu));
        BOOST_CHECK_MESSAGE(res == 42, "invalid result");
        BOOST_CHECK_MESSAGE(proxy.cb_called().get() == 1, "got wrong number of events");
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_full_notification_multiple_notification_buses)
{
    auto scheduler1 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto scheduler1bis = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto scheduler2 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
        boost::asynchronous::guarded_deque<>>>(2);

    auto scheduler_notify = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto notification_ptr = std::make_shared<boost::asynchronous::subscription::notification_proxy<>>
        (scheduler_notify, pool);
    auto notification2_ptr = std::make_shared<boost::asynchronous::subscription::notification_proxy<>>
        (scheduler_notify, pool);

    std::vector<std::future<void>> notification_futures;
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler1.get_weak_scheduler(), notification_ptr, true));
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler2.get_weak_scheduler(), notification_ptr, true));
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler1bis.get_weak_scheduler(), notification2_ptr, true));
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler2.get_weak_scheduler(), notification2_ptr, true));
    boost::wait_for_all(notification_futures.begin(), notification_futures.end());

    std::shared_ptr<ServantProxy> proxy = std::make_shared<ServantProxy>(scheduler1, pool);
    std::shared_ptr<ServantProxy> another_sub_proxy = std::make_shared<ServantProxy>(scheduler1bis, pool);
    ServantProxy2 proxy2(scheduler2, pool);

    try
    {

        auto res_fu = proxy->wait_for_some_event().get();
        auto res_fu2 = another_sub_proxy->wait_for_some_event().get();
        wait_for_subscribe(proxy2);
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
            sched.post([p = std::move(p)]() mutable
                {
                    p->set_value();
                    BOOST_CHECK_MESSAGE(boost::asynchronous::subscription::get_local_subscription_store_<some_event>().m_scheduler_subscribers.empty(), "scheduler subscribers not removed");
                });
        }
        fu.get();
        BOOST_CHECK_MESSAGE(proxy->cb_called().get() == 1, "got wrong number of events");
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_full_notification_auto_unsubscribe)
{
    auto scheduler1 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto scheduler2 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
        boost::asynchronous::guarded_deque<>>>(2);

    auto scheduler_notify = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto notification_ptr = std::make_shared<boost::asynchronous::subscription::notification_proxy<>>
        (scheduler_notify, pool);

    std::vector<std::future<void>> notification_futures;
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler1.get_weak_scheduler(), notification_ptr,true));
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler2.get_weak_scheduler(), notification_ptr,true));
    boost::wait_for_all(notification_futures.begin(), notification_futures.end());


    std::shared_ptr<ServantProxy> proxy = std::make_shared<ServantProxy>(scheduler1, pool);
    ServantProxy2 proxy2(scheduler2, pool);

    try
    {

        auto res_fu = proxy->wait_for_some_event_single_shot().get();
        proxy2.trigger_some_event().get();
        proxy2.trigger_some_event().get();

        auto res = boost::asynchronous::recursive_future_get(std::move(res_fu));
        BOOST_CHECK_MESSAGE(res == 42, "invalid result");

        // servant gone, check for removal
        wait_for_subscribe(proxy2);
        auto wsched = scheduler1.get_weak_scheduler();
        auto sched = wsched.lock();
        std::shared_ptr<std::promise<void> > p(new std::promise<void>);
        auto fu = p->get_future();
        if (sched.is_valid())
        {
            sched.post([p = std::move(p)]() mutable
                {
                    p->set_value();
                    BOOST_CHECK_MESSAGE(boost::asynchronous::subscription::get_local_subscription_store_<some_event>().m_internal_subscribers.empty(), "internal subscribers not removed");
                });
        }
        fu.get();

        BOOST_CHECK_MESSAGE(proxy->cb_called().get() == 1, "got wrong number of events");
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_full_notification_self_unsubscribe)
{
    auto scheduler1 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto scheduler2 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
        boost::asynchronous::guarded_deque<>>>(2);

    auto scheduler_notify = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto notification_ptr = std::make_shared<boost::asynchronous::subscription::notification_proxy<>>
        (scheduler_notify, pool);

    std::vector<std::future<void>> notification_futures;
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler1.get_weak_scheduler(), notification_ptr, true));
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler2.get_weak_scheduler(), notification_ptr, true));
    boost::wait_for_all(notification_futures.begin(), notification_futures.end());


    std::shared_ptr<ServantProxy> proxy = std::make_shared<ServantProxy>(scheduler1, pool);
    ServantProxy2 proxy2(scheduler2, pool);

    try
    {

        auto res_fu = proxy->wait_for_some_event_self_unsubscribe().get();
        proxy2.trigger_some_event().get();

        auto res = boost::asynchronous::recursive_future_get(std::move(res_fu));
        BOOST_CHECK_MESSAGE(res == 42, "invalid result");

        // servant gone, check for removal
        wait_for_subscribe(proxy2);
        auto wsched = scheduler1.get_weak_scheduler();
        auto sched = wsched.lock();
        std::shared_ptr<std::promise<void> > p(new std::promise<void>);
        auto fu = p->get_future();
        if (sched.is_valid())
        {
            sched.post([p = std::move(p)]() mutable
                {
                    p->set_value();
                    BOOST_CHECK_MESSAGE(boost::asynchronous::subscription::get_local_subscription_store_<some_event>().m_internal_subscribers.empty(), "internal subscribers not removed");
                });
        }
        fu.get();

        BOOST_CHECK_MESSAGE(proxy->cb_called().get() == 1, "got wrong number of events");
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_full_notification_trigger_after_unsubscribe)
{
    auto scheduler1 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto scheduler2 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
        boost::asynchronous::guarded_deque<>>>(2);

    auto scheduler_notify = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto notification_ptr = std::make_shared<boost::asynchronous::subscription::notification_proxy<>>
        (scheduler_notify, pool);

    std::vector<std::future<void>> notification_futures;
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler1.get_weak_scheduler(), notification_ptr,true));
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler2.get_weak_scheduler(), notification_ptr, true));
    boost::wait_for_all(notification_futures.begin(), notification_futures.end());


    std::shared_ptr<ServantProxy> proxy = std::make_shared<ServantProxy>(scheduler1, pool);
    ServantProxy2 proxy2(scheduler2, pool);

    try
    {

        auto res_fu = proxy->wait_for_some_event().get();
        wait_for_subscribe(proxy2);
        proxy2.trigger_some_event().get();

        auto res = boost::asynchronous::recursive_future_get(std::move(res_fu));
        BOOST_CHECK_MESSAGE(res == 42, "invalid result");

        proxy->force_unsubscribe().get();
        proxy2.trigger_some_event().get();

        // servant gone, check for removal

        auto wsched = scheduler2.get_weak_scheduler();
        auto sched = wsched.lock();
        std::shared_ptr<std::promise<void> > p(new std::promise<void>);
        auto fu = p->get_future();
        if (sched.is_valid())
        {
            sched.post([p = std::move(p)]() mutable
                {
                    p->set_value();
                    BOOST_CHECK_MESSAGE(boost::asynchronous::subscription::get_local_subscription_store_<some_event>().m_scheduler_subscribers.empty(), "scheduler subscribers not removed");
                });
        }
        fu.get();

        BOOST_CHECK_MESSAGE(proxy->cb_called().get() == 1, "got wrong number of events");
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

//this weird test case checks for the case where on some system, thread ids are recycled
BOOST_AUTO_TEST_CASE(test_full_notification_reused_thread_ids)
{
    auto scheduler1_ = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto scheduler1 = std::make_shared<decltype(scheduler1_)>(std::move(scheduler1_));

    auto scheduler2 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
        boost::asynchronous::guarded_deque<>>>(2);

    auto scheduler_notify = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto notification_ptr = std::make_shared<boost::asynchronous::subscription::notification_proxy<>>
        (scheduler_notify, pool);

    std::vector<std::future<void>> notification_futures;
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler1->get_weak_scheduler(), notification_ptr));
    notification_futures.emplace_back(boost::asynchronous::subscription::register_scheduler_to_notification(scheduler2.get_weak_scheduler(), notification_ptr));
    boost::wait_for_all(notification_futures.begin(), notification_futures.end());


    std::shared_ptr<ServantProxy> proxy = std::make_shared<ServantProxy>(*scheduler1, pool);
    ServantProxy2 proxy2(scheduler2, pool);

    try
    {

        auto res_fu = proxy->wait_for_some_event().get();
        wait_for_subscribe(proxy2);
        proxy2.trigger_some_event().get();

        auto res = boost::asynchronous::recursive_future_get(std::move(res_fu));
        BOOST_CHECK_MESSAGE(res == 42, "invalid result");
        proxy->force_unsubscribe().get();


        proxy.reset();
        scheduler1.reset();
        auto scheduler1_bis = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
            boost::asynchronous::guarded_deque<>>>();
        boost::asynchronous::subscription::register_scheduler_to_notification(scheduler1_bis.get_weak_scheduler(), notification_ptr).get();
        proxy = std::make_shared<ServantProxy>(scheduler1_bis, pool);

        auto res_fu2 = proxy->wait_for_some_event().get();
        wait_for_subscribe(proxy2);
        proxy2.trigger_some_event().get();

        auto res2 = boost::asynchronous::recursive_future_get(std::move(res_fu2));
        BOOST_CHECK_MESSAGE(res2 == 42, "invalid result");


        BOOST_CHECK_MESSAGE(proxy->cb_called().get() == 1, "got wrong number of events");
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}
