// Boost.Asynchronous library
//  Copyright (C) Tobias Holl 2018
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <vector>
#include <random>
#include <future>

#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/algorithm/parallel_grouped_for.hpp>
#include <boost/asynchronous/algorithm/parallel_sort.hpp>

#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

// Here, we test the algorithm implemented in boost/asynchronous/algorithm/parallel_grouped_for.hpp.
// This algorithm relies on sorted data. As input data, we generate a sorted list of integers with
// duplicates (with randomly sized groups). For parallel_grouped_for, we need to verify that (a)
// the functor is called for each element (unary functor) or group (binary functor), and that (b)
// no group is split across multiple tasks. Because we use the same mechanism that parallel_for
// uses for calling a unary functor for each element versus a binary functor once for a range, we
// can simply test here whether a group is split across multiple tasks. We can achieve this by
// counting the number of tasks handling each value in a std::vector<std::atomic<int>>. Afterwards,
// the entry for each value that was generated at least once should be exactly one. The remaining
// entries should be left at zero.

namespace
{

using iterator_type = typename std::vector<int>::iterator;

std::vector<int> generate()
{
    std::vector<int> data;
    data.reserve(10000);

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<> dis(0, 1);

    int key = 0;
    int decay = 0;
    for (int i = 0; i < 10000; ++i)
    {
        data.push_back(key);
        if (dis(mt) < 1.0 / (decay + 1.0)) { ++decay; }
        else { decay = 0; ++key; }
    }

    return data;
}

BOOST_AUTO_TEST_CASE(test_parallel_grouped_for_iterators)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto data = generate();
    std::vector<std::atomic<int>> log(data.back() + 1);

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [&data, &log]() mutable
        {
            return boost::asynchronous::parallel_grouped_for(
                data.begin(),
                data.end(),
                std::less<int>(),
                [&log](iterator_type begin, iterator_type end) mutable
                {
                    int value = -1; // Never generated
                    for (; begin != end; ++begin)
                    {
                        if (*begin != value)
                        {
                            value = *begin;
                            log[value]++;
                        }
                    }
                },
                1500
            );
        },
        "test_parallel_grouped_for_iterators",
        0
    );

    try
    {
        fu.get();
        for (size_t index = 0; index < log.size(); ++index)
        {
            int tasks = log[index].load();
            BOOST_CHECK_MESSAGE(tasks == 1, "Logged " + std::to_string(tasks) + " tasks for index " + std::to_string(index) + ", should be 1");
        }
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_parallel_grouped_for_range_reference)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto data = generate();
    std::vector<std::atomic<int>> log(data.back() + 1);

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [&data, &log]() mutable
        {
            return boost::asynchronous::parallel_grouped_for(
                data,
                std::less<int>(),
                [&log](iterator_type begin, iterator_type end) mutable
                {
                    int value = -1; // Never generated
                    for (; begin != end; ++begin)
                    {
                        if (*begin != value)
                        {
                            value = *begin;
                            log[value]++;
                        }
                    }
                },
                1500
            );
        },
        "test_parallel_grouped_for_range_reference",
        0
    );

    try
    {
        fu.get();
        for (size_t index = 0; index < log.size(); ++index)
        {
            int tasks = log[index].load();
            BOOST_CHECK_MESSAGE(tasks == 1, "Logged " + std::to_string(tasks) + " tasks for index " + std::to_string(index) + ", should be 1");
        }
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_parallel_grouped_for_moved_range)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto data = generate();
    std::vector<std::atomic<int>> log(data.back() + 1);

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [&data, &log]() mutable
        {
            return boost::asynchronous::parallel_grouped_for(
                std::move(data),
                std::less<int>(),
                [&log](iterator_type begin, iterator_type end) mutable
                {
                    int value = -1; // Never generated
                    for (; begin != end; ++begin)
                    {
                        if (*begin != value)
                        {
                            value = *begin;
                            log[value]++;
                        }
                    }
                },
                1500
            );
        },
        "test_parallel_grouped_for_moved_range",
        0
    );

    try
    {
        fu.get();
        for (size_t index = 0; index < log.size(); ++index)
        {
            int tasks = log[index].load();
            BOOST_CHECK_MESSAGE(tasks == 1, "Logged " + std::to_string(tasks) + " tasks for index " + std::to_string(index) + ", should be 1");
        }
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_parallel_grouped_for_continuation)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto data = generate();
    std::vector<std::atomic<int>> log(data.back() + 1);

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [&data, &log]() mutable
        {
            return boost::asynchronous::parallel_grouped_for(
                boost::asynchronous::parallel_sort(
                    std::move(data),
                    std::greater<int>(),
                    1500
                ),
                std::greater<int>(),
                [&log](iterator_type begin, iterator_type end) mutable
                {
                    int value = -1; // Never generated
                    for (; begin != end; ++begin)
                    {
                        if (*begin != value)
                        {
                            value = *begin;
                            log[value]++;
                        }
                    }
                },
                1500
            );
        },
        "test_parallel_grouped_for_continuation",
        0
    );

    try
    {
        fu.get();
        for (size_t index = 0; index < log.size(); ++index)
        {
            int tasks = log[index].load();
            BOOST_CHECK_MESSAGE(tasks == 1, "Logged " + std::to_string(tasks) + " tasks for index " + std::to_string(index) + ", should be 1");
        }
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

}
