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
#include <boost/asynchronous/algorithm/parallel_group_by.hpp>
#include <boost/asynchronous/algorithm/parallel_sort.hpp>

#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

// Here, we test the algorithm implemented in boost/asynchronous/algorithm/parallel_group_by.hpp.
// This algorithm relies on sorted data. As input data, we generate a sorted list of integers with
// duplicates (with randomly sized groups). For parallel_group_by, we only need to verify that the
// generated groups are correct and in the right order (the underlying task distribution relies
// entirely on parallel_grouped_transform, which is tested elsewhere).

namespace
{

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

BOOST_AUTO_TEST_CASE(test_parallel_group_by_iterators)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto data = generate();

    std::vector<size_t> group_map(data.back() + 1);
    for (const int& item : data) group_map[item]++;

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [&data]() mutable
        {
            return boost::asynchronous::parallel_group_by<std::vector<std::vector<int>>>(
                data.cbegin(),
                data.cend(),
                std::less<int>(),
                1500
            );
        },
        "test_parallel_group_by_iterators",
        0
    );

    try
    {
        auto groups = fu.get();

        for (const auto& group : groups)
            BOOST_CHECK_MESSAGE(group.size() == group_map[group[0]], "Found " + std::to_string(group.size()) + " items in group " + std::to_string(group[0]) + ", should have been " + std::to_string(group_map[group[0]]));

        BOOST_CHECK_MESSAGE(std::is_sorted(groups.begin(), groups.end(), [](const std::vector<int>& a, const std::vector<int>& b){ return a[0] < b[0]; }), "Groups are out of order");
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_parallel_group_by_reference)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto data = generate();

    std::vector<size_t> group_map(data.back() + 1);
    for (const int& item : data) group_map[item]++;

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [&data]() mutable
        {
            return boost::asynchronous::parallel_group_by<std::vector<std::vector<int>>>(
                data,
                std::less<int>(),
                1500
            );
        },
        "test_parallel_group_by_reference",
        0
    );

    try
    {
        auto groups = fu.get();

        for (const auto& group : groups)
            BOOST_CHECK_MESSAGE(group.size() == group_map[group[0]], "Found " + std::to_string(group.size()) + " items in group " + std::to_string(group[0]) + ", should have been " + std::to_string(group_map[group[0]]));

        BOOST_CHECK_MESSAGE(std::is_sorted(groups.begin(), groups.end(), [](const std::vector<int>& a, const std::vector<int>& b){ return a[0] < b[0]; }), "Groups are out of order");
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_parallel_group_by_moved_range)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto data = generate();

    std::vector<size_t> group_map(data.back() + 1);
    for (const int& item : data) group_map[item]++;

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [&data]() mutable
        {
            return boost::asynchronous::parallel_group_by<std::vector<std::vector<int>>>(
                std::move(data),
                std::less<int>(),
                1500
            );
        },
        "test_parallel_group_by_moved_range",
        0
    );

    try
    {
        auto groups = fu.get();

        for (const auto& group : groups)
            BOOST_CHECK_MESSAGE(group.size() == group_map[group[0]], "Found " + std::to_string(group.size()) + " items in group " + std::to_string(group[0]) + ", should have been " + std::to_string(group_map[group[0]]));

        BOOST_CHECK_MESSAGE(std::is_sorted(groups.begin(), groups.end(), [](const std::vector<int>& a, const std::vector<int>& b){ return a[0] < b[0]; }), "Groups are out of order");
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_parallel_group_by_continuation)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto data = generate();

    std::vector<size_t> group_map(data.back() + 1);
    for (const int& item : data) group_map[item]++;

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [&data]() mutable
        {
            return boost::asynchronous::parallel_group_by<std::vector<std::vector<int>>>(
                boost::asynchronous::parallel_sort(
                    std::move(data),
                    std::greater<int>(),
                    1500
                ),
                std::greater<int>(),
                1500
            );
        },
        "test_parallel_group_by_continuation",
        0
    );

    try
    {
        auto groups = fu.get();

        for (const auto& group : groups)
            BOOST_CHECK_MESSAGE(group.size() == group_map[group[0]], "Found " + std::to_string(group.size()) + " items in group " + std::to_string(group[0]) + ", should have been " + std::to_string(group_map[group[0]]));

        BOOST_CHECK_MESSAGE(std::is_sorted(groups.begin(), groups.end(), [](const std::vector<int>& a, const std::vector<int>& b){ return a[0] > b[0]; }), "Groups are out of order");
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

}
