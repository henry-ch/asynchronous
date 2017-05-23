
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
#include <boost/asynchronous/trackable_servant.hpp>

#include <boost/test/unit_test.hpp>

namespace
{
// main thread id
boost::thread::id main_thread_id;
bool dtor_called=false;

struct Servant : boost::asynchronous::trackable_servant<>
{
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<>>>(1))
    {
    }
    ~Servant()
    {
        dtor_called=true;
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
    }
    void start_posting(std::shared_ptr<std::promise<void> > done)
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_posting not posted.");
        auto fct_alive = make_check_alive_functor();
        BOOST_CHECK_MESSAGE(fct_alive(),"servant should still be alive.");
        // post task to self
        post_self(
           [done]()mutable
            {
                BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant task not posted.");
                done->set_value();
            }// work
        );
    }
    std::future<int> start_posting2()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_posting not posted.");
        auto fct_alive = make_check_alive_functor();
        BOOST_CHECK_MESSAGE(fct_alive(),"servant should still be alive.");
        // post task to self
        return post_self(
           []()
            {
                BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant task not posted.");
                return 42;
            }// work
        );
    }
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(start_posting)
    BOOST_ASYNC_FUTURE_MEMBER(start_posting2)
};

}

BOOST_AUTO_TEST_CASE( test_post_self )
{
    main_thread_id = boost::this_thread::get_id();
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        std::shared_ptr<std::promise<void> > done(new std::promise<void>);
        std::future<void> end=done->get_future();
        {
            ServantProxy proxy(scheduler);
            proxy.start_posting(done);
            // wait for task to complete
            end.get();
        }
    }
    // at this point, the dtor has been called
    BOOST_CHECK_MESSAGE(dtor_called,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_post_self_int )
{
    main_thread_id = boost::this_thread::get_id();
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        {
            ServantProxy proxy(scheduler);
            auto fu = proxy.start_posting2();
            // wait for task to complete
            int res = fu.get().get();
            BOOST_CHECK_MESSAGE(res==42,"post_self returned wrong result.");
        }
    }
    // at this point, the dtor has been called
    BOOST_CHECK_MESSAGE(dtor_called,"servant dtor not called.");
}
