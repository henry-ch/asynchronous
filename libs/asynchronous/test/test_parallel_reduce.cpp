// Boost.Asynchronous library
//  Copyright (C) Christophe Henry, Tobias Holl 2014
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <vector>
#include <set>
#include <string>

#include <boost/asynchronous/callable_any.hpp>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_reduce.hpp>
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

std::vector<int> mkdata() {
    std::vector<int> data;
    for (int i = 1; i <= 100; ++i) data.push_back(i);
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
        , m_data(mkdata())
    {
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
        servant_dtor = true;
    }

    boost::future<void> start_async_work()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work not posted.");
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
                    return boost::asynchronous::parallel_reduce(this->m_data.begin(),this->m_data.end(),
                                                           [](int const& a, int const& b)
                                                           {
                                                             return a + b;
                                                           },20);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<int> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        int r = res.get();
                        BOOST_CHECK_MESSAGE((r == 5050), ("result of parallel_reduce was " + std::to_string(r) + ", should have been 5050"));
                        // reset
                        m_data = mkdata();
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    boost::future<void> start_async_work_range_fct()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work not posted.");
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
                    return boost::asynchronous::parallel_reduce(this->m_data.begin(),this->m_data.end(),
                                                           [](std::vector<int>::iterator beg, std::vector<int>::iterator end)
                                                           {
                                                                int sum =0;
                                                                for(;beg != end; ++beg)
                                                                {
                                                                    sum += *beg;
                                                                }
                                                                return sum;
                                                           },
                                                           std::plus<int>(),20);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<int> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        int r = res.get();
                        BOOST_CHECK_MESSAGE((r == 5050), ("result of parallel_reduce was " + std::to_string(r) + ", should have been 5050"));
                        // reset
                        m_data = mkdata();
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    boost::future<void> start_async_work_range()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work_range not posted.");
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
                    return boost::asynchronous::parallel_reduce(const_cast<std::vector<int>const&>(this->m_data),
                                                           [](int const& a, int const& b)
                                                           {
                                                             return a + b;
                                                           },20);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<int> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        int r = res.get();
                        BOOST_CHECK_MESSAGE((r == 5050), ("result of parallel_reduce was " + std::to_string(r) + ", should have been 5050"));
                        // reset
                        m_data = mkdata();
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    boost::future<void> start_async_work_range_range_fct()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work_range not posted.");
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
                    return boost::asynchronous::parallel_reduce(const_cast<std::vector<int>const&>(this->m_data),
                                                                [](std::vector<int>::const_iterator beg, std::vector<int>::const_iterator end)
                                                                {
                                                                     int sum =0;
                                                                     for(;beg != end; ++beg)
                                                                     {
                                                                         sum += *beg;
                                                                     }
                                                                     return sum;
                                                                },
                                                                std::plus<int>(),20);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<int> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        int r = res.get();
                        BOOST_CHECK_MESSAGE((r == 5050), ("result of parallel_reduce was " + std::to_string(r) + ", should have been 5050"));
                        // reset
                        m_data = mkdata();
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    boost::future<void> start_async_work_range2()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work_range2 not posted.");
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
                    return boost::asynchronous::parallel_reduce(std::move(this->m_data),
                                                           [](int const& a, int const& b)
                                                           {
                                                             return a + b;
                                                           },20);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<int> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        int r = res.get();
                        BOOST_CHECK_MESSAGE((r == 5050), ("result of parallel_reduce was " + std::to_string(r) + ", should have been 5050"));
                        // reset
                        m_data = mkdata();
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    boost::future<void> start_async_work_range2_range_fct()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work_range2 not posted.");
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
                    return boost::asynchronous::parallel_reduce(std::move(this->m_data),
                                                                [](std::vector<int>::iterator beg, std::vector<int>::iterator end)
                                                                {
                                                                     int sum =0;
                                                                     for(;beg != end; ++beg)
                                                                     {
                                                                         sum += *beg;
                                                                     }
                                                                     return sum;
                                                                },
                                                                std::plus<int>(),20);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<int> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        int r = res.get();
                        BOOST_CHECK_MESSAGE((r == 5050), ("result of parallel_reduce was " + std::to_string(r) + ", should have been 5050"));
                        // reset
                        m_data = mkdata();
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    boost::future<void> start_async_work_range3()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work_range3 not posted.");
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
                    auto for_cont = boost::asynchronous::parallel_for(std::move(this->m_data),
                                                                      [](int const& i)
                                                                      {
                                                                        const_cast<int&>(i) += 1;
                                                                      }, 20);
                    auto add = [](int const& a, int const& b)
                               {
                                 return a + b;
                               };                   
                    return boost::asynchronous::parallel_reduce(for_cont, add, 20);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<int> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        int r = res.get();
                        BOOST_CHECK_MESSAGE((r == 5150), ("result of parallel_reduce after parallel_for was " + std::to_string(r) + ", should have been 5150"));
                        // reset
                        m_data = mkdata();
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    boost::future<void> start_async_work_range3_range_fct()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work_range3 not posted.");
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
                    auto for_cont = boost::asynchronous::parallel_for(std::move(this->m_data),
                                                                      [](int const& i)
                                                                      {
                                                                        const_cast<int&>(i) += 1;
                                                                      }, 20);
                    auto add = [](std::vector<int>::iterator beg, std::vector<int>::iterator end)
                    {
                         int sum =0;
                         for(;beg != end; ++beg)
                         {
                             sum += *beg;
                         }
                         return sum;
                    };
                    return boost::asynchronous::parallel_reduce(for_cont, add, std::plus<int>(),20);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<int> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        int r = res.get();
                        BOOST_CHECK_MESSAGE((r == 5150), ("result of parallel_reduce after parallel_for was " + std::to_string(r) + ", should have been 5150"));
                        // reset
                        m_data = mkdata();
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    boost::future<void> test_empty_range()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        m_data.clear();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_reduce(this->m_data.begin(),this->m_data.end(),
                                                           [](int const& a, int const& b)
                                                           {
                                                             return a + b;
                                                           },20);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<int> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        int r = res.get();
                        BOOST_CHECK_MESSAGE((r == 0), ("result of parallel_reduce was " + std::to_string(r) + ", should have been 0"));
                        // reset
                        m_data = mkdata();
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
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work)
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work_range_fct)
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work_range)
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work_range_range_fct)
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work_range2)
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work_range2_range_fct)
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work_range3)
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work_range3_range_fct)
    BOOST_ASYNC_FUTURE_MEMBER(test_empty_range)
};

}

