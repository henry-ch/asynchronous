// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_ANY_CONTINUATION_TASK_HPP
#define BOOST_ASYNCHRONOUS_ANY_CONTINUATION_TASK_HPP

#include <boost/asynchronous/detail/concept_members.hpp>
#include <boost/mpl/vector.hpp>

namespace boost { namespace asynchronous {

// concept for a minimum form of continuation
// a continuation must be callable, have a set_done and a name
// inheriting from continuation_task will provide all this.
// make_lambda_continuation_wrapper will also turn a simple lambda into a continuation task.
// tested in test_callback_continuation_sequence.cpp

template <class Result>
struct any_continuation_task_concept:
 ::boost::mpl::vector<
    boost::asynchronous::has_get_name<std::string(),const boost::type_erasure::_self>,
    boost::asynchronous::has_set_done_func<void(std::function<void(boost::asynchronous::expected<Result>)>),boost::type_erasure::_self>,
    boost::type_erasure::callable<void(),boost::type_erasure::_self>,
    boost::type_erasure::relaxed,
    boost::type_erasure::copy_constructible<>
>{} ;
template <class Result>
struct any_continuation_task
        : public boost::type_erasure::any<boost::asynchronous::any_continuation_task_concept<Result> >
{
    typedef Result return_type;

    any_continuation_task():
        boost::type_erasure::any<boost::asynchronous::any_continuation_task_concept<Result> > (){}

    template <class U>
    any_continuation_task(U const& u):
        boost::type_erasure::any<boost::asynchronous::any_continuation_task_concept<Result> > (u){}
};

}}
#endif // ANY_CONTINUATION_TASK_HPP

