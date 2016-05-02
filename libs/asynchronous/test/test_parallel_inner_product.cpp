// Boost.Asynchronous library
//  Copyright (C) Christophe Henry, Tobias Holl 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <algorithm>
#include <vector>
#include <set>
#include <numeric>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_inner_product.hpp>
#include <boost/asynchronous/algorithm/parallel_for.hpp>

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

constexpr unsigned long long VECTOR_SIZE = 10000;
constexpr unsigned long long STARTING_VALUE = 42;
constexpr unsigned long long GROUND_TRUTH_WITHOUT_START = 333283335000; // Sum of squares of 0...9999 (excl. 10000)
constexpr unsigned long long GROUND_TRUTH_WITH_START = GROUND_TRUTH_WITHOUT_START + STARTING_VALUE;
constexpr unsigned long long MODIFIED_GROUND_TRUTH_WITHOUT_START = GROUND_TRUTH_WITHOUT_START + (VECTOR_SIZE * VECTOR_SIZE); // When every element is increased by one (continuation test), we need to add 10000*10000
constexpr unsigned long long MODIFIED_GROUND_TRUTH_WITH_START = MODIFIED_GROUND_TRUTH_WITHOUT_START + STARTING_VALUE;

std::vector<unsigned long long> mkdata() {
    std::vector<unsigned long long> data(VECTOR_SIZE);
    std::iota(data.begin(), data.end(), 0);
    return data;
}

struct Servant : boost::asynchronous::trackable_servant<>
{
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<>>>(6))
        , m_data1(mkdata())
        , m_data2(mkdata())
    {
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
        servant_dtor = true;
    }

    boost::shared_future<void> iterators_with_start()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant iterators_with_start not posted.");
        // we need a promise to inform caller when we're done
        boost::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::shared_future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_inner_product(this->m_data1.begin(), this->m_data1.end(), this->m_data2.begin(),
                                                                       [](unsigned long long a, unsigned long long b){ return a * b; },
                                                                       [](unsigned long long a, unsigned long long b){ return a + b; },
                                                                       STARTING_VALUE, 1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<unsigned long long> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(res.get() == GROUND_TRUTH_WITH_START, "Wrong result.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    boost::shared_future<void> iterators_without_start()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant iterators_without_start not posted.");
        // we need a promise to inform caller when we're done
        boost::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::shared_future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_inner_product(this->m_data1.begin(), this->m_data1.end(), this->m_data2.begin(),
                                                                       [](unsigned long long a, unsigned long long b){ return a * b; },
                                                                       [](unsigned long long a, unsigned long long b){ return a + b; },
                                                                       1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<unsigned long long> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(res.get() == GROUND_TRUTH_WITHOUT_START, "Wrong result.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    boost::shared_future<void> moved_ranges_with_start()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant moved_ranges_with_start not posted.");
        // we need a promise to inform caller when we're done
        boost::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::shared_future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_inner_product(std::move(this->m_data1), std::move(this->m_data2),
                                                                       [](unsigned long long a, unsigned long long b){ return a * b; },
                                                                       [](unsigned long long a, unsigned long long b){ return a + b; },
                                                                       STARTING_VALUE, 1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<unsigned long long> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(res.get() == GROUND_TRUTH_WITH_START, "Wrong result.");
                        // reset
                        m_data1 = mkdata();
                        m_data2 = mkdata();
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    boost::shared_future<void> moved_ranges_without_start()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant moved_ranges_without_start not posted.");
        // we need a promise to inform caller when we're done
        boost::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::shared_future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_inner_product(std::move(this->m_data1), std::move(this->m_data2),
                                                                       [](unsigned long long a, unsigned long long b){ return a * b; },
                                                                       [](unsigned long long a, unsigned long long b){ return a + b; },
                                                                       1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<unsigned long long> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(res.get() == GROUND_TRUTH_WITHOUT_START, "Wrong result.");
                        // reset
                        m_data1 = mkdata();
                        m_data2 = mkdata();
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    boost::shared_future<void> continuations_with_start()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant continuations_with_start not posted.");
        // we need a promise to inform caller when we're done
        boost::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::shared_future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_inner_product(
                                boost::asynchronous::parallel_for(
                                    std::move(this->m_data1),
                                    [](unsigned long long & value){ ++value; },
                                    1500
                                ),
                                boost::asynchronous::parallel_for(
                                    std::move(this->m_data2),
                                    [](unsigned long long & value){ ++value; },
                                    1500
                                ),
                                [](unsigned long long a, unsigned long long b){ return a * b; },
                                [](unsigned long long a, unsigned long long b){ return a + b; },
                                STARTING_VALUE, 1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<unsigned long long> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(res.get() == MODIFIED_GROUND_TRUTH_WITH_START, "Wrong result.");
                        // reset
                        m_data1 = mkdata();
                        m_data2 = mkdata();
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    boost::shared_future<void> continuations_without_start()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant continuations_without_start not posted.");
        // we need a promise to inform caller when we're done
        boost::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::shared_future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_inner_product(
                                boost::asynchronous::parallel_for(
                                    std::move(this->m_data1),
                                    [](unsigned long long & value){ ++value; },
                                    1500
                                ),
                                boost::asynchronous::parallel_for(
                                    std::move(this->m_data2),
                                    [](unsigned long long & value){ ++value; },
                                    1500
                                ),
                                [](unsigned long long a, unsigned long long b){ return a * b; },
                                [](unsigned long long a, unsigned long long b){ return a + b; },
                                1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<unsigned long long> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(res.get() == MODIFIED_GROUND_TRUTH_WITHOUT_START, "Wrong result.");
                        // reset
                        m_data1 = mkdata();
                        m_data2 = mkdata();
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
private:
    std::vector<unsigned long long> m_data1;
    std::vector<unsigned long long> m_data2;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(iterators_with_start)
    BOOST_ASYNC_FUTURE_MEMBER(iterators_without_start)
    BOOST_ASYNC_FUTURE_MEMBER(moved_ranges_with_start)
    BOOST_ASYNC_FUTURE_MEMBER(moved_ranges_without_start)
    BOOST_ASYNC_FUTURE_MEMBER(continuations_with_start)
    BOOST_ASYNC_FUTURE_MEMBER(continuations_without_start)
};

}

BOOST_AUTO_TEST_CASE( test_parallel_inner_product_iterators_with_start )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::shared_future<boost::shared_future<void> > fuv = proxy.iterators_with_start();
        try
        {
            boost::shared_future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_parallel_inner_product_iterators_without_start )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::shared_future<boost::shared_future<void> > fuv = proxy.iterators_without_start();
        try
        {
            boost::shared_future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_parallel_inner_product_moved_ranges_with_start )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::shared_future<boost::shared_future<void> > fuv = proxy.moved_ranges_with_start();
        try
        {
            boost::shared_future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_parallel_inner_product_moved_ranges_without_start )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::shared_future<boost::shared_future<void> > fuv = proxy.moved_ranges_without_start();
        try
        {
            boost::shared_future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_parallel_inner_product_continuations_with_start )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::shared_future<boost::shared_future<void> > fuv = proxy.continuations_with_start();
        try
        {
            boost::shared_future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_parallel_inner_product_continuations_without_start )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::shared_future<boost::shared_future<void> > fuv = proxy.continuations_without_start();
        try
        {
            boost::shared_future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

