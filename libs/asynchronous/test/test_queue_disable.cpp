// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2018
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <vector>
#include <set>
#include <random>
#include <future>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/queue/any_queue_container.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>

#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

namespace
{
boost::thread::id main_thread_id;
bool servant_dtor=false;
struct Servant : boost::asynchronous::trackable_servant<>
{
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<>>>(6))
    {
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
        servant_dtor = true;
    }

    void disable_queue()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant disable_queue not posted.");
        BOOST_CHECK_MESSAGE(enabled_,"should be enabled.");
        // disable queue
        boost::asynchronous::any_shared_scheduler<> s = get_scheduler().lock();
        s.enable_queue(2,false);
        enabled_=false;
    }
    void enable_queue()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant enable_queue not posted.");
        BOOST_CHECK_MESSAGE(!enabled_,"should be disabled.");
        BOOST_CHECK_MESSAGE(callback_called_,"callback should have been called.");
        // enable queue
        boost::asynchronous::any_shared_scheduler<> s = get_scheduler().lock();
        s.enable_queue(2,true);
        enabled_=true;
    }
    void foo()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant foo not posted.");
        BOOST_CHECK_MESSAGE(enabled_,"should have only be called when enabled.");
        BOOST_CHECK_MESSAGE(callback_called_,"callback should have been called.");
    }
    std::future<void> foo2()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant foo not posted.");
        BOOST_CHECK_MESSAGE(!enabled_,"should be called disabled.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");
                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return 42;
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<int> res)mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(res.get() == 42,"servant work went wrong.");
                        callback_called_ =true;
                        aPromise->set_value();
           }// callback functor.
        ,"",0,3);
        return fu;
    }
private:
    bool enabled_=true;
    bool callback_called_=false;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
#ifndef _MSC_VER
    BOOST_ASYNC_FUTURE_MEMBER(disable_queue,1)
    BOOST_ASYNC_FUTURE_MEMBER(foo,2)
    BOOST_ASYNC_FUTURE_MEMBER(foo2,1)
    BOOST_ASYNC_FUTURE_MEMBER(enable_queue,1)
#else
    BOOST_ASYNC_FUTURE_MEMBER_1(disable_queue,1)
    BOOST_ASYNC_FUTURE_MEMBER_1(foo,2)
    BOOST_ASYNC_FUTURE_MEMBER_1(foo2,1)
    BOOST_ASYNC_FUTURE_MEMBER_1(enable_queue,1)
#endif
};

}

BOOST_AUTO_TEST_CASE( test_disable_queue )
{
    servant_dtor=false;

    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                boost::asynchronous::single_thread_scheduler<
                        boost::asynchronous::any_queue_container<>>>
        (boost::asynchronous::any_queue_container_config<boost::asynchronous::lockfree_queue<> >(3));
        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);

        auto fuv = proxy.disable_queue();
        fuv.get();
        auto fuv2 = proxy.foo();
        auto fuv3 = proxy.foo2();
        try
        {
            auto resfuv = fuv3.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
        auto fuv4 = proxy.enable_queue();
        fuv4.get();
        fuv2.get();
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

