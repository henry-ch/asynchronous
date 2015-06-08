// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_METAFUNCTIONS_HPP
#define BOOST_ASYNCHRONOUS_METAFUNCTIONS_HPP

#include <boost/mpl/has_xxx.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>

BOOST_MPL_HAS_XXX_TRAIT_DEF(serializable_type)
BOOST_MPL_HAS_XXX_TRAIT_DEF(is_continuation_task)
BOOST_MPL_HAS_XXX_TRAIT_DEF(is_callback_continuation_task)
BOOST_MPL_HAS_XXX_TRAIT_DEF(return_type)

namespace boost { namespace asynchronous { namespace detail {

template <class T>
struct is_serializable
{
    enum {value = has_serializable_type<T>::type::value};
    typedef typename has_serializable_type<T>::type type;
};

template <class T>
struct eval_return_type
{
    typedef typename T::return_type type;
};
template <class T>
struct eval_return_type2
{
    typedef decltype(std::declval<T>()()) type;
};

template <class T>
struct get_return_type
{
    typedef typename boost::mpl::eval_if<
            has_return_type<T>,
            boost::asynchronous::detail::eval_return_type<T>,
            boost::asynchronous::detail::eval_return_type2<T>
            >::type type;
};

// if continuation => continuation::return_type, else identity
template <class T>
struct get_return_type_if_possible_continuation
{
    typedef typename boost::mpl::eval_if<
            has_is_continuation_task<T>,
            boost::asynchronous::detail::eval_return_type<T>,
            boost::mpl::identity<T>
            >::type type;
};

}}}

#endif // BOOST_ASYNCHRONOUS_METAFUNCTIONS_HPP
