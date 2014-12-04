// Boost.Asynchronous library
//  Copyright (C) Tobias Holl, Christophe Henry 2014
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRON_PARALLEL_EXTREMUM_HPP
#define BOOST_ASYNCHRON_PARALLEL_EXTREMUM_HPP

#include <algorithm>
#include <iterator>

#include <boost/asynchronous/algorithm/parallel_reduce.hpp>

#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>

namespace boost { namespace asynchronous {

namespace detail {
    
template <typename Comparison>
struct selector {
    Comparison c;
    selector(Comparison cs) : c(cs) {}
    template <typename T>
    T operator()(T a, T b) const {
        return c(a, b) ? a : b;
    }
};
}

// Moved Ranges
template <class Range, class Comparison, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
auto parallel_extremum(Range&& range, Comparison c, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                       const std::string& task_name, std::size_t prio)
#else
                       const std::string& task_name="", std::size_t prio=0)
#endif
    -> typename boost::disable_if<
            has_is_continuation_task<Range>,
            decltype(boost::asynchronous::parallel_reduce(std::forward<Range>(range), detail::selector<Comparison>(c), cutoff, task_name, prio))>::type
{
    return boost::asynchronous::parallel_reduce(std::forward<Range>(range), detail::selector<Comparison>(c), cutoff, task_name, prio);
}


// Range references
template <class Range, class Comparison, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
auto parallel_extremum(Range const& range, Comparison c,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                       const std::string& task_name, std::size_t prio)
#else
                       const std::string& task_name="", std::size_t prio=0)
#endif
    -> typename boost::disable_if<has_is_continuation_task<Range>,
                                  decltype(boost::asynchronous::parallel_reduce(range, detail::selector<Comparison>(c), cutoff, task_name, prio)) >::type
{
    return boost::asynchronous::parallel_reduce(range, detail::selector<Comparison>(c), cutoff, task_name, prio);
}


// Continuations
template <class Range, class Comparison, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
auto parallel_extremum(Range range, Comparison c, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                       const std::string& task_name, std::size_t prio)
#else
                       const std::string& task_name="", std::size_t prio=0)
#endif
    -> typename boost::enable_if<
                has_is_continuation_task<Range>,
                decltype(boost::asynchronous::parallel_reduce(range, detail::selector<Comparison>(c), cutoff, task_name, prio))>::type
{
    return boost::asynchronous::parallel_reduce(range, detail::selector<Comparison>(c), cutoff, task_name, prio);
}

// Iterators
template <class Iterator, class Comparison, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
auto parallel_extremum(Iterator beg, Iterator end, Comparison c, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                       const std::string& task_name, std::size_t prio)
#else
                       const std::string& task_name="", std::size_t prio=0)
#endif
    -> boost::asynchronous::detail::callback_continuation<typename std::iterator_traits<Iterator>::value_type, Job>
{
    return boost::asynchronous::parallel_reduce(beg, end, detail::selector<Comparison>(c), cutoff, task_name, prio);
}


}}

#endif // BOOST_ASYNCHRON_PARALLEL_EXTREMUM_HPP
