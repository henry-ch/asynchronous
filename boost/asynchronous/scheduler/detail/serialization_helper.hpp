// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_SERIALIZATION_HELPER_HPP
#define BOOST_ASYNCHRONOUS_SERIALIZATION_HELPER_HPP

#include <utility>
#include <boost/asynchronous/any_serializable.hpp>

namespace boost { namespace asynchronous
{
template <class Job= boost::asynchronous::any_serializable >
struct serialization_helper
{
    serialization_helper(Job&& c) : m_callable(std::forward<Job>(c))
    {}
    Job m_callable;
};

}}

#endif // BOOST_ASYNCHRONOUS_SERIALIZATION_HELPER_HPP
