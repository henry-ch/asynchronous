// Boost.Asynchronous library
//  Copyright (C) Tobias Holl, Christophe Henry 2016
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_ALGORITHMS_DETAIL_HELPERS_HPP
#define BOOST_ASYNCHRONOUS_ALGORITHMS_DETAIL_HELPERS_HPP

#include <type_traits>
#include <utility>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

namespace boost { namespace asynchronous
{
namespace detail
{
template <typename Iterator>
struct iterator_value_type
{
    using type = typename std::remove_reference<typename std::remove_cv<decltype(*std::declval<Iterator>())>::type>::type;
};

template <typename Range>
struct range_value_type
{
    using type = typename std::remove_reference<typename std::remove_cv<decltype(*boost::begin(std::declval<Range>()))>::type>::type;
};

// Wrapper to avoid void and void&& arguments and tuple elements
struct void_wrapper {};

template <class T> struct wrap { using type = T; };
template <> struct wrap<void> { using type = void_wrapper; };

}
}}
#endif // BOOST_ASYNCHRONOUS_ALGORITHMS_DETAIL_HELPERS_HPP

