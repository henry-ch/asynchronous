
// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/guarded_deque.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/notification/local_subscription.hpp>
#include <boost/asynchronous/helpers/recursive_future_get.hpp>

#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

namespace
{
// main thread id
boost::thread::id main_thread_id;
bool servant_dtor = false;
struct some_event
{
    int data = 0;
};

using string_topic = boost::asynchronous::subscription::channel_topic<>;

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
        servant_dtor = true;
    }
    std::future<std::tuple<int, std::string>> wait_for_some_event(std::string const& topic)
    {
        std::shared_ptr<std::promise<std::tuple<int, std::string>> > p(new std::promise<std::tuple<int, std::string>>);
        auto fu = p->get_future();
        boost::thread::id threadid = boost::this_thread::get_id();

        auto cb = [this, p = std::move(p), threadid](some_event const& e, string_topic const& published_topic)
        {
            BOOST_CHECK_MESSAGE(threadid == boost::this_thread::get_id(), "notification callback in wrong thread.");
            ++cb_called_;
            p->set_value(std::make_tuple(42, to_string(published_topic)));
        };

        this->subscribe(std::move(cb), string_topic{ topic });


        return fu;
    }

    void trigger(std::string const& topic)
    {
        this->publish(some_event{ 42 }, string_topic{ topic });
    }
    
    int cb_called()const
    {
        return cb_called_;
    }

    int cb_called_ = 0;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler, class Threadpool>
    ServantProxy(Scheduler s, Threadpool p):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s,p)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(trigger)
    BOOST_ASYNC_FUTURE_MEMBER(cb_called)
    BOOST_ASYNC_FUTURE_MEMBER(wait_for_some_event)

};

}

BOOST_AUTO_TEST_CASE( test_local_notification_topic_match )
{
    servant_dtor=false;
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::guarded_deque<>>>();
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
                                                                            boost::asynchronous::guarded_deque<>>>(3);
    {
        ServantProxy proxy(scheduler, pool);
        try
        {
            auto fu = proxy.wait_for_some_event("topics");
            proxy.trigger("topics/topic1");
            auto res = boost::asynchronous::recursive_future_get(std::move(fu));
            BOOST_CHECK_MESSAGE(std::get<int>(res) == 42, "invalid result");
            BOOST_CHECK_MESSAGE(std::get<std::string>(res) == "topics/topic1", "invalid result");

        }
        catch (...)
        {
            BOOST_FAIL("unexpected exception");
        }
    }
    // servant gone, check for removal
    auto wsched = scheduler.get_weak_scheduler();
    auto sched = wsched.lock();
    std::shared_ptr<std::promise<void> > p(new std::promise<void>);
    auto fu = p->get_future();
    if (sched.is_valid())
    {
        sched.post([p=std::move(p)]() mutable
            {
                //boost::asynchronous::subscription::publish(some_event{ 42 }, string_topic{ "topic1" });
                BOOST_CHECK_MESSAGE(boost::asynchronous::subscription::get_local_subscription_store_<some_event>().m_internal_subscribers.empty(), "local subscribers not removed");
                p->set_value();
            });
    }
    fu.get();

}

BOOST_AUTO_TEST_CASE(test_local_notification_topic_no_match)
{
    servant_dtor = false;
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
        boost::asynchronous::guarded_deque<>>>(3);
    {
        ServantProxy proxy(scheduler, pool);
        try
        {
            auto fu = proxy.wait_for_some_event("topic1");
            proxy.trigger("topic2");
            BOOST_CHECK_MESSAGE(proxy.cb_called().get() == 0, "got wrong number of events");
        }
        catch (...)
        {
            BOOST_FAIL("unexpected exception");
        }
    }
    // servant gone, check for removal
    auto wsched = scheduler.get_weak_scheduler();
    auto sched = wsched.lock();
    std::shared_ptr<std::promise<void> > p(new std::promise<void>);
    auto fu = p->get_future();
    if (sched.is_valid())
    {
        sched.post([p = std::move(p)]() mutable
            {
                //boost::asynchronous::subscription::publish(some_event{ 42 }, string_topic{ "topic1" });
                BOOST_CHECK_MESSAGE(boost::asynchronous::subscription::get_local_subscription_store_<some_event>().m_internal_subscribers.empty(), "local subscribers not removed");
                p->set_value();
            });
    }
    fu.get();
}

BOOST_AUTO_TEST_CASE(test_local_notification_topic_match_no_match)
{
    servant_dtor = false;
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
        boost::asynchronous::guarded_deque<>>>(3);
    {
        ServantProxy proxy(scheduler, pool);
        try
        {
            auto fu = proxy.wait_for_some_event("topic1");
            auto fu2 = proxy.wait_for_some_event("topic2");
            proxy.trigger("topic2");

            auto res = boost::asynchronous::recursive_future_get(std::move(fu2));
            BOOST_CHECK_MESSAGE(std::get<int>(res) == 42, "invalid result");
            BOOST_CHECK_MESSAGE(proxy.cb_called().get() == 1, "got wrong number of events");
        }
        catch (...)
        {
            BOOST_FAIL("unexpected exception");
        }
    }
    // servant gone, check for removal
    auto wsched = scheduler.get_weak_scheduler();
    auto sched = wsched.lock();
    std::shared_ptr<std::promise<void> > p(new std::promise<void>);
    auto fu = p->get_future();
    if (sched.is_valid())
    {
        sched.post([p = std::move(p)]() mutable
            {
                //boost::asynchronous::subscription::publish(some_event{ 42 }, string_topic{ "topic1" });
                BOOST_CHECK_MESSAGE(boost::asynchronous::subscription::get_local_subscription_store_<some_event>().m_internal_subscribers.empty(), "local subscribers not removed");
                p->set_value();
            });
    }
    fu.get();
}
