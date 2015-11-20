// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_REPLACE_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_REPLACE_HPP

#include <boost/asynchronous/algorithm/parallel_for.hpp>

namespace boost { namespace asynchronous
{

// versions with iterators
template <class Iterator, class T, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_replace_if(Iterator beg,Iterator end,Func func, T const& new_value,long cutoff,
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
            const_cast<T&>(t)=new_value;
        }
    };

    return boost::asynchronous::parallel_for<Iterator,decltype(l),Job>(beg,end,std::move(l),cutoff,task_name,prio);
}
template <class Iterator, class T, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_replace(Iterator beg,Iterator end,T const& old_value, T const& new_value,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto l = [old_value](T const& t)
    {
        return t == old_value;
    };

    return boost::asynchronous::parallel_replace_if<Iterator,T,decltype(l),Job>
                            (beg,end,std::move(l),new_value,cutoff,task_name,prio);
}

// versions with moved range
template <class Range, class T,class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::disable_if<boost::asynchronous::detail::has_is_continuation_task<Range>,boost::asynchronous::detail::callback_continuation<Range,Job> >::type
parallel_replace_if(Range&& range, Func func, T const& new_value, long cutoff,
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
            const_cast<T&>(t)=new_value;
        }
    };
    return boost::asynchronous::parallel_for<Range,decltype(l),Job>(std::forward<Range>(range),std::move(l),cutoff,task_name,prio);
}
template <class Range, class T, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::disable_if<boost::asynchronous::detail::has_is_continuation_task<Range>,boost::asynchronous::detail::callback_continuation<Range,Job> >::type
parallel_replace(Range&& range, T const& old_value, T const& new_value, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto l = [old_value](T const& t)
    {
        return t == old_value;
    };

    return boost::asynchronous::parallel_replace_if<Range,T,decltype(l),Job>
                            (std::forward<Range>(range),std::move(l),new_value,cutoff,task_name,prio);
}

// versions with continuation
template <class Range, class T,class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::enable_if<boost::asynchronous::detail::has_is_continuation_task<Range>,
                          boost::asynchronous::detail::callback_continuation<typename Range::return_type,Job> >::type
parallel_replace_if(Range range, Func func, T const& new_value, long cutoff,
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
            const_cast<T&>(t)=new_value;
        }
    };
    return boost::asynchronous::parallel_for<Range,decltype(l),Job>(std::move(range),std::move(l),cutoff,task_name,prio);
}
template <class Range, class T, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::enable_if<boost::asynchronous::detail::has_is_continuation_task<Range>,
                          boost::asynchronous::detail::callback_continuation<typename Range::return_type,Job> >::type
parallel_replace(Range range, T const& old_value, T const& new_value, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto l = [old_value](T const& t)
    {
        return t == old_value;
    };

    return boost::asynchronous::parallel_replace_if<Range,T,decltype(l),Job>
                            (std::forward<Range>(range),std::move(l),new_value,cutoff,task_name,prio);
}

}}

#endif // BOOST_ASYNCHRONOUS_PARALLEL_REPLACE_HPP

