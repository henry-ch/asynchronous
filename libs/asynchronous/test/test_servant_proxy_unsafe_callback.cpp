
// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <vector>
#include <set>
#include <future>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>

#include <boost/test/unit_test.hpp>

namespace
{
// main thread id
boost::thread::id s1_thread_id;
boost::thread::id s2_thread_id;


std::promise<void> callback_called;
std::future<void> callback_called_fu=callback_called.get_future();
std::promise<void> callback_called2;
std::future<void> callback_called_fu2=callback_called2.get_future();

struct Servant1 : boost::asynchronous::trackable_servant<>
{
    Servant1(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<>>>(1))
    {
        s1_thread_id = boost::this_thread::get_id();
    }
    int do_it(int v)
    {
        BOOST_CHECK_MESSAGE(s1_thread_id == boost::this_thread::get_id(),"servant do_it in wrong thread.");
        return v;
    }
    void do_it()
    {
        BOOST_CHECK_MESSAGE(s1_thread_id == boost::this_thread::get_id(),"servant do_it in wrong thread.");
    }
};

class ServantProxy1 : public boost::asynchronous::servant_proxy<ServantProxy1,Servant1>
{
public:
    template <class Scheduler>
    ServantProxy1(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy1,Servant1>(s)
    {}
#ifndef _MSC_VER
    BOOST_ASYNC_MEMBER_UNSAFE_CALLBACK(do_it)
#else
    BOOST_ASYNC_MEMBER_UNSAFE_CALLBACK_1(do_it)
#endif
};

struct Servant2 : boost::asynchronous::trackable_servant<>
{
    Servant2(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<>>>(1))
        , m_contained(boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                      boost::asynchronous::lockfree_queue<>>>())
    {
        s2_thread_id = boost::this_thread::get_id();
    }
    void test()
    {
        BOOST_CHECK_MESSAGE(s2_thread_id == boost::this_thread::get_id(),"servant test in wrong thread.");
        m_contained.do_it(
            this->make_safe_callback([](boost::asynchronous::expected<int> res)
            {
                BOOST_CHECK_MESSAGE(res.get()==42,"servant1 returned wrong value.");
                BOOST_CHECK_MESSAGE(!res.has_exception(),"servant1 threw an exception.");
                callback_called.set_value();
            }),
            42);
    }
    void void_test()
    {
        BOOST_CHECK_MESSAGE(s2_thread_id == boost::this_thread::get_id(),"servant test in wrong thread.");
        m_contained.do_it(
            this->make_safe_callback([](boost::asynchronous::expected<void> res)
            {
                BOOST_CHECK_MESSAGE(!res.has_exception(),"servant1 threw an exception.");
                callback_called2.set_value();
            }));
    }
    ServantProxy1 m_contained;
};

class ServantProxy2 : public boost::asynchronous::servant_proxy<ServantProxy2,Servant2>
{
public:
    template <class Scheduler>
    ServantProxy2(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy2,Servant2>(s)
    {}
#ifndef _MSC_VER
    BOOST_ASYNC_FUTURE_MEMBER(test)
    BOOST_ASYNC_FUTURE_MEMBER(void_test)
#else
    BOOST_ASYNC_FUTURE_MEMBER_1(test)
    BOOST_ASYNC_FUTURE_MEMBER_1(void_test)
#endif
};

}

BOOST_AUTO_TEST_CASE( test_trackable_servant_proxy_unsafe_callback )
{
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        {
            ServantProxy2 proxy(scheduler);
            proxy.test();
            callback_called_fu.get();
        }
    }
}

BOOST_AUTO_TEST_CASE( test_trackable_servant_proxy_unsafe_callback_void )
{
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        {
            ServantProxy2 proxy(scheduler);
            proxy.void_test();
            callback_called_fu2.get();
        }
    }
}


