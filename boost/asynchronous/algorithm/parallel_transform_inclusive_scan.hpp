// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_TRANSFORM_INCLUSIVE_SCAN_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_TRANSFORM_INCLUSIVE_SCAN_HPP

#include <boost/asynchronous/algorithm/parallel_scan.hpp>
namespace boost { namespace asynchronous
{

// same as parallel_inclusive_scan but apply a transform on input elements first
template <class Iterator, class OutIterator, class T, class Func, class Transform, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<T,Job>
parallel_transform_inclusive_scan(Iterator beg, Iterator end, OutIterator out, T init,Func f, Transform t, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto reduce = [f,t](Iterator beg, Iterator end)
    {
        int r = T();
        for (;beg != end; ++beg)
        {
            r = f(r , t(*beg));
        }
        return r;
    };
    auto scan = [f,t](Iterator beg, Iterator end, Iterator out, T init) mutable
    {
      for (;beg != end; ++beg)
      {
          init = f(init , t(*beg));
          *out++ = init;
      };
    };

    return boost::asynchronous::parallel_scan<Iterator,OutIterator,T,decltype(reduce),Func,decltype(scan),Job>
                (beg,end,out,std::move(init),std::move(reduce),f,std::move(scan),cutoff,task_name,prio);

}

}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_TRANSFORM_INCLUSIVE_SCAN_HPP
