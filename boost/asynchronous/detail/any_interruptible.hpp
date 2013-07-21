// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRON_ANY_INTERRUPTIBLE_HPP
#define BOOST_ASYNCHRON_ANY_INTERRUPTIBLE_HPP

#include <boost/mpl/vector.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/constructible.hpp>
#include <boost/type_erasure/relaxed.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/member.hpp>

BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_interrupt), interrupt, 0);

namespace boost { namespace asynchronous
{
//TODO shared_ptr
typedef ::boost::mpl::vector<
    boost::asynchronous::has_interrupt<void()>,
    boost::type_erasure::relaxed,
    boost::type_erasure::copy_constructible<>,
    boost::type_erasure::typeid_<>
> any_interruptible_concept;
typedef boost::type_erasure::any<any_interruptible_concept> any_interruptible;

}} // boost::asynchron


#endif // BOOST_ASYNCHRON_ANY_INTERRUPTIBLE_HPP
