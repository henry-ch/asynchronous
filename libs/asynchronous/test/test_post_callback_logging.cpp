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
#include <chrono>
#include <future>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

namespace
{
typedef boost::asynchronous::any_loggable servant_job;
typedef std::map<std::string,std::list<boost::asynchronous::diagnostic_item> > diag_type;
typedef std::vector<std::pair<std::string,boost::asynchronous::diagnostic_item>> current_type;
typedef std::pair<std::string,boost::asynchronous::diagnostic_item> one_current_type;

// main thread id
boost::thread::id main_thread_id;
bool servant_dtor=false;
struct my_exception : virtual boost::exception, virtual std::exception
{
    virtual const char* what() const throw()
    {
        return "my_exception";
    }
};

struct Servant : boost::asynchronous::trackable_servant<servant_job,servant_job>
{
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<servant_job> scheduler)
        : boost::asynchronous::trackable_servant<servant_job,servant_job>(scheduler,
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue< servant_job >>>(3))
    {
    }
    ~Servant()
    {
        servant_dtor = true;
    }
    std::future<void> start_void_async_work()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_void_async_work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<servant_job> tp =get_worker();
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
                        aPromise->set_value();},// callback functor, ignores potential exceptions
            "void_async_work"
        );
        return fu;
    }

    std::future<int> start_async_work()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<int> > aPromise(new std::promise<int>);
        std::future<int> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<servant_job> tp =get_worker();
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
                        },// callback functor.
           "int_async_work"
        );
        return fu;
    }
    std::future<int> start_exception_async_work()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_exception_async_work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<int> > aPromise(new std::promise<int>);
        std::future<int> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<servant_job> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
                    [ids]()->int{
                          BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");                          
                          BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                          boost::this_thread::sleep(boost::posix_time::milliseconds(50));
                          BOOST_THROW_EXCEPTION( my_exception());
                    },// work
                    [aPromise,ids](boost::asynchronous::expected<int> res)mutable{
                           BOOST_CHECK_MESSAGE(res.has_exception(),"servant work did not throw an exception.");
                           BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                           BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                           try{res.get();}
                           catch(...){aPromise->set_exception(std::current_exception());}
                           },// callback functor.
                     "int_async_work"
          );
          return fu;
    }
    std::future<void> test_current()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant test_current not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();

        std::shared_ptr<std::promise<void> > blocking_promise(new std::promise<void>);
        std::shared_future<void> block_fu = blocking_promise->get_future();

        std::shared_ptr<std::promise<void> > blocking_promise2(new std::promise<void>);
        std::shared_future<void> block_fu2 = blocking_promise2->get_future();

        // start long tasks
        post_callback(
                    [block_fu,blocking_promise2]()mutable
                    {
                        blocking_promise2->set_value();
                        block_fu.get();
                    },// work
                    [aPromise,this](boost::asynchronous::expected<void> res)mutable
                    {
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"test_current threw an exception.");
                        current_type current = get_worker().get_diagnostics().current();
                        bool found = (std::find_if(current.begin(),current.end(),
                                                  [](one_current_type const& c){return c.first == "current_async_work";})
                                     != current.end());
                        BOOST_CHECK_MESSAGE(!found,"current job should no more be present.");
                        aPromise->set_value();
                    },// callback functor.
                    "current_async_work"
          );
          block_fu2.get();
          // check current (is being blocked)
          current_type current = get_worker().get_diagnostics().current();
          blocking_promise->set_value();
          bool found = (std::find_if(current.begin(),current.end(),
                                    [](one_current_type const& c){return c.first == "current_async_work";})
                        != current.end());
          BOOST_CHECK_MESSAGE(found,"current job is not present.");
          return fu;
    }
    diag_type get_diagnostics() const
    {
        return get_worker().get_diagnostics().totals();
    }
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant,servant_job>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant,servant_job>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER_LOG(start_void_async_work,"proxy::start_void_async_work")
    BOOST_ASYNC_FUTURE_MEMBER_LOG(start_async_work,"proxy::start_async_work")
    BOOST_ASYNC_FUTURE_MEMBER_LOG(start_exception_async_work,"proxy::start_exception_async_work")
    BOOST_ASYNC_FUTURE_MEMBER_LOG(get_diagnostics,"proxy::get_diagnostics")
    BOOST_ASYNC_FUTURE_MEMBER_LOG(test_current,"proxy::test_current")
};

}

BOOST_AUTO_TEST_CASE( test_void_post_callback_logging )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                        boost::asynchronous::lockfree_queue<servant_job>>>();
        {
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
            auto fu_diag = proxy.get_diagnostics();
            diag_type diag = fu_diag.get();
            BOOST_CHECK_MESSAGE(diag.size()==1,"servant tp worker didn't log the number of works we expected.");// start_void_async_work's task
            for (auto mit = diag.begin(); mit != diag.end() ; ++mit)
            {
                for (auto jit = (*mit).second.begin(); jit != (*mit).second.end();++jit)
                {
                    BOOST_CHECK_MESSAGE(std::chrono::nanoseconds((*jit).get_finished_time() - (*jit).get_started_time()).count() >= 0,"task finished before it started.");
                    BOOST_CHECK_MESSAGE(!(*jit).is_interrupted(),"no task should have been interrupted.");
                    BOOST_CHECK_MESSAGE(!(*jit).is_failed(),"no task should have failed.");
                }
            }
        }
        // wait for servant dtor
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
        diag_type single_thread_sched_diag = scheduler.get_diagnostics().totals();
        // start_void_async_work + get_diagnostics + servant dtor + void_async_work (cvallback)
        BOOST_CHECK_MESSAGE(single_thread_sched_diag.size()==4,"servant scheduler worker didn't log the number of works we expected.");
        for (auto mit = single_thread_sched_diag.begin(); mit != single_thread_sched_diag.end() ; ++mit)
        {
            for (auto jit = (*mit).second.begin(); jit != (*mit).second.end();++jit)
            {
                BOOST_CHECK_MESSAGE(std::chrono::nanoseconds((*jit).get_finished_time() - (*jit).get_started_time()).count() >= 0,"task finished before it started.");
                BOOST_CHECK_MESSAGE(!(*jit).is_interrupted(),"no task should have been interrupted.");
                BOOST_CHECK_MESSAGE(!(*jit).is_failed(),"no task should have failed.");
            }
        }
        // clear diags
        scheduler.clear_diagnostics();
        single_thread_sched_diag = scheduler.get_diagnostics().totals();
        BOOST_CHECK_MESSAGE(single_thread_sched_diag.empty(),"Diags should have been cleared.");
        {
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
            auto fu_diag = proxy.get_diagnostics();
            diag_type diag = fu_diag.get();
            BOOST_CHECK_MESSAGE(diag.size()==1,"servant tp worker after clear_diagnostics didn't log the number of works we expected.");// start_void_async_work's task
        }

    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}
    


BOOST_AUTO_TEST_CASE( test_int_post_callback_logging )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<servant_job>>>();

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
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_post_callback_logging_exception )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<servant_job>>>();

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

        auto fu_diag = proxy.get_diagnostics();
        diag_type diag = fu_diag.get();
        BOOST_CHECK_MESSAGE(diag.size()==1,"servant tp worker didn't log the number of works we expected.");// start_exception_async_work's task
        for (auto mit = diag.begin(); mit != diag.end() ; ++mit)
        {
            for (auto jit = (*mit).second.begin(); jit != (*mit).second.end();++jit)
            {
                BOOST_CHECK_MESSAGE((*jit).is_failed(),"Task should have failed.");
            }
        }

    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_current )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<servant_job>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.test_current();
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
