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
#include <boost/asynchronous/algorithm/parallel_sort.hpp>
#include <boost/asynchronous/algorithm/parallel_grouped_transform.hpp>

#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

// This is the test for the algorithm from boost/asynchronous/algorithm/parallel_grouped_transform.hpp.
// This algorithm relies on sorted data. As input data, we generate a sorted list of integers with
// duplicates (with randomly sized groups). For parallel_grouped_transform, we need to verify that (a)
// the functor is called for each element (unary functor) or group (ternary functor), and that (b)
// no group is split across multiple tasks. Like in the test for parallel_grouped_for, we can achieve
// this by counting the number of tasks handling each value in a std::vector<std::atomic<int>>.
// Afterwards, the entry for each value that was generated at least once should be exactly one.
// Additionally, we need to verify that parallel_grouped_transform actually produces the correct output.

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

BOOST_AUTO_TEST_CASE(test_parallel_grouped_transform_iterators_bulk)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto data = generate();
    std::vector<std::atomic<int>> log(data.back() + 1);
    std::vector<int> output(data.size());

    auto expected_result = data;
    for (int& v : expected_result) v *= 2;

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [&data, &log, &output]() mutable
        {
            return boost::asynchronous::parallel_grouped_transform(
                data.begin(),
                data.end(),
                output.begin(),
                std::less<int>(),
                [&log](iterator_type begin, iterator_type end, iterator_type out) mutable
                {
                    std::transform(begin, end, out, [](const int& v) { return v * 2; });

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
        "test_parallel_grouped_transform_iterators",
        0
    );

    try
    {
        auto output_iter = fu.get();
        BOOST_CHECK_MESSAGE(output_iter == output.end(), "result != output.end() - did not transform all items");
        for (size_t index = 0; index < output.size(); ++index)
            BOOST_CHECK_MESSAGE(output[index] == expected_result[index], "output[" + std::to_string(index) + "] != " + std::to_string(expected_result[index]) + ", was " + std::to_string(output[index]) + " instead");
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

BOOST_AUTO_TEST_CASE(test_parallel_grouped_transform_range_reference_bulk)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto data = generate();
    std::vector<std::atomic<int>> log(data.back() + 1);
    std::vector<int> output(data.size());

    auto expected_result = data;
    for (int& v : expected_result) v *= 2;

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [&data, &log, &output]() mutable
        {
            return boost::asynchronous::parallel_grouped_transform(
                data,
                output.begin(),
                std::less<int>(),
                [&log](iterator_type begin, iterator_type end, iterator_type out) mutable
                {
                    std::transform(begin, end, out, [](const int& v) { return v * 2; });

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
        "test_parallel_grouped_transform_range_reference",
        0
    );

    try
    {
        auto output_iter = fu.get();
        BOOST_CHECK_MESSAGE(output_iter == output.end(), "result != output.end() - did not transform all items");
        for (size_t index = 0; index < output.size(); ++index)
            BOOST_CHECK_MESSAGE(output[index] == expected_result[index], "output[" + std::to_string(index) + "] != " + std::to_string(expected_result[index]) + ", was " + std::to_string(output[index]) + " instead");
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

BOOST_AUTO_TEST_CASE(test_parallel_grouped_transform_moved_range_bulk)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto data = generate();
    std::vector<std::atomic<int>> log(data.back() + 1);

    auto expected_result = data;
    for (int& v : expected_result) v *= 2;

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [&data, &log]() mutable
        {
            return boost::asynchronous::parallel_grouped_transform<std::vector<int>>(
                std::move(data),
                std::less<int>(),
                [&log](iterator_type begin, iterator_type end, iterator_type out) mutable
                {
                    std::transform(begin, end, out, [](const int& v) { return v * 2; });

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
        "test_parallel_grouped_transform_moved_range",
        0
    );

    try
    {
        auto output = fu.get();
        BOOST_CHECK_MESSAGE(output.size() == expected_result.size(), "output.size() != " + std::to_string(expected_result.size()) + " - did not transform all items");
        for (size_t index = 0; index < output.size(); ++index)
            BOOST_CHECK_MESSAGE(output[index] == expected_result[index], "output[" + std::to_string(index) + "] != " + std::to_string(expected_result[index]) + ", was " + std::to_string(output[index]) + " instead");
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

BOOST_AUTO_TEST_CASE(test_parallel_grouped_transform_continuation_bulk)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto data = generate();
    std::vector<std::atomic<int>> log(data.back() + 1);

    auto expected_result = data;
    std::sort(expected_result.begin(), expected_result.end(), std::greater<int>());
    for (int& v : expected_result) v *= 2;

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [&data, &log]() mutable
        {
            return boost::asynchronous::parallel_grouped_transform<std::vector<int>>(
                boost::asynchronous::parallel_sort(
                    std::move(data),
                    std::greater<int>(),
                    1500
                ),
                std::greater<int>(),
                [&log](iterator_type begin, iterator_type end, iterator_type out) mutable
                {
                    std::transform(begin, end, out, [](const int& v) { return v * 2; });

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
        "test_parallel_grouped_transform_continuation",
        0
    );

    try
    {
        auto output = fu.get();
        BOOST_CHECK_MESSAGE(output.size() == expected_result.size(), "output.size() != " + std::to_string(expected_result.size()) + " - did not transform all items");
        for (size_t index = 0; index < output.size(); ++index)
            BOOST_CHECK_MESSAGE(output[index] == expected_result[index], "output[" + std::to_string(index) + "] != " + std::to_string(expected_result[index]) + ", was " + std::to_string(output[index]) + " instead");
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


BOOST_AUTO_TEST_CASE(test_parallel_grouped_transform_iterators_single)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto data = generate();
    std::vector<int> output(data.size());

    auto expected_result = data;
    for (int& v : expected_result) v *= 2;

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [&data, &output]() mutable
        {
            return boost::asynchronous::parallel_grouped_transform(
                data.begin(),
                data.end(),
                output.begin(),
                std::less<int>(),
                [](int& value) { return value * 2; },
                1500
            );
        },
        "test_parallel_grouped_transform_iterators",
        0
    );

    try
    {
        auto output_iter = fu.get();
        BOOST_CHECK_MESSAGE(output_iter == output.end(), "result != output.end() - did not transform all items");
        for (size_t index = 0; index < output.size(); ++index)
            BOOST_CHECK_MESSAGE(output[index] == expected_result[index], "output[" + std::to_string(index) + "] != " + std::to_string(expected_result[index]) + ", was " + std::to_string(output[index]) + " instead");
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_parallel_grouped_transform_range_reference)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto data = generate();
    std::vector<int> output(data.size());

    auto expected_result = data;
    for (int& v : expected_result) v *= 2;

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [&data, &output]() mutable
        {
            return boost::asynchronous::parallel_grouped_transform(
                data,
                output.begin(),
                std::less<int>(),
                [](int& value) { return value * 2; },
                1500
            );
        },
        "test_parallel_grouped_transform_range_reference",
        0
    );

    try
    {
        auto output_iter = fu.get();
        BOOST_CHECK_MESSAGE(output_iter == output.end(), "result != output.end() - did not transform all items");
        for (size_t index = 0; index < output.size(); ++index)
            BOOST_CHECK_MESSAGE(output[index] == expected_result[index], "output[" + std::to_string(index) + "] != " + std::to_string(expected_result[index]) + ", was " + std::to_string(output[index]) + " instead");
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_parallel_grouped_transform_moved_range)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto data = generate();

    auto expected_result = data;
    for (int& v : expected_result) v *= 2;

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [&data]() mutable
        {
            return boost::asynchronous::parallel_grouped_transform<std::vector<int>>(
                std::move(data),
                std::less<int>(),
                [](int& value) { return value * 2; },
                1500
            );
        },
        "test_parallel_grouped_transform_moved_range",
        0
    );

    try
    {
        auto output = fu.get();
        BOOST_CHECK_MESSAGE(output.size() == expected_result.size(), "output.size() != " + std::to_string(expected_result.size()) + " - did not transform all items");
        for (size_t index = 0; index < output.size(); ++index)
            BOOST_CHECK_MESSAGE(output[index] == expected_result[index], "output[" + std::to_string(index) + "] != " + std::to_string(expected_result[index]) + ", was " + std::to_string(output[index]) + " instead");
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_parallel_grouped_transform_continuation)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto data = generate();
    std::vector<std::atomic<int>> log(data.back() + 1);

    auto expected_result = data;
    std::sort(expected_result.begin(), expected_result.end(), std::greater<int>());
    for (int& v : expected_result) v *= 2;

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [&data]() mutable
        {
            return boost::asynchronous::parallel_grouped_transform<std::vector<int>>(
                boost::asynchronous::parallel_sort(
                    std::move(data),
                    std::greater<int>(),
                    1500
                ),
                std::greater<int>(),
                [](int& value) { return value * 2; },
                1500
            );
        },
        "test_parallel_grouped_transform_continuation",
        0
    );

    try
    {
        auto output = fu.get();
        BOOST_CHECK_MESSAGE(output.size() == expected_result.size(), "output.size() != " + std::to_string(expected_result.size()) + " - did not transform all items");
        for (size_t index = 0; index < output.size(); ++index)
            BOOST_CHECK_MESSAGE(output[index] == expected_result[index], "output[" + std::to_string(index) + "] != " + std::to_string(expected_result[index]) + ", was " + std::to_string(output[index]) + " instead");
    }
    catch (...)
    {
        BOOST_FAIL("unexpected exception");
    }
}

}
