// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRON_EXCEPTIONS_HPP
#define BOOST_ASYNCHRON_EXCEPTIONS_HPP

#include <exception>
#include <boost/exception/all.hpp>

namespace boost { namespace asynchronous
{
struct task_aborted_exception : virtual boost::exception, virtual std::exception
{
};
}}
#endif // BOOST_ASYNCHRON_EXCEPTIONS_HPP
