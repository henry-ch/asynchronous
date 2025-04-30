// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_ANY_NOTIFICATION_SERVANT_HPP
#define BOOST_ASYNCHRONOUS_ANY_NOTIFICATION_SERVANT_HPP

#include <any>
#include <vector>
#include <functional>
#include <cstdint>
#include <map>
#include <memory>
#include <thread>

namespace boost { namespace asynchronous { namespace subscription
{ 
struct any_notification_servant
{
	struct subscriber_t
	{
		std::thread::id                 m_subscriber_thread;
		std::function<void(std::any)>   m_callback;
	};

	using event_subscriber_t = std::vector<boost::asynchronous::subscription::any_notification_servant::subscriber_t>;
	using subscribers_t = std::map < std::type_index, boost::asynchronous::subscription::any_notification_servant::event_subscriber_t>;
	using update_subscribers_t = std::function<void(boost::asynchronous::subscription::any_notification_servant::subscribers_t)>;

	virtual ~any_notification_servant() = default;
	virtual void subscribe(std::type_index event_type, std::thread::id scheduler_thread, std::function<void(std::any)> callback) noexcept = 0;
	virtual void unsubscribe(std::type_index event_type, std::thread::id scheduler_thread) noexcept = 0;
	virtual void subscribe_subscriber_updates(boost::asynchronous::subscription::any_notification_servant::update_subscribers_t update_callback) noexcept = 0; // TODO condition?
};
using any_notification_servant_shared_ptr = std::shared_ptr< boost::asynchronous::subscription::any_notification_servant>;



// TODO type erasure version


}}}
#endif // BOOST_ASYNCHRONOUS_ANY_NOTIFICATION_SERVANT_HPP

