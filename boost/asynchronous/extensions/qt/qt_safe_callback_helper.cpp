// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <boost/asynchronous/extensions/qt/qt_safe_callback_helper.hpp>

namespace boost { namespace asynchronous
{
namespace detail {
#ifdef BOOST_ASYNCHRONOUS_QT_WORKAROUND
qt_safe_callback_helper::qt_safe_callback_helper(connect_functor_helper* c, std::function<void()>&& cb, int event_id)
    : QObject(0)
    , m_connect(c)
    , m_cb(std::forward<std::function<void()>>(cb))
    , m_event_id(event_id)
    {}
qt_safe_callback_helper::qt_safe_callback_helper(qt_safe_callback_helper const& rhs)
    : QObject(0)
    , m_connect(rhs.m_connect)
    , m_cb(rhs.m_cb)
    , m_event_id(rhs.m_event_id)
    {}
qt_safe_callback_helper::~qt_safe_callback_helper()
{}
#endif
}}}
