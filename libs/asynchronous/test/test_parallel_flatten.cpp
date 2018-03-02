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
#include <future>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_for.hpp>
#include <boost/asynchronous/algorithm/parallel_flatten.hpp>

#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

namespace
{
// main thread id
boost::thread::id main_thread_id;
bool servant_dtor=false;

void generate(std::vector<std::vector<int>>& data)
{
    data = std::vector<std::vector<int>>{};
    data.reserve(10000);

    for (int i = 0; i < 10000; ++i)
        data.push_back(std::vector<int>(5, i));
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

    std::future<void> start_parallel_flatten_iterators()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"start_parallel_unique_sorted not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();

        // start long tasks
        post_callback(
            [ids,this]()
            {
                BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");
                BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");

                return boost::asynchronous::parallel_flatten(this->m_data.begin(),
                                                             this->m_data.end(),
                                                             1500,
                                                             1500);
            },// work
            [aPromise,ids,this](boost::asynchronous::expected<std::vector<int>> res) mutable
            {
                BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");

                auto vec = std::move(res.get());
                BOOST_CHECK_MESSAGE(vec.size() == 50000, "vec.size() != 50000");
                BOOST_CHECK_MESSAGE(vec[0] == 0, "vec[0] != 0");
                BOOST_CHECK_MESSAGE(vec[25000] == 5000, "vec[25000] != 5000");
                BOOST_CHECK_MESSAGE(vec[49999] == 9999, "vec[49999] != 9999");

                // reset
                generate(this->m_data);
                aPromise->set_value();
            }// callback functor.
        );
        return fu;
    }

    std::future<void> start_parallel_flatten_reference()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"start_parallel_unique_sorted not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();

        // start long tasks
        post_callback(
            [ids,this]()
            {
                BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");
                BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");

                return boost::asynchronous::parallel_flatten(this->m_data,
                                                             1500,
                                                             1500);
            },// work
            [aPromise,ids,this](boost::asynchronous::expected<std::vector<int>> res) mutable
            {
                BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");

                auto vec = std::move(res.get());
                BOOST_CHECK_MESSAGE(vec.size() == 50000, "vec.size() != 50000");
                BOOST_CHECK_MESSAGE(vec[0] == 0, "vec[0] != 0");
                BOOST_CHECK_MESSAGE(vec[25000] == 5000, "vec[25000] != 5000");
                BOOST_CHECK_MESSAGE(vec[49999] == 9999, "vec[49999] != 9999");

                // reset
                generate(this->m_data);
                aPromise->set_value();
            }// callback functor.
        );
        return fu;
    }

    std::future<void> start_parallel_flatten_moved()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"start_parallel_unique_sorted not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();

        // start long tasks
        post_callback(
            [ids,this]()
            {
                BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");
                BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");

                return boost::asynchronous::parallel_flatten(std::move(this->m_data),
                                                             1500,
                                                             1500);
            },// work
            [aPromise,ids,this](boost::asynchronous::expected<std::vector<int>> res) mutable
            {
                BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");

                auto vec = std::move(res.get());
                BOOST_CHECK_MESSAGE(vec.size() == 50000, "vec.size() != 50000");
                BOOST_CHECK_MESSAGE(vec[0] == 0, "vec[0] != 0");
                BOOST_CHECK_MESSAGE(vec[25000] == 5000, "vec[25000] != 5000");
                BOOST_CHECK_MESSAGE(vec[49999] == 9999, "vec[49999] != 9999");

                // reset
                generate(this->m_data);
                aPromise->set_value();
            }// callback functor.
        );
        return fu;
    }

    std::future<void> start_parallel_flatten_continuation()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"start_parallel_unique_sorted not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();

        // start long tasks
        post_callback(
            [ids,this]()
            {
                BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");
                BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");

                return boost::asynchronous::parallel_flatten(
                    boost::asynchronous::parallel_for(
                        std::move(this->m_data),
                        [](const std::vector<int>& item)
                        {
                            for (int i = 0; i < item.size(); ++i)
                                const_cast<std::vector<int>&>(item)[i] = 5 * item[i] + i;
                        },
                        1500
                    ),
                    1500,
                    1500
                );
            },// work
            [aPromise,ids,this](boost::asynchronous::expected<std::vector<int>> res) mutable
            {
                BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");

                auto vec = std::move(res.get());
                BOOST_CHECK_MESSAGE(vec.size() == 50000, "vec.size() != 50000");
                BOOST_CHECK_MESSAGE(vec[0] == 0, "vec[0] != 0");
                BOOST_CHECK_MESSAGE(vec[25000] == 25000, "vec[25000] != 25000");
                BOOST_CHECK_MESSAGE(vec[49999] == 49999, "vec[49999] != 49999");

                // reset
                generate(this->m_data);
                aPromise->set_value();
            }// callback functor.
        );
        return fu;
    }

private:
    std::vector<std::vector<int>> m_data;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
#ifndef _MSC_VER
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_flatten_iterators)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_flatten_reference)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_flatten_moved)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_flatten_continuation)
#else
    BOOST_ASYNC_FUTURE_MEMBER_1(start_parallel_flatten_iterators)
    BOOST_ASYNC_FUTURE_MEMBER_1(start_parallel_flatten_reference)
    BOOST_ASYNC_FUTURE_MEMBER_1(start_parallel_flatten_moved)
    BOOST_ASYNC_FUTURE_MEMBER_1(start_parallel_flatten_continuation)
#endif
};

}

#define ASYNCHRONOUS_AUTO_TEST_CASE(func) \
    BOOST_AUTO_TEST_CASE( test_##func ) \
    { \
        servant_dtor=false; \
        { \
            auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler< \
                                                                                boost::asynchronous::lockfree_queue<>>>(); \
            main_thread_id = boost::this_thread::get_id(); \
            ServantProxy proxy(scheduler); \
            auto fuv = proxy.start_##func(); \
            try \
            { \
                auto resfuv = fuv.get(); \
                resfuv.get(); \
            } \
            catch(...) \
            { \
                BOOST_FAIL( "unexpected exception" ); \
            } \
        } \
        BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called."); \
    }

ASYNCHRONOUS_AUTO_TEST_CASE(parallel_flatten_iterators)
ASYNCHRONOUS_AUTO_TEST_CASE(parallel_flatten_reference)
ASYNCHRONOUS_AUTO_TEST_CASE(parallel_flatten_moved)
ASYNCHRONOUS_AUTO_TEST_CASE(parallel_flatten_continuation)
