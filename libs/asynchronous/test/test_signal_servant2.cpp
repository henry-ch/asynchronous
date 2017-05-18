// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <iostream>
#include <future>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>

#include <boost/test/unit_test.hpp>

#include "test_common.hpp"

using namespace std;
using namespace boost::asynchronous::test;

namespace
{
typedef boost::signals2::signal<void (int)> signal_type;
// thread ids
boost::thread::id main_thread_id;
boost::thread::id servant1_thread_id;
boost::thread::id servant2_thread_id;

struct Servant : boost::asynchronous::trackable_servant<>
{
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler,int data)
        : boost::asynchronous::trackable_servant<>(scheduler)
        , m_data(data)
    {
        servant1_thread_id = boost::this_thread::get_id();
    }

    signal_type* get_signal()
    {
        return &m_int_signal;
    }
    void fire()
    {
        BOOST_CHECK_MESSAGE(m_data==42,"servant got wrong value.");
        m_int_signal(m_data);
    }

    int m_data;
    signal_type m_int_signal;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s, int data):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s, data)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(get_signal)
    BOOST_ASYNC_FUTURE_MEMBER(fire)
};

struct Servant2 : boost::asynchronous::trackable_servant<>
{

    Servant2(boost::asynchronous::any_weak_scheduler<> scheduler)
        :boost::asynchronous::trackable_servant<>(scheduler)
        ,m_data(0)
        ,m_future_value()
        ,m_sub(std::make_shared<ServantProxy>(boost::asynchronous::make_shared_scheduler_proxy<
                                                        boost::asynchronous::single_thread_scheduler<
                                                                boost::asynchronous::lockfree_queue<>>>(), 42))

    {
        servant2_thread_id = boost::this_thread::get_id();
        connect_signal();
    }
    void connect_signal()
    {
        signal_type* sig = m_sub->get_signal().get();
        safe_slot(*sig,[this](int i){int_slot(i);});
    }
    void int_slot(int i)
    {
        BOOST_CHECK_MESSAGE(i==42,"servant2 got wrong value.");
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant2 called in main thread.");
        BOOST_CHECK_MESSAGE(servant1_thread_id!=boost::this_thread::get_id(),"servant2 called in servant1 thread.");
        BOOST_CHECK_MESSAGE(servant2_thread_id==boost::this_thread::get_id(),"servant2 not called in its thread.");
        m_data = i;
        m_future_value.set_value(42);
    }
    void fire()
    {
        m_sub->fire();
    }

    std::future<int> get_data()
    {
        return m_future_value.get_future();
    }

private:
    int m_data;
    std::promise<int> m_future_value;
    std::shared_ptr<ServantProxy> m_sub;
};

class ServantProxy2 : public boost::asynchronous::servant_proxy<ServantProxy2,Servant2>
{
public:
    template <class Scheduler>
    ServantProxy2(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy2,Servant2>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(get_data)
    BOOST_ASYNC_FUTURE_MEMBER(fire)
};

}

BOOST_AUTO_TEST_CASE( test_signal_servant2 )
{
    {
        main_thread_id = boost::this_thread::get_id();
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                    boost::asynchronous::single_thread_scheduler<
                          boost::asynchronous::lockfree_queue<>>>();

        {
            ServantProxy2 proxy2(scheduler);
            proxy2.fire();
            int res = proxy2.get_data().get().get();
            BOOST_CHECK_MESSAGE(res==42,"test_signal_servant got wrong value.");
        }
    }
}



