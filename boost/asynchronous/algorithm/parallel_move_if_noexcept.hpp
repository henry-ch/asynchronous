// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_MOVE_IF_NOEXCEPT_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_MOVE_IF_NOEXCEPT_HPP

#include  <algorithm>

#include <boost/asynchronous/algorithm/parallel_move.hpp>
#include <boost/asynchronous/algorithm/parallel_copy.hpp>
#include <boost/asynchronous/algorithm/detail/helpers.hpp>

#include <boost/mpl/and.hpp>
#include <boost/mpl/not.hpp>


namespace boost { namespace asynchronous {
// non parallel version
template<class Iterator, class ResultIterator>
typename boost::enable_if<std::is_nothrow_move_constructible<typename boost::asynchronous::detail::iterator_value_type<Iterator>::type>,void>::type
serial_move_if_noexcept(Iterator begin, Iterator end, ResultIterator result)
{
    std::move(begin,end,result);
}
template<class Iterator, class ResultIterator>
typename boost::disable_if<std::is_nothrow_move_constructible<typename boost::asynchronous::detail::iterator_value_type<Iterator>::type>,void>::type
serial_move_if_noexcept(Iterator begin, Iterator end, ResultIterator result)
{
    std::copy(begin,end,result);
}

// version for iterators
// nothrow move
template<class Iterator, class ResultIterator, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::enable_if<boost::mpl::and_<
                                boost::asynchronous::detail::has_iterator_category<std::iterator_traits<Iterator>>,
                                std::is_nothrow_move_constructible<typename boost::asynchronous::detail::iterator_value_type<Iterator>::type>>,
                          boost::asynchronous::detail::callback_continuation<void, Job>>::type
parallel_move_if_noexcept(Iterator begin, Iterator end, ResultIterator result, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio=0)
#else
                     const std::string& task_name = "", std::size_t prio = 0)
#endif
{
    return boost::asynchronous::parallel_move<Iterator,ResultIterator, Job>
               (begin, end, result, cutoff, task_name, prio);
}

// no-nothrow move
template<class Iterator, class ResultIterator, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::enable_if<boost::mpl::and_<
                                boost::asynchronous::detail::has_iterator_category<std::iterator_traits<Iterator>>,
                                boost::mpl::not_<std::is_nothrow_move_constructible<typename boost::asynchronous::detail::iterator_value_type<Iterator>::type>>>,
                          boost::asynchronous::detail::callback_continuation<void, Job>>::type
parallel_move_if_noexcept(Iterator begin, Iterator end, ResultIterator result, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio=0)
#else
                     const std::string& task_name = "", std::size_t prio = 0)
#endif
{
    return boost::asynchronous::parallel_copy<Iterator,ResultIterator, Job>
               (begin, end, result, cutoff, task_name, prio);
}
}}

#endif // BOOST_ASYNCHRONOUS_PARALLEL_MOVE_IF_NOEXCEPT_HPP

