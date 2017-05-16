// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org
#include <future>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_for.hpp>

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
        , m_ready(new std::promise<void>)
        , m_posted(0)
    {
    }
    ~Servant(){BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");}

    boost::future<boost::asynchronous::any_interruptible> start_async_work(std::shared_ptr<std::promise<void> > p)
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work not posted.");
        boost::promise<boost::asynchronous::any_interruptible> apromise;
        boost::future<boost::asynchronous::any_interruptible> fu = apromise.get_future();
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
       std::shared_ptr<std::promise<void> >,
       boost::future<boost::asynchronous::any_interruptible>
    >
    start_async_work2()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work2 not posted.");
        
        // post blocking funtion, until dtor done
        std::shared_future<void> fu=m_ready->get_future();
        auto blocking = [fu]() mutable {fu.get();};
        get_worker().post(blocking);
        
        boost::promise<boost::asynchronous::any_interruptible> apromise;
        boost::future<boost::asynchronous::any_interruptible> fu2 = apromise.get_future();
        // start long tasks
        boost::asynchronous::any_interruptible interruptible =
        interruptible_post_callback(
           [](){BOOST_ERROR( "unexpected call of task" );},
           [](boost::asynchronous::expected<void> ){BOOST_ERROR( "unexpected call of callback" );}// should not be called
        );
        apromise.set_value(interruptible);
        return std::make_pair(m_ready,std::move(fu2));
    }
    boost::future<int> test_no_interrupt(int runs, int disturbances)
    {
        /*this->set_worker(boost::asynchronous::make_shared_scheduler_proxy<
                         boost::asynchronous::threadpool_scheduler<
                          boost::asynchronous::lockfree_queue<>>>(10));*/
        boost::future<int> fu = m_promise.get_future();
        for (int i = 0; i < disturbances ;++i)
        {
            m_interruptibles.push_back(interruptible_post_callback(
                   [](){boost::this_thread::sleep(boost::posix_time::milliseconds(10));},
                   [](boost::asynchronous::expected<void> ){}));
        }
        // start long tasks
        for (int i = 0; i < runs ;++i)
        {
            interruptible_post_callback(
                   [this](){++m_posted;},
                   [this,runs](boost::asynchronous::expected<void> )
                   {
                     //boost::this_thread::sleep(boost::posix_time::milliseconds(10));
                     ++m_cb;
                     if (m_cb == 2*runs)
                     {
                         m_promise.set_value(m_cb);
                     }
                   });
        }
        for (auto& i : m_interruptibles)
        {
            i.interrupt();
        }
        for (int i = 0; i < runs ;++i)
        {
            interruptible_post_callback(
                   [this](){++m_posted;},
                   [this,runs](boost::asynchronous::expected<void> )
                   {
                     //boost::this_thread::sleep(boost::posix_time::milliseconds(10));
                     ++m_cb;
                     if (m_cb == 2*runs)
                     {
                         m_promise.set_value(m_cb);
                     }
                   });
        }
        return fu;
    }
    boost::future<int> test_no_interrupt_continuation(int runs, int disturbances, bool short_continuation)
    {
        this->set_worker(boost::asynchronous::make_shared_scheduler_proxy<
                         boost::asynchronous::threadpool_scheduler<
                          boost::asynchronous::lockfree_queue<>>>(10));
        m_data = std::vector<int>(short_continuation?1:10000,1);
        boost::future<int> fu = m_promise.get_future();
        for (int i = 0; i < disturbances ;++i)
        {
            m_interruptibles.push_back(interruptible_post_callback(
                   [](){boost::this_thread::sleep(boost::posix_time::milliseconds(10));},
                   [](boost::asynchronous::expected<void> ){}));
        }
        // start long tasks
        for (int i = 0; i < runs ;++i)
        {
            interruptible_post_callback(
                   [this](){
                        ++m_posted;
                        return boost::asynchronous::parallel_for(m_data.begin(),m_data.end(),
                                                        [](int& i)
                                                        {
                                                          i += 2;
                                                        },1500);
                   },
                   [this,runs](boost::asynchronous::expected<void> )
                   {
                     ++m_cb;
                     if (m_cb == 2*runs)
                     {
                         m_promise.set_value(m_cb);
                     }
                     if(m_cb > 2*runs)
                     {
                         BOOST_FAIL( "too many callbacks" );
                     }
                   });
        }
        for (auto& i : m_interruptibles)
        {
            i.interrupt();
        }
        for (int i = 0; i < runs ;++i)
        {
            interruptible_post_callback(
                   [this]()
                   {
                      ++m_posted;
                      return boost::asynchronous::parallel_for(m_data.begin(),m_data.end(),
                                                      [](int& i)
                                                      {
                                                        i += 2;
                                                      },1500);
                   },
                   [this,runs](boost::asynchronous::expected<void> )
                   {
                     ++m_cb;
                     if (m_cb == 2*runs)
                     {
                         m_promise.set_value(m_cb);
                     }
                     if(m_cb > 2*runs)
                     {
                         BOOST_FAIL( "too many callbacks" );
                     }
                   });
        }
        return fu;
    }

private:
    std::shared_ptr<std::promise<void> > m_ready;
    boost::promise<int> m_promise;
    std::atomic<int> m_posted;
    int m_cb=0;
    std::vector<boost::asynchronous::any_interruptible> m_interruptibles;
    std::vector<int> m_data;
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
    BOOST_ASYNC_FUTURE_MEMBER(test_no_interrupt_continuation)
#else
    BOOST_ASYNC_FUTURE_MEMBER_1(start_async_work)
    BOOST_ASYNC_FUTURE_MEMBER_1(start_async_work2)
    BOOST_ASYNC_FUTURE_MEMBER_1(test_no_interrupt)
    BOOST_ASYNC_FUTURE_MEMBER_1(test_no_interrupt_continuation)
#endif
};
}

BOOST_AUTO_TEST_CASE( test_interrupt_running_task )
{     
    main_thread_id = boost::this_thread::get_id();
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        std::shared_ptr<std::promise<void> > p(new std::promise<void>);
        std::future<void> end=p->get_future();
        {
            ServantProxy proxy(scheduler);
            boost::future<boost::future<boost::asynchronous::any_interruptible> > fu = proxy.start_async_work(p);
            boost::future<boost::asynchronous::any_interruptible> resfu = fu.get();
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
                    std::shared_ptr<std::promise<void> >,
                    boost::future<boost::asynchronous::any_interruptible>
                 > res_type;
            
            ServantProxy proxy(scheduler);
            boost::future<res_type> fu = proxy.start_async_work2();
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
        boost::future<boost::future<int> > fuv = proxy.test_no_interrupt(1000,10);
        try
        {
            boost::future<int> resfuv = fuv.get();
            int res = resfuv.get();
            BOOST_CHECK_MESSAGE(res==2000,"not matching number of callbacks: " << res);
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
}

BOOST_AUTO_TEST_CASE( test_no_interrupt_continuation )
{
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<int> > fuv = proxy.test_no_interrupt_continuation(10000,100,false);
        try
        {
            boost::future<int> resfuv = fuv.get();
            int res = resfuv.get();
            BOOST_CHECK_MESSAGE(res==2*10000,"not matching number of callbacks: " << res);
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
}

BOOST_AUTO_TEST_CASE( test_no_interrupt_continuation_short )
{
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<int> > fuv = proxy.test_no_interrupt_continuation(10000,100,true);
        try
        {
            boost::future<int> resfuv = fuv.get();
            int res = resfuv.get();
            BOOST_CHECK_MESSAGE(res==2*10000,"not matching number of callbacks: " << res);
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
}

