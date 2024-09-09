// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2024
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_TYPE_TRAITS_HPP
#define BOOST_ASYNCHRONOUS_TYPE_TRAITS_HPP

#include <type_traits>

namespace boost { namespace asynchronous
{
    template < class T, template<class...> class Primary >
    struct is_specialization_of : std::false_type {};

    template < template<class...> class Primary, class... Args >
    struct is_specialization_of< Primary<Args...>, Primary> : std::true_type {};

    template< class T, template<class...> class Primary >
    inline constexpr bool is_specialization_of_v = is_specialization_of<T, Primary>::value;

}}
#endif // BOOST_ASYNCHRONOUS_TYPE_TRAITS_HPP
