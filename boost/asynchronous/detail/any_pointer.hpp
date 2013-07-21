// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_ANY_POINTER_HPP
#define BOOST_ASYNC_ANY_POINTER_HPP

#include <boost/mpl/vector.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/pointee.hpp>

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/constructible.hpp>
#include <boost/type_erasure/relaxed.hpp>

#include <boost/asynchronous/detail/concept_members.hpp>

namespace boost { namespace asynchronous
{
template<class T>
struct is_valid
{
    static bool apply(const T& arg) { return !!(arg); }
};
}}

namespace boost {
namespace type_erasure {
template<class T, class Base>
struct concept_interface< ::boost::asynchronous::is_valid<T>, Base, T>: Base
{
    bool is_valid() const
    {
      return detail::access::data(*this).data &&
          ::boost::type_erasure::call( ::boost::asynchronous::is_valid<T>(), *this);
    }

};
}
}

namespace boost { namespace asynchronous
{
template<class T>
struct pointee
{
    typedef typename boost::mpl::eval_if<
        boost::type_erasure::is_placeholder<T>,
        boost::mpl::identity<void>,
        boost::pointee<T>
    >::type type;
};
template<class T = boost::type_erasure::_self>
struct pointer :
    boost::mpl::vector<
        boost::type_erasure::copy_constructible<T>,
        boost::type_erasure::dereferenceable< boost::type_erasure::deduced< boost::asynchronous::pointee<T> >&, T>,
        boost::asynchronous::is_valid<T>
    >
{
    typedef boost::type_erasure::deduced< boost::asynchronous::pointee<T> > element_type;
};
}}

#endif // BOOST_ASYNC_ANY_POINTER_HPP
