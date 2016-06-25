// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2016
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <vector>
#include <set>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>

#include <boost/test/unit_test.hpp>

namespace
{
// main thread id
boost::thread::id main_thread_id;
bool foo_called=false;

struct Base
{
    virtual void foo(){}
};

struct Servant : public Base
{
    Servant(int data): m_data(data)
    {
        BOOST_CHECK_MESSAGE(m_data==42,"servant got incorrect data in ctor. Expected 42.");
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant ctor not posted.");
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
    }
    void foo()
    {
        foo_called=true;
    }
    int m_data;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s, int data):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s, data)
    {}

    BOOST_ASYNC_FUTURE_MEMBER(foo)
};
class BaseServantProxy : public boost::asynchronous::servant_proxy<BaseServantProxy,Base>
{
public:
    template <class DerivedProxy>
    BaseServantProxy(DerivedProxy d):
        boost::asynchronous::servant_proxy<BaseServantProxy,Base>(d)
    {}

    BOOST_ASYNC_FUTURE_MEMBER(foo)
};
}

BOOST_AUTO_TEST_CASE( test_dynamic_servant_proxy_cast )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                    boost::asynchronous::lockfree_queue<>>>();

    main_thread_id = boost::this_thread::get_id();
    ServantProxy proxy(scheduler,42);
    BaseServantProxy base_proxy = boost::asynchronous::dynamic_servant_proxy_cast<BaseServantProxy>(proxy);
    auto fu = base_proxy.foo();
    fu.get();
    BOOST_CHECK_MESSAGE(foo_called,"virtual member not called");
}
