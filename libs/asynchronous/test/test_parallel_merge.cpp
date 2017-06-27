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
#include <functional>
#include <random>
#include <boost/lexical_cast.hpp>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_merge.hpp>

#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

namespace
{
// main thread id
boost::thread::id main_thread_id;
bool servant_dtor=false;

struct my_exception : virtual boost::exception, virtual std::exception
{
    virtual const char* what() const throw()
    {
        return "my_exception";
    }
};
void generate(std::vector<int>& data, unsigned elements, unsigned dist)
{
    data = std::vector<int>(elements,1);
    std::random_device rd;
    std::mt19937 mt(rd());
    //std::mt19937 mt(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<> dis(0, dist);
    std::generate(data.begin(), data.end(), std::bind(dis, std::ref(mt)));
}

struct Servant : boost::asynchronous::trackable_servant<>
{
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               boost::asynchronous::create_shared_scheduler_proxy(
                                                   new boost::asynchronous::threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<> >(boost::thread::hardware_concurrency())))
    {
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
        servant_dtor = true;
    }

    std::shared_future<void> start_parallel_merge_1000_700()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        generate(m_data1,1000,700);
        generate(m_data2,1000,700);
        std::sort(m_data1.begin(),m_data1.end(),std::less<int>());
        std::sort(m_data2.begin(),m_data2.end(),std::less<int>());
        m_res.resize(2000);
        // we need a promise to inform caller when we're done
        boost::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::shared_future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        auto data_copy1 = m_data1;
        auto data_copy2 = m_data2;
        // start long tasks
        post_callback(
           [tp,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");
                    std::vector<boost::thread::id> ids = tp.thread_ids();
                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_merge(m_data1.begin(),m_data1.end(),
                                                               m_data2.begin(),m_data2.end(),
                                                               m_res.begin(),
                                                               std::less<int>(),100);
                    },// work
           [aPromise,tp,data_copy1,data_copy2,this](boost::asynchronous::expected<void> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        std::vector<boost::thread::id> ids = tp.thread_ids();
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        std::vector<int> merged;
                        merged.resize(2000);
                        std::merge(data_copy1.begin(),data_copy1.end(),data_copy2.begin(),data_copy2.end(),merged.begin(),std::less<int>());
                        BOOST_CHECK_MESSAGE(merged == m_res,"parallel_merge gave a wrong value.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    std::shared_future<void> start_parallel_merge_1000_1000()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        generate(m_data1,1000,1000);
        generate(m_data2,1000,1000);
        std::sort(m_data1.begin(),m_data1.end(),std::less<int>());
        std::sort(m_data2.begin(),m_data2.end(),std::less<int>());
        m_res.resize(2000);
        // we need a promise to inform caller when we're done
        boost::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::shared_future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        auto data_copy1 = m_data1;
        auto data_copy2 = m_data2;
        // start long tasks
        post_callback(
           [tp,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");
                    std::vector<boost::thread::id> ids = tp.thread_ids();
                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_merge(m_data1.begin(),m_data1.end(),
                                                               m_data2.begin(),m_data2.end(),
                                                               m_res.begin(),
                                                               std::less<int>(),100);
                    },// work
           [aPromise,tp,data_copy1,data_copy2,this](boost::asynchronous::expected<void> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        std::vector<boost::thread::id> ids = tp.thread_ids();
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        std::vector<int> merged;
                        merged.resize(2000);
                        std::merge(data_copy1.begin(),data_copy1.end(),data_copy2.begin(),data_copy2.end(),merged.begin(),std::less<int>());
                        BOOST_CHECK_MESSAGE(merged == m_res,"parallel_merge gave a wrong value.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    std::shared_future<void> start_parallel_merge_100000000_80000000()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        generate(m_data1,100000000,80000000);
        generate(m_data2,100000000,80000000);
        std::sort(m_data1.begin(),m_data1.end(),std::less<int>());
        std::sort(m_data2.begin(),m_data2.end(),std::less<int>());
        m_res.resize(200000000);
        // we need a promise to inform caller when we're done
        boost::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::shared_future<void> fu = aPromise->get_future();
        // single-threaded
        std::vector<int> merged;
        merged.resize(200000000);
        std::merge(m_data1.begin(),m_data1.end(),m_data2.begin(),m_data2.end(),merged.begin(),std::less<int>());
        // start long tasks
        post_callback(
           [this](){
                    return boost::asynchronous::parallel_merge(m_data1.begin(),m_data1.end(),
                                                               m_data2.begin(),m_data2.end(),
                                                               m_res.begin(),
                                                               std::less<int>(),10000000);
                    },// work
           [aPromise,this](boost::asynchronous::expected<void>) mutable{
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
private:
    std::vector<int> m_data1;
    std::vector<int> m_data2;
    std::vector<int> m_res;
};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_merge_1000_700)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_merge_1000_1000)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_merge_100000000_80000000)
};
}

BOOST_AUTO_TEST_CASE( test_parallel_merge_1000_700 )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<> >);

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        std::shared_future<std::shared_future<void> > fuv = proxy.start_parallel_merge_1000_700();
        try
        {
            std::shared_future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}
BOOST_AUTO_TEST_CASE( test_parallel_merge_1000_1000 )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<> >);

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        std::shared_future<std::shared_future<void> > fuv = proxy.start_parallel_merge_1000_1000();
        try
        {
            std::shared_future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}
// for performance tests
BOOST_AUTO_TEST_CASE( test_parallel_merge_100000000_80000000 )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<> >);

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        std::shared_future<std::shared_future<void> > fuv = proxy.start_parallel_merge_100000000_80000000();
        try
        {
            std::shared_future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}
