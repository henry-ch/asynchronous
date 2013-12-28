// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_CALLABLE_ANY_HPP
#define BOOST_ASYNC_CALLABLE_ANY_HPP

#include <boost/mpl/vector.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/constructible.hpp>
#include <boost/type_erasure/relaxed.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/member.hpp>
#include <boost/type_erasure/callable.hpp>
//TODO find better
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
namespace boost { namespace asynchronous
{

typedef ::boost::mpl::vector<
    boost::type_erasure::callable<void()>,
    boost::type_erasure::relaxed,
    boost::type_erasure::copy_constructible<>,
    //boost::type_erasure::constructible<boost::type_erasure::_self(boost::type_erasure::_self&&)>,
    //boost::type_erasure::destructible<>,
    boost::type_erasure::typeid_<>
> any_callable_concept;
typedef boost::type_erasure::any<any_callable_concept> any_callable_helper;

struct any_callable: public boost::asynchronous::any_callable_helper
{
    any_callable(){}
    template <class T>
    any_callable(T t):any_callable_helper(t){}
    // dummies
    typedef boost::archive::text_oarchive oarchive;
    typedef boost::archive::text_iarchive iarchive;
};

}} // boost::async


#endif /* BOOST_ASYNC_CALLABLE_ANY_HPP */
