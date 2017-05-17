
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
#include <boost/asynchronous/post.hpp>
#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

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

template <class T=void>
struct Servant
{
    // optional, ctor is simple enough not to be posted
    typedef int simple_ctor;
    // please give us our scheduler, for callbacks
    typedef int requires_weak_scheduler;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler): m_scheduler(scheduler)
    {
        m_threadpool = boost::asynchronous::make_shared_scheduler_proxy<
                            boost::asynchronous::threadpool_scheduler<
                                    boost::asynchronous::lockfree_queue<>>>(3,std::string("pool"));
    }
    std::future<void> start_void_async_work()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_void_async_work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        std::vector<boost::thread::id> ids = m_threadpool.thread_ids();
        // start long tasks
        boost::asynchronous::post_callback(
           m_threadpool, // worker scheduler
           [ids](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");                    
                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    boost::this_thread::sleep(boost::posix_time::milliseconds(50));},// work
           m_scheduler, // our scheduler for the callback
           [aPromise,ids](boost::asynchronous::expected<void> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        aPromise->set_value();}// callback functor, ignores potential exceptions
        );
        return fu;
    }

    std::future<int> start_async_work()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<int> > aPromise(new std::promise<int>);
        std::future<int> fu = aPromise->get_future();
        std::vector<boost::thread::id> ids = m_threadpool.thread_ids();
        // start long tasks
        boost::asynchronous::post_callback(
           m_threadpool, // worker scheduler
           [ids](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");                    
                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    boost::this_thread::sleep(boost::posix_time::milliseconds(50));
                    return 42;},// work
           m_scheduler, // our scheduler for the callback
           [aPromise,ids](boost::asynchronous::expected<int> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        aPromise->set_value(res.get());
           }// callback functor.
        );
        return fu;
    }
    std::future<int> start_exception_async_work()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_exception_async_work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<int> > aPromise(new std::promise<int>);
        std::future<int> fu = aPromise->get_future();
        std::vector<boost::thread::id> ids = m_threadpool.thread_ids();
        // start long tasks
        boost::asynchronous::post_callback(
                    m_threadpool, // worker scheduler
                    [ids]()->int{
                          BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");                          
                          BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                          boost::this_thread::sleep(boost::posix_time::milliseconds(50));
                          BOOST_THROW_EXCEPTION( my_exception());
                          return 42;//not called
                    },// work
                    m_scheduler, // our scheduler for the callback
                    [aPromise,ids](boost::asynchronous::expected<int> res)mutable{
                           BOOST_CHECK_MESSAGE(res.has_exception(),"servant work did not throw an exception.");
                           BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                           BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                           try{res.get();}
                           catch(...){aPromise->set_exception(std::current_exception());}
                     }// callback functor.
          );
          return fu;
    }
    
    boost::asynchronous::any_weak_scheduler<> m_scheduler;
    // our worker pool
    boost::asynchronous::any_shared_scheduler_proxy<> m_threadpool;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant<> >
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant<> >(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(start_void_async_work)
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work)
    BOOST_ASYNC_FUTURE_MEMBER(start_exception_async_work)
};

}

BOOST_AUTO_TEST_CASE( test_void_post_callback )
{        
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(std::string("servant thread"));
    
    main_thread_id = boost::this_thread::get_id();   
    ServantProxy proxy(scheduler);
    auto fuv = proxy.start_void_async_work();
    try
    {
        auto resfuv = fuv.get();
        resfuv.get();
    }
    catch(...)
    {
        BOOST_FAIL( "unexpected exception" );
    }
}

BOOST_AUTO_TEST_CASE( test_int_post_callback )
{        
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>();
    
    main_thread_id = boost::this_thread::get_id();   
    ServantProxy proxy(scheduler);
    auto fuv = proxy.start_async_work();
    try
    {
        auto resfuv = fuv.get();
        int res= resfuv.get();
        BOOST_CHECK_MESSAGE(res==42,"servant work return wrong result.");
    }
    catch(...)
    {
        BOOST_FAIL( "unexpected exception" );
    }
}

BOOST_AUTO_TEST_CASE( test_post_callback_exception )
{        
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>();
    
    main_thread_id = boost::this_thread::get_id();   
    ServantProxy proxy(scheduler);
    auto fuv = proxy.start_exception_async_work();
    bool got_exception=false;
    try
    {
        auto resfuv = fuv.get();
        resfuv.get();
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
