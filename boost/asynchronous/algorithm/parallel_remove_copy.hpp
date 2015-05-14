// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_REMOVE_COPY_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_REMOVE_COPY_HPP

#include <boost/asynchronous/algorithm/parallel_copy_if.hpp>

namespace boost { namespace asynchronous
{

template <class Iterator, class Iterator2,class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<Iterator2,Job>
parallel_remove_copy_if(Iterator beg, Iterator end, Iterator2 out, Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto l = [func](const typename std::iterator_traits<Iterator>::value_type& i)
    {
        return !func(i);
    };

    return boost::asynchronous::parallel_copy_if<Iterator,Iterator2,decltype(l),Job>
                            (beg,end,out,std::move(l),cutoff,task_name,prio);

}

template <class Iterator, class Iterator2,class T, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<Iterator2,Job>
parallel_remove_copy(Iterator beg, Iterator end, Iterator2 out, T const& value,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto l = [value](const typename std::iterator_traits<Iterator>::value_type& i)
    {
        return !(value == i);
    };

    return boost::asynchronous::parallel_copy_if<Iterator,Iterator2,decltype(l),Job>
                            (beg,end,out,std::move(l),cutoff,task_name,prio);

}

}}

#endif // BOOST_ASYNCHRONOUS_PARALLEL_REMOVE_COPY_HPP

