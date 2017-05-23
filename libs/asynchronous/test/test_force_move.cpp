// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2014
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <functional>
#include <future>

#include <boost/asynchronous/helpers.hpp>
#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include "test_common.hpp"
#include <boost/thread/future.hpp>

#include <boost/test/unit_test.hpp>

namespace
{

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
    std::future<int> start_async_work()
    {
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<int> > aPromise(new std::promise<int>);
        std::future<int> fu = aPromise->get_future();
        auto cb = make_safe_callback(
                    [aPromise](int,boost::future<int> fu)mutable
                    {
                        aPromise->set_value(fu.get());
                    });

        post_callback(
           [cb]()
           {
            boost::future<int> fu (boost::make_ready_future(42));
            int i = 5;
            cb(i,std::move(fu));
           },
           [](boost::asynchronous::expected<void>){}
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
#ifndef _MSC_VER
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work)
#else
    BOOST_ASYNC_FUTURE_MEMBER_1(start_async_work)
#endif
};

}


BOOST_AUTO_TEST_CASE( test_force_move )
{
    bool called=false;
    std::function<void(boost::future<int>)> f =
            [&](boost::future<int> fui)
            {
                called=true;
                BOOST_CHECK_MESSAGE(fui.get()==42,"future should be 42.");
            };
    boost::future<int> fu (boost::make_ready_future(42));
    f(boost::asynchronous::force_move(std::move(fu)));
    BOOST_CHECK_MESSAGE(called,"function not called.");
}

BOOST_AUTO_TEST_CASE( test_force_move_safe_callback )
{
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_async_work();
        try
        {
            auto resfuv = fuv.get();
            int i = resfuv.get();
            BOOST_CHECK_MESSAGE(i==42,"result should be 42.");
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
    }
}



