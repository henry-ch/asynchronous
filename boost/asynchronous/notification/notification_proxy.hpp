// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2024
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_NOTIFICATION_PROXY_HPP
#define BOOST_ASYNCHRONOUS_NOTIFICATION_PROXY_HPP

#include <vector>
#include <algorithm>

#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/notification/any_notification_servant.hpp>


// the task of notification servant and proxy is to distribute (de)registrations among schedulers within an application. 
// Using TLS, it can be accessed from within the schedulers.
// It is also possible to have several instances of them in order to create different event buses.

namespace boost { namespace asynchronous { namespace subscription
{
    // TODO make template with job type?
    struct notification_servant : boost::asynchronous::trackable_servant<>
    {
        template <class Threadpool>
        notification_servant(boost::asynchronous::any_weak_scheduler<> scheduler, Threadpool p)
            : boost::asynchronous::trackable_servant<>(scheduler, p)
        {
        }

        void subscribe_(std::type_index event_type, std::thread::id scheduler_thread,
                        std::function<void(std::any)> event_callback) noexcept
        {
            m_subscribers[event_type].push_back(boost::asynchronous::subscription::any_notification_servant::subscriber_t{ scheduler_thread,std::move(event_callback)});
            // send an update to all schedulers
            send_updates();
        }

        void unsubscribe_(std::type_index event_type, std::thread::id scheduler_thread) noexcept
        {
            auto it = m_subscribers.find(event_type);
            if (it != m_subscribers.end())
            {
                (*it).second.erase(
                    std::remove_if((*it).second.begin(), (*it).second.end(), [scheduler_thread](auto const& e) {return e.m_subscriber_thread == scheduler_thread; }),
                    (*it).second.end());
            }
            send_updates();
        }

        void subscribe_subscriber_updates_(boost::asynchronous::subscription::any_notification_servant::update_subscribers_t update_callback) noexcept // TODO condition?
        {
            update_callback(m_subscribers);
            m_subscribers_updates.emplace_back(std::move(update_callback));
        }

        void send_updates()
        {
            std::for_each(m_subscribers_updates.begin(), m_subscribers_updates.end(), [this](auto& sub) {sub(m_subscribers); });
        }

    private:
        boost::asynchronous::subscription::any_notification_servant::subscribers_t m_subscribers;
        std::vector< boost::asynchronous::subscription::any_notification_servant::update_subscribers_t> m_subscribers_updates;

    };

    // TODO make template with job type?
    class notification_proxy : public boost::asynchronous::servant_proxy<notification_proxy, notification_servant>
                             , public boost::asynchronous::subscription::any_notification_servant
    {
    public:
        template <class Scheduler, class Threadpool>
        notification_proxy(Scheduler s, Threadpool p) :
            boost::asynchronous::servant_proxy<notification_proxy, notification_servant>(s, p)
        {}
        BOOST_ASYNC_POST_MEMBER_LOG(subscribe_,"subscribe")        
        BOOST_ASYNC_POST_MEMBER_LOG(unsubscribe_,"unsubscribe")
        BOOST_ASYNC_POST_MEMBER_LOG(subscribe_subscriber_updates_, "subscribe_subscriber_updates")

        void subscribe(std::type_index event_type, std::thread::id scheduler_thread, std::function<void(std::any)> event_callback) noexcept override
        {
            this->subscribe_(std::move(event_type), scheduler_thread, std::move(event_callback));
        }
        void unsubscribe(std::type_index event_type, std::thread::id scheduler_thread) noexcept override
        {
            this->unsubscribe_(std::move(event_type), scheduler_thread);
        }
        void subscribe_subscriber_updates(boost::asynchronous::subscription::any_notification_servant::update_subscribers_t update_callback) noexcept override
        {
            this->subscribe_subscriber_updates_(std::move(update_callback));
        }
    };

}}}
#endif // BOOST_ASYNCHRONOUS_NOTIFICATION_PROXY_HPP
