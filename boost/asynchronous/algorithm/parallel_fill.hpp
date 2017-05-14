// Boost.Asynchronous library
//  Copyright (C) Christophe Henry, Tobias Holl 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_FILL_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_FILL_HPP

#include <type_traits>
#include <boost/asynchronous/algorithm/parallel_for.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/detail/any_interruptible.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/scheduler/serializable_task.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>


namespace boost { namespace asynchronous {

// Iterators
template <class Iterator, class Value, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void, Job>
parallel_fill(Iterator beg, Iterator end, const Value& value, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
            const std::string& task_name, std::size_t prio=0)
#else
            const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto l = [value](const typename std::iterator_traits<Iterator>::value_type& ref)
    {
        const_cast<typename std::iterator_traits<Iterator>::value_type&>(ref) = value;
    };
    return boost::asynchronous::parallel_for<Iterator,decltype(l),Job>(beg, end,std::move(l),
                                                                       cutoff, task_name, prio);
}

// Moved range
template <class Range, class Value, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<!boost::asynchronous::detail::has_is_continuation_task<Range>::value, boost::asynchronous::detail::callback_continuation<Range, Job>>::type
parallel_fill(Range&& range, const Value& value, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio=0)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto l = [value](const typename std::iterator_traits<decltype(boost::begin(range))>::value_type& ref)
    {
        const_cast<typename std::iterator_traits<decltype(boost::begin(range))>::value_type&>(ref) = value;
    };
    return boost::asynchronous::parallel_for<Range,decltype(l),Job>(std::forward<Range>(range),std::move(l),
                                                                    cutoff, task_name, prio);
}

// Range reference
template <class Range, class Value, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<!boost::asynchronous::detail::has_is_continuation_task<Range>::value, boost::asynchronous::detail::callback_continuation<void, Job>>::type
parallel_fill(const Range& range, const Value& value, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio=0)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto l = [value](const typename std::iterator_traits<decltype(boost::begin(range))>::value_type& ref)
    {
        const_cast<typename std::iterator_traits<decltype(boost::begin(range))>::value_type&>(ref) = value;
    };
    return boost::asynchronous::parallel_for<Range,decltype(l),Job>(range,std::move(l),
                                                                    cutoff, task_name, prio);
}

// Continuations
template <class Range, class Value, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<Range>::value, boost::asynchronous::detail::callback_continuation<typename Range::return_type, Job>>::type
parallel_fill(Range range, const Value& value, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio=0)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto l = [value](typename std::iterator_traits<decltype(boost::begin(std::declval<typename Range::return_type>()))>::value_type const& ref)
    {
        const_cast<typename std::iterator_traits<decltype(boost::begin(std::declval<typename Range::return_type>()))>::value_type&>(ref) = value;
    };
    return boost::asynchronous::parallel_for<Range,decltype(l),Job>(range,std::move(l),
                                                                    cutoff, task_name, prio);
}

}}

#endif // BOOST_ASYNCHRONOUS_PARALLEL_FILL_HPP
