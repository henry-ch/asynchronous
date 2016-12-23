// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include "qt_post_helper.hpp"

namespace boost { namespace asynchronous
{
namespace detail {
#ifdef BOOST_ASYNCHRONOUS_QT_WORKAROUND
qt_post_helper::qt_post_helper(connect_functor_helper* c)
    : QObject(0)
    , m_connect(c)
{}
qt_post_helper::qt_post_helper(qt_post_helper const& rhs)
    : QObject(0)
    , m_connect(rhs.m_connect)
{}
qt_post_helper::~qt_post_helper()
{}
#endif
}}}
