
// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

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
    std::future<int> trigger()
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

        this->publish(some_event{42});


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
    BOOST_ASYNC_FUTURE_MEMBER(trigger)

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

    auto fus = scheduler1.execute_in_all_threads(
        [notification_ptr]() 
        {
            boost::asynchronous::subscription::set_notification_tls(notification_ptr); 
            auto wsched = boost::asynchronous::get_thread_scheduler<>();
            auto sched = wsched.lock();
            if (sched.is_valid())
            {
                notification_ptr->subscribe_subscriber_updates(sched.get_scheduler_event_update_callback());
            }
        });
    auto fus2 = scheduler2.execute_in_all_threads(
        [notification_ptr]()
        {
            boost::asynchronous::subscription::set_notification_tls(notification_ptr);
            auto wsched = boost::asynchronous::get_thread_scheduler<>();
            auto sched = wsched.lock();
            if (sched.is_valid())
            {
                notification_ptr->subscribe_subscriber_updates(sched.get_scheduler_event_update_callback());
            }
        });

    boost::wait_for_all(fus.begin(), fus.end());
    boost::wait_for_all(fus2.begin(), fus2.end());


    ServantProxy proxy(scheduler1,pool);
    try
    {
        auto res = boost::asynchronous::recursive_future_get(proxy.trigger());
        BOOST_CHECK_MESSAGE(res == 42, "invalid result");

    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

