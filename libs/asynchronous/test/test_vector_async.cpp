// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE
#endif
#include <algorithm>
#include <vector>

#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/asynchronous/container/vector.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/diagnostics/any_loggable.hpp>
#include "test_common.hpp"

#include <boost/test/unit_test.hpp>


using namespace boost::asynchronous::test;

namespace
{
struct some_type
{
    some_type(int d=0)
        :data(d)
    {
    }
    int data;
};
}

// asynchronous interface tests
BOOST_AUTO_TEST_CASE( test_make_asynchronous_range)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::future<boost::asynchronous::vector<some_type>> fu = boost::asynchronous::post_future(scheduler,
    []()mutable
    {
        return boost::asynchronous::make_asynchronous_range<boost::asynchronous::vector<some_type>>
                        (10000,100);
    },
    "test_vector_async_ctor_push_back",0);
    boost::asynchronous::vector<some_type> v (std::move(fu.get()));
    BOOST_CHECK_MESSAGE(v.size() == 10000,"vector size should be 10000.");
    BOOST_CHECK_MESSAGE(v[0].data == 0,"vector[0] should have value 0.");
    // vectors created this way have no scheduler as worker (would be unsafe to have a scheduler in its own thread)
    // so we need to add one (note: this vector is then no more allowed to be posted into this scheduler)
    v.set_scheduler(scheduler);
    v.push_back(some_type(42));
    BOOST_CHECK_MESSAGE(v.size() == 10001,"vector size should be 10001.");
    BOOST_CHECK_MESSAGE(v[0].data == 0,"vector[0] should have value 0.");
    BOOST_CHECK_MESSAGE(v[10000].data == 42,"vector[10000] should have value 42.");
}

BOOST_AUTO_TEST_CASE( test_vector_async_ctor_push_back)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::future<boost::asynchronous::vector<some_type>> fu = boost::asynchronous::post_future(scheduler,
    []()mutable
    {
        return boost::asynchronous::async_push_back(
                    boost::asynchronous::async_push_back(
                        boost::asynchronous::make_asynchronous_range<boost::asynchronous::vector<some_type>>
                            (10000,100),
                        some_type(42)),
                    some_type(41));
    },
    "test_vector_async_ctor_push_back",0);
    boost::asynchronous::vector<some_type> v (std::move(fu.get()));
    BOOST_CHECK_MESSAGE(v.size() == 10002,"vector size should be 10001.");
    BOOST_CHECK_MESSAGE(v[0].data == 0,"vector[0] should have value 0.");
    BOOST_CHECK_MESSAGE(v[10000].data == 42,"vector[10000] should have value 42.");
    BOOST_CHECK_MESSAGE(v[10001].data == 41,"vector[10001] should have value 41.");
}
