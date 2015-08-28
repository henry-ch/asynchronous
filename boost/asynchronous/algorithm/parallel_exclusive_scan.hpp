// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_EXCLUSIVE_SCAN_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_EXCLUSIVE_SCAN_HPP


#include <boost/asynchronous/algorithm/parallel_scan.hpp>
namespace boost { namespace asynchronous
{
template <class Iterator, class OutIterator, class T, class Func,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<OutIterator,Job>
parallel_exclusive_scan(Iterator beg, Iterator end, OutIterator out, T init,Func f,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto reduce = [f](Iterator beg, Iterator end)
    {
        int r = T();
        for (;beg != end; ++beg)
        {
            r = f(r , *beg);
        }
        return r;
    };
    auto scan = [f](Iterator beg, Iterator end, Iterator out, T init) mutable
    {
      for (;beg != end; ++beg)
      {
          *out++ = init;
          init = f(init , *beg);
      };
    };

    return boost::asynchronous::parallel_scan<Iterator,OutIterator,T,decltype(reduce),Func,decltype(scan),Job>
                (beg,end,out,std::move(init),std::move(reduce),f,std::move(scan),cutoff,task_name,prio);

}

}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_EXCLUSIVE_SCAN_HPP
