// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
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

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_search_n.hpp>

#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

namespace
{
// main thread id
boost::thread::id main_thread_id;
bool servant_dtor=false;
void generate(std::vector<int>& data)
{
    data = std::vector<int>(10000,1);
    std::mt19937 mt(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<> dis(0, 1000);
    std::generate(data.begin(), data.end(), std::bind(dis, std::ref(mt)));
}

struct Servant : boost::asynchronous::trackable_servant<>
{
    typedef std::vector<int>::iterator Iterator;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<>>>(6))
    {
        generate(m_data);
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
        servant_dtor = true;
    }

    std::future<void> start_async_work_ok()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work_ok not posted.");
        m_data[3100]=42;
        m_data[3101]=42;
        m_data[3102]=42;
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
                    return boost::asynchronous::parallel_search_n(this->m_data.begin(),this->m_data.end(),
                                                                3,42,
                                                                [](int i, int j){return i == j;},1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<Iterator> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");

                        auto it = std::search_n(this->m_data.begin(),this->m_data.end(),
                                                3,42,
                                                [](int i, int j){return i == j;});

                        BOOST_CHECK_MESSAGE(it != m_data.end(),"parallel_search_n should have found value.");
                        BOOST_CHECK_MESSAGE(it == res.get(),"parallel_search_n found wrong value.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    std::future<void> start_async_work_nok()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work_nok not posted.");
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
                    return boost::asynchronous::parallel_search_n(this->m_data.begin(),this->m_data.end(),
                                                                  3,-1,
                                                                  [](int i, int j){return i == j;},1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<Iterator> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");

                        auto it = std::search_n(this->m_data.begin(),this->m_data.end(),
                                                3,-1,
                                                [](int i, int j){return i == j;});

                        BOOST_CHECK_MESSAGE(it == m_data.end(),"parallel_search_n should not have found value.");
                        BOOST_CHECK_MESSAGE(it == res.get(),"parallel_search_n found wrong value.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }

    std::future<void> start_async_work_ok_no_fct()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work_ok not posted.");
        m_data[3100]=42;
        m_data[3101]=42;
        m_data[3102]=42;
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
                    return boost::asynchronous::parallel_search_n(this->m_data.begin(),this->m_data.end(),
                                                                  3,42,1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<Iterator> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");

                        auto it = std::search_n(this->m_data.begin(),this->m_data.end(),3,42);

                        BOOST_CHECK_MESSAGE(it != m_data.end(),"parallel_search_n should have found value.");
                        BOOST_CHECK_MESSAGE(it == res.get(),"parallel_search_n found wrong value.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }

    std::future<void> start_async_work_ok2()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work_ok not posted.");
        m_data[2999]=42;
        m_data[3000]=42;
        m_data[3001]=42;
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
                    return boost::asynchronous::parallel_search_n(this->m_data.begin(),this->m_data.end(),
                                                                  3,42,
                                                                  [](int i, int j){return i == j;},1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<Iterator> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");

                        auto it = std::search_n(this->m_data.begin(),this->m_data.end(),
                                                3,42,
                                                [](int i, int j){return i == j;});

                        BOOST_CHECK_MESSAGE(it != m_data.end(),"parallel_search_n should have found value.");
                        BOOST_CHECK_MESSAGE(it == res.get(),"parallel_search_n found wrong value.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }

private:
    std::vector<int> m_data;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work_ok)
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work_nok)
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work_ok2)
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work_ok_no_fct)
};

}

BOOST_AUTO_TEST_CASE( test_parallel_search_n_ok )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_async_work_ok();
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

BOOST_AUTO_TEST_CASE( test_parallel_search_n_nok )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_async_work_nok();
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

BOOST_AUTO_TEST_CASE( test_parallel_search_n_ok2 )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_async_work_ok2();
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

BOOST_AUTO_TEST_CASE( test_parallel_search_n_ok_no_fct )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_async_work_ok_no_fct();
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


