// Boost.Asynchronous library
//  Copyright (C) Tobias Holl 2018
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_FLATTEN_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_FLATTEN_HPP

#include <algorithm>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>

#include <boost/asynchronous/algorithm/parallel_for.hpp>
#include <boost/asynchronous/algorithm/parallel_transform_exclusive_scan.hpp>
#include <boost/asynchronous/algorithm/then.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>

// Unfortunately, Boost.Range leads to some pretty ugly ambiguities with std::begin/std::end if we rely on ADL.
// If this is fixed at some point in the future, we can replace all the calls to std::begin, std::end, and std::size
// with the matching using-declaration (using std::begin) and leave out the std:: prefix.

namespace boost
{
namespace asynchronous
{

namespace detail
{
    // Helper namespace to avoid polluting boost::asynchronous::detail
    namespace flatten_helper
    {
        struct invalid {};

        // Iterators

        template <typename Container, typename Iterator = decltype(std::begin(std::declval<Container>()))>
        Iterator begin_detector(const Container&);
        invalid  begin_detector(...);

        template <typename Container> struct iterator_type_detector { using type = decltype(begin_detector(std::declval<Container>())); };

        // Size

#if __cpp_lib_nonmember_container_access
        template <typename Container, typename Size = decltype(std::size(std::declval<Container>()))>
        Size    size_detector(const Container&);
        invalid size_detector(...);
#else
        template <typename Container, typename Size = decltype(std::distance(std::declval<typename iterator_type_detector<Container>::type>(), std::declval<typename iterator_type_detector<Container>::type>()))>
        Size    size_detector(const Container&);
        invalid size_detector(...);
#endif

        template <typename Container> struct size_type_detector { using type = decltype(size_detector(std::declval<Container>())); };

        // Values

        template <typename Container, typename Value = typename std::iterator_traits<typename iterator_type_detector<Container>::type>::value_type>
        Value   value_detector(const Container&);
        invalid value_detector(...);

        template <typename Container> struct value_type_detector { using type = decltype(value_detector(std::declval<Container>())); };
    }

    // Container traits
    template <class Container>
    struct container_traits
    {
        using iterator_type = typename flatten_helper::iterator_type_detector<Container>::type;
        using size_type     = typename flatten_helper::size_type_detector<Container>::type;
        using value_type    = typename flatten_helper::value_type_detector<Container>::type;
    };

