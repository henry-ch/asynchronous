// Boost.Asynchronous library
//  Copyright (C) Christophe Henry, Tobias Holl 2015
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

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_mismatch.hpp>

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

    boost::future<void> start_parallel_mismatch_none()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"start_parallel_mismatch_none not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        m_data2 = m_data;
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_mismatch(this->m_data.begin(),this->m_data.end(),this->m_data2.begin(),
                                                               [](int i,int j){return i == j;},1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<std::pair<decltype(this->m_data.begin()), decltype(this->m_data2.begin())>> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        std::pair<decltype(this->m_data.begin()), decltype(this->m_data2.begin())> pair = res.get();
                        BOOST_CHECK_MESSAGE(pair.first == this->m_data.end() && pair.second == this->m_data2.end(),"parallel_mismatch gave a wrong value.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    boost::future<void> start_parallel_mismatch_normal()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"start_parallel_mismatch_normal not posted.");
        m_data2=m_data;
        // create mismatch
        std::random_shuffle(m_data.begin(),m_data.end());
        // we need a promise to inform caller when we're done
        std::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_mismatch(this->m_data.begin(),this->m_data.end(),this->m_data2.begin(),
                                                               [](int i,int j){return i == j;},1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<std::pair<decltype(this->m_data.begin()), decltype(this->m_data2.begin())>> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        auto std_res = std::mismatch(m_data.begin(),m_data.end(),m_data2.begin(),[](int i,int j){return i == j;});
                        auto pres = res.get();
                        BOOST_CHECK_MESSAGE(std::distance(m_data.begin(),pres.first) == std::distance(m_data.begin(),std_res.first),
                                            "parallel_mismatch gave a wrong value.");
                        BOOST_CHECK_MESSAGE(std::distance(m_data2.begin(),pres.second) == std::distance(m_data2.begin(),std_res.second),
                                            "parallel_mismatch gave a wrong value.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    boost::future<void> start_parallel_mismatch2_none()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"start_parallel_mismatch2_none not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        m_data2 = m_data;
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_mismatch(this->m_data.begin(),this->m_data.end(),this->m_data2.begin(),
                                                               1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<std::pair<decltype(this->m_data.begin()), decltype(this->m_data2.begin())>> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        std::pair<decltype(this->m_data.begin()), decltype(this->m_data2.begin())> pair = res.get();
                        BOOST_CHECK_MESSAGE(pair.first == this->m_data.end() && pair.second == this->m_data2.end(),"parallel_mismatch gave a wrong value.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    boost::future<void> start_parallel_mismatch2_normal()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"start_parallel_mismatch2_normal not posted.");
        m_data2=m_data;
        // create mismatch
        std::random_shuffle(m_data.begin(),m_data.end());
        // we need a promise to inform caller when we're done
        std::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_mismatch(this->m_data.begin(),this->m_data.end(),this->m_data2.begin(),
                                                               1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<std::pair<decltype(this->m_data.begin()), decltype(this->m_data2.begin())>> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        auto std_res = std::mismatch(m_data.begin(),m_data.end(),m_data2.begin());
                        auto pres = res.get();
                        BOOST_CHECK_MESSAGE(std::distance(m_data.begin(),pres.first) == std::distance(m_data.begin(),std_res.first),
                                            "parallel_mismatch gave a wrong value.");
                        BOOST_CHECK_MESSAGE(std::distance(m_data2.begin(),pres.second) == std::distance(m_data2.begin(),std_res.second),
                                            "parallel_mismatch gave a wrong value.");
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
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_mismatch_none)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_mismatch_normal)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_mismatch2_none)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_mismatch2_normal)
};

}

BOOST_AUTO_TEST_CASE( test_parallel_mismatch_none )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<void> > fuv = proxy.start_parallel_mismatch_none();
        try
        {
            boost::future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}
BOOST_AUTO_TEST_CASE( test_parallel_mismatch_normal )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<void> > fuv = proxy.start_parallel_mismatch_normal();
        try
        {
            boost::future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}BOOST_AUTO_TEST_CASE( test_parallel_mismatch2_none )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<void> > fuv = proxy.start_parallel_mismatch2_none();
        try
        {
            boost::future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}
BOOST_AUTO_TEST_CASE( test_parallel_mismatch2_normal )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<void> > fuv = proxy.start_parallel_mismatch2_normal();
        try
        {
            boost::future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}


