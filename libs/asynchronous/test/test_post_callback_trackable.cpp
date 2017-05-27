
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
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

namespace
{
// main thread id
boost::thread::id main_thread_id;
bool servant_dtor=false;

struct my_exception : public boost::asynchronous::asynchronous_exception
{
    virtual const char* what() const throw()
    {
        return "my_exception";
    }
};

struct Servant : boost::asynchronous::trackable_servant<>
{
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<>>>(3))
    {
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
        servant_dtor = true;
    }
    std::future<void> start_void_async_work()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_void_async_work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");                    
                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    boost::this_thread::sleep(boost::posix_time::milliseconds(50));},// work
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
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");                   
                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    boost::this_thread::sleep(boost::posix_time::milliseconds(50));
                    return 42;},// work
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
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
                    [ids]()->int{
                          BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");                          
                          BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                          boost::this_thread::sleep(boost::posix_time::milliseconds(50));
                          ASYNCHRONOUS_THROW( my_exception());
                    },// work
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
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(start_void_async_work)
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work)
    BOOST_ASYNC_FUTURE_MEMBER(start_exception_async_work)
};

}

BOOST_AUTO_TEST_CASE( test_void_post_callback_trackable )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

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
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_int_post_callback_trackable )
{        
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        {
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
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_post_callback_trackable_exception )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        {
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
                BOOST_CHECK_MESSAGE(std::string(e.what_) == "my_exception","no what data");
                BOOST_CHECK_MESSAGE(!std::string(e.file_).empty(),"no file data");
                BOOST_CHECK_MESSAGE(e.line_ != -1,"no line data");
            }
            catch(...)
            {
                BOOST_FAIL( "unexpected exception" );
            }
            BOOST_CHECK_MESSAGE(got_exception,"servant didn't send an expected exception.");
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_void_post_callback_trackable_lf )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

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
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_int_post_callback_trackable_lf )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        {
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
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_post_callback_trackable_exception_lf )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        {
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
                BOOST_CHECK_MESSAGE(std::string(e.what_) == "my_exception","no what data");
                BOOST_CHECK_MESSAGE(!std::string(e.file_).empty(),"no file data");
                BOOST_CHECK_MESSAGE(e.line_ != -1,"no line data");
            }
            catch(...)
            {
                BOOST_FAIL( "unexpected exception" );
            }
            BOOST_CHECK_MESSAGE(got_exception,"servant didn't send an expected exception.");
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