    // Size of the container
    template <class Container>
    typename boost::asynchronous::detail::container_traits<Container>::size_type container_size(const Container& c)
    {
#if __cpp_lib_nonmember_container_access
        return std::size(c);
#else
        return std::distance(std::begin(c), std::end(c));
#endif
    }
} // namespace detail

// Flattens a container of containers into a single container
// By default, returns an std::vector<T>, where T is the type contained in the inner vectors.
// Iterator version
template <
    class OuterIterator,
    class Result        = std::vector<typename boost::asynchronous::detail::container_traits<typename std::iterator_traits<OuterIterator>::value_type>::value_type>,
    class Job           = BOOST_ASYNCHRONOUS_DEFAULT_JOB,
    class OffsetStorage = std::vector<typename boost::asynchronous::detail::container_traits<typename std::iterator_traits<OuterIterator>::value_type>::size_type>
>
boost::asynchronous::detail::callback_continuation<Result, Job>
parallel_flatten(
    OuterIterator outer_begin,
    OuterIterator outer_end,
    long offset_calculation_cutoff,
    long move_cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    const std::string& task_name,
    std::size_t        prio
#else
    const std::string& task_name = "",
    std::size_t        prio = 0
#endif
)
{
    using InnerContainer = typename std::iterator_traits<OuterIterator>::value_type;
    using InnerIterator  = typename boost::asynchronous::detail::container_traits<InnerContainer>::iterator_type;
    using InnerSize      = typename boost::asynchronous::detail::container_traits<InnerContainer>::size_type;

    auto outer_size = std::distance(outer_begin, outer_end);
    auto offsets = std::make_shared<OffsetStorage>(outer_size);

    return boost::asynchronous::then(
        boost::asynchronous::parallel_transform_exclusive_scan(
            outer_begin,
            outer_end,
            std::begin(*offsets),
            static_cast<InnerSize>(0),
            std::plus<InnerSize>(),
            [](const InnerContainer& container)
            {
                return boost::asynchronous::detail::container_size(container);
            },
            offset_calculation_cutoff,
            task_name + ": offset calculation",
            prio
        ),
        [outer_begin, outer_size, offsets, move_cutoff, task_name, prio](boost::asynchronous::expected<InnerSize> expected_total_size)
        {
            InnerSize total_size = expected_total_size.get();
            auto results_container = std::make_shared<Result>(total_size);
            auto results_container_begin = std::begin(*results_container);

            return boost::asynchronous::then(
                boost::asynchronous::parallel_for(
                    static_cast<InnerSize>(0),
                    outer_size,
                    [outer_begin, offsets, results_container_begin](const InnerSize& index)
                    {
                        auto target_iterator = results_container_begin + offsets->operator[](index);
                        auto& inner_container = *(outer_begin + index);
                        std::move(std::begin(inner_container), std::end(inner_container), target_iterator);
                    },
                    move_cutoff,
                    task_name + ": move",
                    prio
                ),
                [results_container](boost::asynchronous::expected<void> result)
                {
                    result.get();
                    return *results_container;
                },
                task_name + ": unwrapping"
            );
        },
        task_name + ": then"
    );
}


// Flattens a container of containers into a single container
// By default, returns an std::vector<T>, where T is the type contained in the inner vectors.
// Range version (reference)
template <
    class OuterRange,
    class Result        = std::vector<typename boost::asynchronous::detail::container_traits<typename boost::asynchronous::detail::container_traits<OuterRange>::value_type>::value_type>,
    class Job           = BOOST_ASYNCHRONOUS_DEFAULT_JOB,
    class OffsetStorage = std::vector<typename boost::asynchronous::detail::container_traits<typename boost::asynchronous::detail::container_traits<OuterRange>::value_type>::size_type>
>
typename std::enable_if<!boost::asynchronous::detail::has_is_continuation_task<OuterRange>::value, boost::asynchronous::detail::callback_continuation<Result, Job>>::type
parallel_flatten(
    OuterRange& range,
    long offset_calculation_cutoff,
    long move_cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    const std::string& task_name,
    std::size_t        prio
#else
    const std::string& task_name = "",
    std::size_t        prio = 0
#endif
)
{
    using InnerContainer = typename boost::asynchronous::detail::container_traits<OuterRange>::value_type;
    using InnerIterator  = typename boost::asynchronous::detail::container_traits<InnerContainer>::iterator_type;
    using InnerSize      = typename boost::asynchronous::detail::container_traits<InnerContainer>::size_type;

    auto outer_size = boost::asynchronous::detail::container_size(range);
    auto offsets = std::make_shared<OffsetStorage>(outer_size);

    auto outer_begin = std::begin(range);

    return boost::asynchronous::then(
        boost::asynchronous::parallel_transform_exclusive_scan(
            outer_begin,
            std::end(range),
            std::begin(*offsets),
            static_cast<InnerSize>(0),
            std::plus<InnerSize>(),
            [](const InnerContainer& container)
            {
                return boost::asynchronous::detail::container_size(container);
            },
            offset_calculation_cutoff,
            task_name + ": offset calculation",
            prio
        ),
        [outer_begin, outer_size, offsets, move_cutoff, task_name, prio](boost::asynchronous::expected<InnerSize> expected_total_size)
        {
            InnerSize total_size = expected_total_size.get();
            auto results_container = std::make_shared<Result>(total_size);
            auto results_container_begin = std::begin(*results_container);

            return boost::asynchronous::then(
                boost::asynchronous::parallel_for(
                    static_cast<InnerSize>(0),
                    outer_size,
                    [outer_begin, offsets, results_container_begin](const InnerSize& index)
                    {
                        auto target_iterator = results_container_begin + offsets->operator[](index);
                        auto& inner_container = *(outer_begin + index);
                        std::move(std::begin(inner_container), std::end(inner_container), target_iterator);
                    },
                    move_cutoff,
                    task_name + ": move",
                    prio
                ),
                [results_container](boost::asynchronous::expected<void> result)
                {
                    result.get();
                    return *results_container;
                },
                task_name + ": unwrapping"
            );
        },
        task_name + ": then"
    );
}

// Flattens a container of containers into a single container
// By default, returns an std::vector<T>, where T is the type contained in the inner vectors.
// Range version (moved ranges)
template <
    class OuterRange,
    class Result        = std::vector<typename boost::asynchronous::detail::container_traits<typename boost::asynchronous::detail::container_traits<OuterRange>::value_type>::value_type>,
    class Job           = BOOST_ASYNCHRONOUS_DEFAULT_JOB,
    class OffsetStorage = std::vector<typename boost::asynchronous::detail::container_traits<typename boost::asynchronous::detail::container_traits<OuterRange>::value_type>::size_type>
>
typename std::enable_if<!boost::asynchronous::detail::has_is_continuation_task<OuterRange>::value, boost::asynchronous::detail::callback_continuation<Result, Job>>::type
parallel_flatten(
    OuterRange&& range,
    long offset_calculation_cutoff,
    long move_cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    const std::string& task_name,
    std::size_t        prio
#else
    const std::string& task_name = "",
    std::size_t        prio = 0
#endif
)
{
    using InnerContainer = typename boost::asynchronous::detail::container_traits<OuterRange>::value_type;
    using InnerIterator  = typename boost::asynchronous::detail::container_traits<InnerContainer>::iterator_type;
    using InnerSize      = typename boost::asynchronous::detail::container_traits<InnerContainer>::size_type;

    auto outer_size = boost::asynchronous::detail::container_size(range);
    auto offsets = std::make_shared<OffsetStorage>(outer_size);

    auto outer = std::make_shared<OuterRange>(std::move(range));
    auto outer_begin = std::begin(*outer);

    return boost::asynchronous::then(
        boost::asynchronous::parallel_transform_exclusive_scan(
            outer_begin,
            std::end(*outer),
            std::begin(*offsets),
            static_cast<InnerSize>(0),
            std::plus<InnerSize>(),
            [](const InnerContainer& container)
            {
                return boost::asynchronous::detail::container_size(container);
            },
            offset_calculation_cutoff,
            task_name + ": offset calculation",
            prio
        ),
        [outer, outer_begin, outer_size, offsets, move_cutoff, task_name, prio](boost::asynchronous::expected<InnerSize> expected_total_size)
        {
            InnerSize total_size = expected_total_size.get();
            auto results_container = std::make_shared<Result>(total_size);
            auto results_container_begin = std::begin(*results_container);

            return boost::asynchronous::then(
                boost::asynchronous::parallel_for(
                    static_cast<InnerSize>(0),
                    outer_size,
                    [outer, outer_begin, offsets, results_container_begin](const InnerSize& index)
                    {
                        auto target_iterator = results_container_begin + offsets->operator[](index);
                        auto& inner_container = *(outer_begin + index);
                        std::move(std::begin(inner_container), std::end(inner_container), target_iterator);
                    },
                    move_cutoff,
                    task_name + ": move",
                    prio
                ),
                [results_container](boost::asynchronous::expected<void> result)
                {
                    result.get();
                    return *results_container;
                },
                task_name + ": unwrapping"
            );
        },
        task_name + ": then"
    );
}

// Flattens a container of containers into a single container
// By default, returns an std::vector<T>, where T is the type contained in the inner vectors.
// Continuation version
template <
    class Continuation,
    class Result        = std::vector<typename boost::asynchronous::detail::container_traits<typename boost::asynchronous::detail::container_traits<typename Continuation::return_type>::value_type>::value_type>,
    class Job           = typename Continuation::job_type,
    class OffsetStorage = std::vector<typename boost::asynchronous::detail::container_traits<typename boost::asynchronous::detail::container_traits<typename Continuation::return_type>::value_type>::size_type>
>
typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<Continuation>::value, boost::asynchronous::detail::callback_continuation<Result, Job>>::type
parallel_flatten(
    Continuation continuation,
    long offset_calculation_cutoff,
    long move_cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    const std::string& task_name,
    std::size_t        prio
#else
    const std::string& task_name = "",
    std::size_t        prio = 0
#endif
)
{
    return boost::asynchronous::then(
        std::move(continuation),
        [offset_calculation_cutoff, move_cutoff, task_name, prio](boost::asynchronous::expected<typename Continuation::return_type> expected)
        {
            return boost::asynchronous::parallel_flatten<typename Continuation::return_type, Result, Job, OffsetStorage>(
                std::move(expected.get()),
                offset_calculation_cutoff,
                move_cutoff,
                task_name,
                prio
            );
        },
        task_name + ": continuation"
    );
}


} // namespace asynchronous
} // namespace boost

#endif // BOOST_ASYNCHRONOUS_PARALLEL_FLATTEN_HPP
