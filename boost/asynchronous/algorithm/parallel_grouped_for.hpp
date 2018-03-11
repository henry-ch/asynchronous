// Boost.Asynchronous library
//  Copyright (C) Tobias Holl 2018
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_GROUPED_FOR_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_GROUPED_FOR_HPP

#include <iterator>
#include <memory>
#include <type_traits>

#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/algorithm/then.hpp>
#include <boost/asynchronous/continuation_task.hpp>
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
// parallel_grouped_for acts like parallel_for, except that it is guaranteed that a block
// of grouped elements will never be split into separate tasks. Therefore, the actions
// performed on such a block will never occur in multiple threads.

namespace detail
{

template <size_t Arity, class Iterator, class Functor>
struct grouped_for_helper
{
    static void invoke(Iterator first, Iterator last, Functor functor)
    {
        for (; first != last; ++first)
            functor(*first);
    }
};

template <class Iterator, class Functor>
struct grouped_for_helper<2, Iterator, Functor>
{
    static void invoke(Iterator first, Iterator last, Functor functor)
    {
        functor(first, last);
    }
};

template <class Iterator, class Compare, class Functor, class Job>
struct parallel_grouped_for_helper : public boost::asynchronous::continuation_task<void>
{
    parallel_grouped_for_helper(Iterator first, Iterator last, Compare comparison, Functor functor, long cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<void>(task_name)
        , first_(first)
        , last_(last)
        , comparison_(std::move(comparison))
        , functor_(std::move(functor))
        , cutoff_(cutoff)
        , prio_(prio)
    {}

    void operator()()
    {
        boost::asynchronous::continuation_result<void> task_res = this_task_result();
        try
        {
            // Advance first_, depending on the iterator type (either by the specified cutoff exactly, or to the half-way mark)
            Iterator middle = boost::asynchronous::detail::find_cutoff(first_, cutoff_, last_);

            // Advance middle to the end of the group
            middle = std::upper_bound(middle, last_, *middle, comparison_);

            // For full blocks, run the matching grouped_for_helper. Otherwise, recurse.
            if (middle == last_)
            {
                boost::asynchronous::detail::grouped_for_helper<boost::asynchronous::function_traits<Functor>::arity, Iterator, Functor>::invoke(first_, middle, std::move(functor_));
                task_res.set_value();
            }
            else
            {
                boost::asynchronous::create_callback_continuation_job<Job>(
                    [task_res](std::tuple<boost::asynchronous::expected<void>, boost::asynchronous::expected<void>> result) mutable
                    {
                        try
                        {
                            // Check for exceptions
                            std::get<0>(result).get();
                            std::get<1>(result).get();
                            task_res.set_value();
                        }
                        catch(...)
                        {
                            task_res.set_exception(std::current_exception());
                        }
                    },
                    parallel_grouped_for_helper<Iterator, Compare, Functor, Job>(first_, middle, comparison_, functor_, cutoff_, this->get_name(), prio_),
                    parallel_grouped_for_helper<Iterator, Compare, Functor, Job>(middle, last_,  comparison_, functor_, cutoff_, this->get_name(), prio_)
                );
            }
        }
        catch (...)
        {
            task_res.set_exception(std::current_exception());
        }
    }
    Iterator    first_;
    Iterator    last_;
    Compare     comparison_;
    Functor     functor_;
    long        cutoff_;
    std::size_t prio_;
};

} // namespace detail


// parallel_grouped_for - Iterator version
template <
    class Iterator,
    class Compare,
    class Functor,
    class Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB
>
boost::asynchronous::detail::callback_continuation<void, Job>
parallel_grouped_for(
    Iterator first,
    Iterator last,
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
    return boost::asynchronous::top_level_callback_continuation_job<void, Job>(
        boost::asynchronous::detail::parallel_grouped_for_helper<Iterator, Compare, Functor, Job>(
            first, last, std::move(comparison), std::move(functor), cutoff, task_name, prio
        )
    );
}


// parallel_grouped_for - Range (reference) version
template <
    class Range,
    class Compare,
    class Functor,
    class Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB
>
typename std::enable_if<!boost::asynchronous::detail::has_is_continuation_task<Range>::value, boost::asynchronous::detail::callback_continuation<void, Job>>::type
parallel_grouped_for(
    Range&   range,
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
    using std::begin;
    using std::end;
    return boost::asynchronous::parallel_grouped_for<decltype(begin(range)), Compare, Functor, Job>(
        begin(range),
        end(range),
        std::move(comparison),
        std::move(functor),
        cutoff,
        task_name,
        prio
    );
}


// parallel_grouped_for - Range (moved) version
template <
    class Range,
    class Compare,
    class Functor,
    class Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB
>
typename std::enable_if<!boost::asynchronous::detail::has_is_continuation_task<Range>::value, boost::asynchronous::detail::callback_continuation<Range, Job>>::type
parallel_grouped_for(
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

    using std::begin;
    using std::end;
    return boost::asynchronous::then(
        boost::asynchronous::parallel_grouped_for<decltype(begin(*stored_range)), Compare, Functor, Job>(
            begin(*stored_range),
            end(*stored_range),
            std::move(comparison),
            std::move(functor),
            cutoff,
            task_name,
            prio
        ),
        [stored_range](boost::asynchronous::expected<void> result)
        {
            result.get();
            return *stored_range;
        },
        task_name + ": parallel_grouped_for<Range>::then"
    );
}


// parallel_grouped_for - Continuation version
template <
    class Continuation,
    class Compare,
    class Functor,
    class Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB
>
typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<Continuation>::value, boost::asynchronous::detail::callback_continuation<typename Continuation::return_type, Job>>::type
parallel_grouped_for(
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
            return boost::asynchronous::parallel_grouped_for<typename Continuation::return_type, Compare, Functor, Job>(
                std::move(result.get()),
                std::move(comparison),
                std::move(functor),
                cutoff,
                task_name,
                prio
            );
        },
        task_name + ": parallel_grouped_for<Continuation>::then"
    );
}

} // namespace asynchronous
} // namespace boost

#endif // BOOST_ASYNCHRONOUS_PARALLEL_GROUPED_FOR_HPP