BOOST_AUTO_TEST_CASE( test_parallel_reduce )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<void> > fuv = proxy.start_async_work();
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

BOOST_AUTO_TEST_CASE( test_parallel_reduce_range )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<void> > fuv = proxy.start_async_work_range();
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

BOOST_AUTO_TEST_CASE( test_parallel_reduce_range2 )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<void> > fuv = proxy.start_async_work_range2();
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

BOOST_AUTO_TEST_CASE( test_parallel_reduce_range3 )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<void> > fuv = proxy.start_async_work_range3();
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

BOOST_AUTO_TEST_CASE( test_parallel_reduce_empty_range )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<void> > fuv = proxy.test_empty_range();
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


BOOST_AUTO_TEST_CASE( test_parallel_reduce_range_fct )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<void> > fuv = proxy.start_async_work_range_fct();
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
BOOST_AUTO_TEST_CASE( test_parallel_reduce_range_range_fct )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<void> > fuv = proxy.start_async_work_range_range_fct();
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

BOOST_AUTO_TEST_CASE( test_parallel_reduce_range2_range_fct )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<void> > fuv = proxy.start_async_work_range2_range_fct();
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

BOOST_AUTO_TEST_CASE( test_parallel_reduce_range3_range_fct )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<void> > fuv = proxy.start_async_work_range3_range_fct();
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
