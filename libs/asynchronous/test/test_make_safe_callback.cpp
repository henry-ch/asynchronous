
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
bool f_called = false;
typedef boost::asynchronous::any_loggable servant_job;
typedef std::map<std::string,std::list<boost::asynchronous::diagnostic_item> > diag_type;
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
                                                           boost::asynchronous::lockfree_queue<servant_job>>>(3))
    {
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
        servant_dtor = true;
    }
    boost::future<void> start_async_work()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<servant_job> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();

        auto cb = make_safe_callback(
                    [aPromise,ids]()
                    {
                        f_called = true;
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");

                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread");
                        aPromise->set_value();
                    },
                    "safe_callback");

        post_callback(
           [cb]()
           {
             cb();
           },
           [](boost::asynchronous::expected<void>){}
        );
        return fu;
    }

    boost::future<void> start_async_work_failed()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<servant_job> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();

        auto cb = make_safe_callback(
                    [ids]()
                    {
                        f_called = true;
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");

                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread");
                        throw my_exception();
                    },
                    "safe_callback");

        post_callback(
           [cb]()
           {
             cb();
           },
           [aPromise](boost::asynchronous::expected<void>){aPromise->set_value();}
        );
        return fu;
    }
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant,servant_job>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant,servant_job>(s)
    {}
#ifndef _MSC_VER
    BOOST_ASYNC_FUTURE_MEMBER_LOG(start_async_work,"proxy::start_async_work")
    BOOST_ASYNC_FUTURE_MEMBER_LOG(start_async_work_failed,"proxy::start_async_work_failed")
#else
    BOOST_ASYNC_FUTURE_MEMBER_LOG_2(start_async_work, "proxy::start_async_work")
    BOOST_ASYNC_FUTURE_MEMBER_LOG_2(start_async_work_failed, "proxy::start_async_work_failed")
#endif
};

}

BOOST_AUTO_TEST_CASE( test_make_safe_callback )
{
    servant_dtor=false;
    diag_type single_thread_sched_diag;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<servant_job>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<void> > fuv = proxy.start_async_work();
        try
        {
            boost::future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(std::exception& e)
        {
            std::cout << "exception: " << e.what() << std::endl;
            BOOST_FAIL( "unexpected exception" );
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
        single_thread_sched_diag = scheduler.get_diagnostics().totals();
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");

    bool found_safe_cb=false;
    for (auto mit = single_thread_sched_diag.begin(); mit != single_thread_sched_diag.end() ; ++mit)
    {
        if ((*mit).first == "safe_callback")
        {
            found_safe_cb = true;
        }
    }
    BOOST_CHECK_MESSAGE(found_safe_cb,"found_safe_cb not called.");
    BOOST_CHECK_MESSAGE(f_called,"found_safe_cb not called.");
    f_called=false;
}

BOOST_AUTO_TEST_CASE( test_make_safe_callback_failed )
{
    servant_dtor=false;
    diag_type single_thread_sched_diag;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<servant_job>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<void> > fuv = proxy.start_async_work_failed();
        try
        {
            boost::future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(std::exception& e)
        {
            std::cout << "exception: " << e.what() << std::endl;
            BOOST_FAIL( "unexpected exception" );
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
        single_thread_sched_diag = scheduler.get_diagnostics().totals();
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");

    bool found_safe_cb=false;
    for (auto mit = single_thread_sched_diag.begin(); mit != single_thread_sched_diag.end() ; ++mit)
    {
        if ((*mit).first == "safe_callback")
        {
            found_safe_cb = true;
        }
        else
        {
            continue;
        }
        for (auto jit = (*mit).second.begin(); jit != (*mit).second.end();++jit)
        {
            BOOST_CHECK_MESSAGE((*jit).is_failed(),"task should have been marked as failed.");
        }
    }
    BOOST_CHECK_MESSAGE(found_safe_cb,"found_safe_cb not called.");
    BOOST_CHECK_MESSAGE(f_called,"found_safe_cb not called.");
    f_called=false;
}
