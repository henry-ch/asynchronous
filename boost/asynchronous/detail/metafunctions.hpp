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

BOOST_MPL_HAS_XXX_TRAIT_DEF(serializable_type)
BOOST_MPL_HAS_XXX_TRAIT_DEF(is_continuation_task)

namespace boost { namespace asynchronous { namespace detail {

template <class T>
struct is_serializable
{
    enum {value = has_serializable_type<T>::type::value};
    typedef typename has_serializable_type<T>::type type;
};

}}}

#endif // BOOST_ASYNCHRONOUS_METAFUNCTIONS_HPP
