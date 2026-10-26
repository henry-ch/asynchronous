// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_BOOST_ASYNCRONOUS_HAS_CPP20_HPP
#define BOOST_ASYNCHRONOUS_BOOST_ASYNCRONOUS_HAS_CPP20_HPP

#if defined(_MSC_VER)
#if _MSVC_LANG >= 202002L
#define BOOST_ASYNCRONOUS_HAS_CPP20 1
#endif
#elif __cplusplus >= 202002L
#define BOOST_ASYNCRONOUS_HAS_CPP20 1
#endif


#endif // BOOST_ASYNCHRONOUS_BOOST_ASYNCRONOUS_HAS_CPP20_HPP
