
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
#include <boost/asynchronous/queue/any_queue_container.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>

#include <boost/test/unit_test.hpp>
 
namespace
{
// main thread id
boost::thread::id main_thread_id;

struct my_exception : virtual boost::exception, virtual std::exception
{
    virtual const char* what() const throw()
    {
        return "my_exception";
    }
};

struct Servant
{
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<>,int data): m_data(data)
    {
        BOOST_CHECK_MESSAGE(m_data==42,"servant got incorrect data in ctor. Expected 42.");
        BOOST_CHECK_MESSAGE(main_thread_id==boost::this_thread::get_id(),"servant ctor posted instead of being called directly.");
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
    }
    int doIt()const
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant doIt not posted.");
        return 5;
    }
    void foo(int& i)const
    {
        BOOST_CHECK_MESSAGE(i==3,"servant got incorrect data in foo. Expected 3.");
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant foo not posted.");
        i = 100;
    }
    void foobar(int i, char c)const
    {
        BOOST_CHECK_MESSAGE(i==1,"servant got incorrect data in foobar. Expected 1.");
        BOOST_CHECK_MESSAGE(c=='a',"servant got incorrect data in foobar. Expected 'a'.");
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant foobar not posted.");
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
    BOOST_ASYNC_POST_MEMBER(foobar)
    BOOST_ASYNC_FUTURE_MEMBER(doIt)
};

struct Servant2
{
    Servant2()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant ctor not posted.");
    }
    void wait_end_test()
    {
        BOOST_TEST_CHECKPOINT( "wait_end_test" );
    }
    void generate_exception()
    {
        BOOST_THROW_EXCEPTION( my_exception());
    }
    ~Servant2()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
    }
};
class ServantProxy2 : public boost::asynchronous::servant_proxy<ServantProxy2,Servant2>
{
public:
    template <class Scheduler>
    ServantProxy2(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy2,Servant2>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(wait_end_test)
    BOOST_ASYNC_FUTURE_MEMBER(generate_exception)
};
}

BOOST_AUTO_TEST_CASE( test_servant_simple_ctor )
{        
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                    boost::asynchronous::lockfree_queue<>>>();
    
    main_thread_id = boost::this_thread::get_id();   
    ServantProxy proxy(scheduler,42);
}

BOOST_AUTO_TEST_CASE( test_posted_ctor )
{        
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                    boost::asynchronous::lockfree_queue<>>>();
    
    main_thread_id = boost::this_thread::get_id();   
    ServantProxy2 proxy(scheduler);
    boost::future<void> fu = proxy.wait_end_test();
    fu.get();
}

BOOST_AUTO_TEST_CASE( test_exception_member )
{        
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                    boost::asynchronous::lockfree_queue<>>>();
    
    main_thread_id = boost::this_thread::get_id();   
    ServantProxy2 proxy(scheduler);
    boost::future<void> fu = proxy.generate_exception();
    bool got_exception=false;
    try
    {
        fu.get();
    }
    catch ( my_exception& e)
    {
        got_exception=true;
    }
    catch(...)
    {
        BOOST_FAIL( "unexpected exception" );
    }

    BOOST_CHECK_MESSAGE(got_exception,"servant didn't send an expected exception.");
}

BOOST_AUTO_TEST_CASE( test_servant_proxy_members )
{        
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                    boost::asynchronous::lockfree_queue<>>>();
    
    ServantProxy proxy(scheduler,42);

    proxy.foobar(1,'a');
    
    int something = 3;
    boost::future<void> res = proxy.foo(boost::ref(something));
    res.get();
    // references ought to be ignored
    BOOST_CHECK_MESSAGE(something==100,"servant did not manage to modify data. something=" << something);
    
    boost::future<int> fu = proxy.doIt();
    BOOST_CHECK_MESSAGE(fu.get()==5,"servant returned incorrect data. Expected 5."); 
}

