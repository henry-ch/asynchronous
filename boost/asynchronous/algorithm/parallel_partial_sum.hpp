// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_PARTIAL_SUM_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_PARTIAL_SUM_HPP

#include <boost/asynchronous/algorithm/parallel_inclusive_scan.hpp>

namespace boost { namespace asynchronous
{

// version with iterators
template <class Iterator, class OutIterator, class Func,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<OutIterator,Job>
parallel_partial_sum(Iterator beg, Iterator end, OutIterator out, Func f,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    using element_type = typename std::iterator_traits<Iterator>::value_type;
    return boost::asynchronous::parallel_inclusive_scan<Iterator,OutIterator,element_type,Func,Job>(beg,end,out,element_type(),std::move(f),cutoff,task_name,prio);
}

// version for moved ranges
template <class Range, class OutRange, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::disable_if<has_is_continuation_task<Range>,
                           boost::asynchronous::detail::callback_continuation<std::pair<Range,OutRange>,Job> >::type
parallel_partial_sum(Range&& range,OutRange&& out_range,Func f,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
    using element_type = typename std::remove_const<typename std::remove_reference<decltype(*boost::begin(range))>::type>::type;
    return boost::asynchronous::parallel_inclusive_scan<Range,OutRange,element_type,Func,Job>
            (std::forward<Range>(range),std::forward<OutRange>(out_range),element_type(),std::move(f),cutoff,task_name,prio);
}

// version for a single moved range (in/out) => will return the range as continuation
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::disable_if<has_is_continuation_task<Range>,
                           boost::asynchronous::detail::callback_continuation<Range,Job> >::type
parallel_partial_sum(Range&& range,Func f,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
    using element_type = typename std::remove_const<typename std::remove_reference<decltype(*boost::begin(range))>::type>::type;
    return boost::asynchronous::parallel_inclusive_scan<Range,element_type,Func,Job>(std::forward<Range>(range),element_type(),std::move(f),cutoff,task_name,prio);
}

// version for ranges given as continuation => will return the range as continuation
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::enable_if<has_is_continuation_task<Range>,
                          boost::asynchronous::detail::callback_continuation<typename Range::return_type,Job> >::type
parallel_partial_sum(Range range,Func f,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
    using element_type = typename std::remove_const<typename std::remove_reference<decltype(*boost::begin(std::declval<typename Range::return_type>()))>::type>::type;
    return boost::asynchronous::parallel_inclusive_scan<Range,element_type,Func,Job>(std::move(range),element_type(),std::move(f),cutoff,task_name,prio);
}

}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_PARTIAL_SUM_HPP

