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
#include <functional>
#include <random>
#include <future>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_lexicographical_compare.hpp>

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
    typedef int simple_ctor;
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

    std::future<void> start_parallel_lexicographical_compare_equal()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"start_parallel_lexicographical_compare_equal not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        m_data2 = m_data;
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_lexicographical_compare(this->m_data.begin(),this->m_data.end(),
                                                                                 this->m_data2.begin(),this->m_data2.end(),
                                                                                 [](int i,int j){return i < j;},1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<bool> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(false == res.get(),"parallel_lexicographical_compare gave a wrong value.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    std::future<void> start_parallel_lexicographical_compare_less()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"start_parallel_lexicographical_compare_less not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        m_data2 = m_data;
        --m_data2[3500];
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_lexicographical_compare(this->m_data.begin(),this->m_data.end(),
                                                                                 this->m_data2.begin(),this->m_data2.end(),
                                                                                 [](int i,int j){return i < j;},1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<bool> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(false == res.get(),"parallel_lexicographical_compare gave a wrong value.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    std::future<void> start_parallel_lexicographical_compare_greater()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"start_parallel_lexicographical_compare_greater not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        m_data2 = m_data;
        ++m_data2[3500];
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_lexicographical_compare(this->m_data.begin(),this->m_data.end(),
                                                                                 this->m_data2.begin(),this->m_data2.end(),
                                                                                 [](int i,int j){return i < j;},1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<bool> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(true == res.get(),"parallel_lexicographical_compare gave a wrong value.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    std::future<void> start_parallel_lexicographical_compare_equal2()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"start_parallel_lexicographical_compare_equal2 not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        m_data2 = m_data;
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_lexicographical_compare(this->m_data.begin(),this->m_data.end(),
                                                                                 this->m_data2.begin(),this->m_data2.end(),
                                                                                 1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<bool> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(false == res.get(),"parallel_lexicographical_compare gave a wrong value.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    std::future<void> start_parallel_lexicographical_compare_less2()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"start_parallel_lexicographical_compare_less2 not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        m_data2 = m_data;
        --m_data2[3500];
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_lexicographical_compare(this->m_data.begin(),this->m_data.end(),
                                                                                 this->m_data2.begin(),this->m_data2.end(),
                                                                                 1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<bool> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(false == res.get(),"parallel_lexicographical_compare gave a wrong value.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    std::future<void> start_parallel_lexicographical_compare_greater2()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"start_parallel_lexicographical_compare_greater2 not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        m_data2 = m_data;
        ++m_data2[3500];
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_lexicographical_compare(this->m_data.begin(),this->m_data.end(),
                                                                                 this->m_data2.begin(),this->m_data2.end(),
                                                                                 1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<bool> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(true == res.get(),"parallel_lexicographical_compare gave a wrong value.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
private:
    std::vector<int> m_data;
    std::vector<int> m_data2;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_lexicographical_compare_equal)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_lexicographical_compare_less)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_lexicographical_compare_greater)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_lexicographical_compare_equal2)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_lexicographical_compare_less2)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_lexicographical_compare_greater2)
};

}

BOOST_AUTO_TEST_CASE( test_parallel_lexicographical_compare_equal )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_parallel_lexicographical_compare_equal();
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
BOOST_AUTO_TEST_CASE( test_parallel_lexicographical_compare_less )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_parallel_lexicographical_compare_less();
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
BOOST_AUTO_TEST_CASE( test_parallel_lexicographical_compare_greater )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_parallel_lexicographical_compare_greater();
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
BOOST_AUTO_TEST_CASE( test_parallel_lexicographical_compare_equal2 )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_parallel_lexicographical_compare_equal2();
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
BOOST_AUTO_TEST_CASE( test_parallel_lexicographical_compare_less2 )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_parallel_lexicographical_compare_less2();
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
BOOST_AUTO_TEST_CASE( test_parallel_lexicographical_compare_greater2 )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_parallel_lexicographical_compare_greater2();
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
