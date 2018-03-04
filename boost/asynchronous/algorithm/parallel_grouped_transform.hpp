// Boost.Asynchronous library
//  Copyright (C) Tobias Holl 2018
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_GROUPED_TRANSFORM_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_GROUPED_TRANSFORM_HPP

#include <iterator>
#include <memory>
#include <type_traits>

#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/algorithm/then.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/detail/container_traits.hpp>
#include <boost/asynchronous/detail/function_traits.hpp>

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
// parallel_grouped_transform is very similar to parallel_transform, except that it is
// guaranteed that a block of grouped elements will never be split into separate tasks.
// Therefore, the actions performed on such a block will never occur in multiple threads.
// Because we rely on sorted input, and it is rare to have multiple ranges sorted alongside
// a "master range" on which the actual comparison is applied, this version only implements
// transformation operations on a single range (unlike parallel_transform). Instead, we
// allow bulk operations to be performed by passing a three-argument functor taking
// iterators pointing to the begin and end of the input block and the beginning of the
// output block (much like most algorithms from <algorithm>).

namespace detail
{

template <size_t Arity, class Iterator, class OutputIterator, class Functor>
struct grouped_transform_helper
{
    static OutputIterator invoke(Iterator first, Iterator last, OutputIterator out, Functor functor)
    {
        return std::transform(first, last, out, std::move(functor));
    }
};

template <class Iterator, class OutputIterator, class Functor>
struct grouped_transform_helper<3, Iterator, OutputIterator, Functor>
{
    static OutputIterator invoke(Iterator first, Iterator last, OutputIterator out, Functor functor)
    {
        functor(first, last, out);
        return out + std::distance(first, last);
    }
};

template <class Iterator, class OutputIterator, class Compare, class Functor, class Job>
struct parallel_grouped_transform_helper : public boost::asynchronous::continuation_task<OutputIterator>
{
    parallel_grouped_transform_helper(Iterator first, Iterator last, OutputIterator out, Compare comparison, Functor functor, long cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<OutputIterator>(task_name)
        , first_(first)
        , last_(last)
        , out_(out)
        , comparison_(std::move(comparison))
        , functor_(std::move(functor))
        , cutoff_(cutoff)
        , prio_(prio)
    {}

    void operator()()
    {
        boost::asynchronous::continuation_result<OutputIterator> task_res = this->this_task_result();
        try
        {
            // Advance first_, depending on the iterator type (either by the specified cutoff exactly, or to the half-way mark)
            Iterator middle = boost::asynchronous::detail::find_cutoff(first_, cutoff_, last_);

            // Advance middle to the end of the group
            middle = std::upper_bound(middle, last_, *middle, comparison_);

            // For full blocks, run the matching grouped_transform_helper. Otherwise, recurse.
            if (middle == last_)
            {
                auto result = boost::asynchronous::detail::grouped_transform_helper<
                    boost::asynchronous::function_traits<Functor>::arity,
                    Iterator,
                    OutputIterator,
                    Functor
                >::invoke(first_, middle, out_, std::move(functor_));
                task_res.set_value(std::move(result));
            }
            else
            {
                // Advance the output iterator accordingly
                OutputIterator middle_output = out_;
                std::advance(middle_output, std::distance(first_, middle));

                // Recursion
                boost::asynchronous::create_callback_continuation_job<Job>(
                    [task_res](std::tuple<boost::asynchronous::expected<OutputIterator>, boost::asynchronous::expected<OutputIterator>> result) mutable
                    {
                        try
                        {
                            // Check for exceptions
                            std::get<0>(result).get();
                            task_res.set_value(std::move(std::get<1>(result).get()));
                        }
                        catch(...)
                        {
                            task_res.set_exception(std::current_exception());
                        }
                    },
                    parallel_grouped_transform_helper<Iterator, OutputIterator, Compare, Functor, Job>(first_, middle, out_,          comparison_, functor_, cutoff_, this->get_name(), prio_),
                    parallel_grouped_transform_helper<Iterator, OutputIterator, Compare, Functor, Job>(middle, last_,  middle_output, comparison_, functor_, cutoff_, this->get_name(), prio_)
                );
            }
        }
        catch (...)
        {
            task_res.set_exception(std::current_exception());
        }
    }
    Iterator       first_;
    Iterator       last_;
    OutputIterator out_;
    Compare        comparison_;
    Functor        functor_;
    long           cutoff_;
    std::size_t    prio_;
};

} // namespace detail


// parallel_grouped_transform - Iterator version
template <
    class Iterator,
    class OutputIterator,
    class Compare,
    class Functor,
    class Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB
>
boost::asynchronous::detail::callback_continuation<OutputIterator, Job>
parallel_grouped_transform(
    Iterator       first,
    Iterator       last,
    OutputIterator out,
    Compare        comparison,
    Functor        functor,
    long           cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    const std::string& task_name,
    std::size_t        prio
#else
    const std::string& task_name = "",
    std::size_t        prio = 0
#endif
)
{
    return boost::asynchronous::top_level_callback_continuation_job<OutputIterator, Job>(
        boost::asynchronous::detail::parallel_grouped_transform_helper<Iterator, OutputIterator, Compare, Functor, Job>(
            first, last, out, std::move(comparison), std::move(functor), cutoff, task_name, prio
        )
    );
}


// parallel_grouped_transform - Range (reference) version
template <
    class Range,
    class OutputIterator,
    class Compare,
    class Functor,
    class Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB
>
typename std::enable_if<!boost::asynchronous::detail::has_is_continuation_task<Range>::value, boost::asynchronous::detail::callback_continuation<OutputIterator, Job>>::type
parallel_grouped_transform(
    Range&         range,
    OutputIterator out,
    Compare        comparison,
    Functor        functor,
    long           cutoff,
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
    return boost::asynchronous::parallel_grouped_transform<decltype(begin(range)), OutputIterator, Compare, Functor, Job>(
        begin(range),
        end(range),
        out,
        std::move(comparison),
        std::move(functor),
        cutoff,
        task_name,
        prio
    );
}


// parallel_grouped_transform - Range (moved) version
// Because we don't know what type of range to construct, you must specify an output type
template <
    class OutputRange,
    class Range,
    class Compare,
    class Functor,
    class Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB
>
typename std::enable_if<!boost::asynchronous::detail::has_is_continuation_task<Range>::value, boost::asynchronous::detail::callback_continuation<OutputRange, Job>>::type
parallel_grouped_transform(
    Range&&  range,
    Compare  comparison,
    Functor  functor,
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
    auto stored_output = std::make_shared<OutputRange>(boost::asynchronous::container_size(*stored_range));

    using std::begin;
    using std::end;
    return boost::asynchronous::then(
        boost::asynchronous::parallel_grouped_transform<decltype(begin(*stored_range)), decltype(begin(*stored_output)), Compare, Functor, Job>(
            begin(*stored_range),
            end(*stored_range),
            begin(*stored_output),
            std::move(comparison),
            std::move(functor),
            cutoff,
            task_name,
            prio
        ),
        [stored_range, stored_output](boost::asynchronous::expected<decltype(begin(*stored_output))> result)
        {
            result.get();
            return *stored_output;
        },
        task_name + ": parallel_grouped_transform<Range>::then"
    );
}


// parallel_grouped_transform - Continuation version
// Again, an output type is necessary
template <
    class OutputRange,
    class Continuation,
    class Compare,
    class Functor,
    class Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB
>
typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<Continuation>::value, boost::asynchronous::detail::callback_continuation<OutputRange, Job>>::type
parallel_grouped_transform(
    Continuation continuation,
    Compare      comparison,
    Functor      functor,
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
        [comparison = std::move(comparison), functor = std::move(functor), cutoff, task_name, prio](boost::asynchronous::expected<typename Continuation::return_type> result)
#else
        [comparison, functor, cutoff, task_name, prio](boost::asynchronous::expected<typename Continuation::return_type> result)
#endif
        {
            return boost::asynchronous::parallel_grouped_transform<OutputRange, typename Continuation::return_type, Compare, Functor, Job>(
                std::move(result.get()),
                std::move(comparison),
                std::move(functor),
                cutoff,
                task_name,
                prio
            );
        },
        task_name + ": parallel_grouped_transform<Continuation>::then"
    );
}

} // namespace asynchronous
} // namespace boost

#endif // BOOST_ASYNCHRONOUS_PARALLEL_GROUPED_TRANSFORM_HPP
