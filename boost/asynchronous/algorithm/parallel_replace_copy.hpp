// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_REPLACE_COPY_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_REPLACE_COPY_HPP

#include <boost/asynchronous/algorithm/parallel_transform.hpp>


namespace boost { namespace asynchronous
{

// versions with iterators
template <class Iterator, class Iterator2, class T, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_replace_copy_if(Iterator beg,Iterator end,Iterator2 beg2, Func func, T const& new_value,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto l = [func, new_value](T const& t)
    {
        if(func(t))
        {
            return new_value;
        }
        else
        {
            return t;
        }
    };

    return boost::asynchronous::parallel_transform<Iterator,Iterator2,decltype(l),Job>
            (beg,end,beg2,std::move(l),cutoff,task_name,prio);
}

template <class Iterator, class Iterator2, class T, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_replace_copy(Iterator beg,Iterator end,Iterator2 beg2, T const& old_value, T const& new_value,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto l = [old_value, new_value](T const& t)
    {
        if(t == old_value)
        {
            return new_value;
        }
        else
        {
            return t;
        }
    };

    return boost::asynchronous::parallel_transform<Iterator,Iterator2,decltype(l),Job>
            (beg,end,beg2,std::move(l),cutoff,task_name,prio);
}

}}

#endif // BOOST_ASYNCHRONOUS_PARALLEL_REPLACE_COPY_HPP

