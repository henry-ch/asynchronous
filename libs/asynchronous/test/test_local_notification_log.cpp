
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
typedef boost::asynchronous::any_loggable servant_job;
typedef std::map<std::string, std::list<boost::asynchronous::diagnostic_item> > diag_type;

// main thread id
boost::thread::id main_thread_id;
bool servant_dtor = false;
struct some_event
{
    int data = 0;
};
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

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant, servant_job>
{
public:
    template <class Scheduler, class Threadpool>
    ServantProxy(Scheduler s, Threadpool p):
        boost::asynchronous::servant_proxy<ServantProxy,Servant, servant_job>(s,p)
    {}
    BOOST_ASYNC_FUTURE_MEMBER_LOG(trigger,"trigger",1)

};

}

BOOST_AUTO_TEST_CASE( test_local_notification_log )
{
    servant_dtor=false;
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::guarded_deque<servant_job>>>();
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
                                                                            boost::asynchronous::guarded_deque<servant_job>>>(3);
    {
        ServantProxy proxy(scheduler, pool);
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
    // servant gone, check for removal
    auto wsched = scheduler.get_weak_scheduler();
    auto sched = wsched.lock();
    if (sched.is_valid())
    {
        std::shared_ptr<std::promise<void> > p(new std::promise<void>);
        auto fu = p->get_future();

        auto fu2 = boost::asynchronous::post_future(sched,
            [p = std::move(p)]() mutable
            {
                boost::asynchronous::subscription::publish_(some_event{ 42 });
                BOOST_CHECK_MESSAGE(boost::asynchronous::subscription::local_subscription_store_<some_event>.m_internal_subscribers.empty(), "local subscribers not removed");
                p->set_value();
            },"check_local_removed",0);

        fu.get();
        fu2.get();

        diag_type diag = scheduler.get_diagnostics().totals();
        BOOST_CHECK_MESSAGE(!diag.empty(), "servant should have diagnostics.");
        for (auto mit = diag.begin(); mit != diag.end(); ++mit)
        {
            for (auto jit = (*mit).second.begin(); jit != (*mit).second.end(); ++jit)
            {
                BOOST_CHECK_MESSAGE(std::chrono::nanoseconds((*jit).get_finished_time() - (*jit).get_started_time()).count() >= 0, "task finished before it started.");
                BOOST_CHECK_MESSAGE(!(*jit).is_interrupted(), "no task should have been interrupted.");
                BOOST_CHECK_MESSAGE(!(*jit).is_failed(), "no task should have failed.");
            }
        }


    }
}

