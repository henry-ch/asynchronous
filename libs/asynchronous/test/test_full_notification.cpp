
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
#include <boost/asynchronous/notification/notification_tls.hpp>
#include <boost/asynchronous/notification/local_subscription.hpp>

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

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler, class Threadpool>
    ServantProxy(Scheduler s, Threadpool p):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s,p)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(wait_for_some_event)

};

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
        servant_dtor = true;
    }
    void trigger_some_event()
    {
        this->publish(some_event{ 42 });
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

};

}


BOOST_AUTO_TEST_CASE( test_full_notification )
{
    servant_dtor=false;
    auto scheduler1 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::guarded_deque<>>>();
    auto scheduler2 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::guarded_deque<>>>();
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
                                                                            boost::asynchronous::guarded_deque<>>>(2);

    auto scheduler_notify = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::guarded_deque<>>>();
    auto notification_ptr = std::make_shared<boost::asynchronous::subscription::notification_proxy>
                                (scheduler_notify,pool);


    //boost::asynchronous::subscription::set_notification_tls(notification_ptr);
    boost::asynchronous::subscription::register_scheduler_to_notification(scheduler1.get_weak_scheduler(), notification_ptr);
    boost::asynchronous::subscription::register_scheduler_to_notification(scheduler2.get_weak_scheduler(), notification_ptr);

    ServantProxy proxy(scheduler1,pool);
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

BOOST_AUTO_TEST_CASE(test_full_notification2)
{
    servant_dtor = false;
    auto scheduler1 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto scheduler2 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
        boost::asynchronous::guarded_deque<>>>(2);

    auto scheduler_notify = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto notification_ptr = std::make_shared<boost::asynchronous::subscription::notification_proxy>
        (scheduler_notify, pool);

    //boost::asynchronous::subscription::set_notification_tls(notification_ptr);
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

