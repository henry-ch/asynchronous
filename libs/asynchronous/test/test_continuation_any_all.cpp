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
#include <boost/asynchronous/algorithm/all_of.hpp>
#include <boost/asynchronous/algorithm/any_of.hpp>
#include <boost/asynchronous/algorithm/parallel_for.hpp>
#include <boost/asynchronous/algorithm/parallel_reduce.hpp>
#include <boost/asynchronous/algorithm/parallel_sort.hpp>

#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

namespace
{

std::vector<int> generate(size_t size = 10000)
{
    std::vector<int> data(size);
    std::random_device rd;
    std::mt19937 mt(rd());
    std::generate(data.begin(), data.end(), mt);
    return data;
}

BOOST_AUTO_TEST_CASE(test_continuation_all_of)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto a = generate();
    auto b = generate();
    auto c = std::vector<int>(10000, 42);

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [&a, &b, &c]() mutable
        {
            return boost::asynchronous::all_of(
                boost::asynchronous::parallel_sort(a.begin(), a.end(), std::less<int>(), 1500, "sort a", 0),
                boost::asynchronous::parallel_sort(b.begin(), b.end(), std::less<int>(), 1500, "sort b", 0),
                boost::asynchronous::parallel_reduce(c.begin(), c.end(), std::plus<int>(), 1500, "reduce c", 0)
            );
        },
        "test_continuation_all_of",
        0
    );

    try
    {
        auto tup = fu.get();
        BOOST_CHECK_MESSAGE(std::is_sorted(a.begin(), a.end()), "Vector 'a'  was not sorted");
        BOOST_CHECK_MESSAGE(std::is_sorted(b.begin(), b.end()), "Vector 'b'  was not sorted");
        BOOST_CHECK_MESSAGE(std::get<2>(tup) == 420000, "parallel_reduce of vector 'c' returned the wrong result (" + std::to_string(std::get<2>(tup)) + ", should have been 420000)");
        static_assert(std::is_same<typename std::decay<decltype(std::get<0>(tup))>::type, boost::asynchronous::detail::void_wrapper>::value, "parallel_sort of 'a' returned wrong type through all(...)");
        static_assert(std::is_same<typename std::decay<decltype(std::get<1>(tup))>::type, boost::asynchronous::detail::void_wrapper>::value, "parallel_sort of 'b' returned wrong type through all(...)");
        static_assert(std::is_same<typename std::decay<decltype(std::get<2>(tup))>::type, int>::value, "parallel_reduce of 'c' returned wrong type through all(...)");
    }
    catch (...)
    {
        BOOST_FAIL("Unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_continuation_all_of_with_exception)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto a = generate();
    auto b = generate();

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [&a, &b]() mutable
        {
            return boost::asynchronous::all_of(
                boost::asynchronous::parallel_sort(a.begin(), a.end(), std::less<int>(), 1500, "sort a", 0),
                boost::asynchronous::parallel_for(b.begin(), b.end(), [](const int&) { throw std::logic_error("Testing exception propagation"); }, 1500, "parallel_for of b", 0)
            );
        },
        "test_continuation_all_of_with_exception",
        0
    );

    try
    {
        fu.get();
        BOOST_FAIL("Should have gotten an exception");
    }
    catch (const std::logic_error& e)
    {
        BOOST_CHECK_MESSAGE(e.what() == std::string("Testing exception propagation"), "Got incorrect exception message");
    }
    catch (...)
    {
        BOOST_FAIL("Unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_continuation_all_of_tuple)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto a = generate();
    auto b = generate();
    auto c = std::vector<int>(10000, 42);

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [&a, &b, &c]() mutable
        {
            return boost::asynchronous::all_of(
                std::make_tuple(
                    boost::asynchronous::parallel_sort(a.begin(), a.end(), std::less<int>(), 1500, "sort a", 0),
                    boost::asynchronous::parallel_sort(b.begin(), b.end(), std::less<int>(), 1500, "sort b", 0),
                    boost::asynchronous::parallel_reduce(c.begin(), c.end(), std::plus<int>(), 1500, "reduce c", 0)
                )
            );
        },
        "test_continuation_all_of_tuple",
        0
    );

    try
    {
        auto tup = fu.get();
        BOOST_CHECK_MESSAGE(std::is_sorted(a.begin(), a.end()), "Vector 'a'  was not sorted");
        BOOST_CHECK_MESSAGE(std::is_sorted(b.begin(), b.end()), "Vector 'b'  was not sorted");
        BOOST_CHECK_MESSAGE(std::get<2>(tup) == 420000, "parallel_reduce of vector 'c' returned the wrong result (" + std::to_string(std::get<2>(tup)) + ", should have been 420000)");
        static_assert(std::is_same<typename std::decay<decltype(std::get<0>(tup))>::type, boost::asynchronous::detail::void_wrapper>::value, "parallel_sort of 'a' returned wrong type through all(...)");
        static_assert(std::is_same<typename std::decay<decltype(std::get<1>(tup))>::type, boost::asynchronous::detail::void_wrapper>::value, "parallel_sort of 'b' returned wrong type through all(...)");
        static_assert(std::is_same<typename std::decay<decltype(std::get<2>(tup))>::type, int>::value, "parallel_reduce of 'c' returned wrong type through all(...)");
    }
    catch (...)
    {
        BOOST_FAIL("Unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_continuation_all_of_tuple_with_exception)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto a = generate();
    auto b = generate();

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [&a, &b]() mutable
        {
            return boost::asynchronous::all_of(
                std::make_tuple(
                    boost::asynchronous::parallel_sort(a.begin(), a.end(), std::less<int>(), 1500, "sort a", 0),
                    boost::asynchronous::parallel_for(b.begin(), b.end(), [](const int&) { throw std::logic_error("Testing exception propagation"); }, 1500, "parallel_for of b", 0)
                )
            );
        },
        "test_continuation_all_of_tuple_with_exception",
        0
    );

    try
    {
        fu.get();
        BOOST_FAIL("Should have gotten an exception");
    }
    catch (const std::logic_error& e)
    {
        BOOST_CHECK_MESSAGE(e.what() == std::string("Testing exception propagation"), "Got incorrect exception message");
    }
    catch (...)
    {
        BOOST_FAIL("Unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_continuation_all_of_vector)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto a = generate();
    auto b = generate();

    auto fu1 = boost::asynchronous::post_future(
        scheduler,
        [&a, &b]() mutable
        {
            std::vector<decltype(boost::asynchronous::parallel_sort(a.begin(), a.end(), std::less<int>(), 1500, "sort a", 0))> continuations = {
                boost::asynchronous::parallel_sort(a.begin(), a.end(), std::less<int>(), 1500, "sort a", 0),
                boost::asynchronous::parallel_sort(b.begin(), b.end(), std::less<int>(), 1500, "sort b", 0)
            };
            return boost::asynchronous::all_of(std::move(continuations));
        },
        "test_continuation_all_of_vector_1",
        0
    );

    try
    {
        auto vec = fu1.get();
        BOOST_CHECK_MESSAGE(std::is_sorted(a.begin(), a.end()), "Vector 'a'  was not sorted");
        BOOST_CHECK_MESSAGE(std::is_sorted(b.begin(), b.end()), "Vector 'b'  was not sorted");
        static_assert(std::is_same<typename std::decay<decltype(vec[0])>::type, boost::asynchronous::detail::void_wrapper>::value, "parallel_sort of 'a' returned wrong type through all(...)");
        static_assert(std::is_same<typename std::decay<decltype(vec[1])>::type, boost::asynchronous::detail::void_wrapper>::value, "parallel_sort of 'b' returned wrong type through all(...)");
    }
    catch (...)
    {
        BOOST_FAIL("Unexpected exception");
    }

    auto c = std::vector<int>(10000, 42);
    auto d = std::vector<int>(10000, 123);

    auto fu2 = boost::asynchronous::post_future(
        scheduler,
        [&c, &d]() mutable
        {
            std::vector<decltype(boost::asynchronous::parallel_reduce(c.begin(), c.end(), std::plus<int>(), 1500, "reduce c", 0))> continuations = {
                boost::asynchronous::parallel_reduce(c.begin(), c.end(), std::plus<int>(), 1500, "reduce c", 0),
                boost::asynchronous::parallel_reduce(d.begin(), d.end(), std::plus<int>(), 1500, "reduce d", 0)
            };
            return boost::asynchronous::all_of(std::move(continuations));
        },
        "test_continuation_all_of_vector_2",
        0
    );

    try
    {
        auto vec = fu2.get();
        BOOST_CHECK_MESSAGE(vec.at(0) == 420000,  "parallel_reduce of vector 'c' returned the wrong result (" + std::to_string(vec.at(0)) + ", should have been 420000)");
        BOOST_CHECK_MESSAGE(vec.at(1) == 1230000, "parallel_reduce of vector 'd' returned the wrong result (" + std::to_string(vec.at(1)) + ", should have been 1230000)");
        static_assert(std::is_same<typename std::decay<decltype(vec[0])>::type, int>::value, "parallel_reduce of 'c' returned wrong type through all(...)");
        static_assert(std::is_same<typename std::decay<decltype(vec[1])>::type, int>::value, "parallel_reduce of 'd' returned wrong type through all(...)");
    }
    catch (...)
    {
        BOOST_FAIL("Unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_continuation_all_of_vector_with_exception)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto a = generate();
    auto b = generate();

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [&a, &b]() mutable
        {
            std::vector<boost::asynchronous::detail::callback_continuation<void, BOOST_ASYNCHRONOUS_DEFAULT_JOB>> continuations = {
                boost::asynchronous::parallel_for(a.begin(), a.end(), [](const int&) { /* Does nothing. */ }, 1500, "parallel_for_of_a", 0),
                boost::asynchronous::parallel_for(b.begin(), b.end(), [](const int&) { throw std::logic_error("Testing exception propagation"); }, 1500, "parallel_for of b", 0)
            };
            return boost::asynchronous::all_of(std::move(continuations));
        },
        "test_continuation_all_of_with_exception",
        0
    );

    try
    {
        fu.get();
        BOOST_FAIL("Should have gotten an exception");
    }
    catch (const std::logic_error& e)
    {
        BOOST_CHECK_MESSAGE(e.what() == std::string("Testing exception propagation"), "Got incorrect exception message");
    }
    catch (...)
    {
        BOOST_FAIL("Unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_continuation_any_of)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto a = std::make_shared<std::vector<int>>(10000000, 42);
    auto b = std::make_shared<std::vector<int>>(  100000, 42);
    auto c = std::make_shared<std::vector<int>>(     100, 42);
    auto d = std::make_shared<std::vector<int>>(  100000, 42);

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [a, b, c, d]() mutable
        {
            return boost::asynchronous::any_of(
                boost::asynchronous::parallel_reduce(std::move(*a), std::plus<int>(), 1500, "reduce a", 0),
                boost::asynchronous::parallel_reduce(std::move(*b), std::plus<int>(), 1500, "reduce b", 0),
                boost::asynchronous::parallel_reduce(std::move(*c), std::plus<int>(), 1500, "reduce c", 0),
                boost::asynchronous::parallel_for(std::move(*d), [](const int&) { throw std::logic_error("Testing exception propagation"); }, 1500, "exception", 0)
            );
        },
        "test_continuation_any_of",
        0
    );

    try
    {
        auto result = fu.get();
        static_assert(std::is_same<typename std::decay<decltype(result)>::type, boost::asynchronous::any_of_result<int, int, int, std::vector<int>>>::value, "Incorrect return type for any_of");
        BOOST_CHECK_MESSAGE(result.index() == 2, "Smallest continuation with result did not finish first");
        BOOST_CHECK_MESSAGE(boost::get<int>(result.value()) == 4200, "Smallest continuation returned the wrong result (" + std::to_string(boost::get<int>(result.value())) + ", should have been 4200)");
    }
    catch (...)
    {
        BOOST_FAIL("Unexpected exception");
    }
}

BOOST_AUTO_TEST_CASE(test_continuation_any_of_only_exceptions)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6);

    auto a = std::make_shared<std::vector<int>>(10000000, 42);
    auto b = std::make_shared<std::vector<int>>(100000, 42);
    auto c = std::make_shared<std::vector<int>>(100, 42);

    auto fu = boost::asynchronous::post_future(
        scheduler,
        [a, b, c]() mutable
        {
            return boost::asynchronous::any_of(
                boost::asynchronous::parallel_for(std::move(*a), [](const int&) { throw std::logic_error("Testing exception propagation from 'a'"); }, 1500, "exception a", 0),
                boost::asynchronous::parallel_for(std::move(*b), [](const int&) { throw std::logic_error("Testing exception propagation from 'b'"); }, 1500, "exception b", 0),
                boost::asynchronous::parallel_for(std::move(*c), [](const int&) { throw std::logic_error("Testing exception propagation from 'c'"); }, 1500, "exception c", 0)
            );
        },
        "test_continuation_any_of_only_exceptions",
        0
    );

    try
    {
        auto result = fu.get();
        static_assert(std::is_same<typename std::decay<decltype(result)>::type, boost::asynchronous::any_of_result<std::vector<int>, std::vector<int>, std::vector<int>>>::value, "Incorrect return type for any_of");
        BOOST_FAIL("Should have gotten an exception");
    }
    catch (const boost::asynchronous::combined_exception& e)
    {
        std::vector<std::string> messages = {"Testing exception propagation from 'a'", "Testing exception propagation from 'b'", "Testing exception propagation from 'c'"};

        auto& underlying = e.underlying_exceptions();
        BOOST_CHECK_MESSAGE(underlying.size() == messages.size(), "Stored wrong number (" + std::to_string(underlying.size()) + ") of underlying exceptions");

        for (size_t index = 0; index < messages.size(); ++index)
        {
            try
            {
                std::rethrow_exception(underlying[index]);
            }
            catch (const std::logic_error& u)
            {
                BOOST_CHECK_MESSAGE(u.what() == messages[index], "Got incorrect exception message for underlying exception at index " + std::to_string(index));
            }
            catch (...)
            {
                BOOST_FAIL("Underlying exception at index " + std::to_string(index) + " reported wrong type on rethrowing");
            }
        }
    }
    catch (...)
    {
        BOOST_FAIL("any_of threw incorrect exception");
    }
}

}
