// Boost.Asynchronous library
//  Copyright (C) Tobias Holl 2018
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_GROUP_BY_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_GROUP_BY_HPP

#include <iterator>
#include <memory>
#include <type_traits>

#include <boost/asynchronous/algorithm/parallel_find_all.hpp>
#include <boost/asynchronous/algorithm/parallel_grouped_transform.hpp>
#include <boost/asynchronous/algorithm/then.hpp>
#include <boost/asynchronous/detail/container_traits.hpp>

namespace boost
{
namespace asynchronous
{

// This file implements one of a number of operations on sorted containers.
// In a sorted container, we consider elements "grouped" if they do not have a defined
// sorting order (i.e. neither a < b nor b < a) under the specified comparison operation.
// If the container is sorted based on a specific key (e.g. a particular property of the
// elements), this generally means that in elements of each "group", that key has the
// same value.
// parallel_group_by is essentially the inverse operation to parallel_flatten. It
// extracts each group of items into a separate container and returns that result.

namespace detail
{

// Transformation operation: Builds groups
template <typename InputIterator, typename OutputIterator, typename Comparison>
struct group_by_helper_1
{
    void operator()(InputIterator first, InputIterator last, OutputIterator out)
    {
        while (first != last)
        {
            auto next = std::upper_bound(first, last, *first, compare_);
            *out++ = typename std::iterator_traits<OutputIterator>::value_type(std::make_move_iterator(first), std::make_move_iterator(next));
            first = next;
        }
    }

    Comparison compare_;
};

// Filter operation: Remove empty groups
template <typename OutputRange>
struct group_by_helper_2
{
    bool operator()(const typename boost::asynchronous::container_traits<OutputRange>::value_type& group) const
    {
        return boost::asynchronous::container_size(group) > 0;
    }
};

} // namespace detail

// parallel_group_by - Iterator version
template <
    class OutputRange,
    class Iterator,
    class Compare,
    class Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB
>
boost::asynchronous::detail::callback_continuation<OutputRange, Job>
parallel_group_by(
    Iterator first,
    Iterator last,
    Compare  comparison,
    long     cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    const std::string& task_name,
    std::size_t        prio
#else
    const std::string& task_name = "",
    std::size_t        prio = 0
#endif
)
{
    using std::begin;
    auto output = std::make_shared<OutputRange>(std::distance(first, last));
    return boost::asynchronous::then(
        boost::asynchronous::parallel_grouped_transform(
            first,
            last,
            begin(*output),
            comparison,
            detail::group_by_helper_1<Iterator, decltype(begin(*output)), Compare> { comparison },
            cutoff,
            task_name + ": parallel_grouped_transform",
            prio
        ),
        [output, cutoff, task_name, prio](boost::asynchronous::expected<decltype(begin(*output))> transform_result)
        {
            transform_result.get();

            using std::begin;
            using std::end;
            return boost::asynchronous::parallel_find_all<OutputRange, detail::group_by_helper_2<OutputRange>, OutputRange, Job>(
                std::move(*output),
                detail::group_by_helper_2<OutputRange> {},
                cutoff,
                task_name + ": parallel_find_all",
                prio
            );
        },
        task_name + ": then"
    );
}


// parallel_group_by - Range (reference) version
template <
    class OutputRange,
    class Range,
    class Compare,
    class Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB
>
typename std::enable_if<!boost::asynchronous::detail::has_is_continuation_task<Range>::value, boost::asynchronous::detail::callback_continuation<OutputRange, Job>>::type
parallel_group_by(
    Range&  range,
    Compare comparison,
    long    cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    const std::string& task_name,
    std::size_t        prio
#else
    const std::string& task_name = "",
    std::size_t        prio = 0
#endif
)
{
    using std::begin;
    using std::end;
    return boost::asynchronous::parallel_group_by<OutputRange, decltype(begin(range)), Compare, Job>(
        begin(range),
        end(range),
        std::move(comparison),
        cutoff,
        task_name,
        prio
    );
}


// parallel_group_by - Range (moved) version
template <
    class OutputRange,
    class Range,
    class Compare,
    class Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB
>
typename std::enable_if<!boost::asynchronous::detail::has_is_continuation_task<Range>::value, boost::asynchronous::detail::callback_continuation<OutputRange, Job>>::type
parallel_group_by(
    Range&&  range,
    Compare  comparison,
    long     cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    const std::string& task_name,
    std::size_t        prio
#else
    const std::string& task_name = "",
    std::size_t        prio = 0
#endif
)
{
    auto stored_range = std::make_shared<Range>(std::move(range));

    using std::begin;
    using std::end;
    return boost::asynchronous::then(
        boost::asynchronous::parallel_group_by<OutputRange, decltype(begin(range)), Compare, Job>(
            begin(*stored_range),
            end(*stored_range),
            std::move(comparison),
            cutoff,
            task_name,
            prio
        ),
        [stored_range](boost::asynchronous::expected<OutputRange> result)
        {
            return result.get();
        },
        task_name + ": parallel_group_by<Range>::then"
    );
}


// parallel_group_by - Continuation version
template <
    class OutputRange,
    class Continuation,
    class Compare,
    class Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB
>
typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<Continuation>::value, boost::asynchronous::detail::callback_continuation<OutputRange, Job>>::type
parallel_group_by(
    Continuation continuation,
    Compare      comparison,
    long         cutoff,
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
#if __cpp_init_captures
        [comparison = std::move(comparison), cutoff, task_name, prio](boost::asynchronous::expected<typename Continuation::return_type> result)
#else
        [comparison, cutoff, task_name, prio](boost::asynchronous::expected<typename Continuation::return_type> result)
#endif
        {
            return boost::asynchronous::parallel_group_by<OutputRange, typename Continuation::return_type, Compare, Job>(
                std::move(result.get()),
                std::move(comparison),
                cutoff,
                task_name,
                prio
            );
        },
        task_name + ": parallel_group_by<Continuation>::then"
    );
}

} // namespace asynchronous
} // namespace boost

#endif // BOOST_ASYNCHRONOUS_PARALLEL_GROUP_BY_HPP
