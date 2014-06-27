
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

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include "test_common.hpp"

#include <boost/test/unit_test.hpp>

namespace
{
bool foo_servant3_called=false;
int test_data=0;

struct Servant3 : boost::asynchronous::trackable_servant<>
{
    Servant3(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler)
    {
    }
    void foo()
    {
       foo_servant3_called = true;
    }
};
class ServantProxy3 : public boost::asynchronous::servant_proxy<ServantProxy3,Servant3>
{
public:
    template <class Scheduler>
    ServantProxy3(Scheduler s, boost::future<boost::shared_ptr<Servant3> > servant):
        boost::asynchronous::servant_proxy<ServantProxy3,Servant3>(s, std::move(servant))
    {}
    BOOST_ASYNC_FUTURE_MEMBER(foo)
};

struct Servant2 : boost::asynchronous::trackable_servant<>
{
    Servant2(boost::asynchronous::any_weak_scheduler<> scheduler, boost::asynchronous::any_shared_scheduler_proxy<> servant1_scheduler)
      : boost::asynchronous::trackable_servant<>(scheduler)
      , m_servant1_scheduler(servant1_scheduler)
    {

    }
    void foo(boost::future<boost::shared_ptr<Servant3>> s)
    {
        // create a servant3 in the same thread as servant1
        ServantProxy3 servant(m_servant1_scheduler,std::move(s));
        servant.foo().get();
    }
    void foo2(int& i)
    {
        test_data = i;
    }
    boost::asynchronous::any_shared_scheduler_proxy<> m_servant1_scheduler;
};
class ServantProxy2 : public boost::asynchronous::servant_proxy<ServantProxy2,Servant2 >
{
public:
    template <class Scheduler,class Scheduler2>
    ServantProxy2(Scheduler s, Scheduler2 s2):
        boost::asynchronous::servant_proxy<ServantProxy2,Servant2 >(s,s2)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(foo)
    BOOST_ASYNC_FUTURE_MEMBER(foo2)
};

struct Servant1 : boost::asynchronous::trackable_servant<>
{
    Servant1(boost::asynchronous::any_weak_scheduler<> scheduler, ServantProxy2 sink)
      : boost::asynchronous::trackable_servant<>(scheduler)
      , m_sink(sink)
    {

    }
    boost::future<void> foo()
    {
        int i=5;
        m_sink.foo2(boost::ref(i)).get();

        boost::shared_ptr<Servant3> s3 (boost::make_shared<Servant3>(get_scheduler()));
        boost::promise<boost::shared_ptr<Servant3>> p;
        boost::future<boost::shared_ptr<Servant3>> fu = p.get_future();
        p.set_value(s3);
        return m_sink.foo(std::move(fu));
    }

    ServantProxy2 m_sink;
};

class ServantProxy1 : public boost::asynchronous::servant_proxy<ServantProxy1,Servant1 >
{
public:
    template <class Scheduler>
    ServantProxy1(Scheduler s, ServantProxy2 sink):
        boost::asynchronous::servant_proxy<ServantProxy1,Servant1 >(s,sink)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(foo)
};

struct outer_owner
{
    outer_owner()
        : m_servant1_scheduler(boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::single_thread_scheduler<
                                                                                                boost::asynchronous::lockfree_queue<> >))
        , m_servant2(boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::single_thread_scheduler<
                                                                        boost::asynchronous::lockfree_queue<> >),
                     m_servant1_scheduler)
        , m_servant1(m_servant1_scheduler,m_servant2)
    {

    }
    void test()
    {
        m_servant1.foo().get().get();
    }

    boost::asynchronous::any_shared_scheduler_proxy<> m_servant1_scheduler;
    ServantProxy2 m_servant2;
    ServantProxy1 m_servant1;

};

}

BOOST_AUTO_TEST_CASE( test_interconnected_servants )
{
    try
    {
        outer_owner owner;
        owner.test();
        BOOST_CHECK_MESSAGE(foo_servant3_called,"foo_servant3_called false");
        BOOST_CHECK_MESSAGE(test_data==5,"test_data not == 5");
    }
    catch(...)
    {
        BOOST_FAIL( "unexpected exception" );
    }
}

