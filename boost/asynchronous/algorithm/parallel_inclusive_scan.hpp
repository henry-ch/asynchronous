// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_INCLUSIVE_SCAN_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_INCLUSIVE_SCAN_HPP

#include <type_traits>
#include <boost/asynchronous/algorithm/parallel_scan.hpp>

namespace boost { namespace asynchronous
{

// parallel_inclusive/exclusive_scan is a simple, reduced version of parallel_scan
template <class Iterator, class OutIterator, class T, class Func,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<T,Job>
parallel_inclusive_scan(Iterator beg, Iterator end, OutIterator out, T init,Func f,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio=0)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto reduce = [f](Iterator beg, Iterator end)
    {
        T r = T();
        for (;beg != end; ++beg)
        {
            r = f(r , *beg);
        }
        return r;
    };
    auto scan = [f](Iterator beg, Iterator end, OutIterator out, T init) mutable
    {
      for (;beg != end; ++beg)
      {
          init = f(init , *beg);
          *out++ = init;
      };
    };

    return boost::asynchronous::parallel_scan<Iterator,OutIterator,T,decltype(reduce),Func,decltype(scan),Job>
                (beg,end,out,std::move(init),std::move(reduce),f,std::move(scan),cutoff,task_name,prio);

}

// version for moved ranges
template <class Range, class OutRange, class T, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<!boost::asynchronous::detail::has_is_continuation_task<Range>::value,
                           boost::asynchronous::detail::callback_continuation<std::pair<Range,OutRange>,Job> >::type
parallel_inclusive_scan(Range&& range,OutRange&& out_range,T init,Func f,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio=0)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto reduce = [f](decltype(boost::begin(range)) beg, decltype(boost::begin(range)) end)
    {
        T r = T();
        for (;beg != end; ++beg)
        {
            r = f(r , *beg);
        }
        return r;
    };
    auto scan = [f](decltype(boost::begin(range)) beg, decltype(boost::begin(range)) end, decltype(boost::begin(out_range)) out, T init) mutable
    {
      for (;beg != end; ++beg)
      {
          init = f(init , *beg);
          *out++ = init;
      };
    };

    return boost::asynchronous::parallel_scan<Range,OutRange,T,decltype(reduce),Func,decltype(scan),Job>
            (std::forward<Range>(range),std::forward<OutRange>(out_range),std::move(init),
             std::move(reduce),f,std::move(scan),cutoff,task_name,prio);
}

// version for a single moved range (in/out) => will return the range as continuation
template <class Range, class T, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<!boost::asynchronous::detail::has_is_continuation_task<Range>::value,
                           boost::asynchronous::detail::callback_continuation<Range,Job> >::type
parallel_inclusive_scan(Range&& range,T init,Func f,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio=0)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto reduce = [f](decltype(boost::begin(range)) beg, decltype(boost::begin(range)) end)
    {
        T r = T();
        for (;beg != end; ++beg)
        {
            r = f(r , *beg);
        }
        return r;
    };
    auto scan = [f](decltype(boost::begin(range)) beg, decltype(boost::begin(range)) end, decltype(boost::begin(range)) out, T init) mutable
    {
      for (;beg != end; ++beg)
      {
          init = f(init , *beg);
          *out++ = init;
      };
    };

    return boost::asynchronous::parallel_scan<Range,T,decltype(reduce),Func,decltype(scan),Job>
            (std::forward<Range>(range),std::move(init),
             std::move(reduce),f,std::move(scan),cutoff,task_name,prio);
}

// version for ranges given as continuation => will return the range as continuation
template <class Range, class T, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<Range>::value,
                          boost::asynchronous::detail::callback_continuation<typename Range::return_type,Job> >::type
parallel_inclusive_scan(Range range,T init,Func f,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio=0)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto reduce = [f](decltype(boost::begin(std::declval<typename Range::return_type>())) beg,
                      decltype(boost::begin(std::declval<typename Range::return_type>())) end)
    {
        T r = T();
        for (;beg != end; ++beg)
        {
            r = f(r , *beg);
        }
        return r;
    };
    auto scan = [f](decltype(boost::begin(std::declval<typename Range::return_type>())) beg,
                    decltype(boost::begin(std::declval<typename Range::return_type>())) end,
                    decltype(std::declval<typename Range::return_type>().begin()) out, T init) mutable
    {
      for (;beg != end; ++beg)
      {
          init = f(init , *beg);
          *out++ = init;
      };
    };

    return boost::asynchronous::parallel_scan<Range,T,decltype(reduce),Func,decltype(scan),Job>
            (std::forward<Range>(range),std::move(init),
             std::move(reduce),f,std::move(scan),cutoff,task_name,prio);
}

}}

#endif // BOOST_ASYNCHRONOUS_PARALLEL_INCLUSIVE_SCAN_HPP
