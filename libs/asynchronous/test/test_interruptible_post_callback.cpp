// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/test/unit_test.hpp>

namespace
{
// main thread id
boost::thread::id main_thread_id;

struct Servant : boost::asynchronous::trackable_servant<>
{
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::threadpool_scheduler<
                                                    boost::asynchronous::lockfree_queue<>>>(1))
        , m_ready(new boost::promise<void>)
        , m_posted(0)
    {
    }
    ~Servant(){BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");}

    boost::shared_future<boost::asynchronous::any_interruptible> start_async_work(boost::shared_ptr<boost::promise<void> > p)
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work not posted.");
        boost::promise<boost::asynchronous::any_interruptible> apromise;
        boost::shared_future<boost::asynchronous::any_interruptible> fu = apromise.get_future();
        // start long tasks
        boost::asynchronous::any_interruptible interruptible =
        interruptible_post_callback(
               [p](){p->set_value();boost::this_thread::sleep(boost::posix_time::milliseconds(1000000));},
               [](boost::asynchronous::expected<void> ){BOOST_ERROR( "unexpected call of callback" );}// should not be called
        );
        apromise.set_value(interruptible);
        return fu;
    }
    std::pair<
       boost::shared_ptr<boost::promise<void> >,
       boost::shared_future<boost::asynchronous::any_interruptible>
    >
    start_async_work2()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work2 not posted.");
        
        // post blocking funtion, until dtor done
        boost::shared_future<void> fu=m_ready->get_future();
        auto blocking = [fu]() mutable {fu.get();};
        get_worker().post(blocking);
        
        boost::promise<boost::asynchronous::any_interruptible> apromise;
        boost::shared_future<boost::asynchronous::any_interruptible> fu2 = apromise.get_future();
        // start long tasks
        boost::asynchronous::any_interruptible interruptible =
        interruptible_post_callback(
           [](){BOOST_ERROR( "unexpected call of task" );},
           [](boost::asynchronous::expected<void> ){BOOST_ERROR( "unexpected call of callback" );}// should not be called
        );
        apromise.set_value(interruptible);
        return std::make_pair(m_ready,fu2);
    }
    boost::shared_future<int> test_no_interrupt(int runs, int disturbances)
    {
        boost::shared_future<int> fu = m_promise.get_future();
        // start long tasks
        for (int i = 0; i < runs ;++i)
        {
            interruptible_post_callback(
                   [this](){++m_posted;},
                   [this,runs](boost::asynchronous::expected<void> )
                   {
                     ++m_cb;
                     if (m_cb == 2*runs)
                     {
                         m_promise.set_value(m_cb);
                     }
                   });
        }
        for (int i = 0; i < disturbances ;++i)
        {
            interruptible_post_callback(
                   [](){},
                   [](boost::asynchronous::expected<void> )
                   {});
        }
        for (int i = 0; i < runs ;++i)
        {
            interruptible_post_callback(
                   [this](){++m_posted;},
                   [this,runs](boost::asynchronous::expected<void> )
                   {
                     ++m_cb;
                     if (m_cb == 2*runs)
                     {
                         m_promise.set_value(m_cb);
                     }
                   });
        }
        return fu;
    }

private:
    boost::shared_ptr<boost::promise<void> > m_ready;
    boost::promise<int> m_promise;
    std::atomic<int> m_posted;
    int m_cb=0;
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
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work2)
    BOOST_ASYNC_FUTURE_MEMBER(test_no_interrupt)
#else
    BOOST_ASYNC_FUTURE_MEMBER_1(start_async_work)
    BOOST_ASYNC_FUTURE_MEMBER_1(start_async_work2)
    BOOST_ASYNC_FUTURE_MEMBER_1(test_no_interrupt)
#endif
};
}

BOOST_AUTO_TEST_CASE( test_interrupt_running_task )
{     
    main_thread_id = boost::this_thread::get_id();
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        boost::shared_ptr<boost::promise<void> > p(new boost::promise<void>);
        boost::shared_future<void> end=p->get_future();
        {
            ServantProxy proxy(scheduler);
            boost::shared_future<boost::shared_future<boost::asynchronous::any_interruptible> > fu = proxy.start_async_work(p);
            boost::shared_future<boost::asynchronous::any_interruptible> resfu = fu.get();
            boost::asynchronous::any_interruptible res = resfu.get();
            end.get();
            res.interrupt();
        }
    }
}

BOOST_AUTO_TEST_CASE( test_interrupt_not_running_task )
{     
    main_thread_id = boost::this_thread::get_id();
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        {
            typedef std::pair<
                    boost::shared_ptr<boost::promise<void> >,
                    boost::shared_future<boost::asynchronous::any_interruptible>
                 > res_type;
            
            ServantProxy proxy(scheduler);
            boost::shared_future<res_type> fu = proxy.start_async_work2();
            res_type res = fu.get();
            boost::asynchronous::any_interruptible i = res.second.get();
            // provoke interrupt before job starts
            i.interrupt();
            // now let the job try to execute
            res.first->set_value();
        }
    }
}

BOOST_AUTO_TEST_CASE( test_no_interrupt )
{
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::shared_future<boost::shared_future<int> > fuv = proxy.test_no_interrupt(10000,1000);
        try
        {
            boost::shared_future<int> resfuv = fuv.get();
            int res = resfuv.get();
            BOOST_CHECK_MESSAGE(res==20000,"not matching number of callbacks.");
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
}
