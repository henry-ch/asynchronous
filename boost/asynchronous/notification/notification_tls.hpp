// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2024
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_NOTIFICATION_TLS_HPP
#define BOOST_ASYNCHRONOUS_NOTIFICATION_TLS_HPP

#include <memory>

#include <boost/asynchronous/notification/notification_proxy.hpp>

namespace boost { namespace asynchronous { namespace subscription
{

inline thread_local std::shared_ptr<boost::asynchronous::subscription::notification_proxy> notification_proxy_;

inline static void set_notification_tls(auto notification) noexcept
{
	notification_proxy_ = std::move(notification);
}

inline static auto get_notification_tls() noexcept
{
	return notification_proxy_;
}


}}}
#endif // BOOST_ASYNCHRONOUS_NOTIFICATION_TLS_HPP
