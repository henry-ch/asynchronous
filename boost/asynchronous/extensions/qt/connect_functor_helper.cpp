// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <boost/asynchronous/extensions/qt/connect_functor_helper.hpp>

namespace boost { namespace asynchronous
{
namespace detail {
#ifdef BOOST_ASYNCHRONOUS_QT_WORKAROUND
connect_functor_helper::connect_functor_helper(unsigned long id, const std::function<void(QEvent*)> &f)
    : QObject(0)
    , m_id(id)
    , m_function(f)
    {}
connect_functor_helper::connect_functor_helper(connect_functor_helper const& rhs)
    :QObject(0), m_id(rhs.m_id),m_function(rhs.m_function)
    {}
connect_functor_helper::~connect_functor_helper()
{}
#endif
}}}
