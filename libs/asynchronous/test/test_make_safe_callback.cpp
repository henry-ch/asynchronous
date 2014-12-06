
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
typedef boost::asynchronous::any_loggable<boost::chrono::high_resolution_clock> servant_job;
typedef std::map<std::string,std::list<boost::asynchronous::diagnostic_item<boost::chrono::high_resolution_clock> > > diag_type;

struct Servant : boost::asynchronous::trackable_servant<servant_job,servant_job>
{
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<servant_job> scheduler)
        : boost::asynchronous::trackable_servant<servant_job,servant_job>(scheduler,
                                               boost::asynchronous::create_shared_scheduler_proxy(
                                                   new boost::asynchronous::threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<servant_job> >(3)))
    {
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
        servant_dtor = true;
    }
    boost::shared_future<void> start_async_work()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work not posted.");
        // we need a promise to inform caller when we're done
        boost::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::shared_future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<servant_job> tp =get_worker();

        auto cb = make_safe_callback(
                    [aPromise,tp]()
                    {
                        f_called = true;
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        std::vector<boost::thread::id> ids = tp.thread_ids();
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
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant,servant_job>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant,servant_job>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER_LOG(start_async_work,"proxy::start_async_work")
};

}

BOOST_AUTO_TEST_CASE( test_make_safe_callback )
{
    servant_dtor=false;
    diag_type single_thread_sched_diag;
    {
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<servant_job> >);

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::shared_future<boost::shared_future<void> > fuv = proxy.start_async_work();
        try
        {
            boost::shared_future<void> resfuv = fuv.get();
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
        single_thread_sched_diag = scheduler.get_diagnostics();
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");

    //bool found_safe_cb=false;
//    for (auto mit = single_thread_sched_diag.begin(); mit != single_thread_sched_diag.end() ; ++mit)
//    {
//        if ((*mit).first == "safe_callback")
//        {
//            found_safe_cb = true;
//        }
//    }
    //TODO fix this, log error
    //BOOST_CHECK_MESSAGE(found_safe_cb,"found_safe_cb not called.");
    BOOST_CHECK_MESSAGE(f_called,"found_safe_cb not called.");
}

