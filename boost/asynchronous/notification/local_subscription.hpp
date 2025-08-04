// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2024
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_NOTIFICATION_LOCAL_SUBSCRIPTION_HPP
#define BOOST_ASYNCHRONOUS_NOTIFICATION_LOCAL_SUBSCRIPTION_HPP

#include <vector>
#include <functional>
#include <optional>
#include <future>
#include <cstdint>
#include <iostream>

#include <boost/asynchronous/detail/function_traits.hpp>
#include <boost/uuid/uuid.hpp>

namespace boost { namespace asynchronous { namespace subscription
{
template <class Event>
struct local_subscription
{
    using subscriber_t = std::function<std::optional<bool>(Event const&)>;
    
    using internal_subscriber_t = std::pair<
        std::function<std::optional<bool>(Event const&)>, // subscriber
        bool>; // true: to be removed 

    void cleanup_subscriber()
    {
        for (auto it = m_internal_subscribers.begin(); it != m_internal_subscribers.end(); ) 
        {
            if ((*it).second.second)
            {
                it = m_internal_subscribers.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    template <class Sub>
    void subscribe(Sub&& sub, std::int64_t token)
    {
        m_internal_subscribers.insert_or_assign(token, std::make_pair(std::forward<Sub>(sub),false));
    }

    template <class Sub>
    void subscribe_scheduler(Sub&& sub, boost::uuids::uuid scheduler_id)
    {
        // do not register double entries
        auto it = std::find_if(m_scheduler_subscribers.begin(), m_scheduler_subscribers.end(),
            [&](const auto& s)
            {
                return scheduler_id == s.first;
            });

        if (it == m_scheduler_subscribers.end())
        {
            m_scheduler_subscribers.emplace_back(std::make_pair(std::move(scheduler_id), std::forward<Sub>(sub)));
        }
        else
        {
            // replace entry
            *it = std::make_pair(std::move(scheduler_id), std::forward<Sub>(sub));
        }
    }

    // unsubscribes servant, return true if no more servant
    bool unsubscribe(std::int64_t token)
    {
        if (m_in_publish)
        {
            // only mark
            auto it = m_internal_subscribers.find(token);
            if (it != m_internal_subscribers.end())
            {
                (*it).second.second = true; // mark as removeable
            }
        }
        else
        {
            auto it = m_internal_subscribers.find(token);
            if (it != m_internal_subscribers.end())
            {
                // we can immediately remove
                m_internal_subscribers.erase(it);
            }
        }
        // something left? As only marked as removeable, real removal will need a publish
        return m_internal_subscribers.empty();
    }

    void unsubscribe_scheduler(boost::uuids::uuid scheduler_id)
    {
        m_scheduler_subscribers.erase(
            std::remove_if(m_scheduler_subscribers.begin(), m_scheduler_subscribers.end(),
                [&](auto const& sub) {return sub.first == scheduler_id; }),
            m_scheduler_subscribers.end());
    }
    
    template <class Ev>
    bool publish(Ev&& e)
    {
        // inform first other schedulers
        // to avoid event order inversion in case processing an event would send another event
        // external schedulers execute publishing asynchronously and therefore are not counted as handling the event
        for (auto& sched_sub : m_scheduler_subscribers)
        {
            sched_sub.second(e);
        }
        return publish_internal(std::forward<Ev>(e));
    }

    template <class Ev>
    bool publish_internal(Ev&& e)
    {
        m_in_publish = true;
        bool someone_handled = false;
        for (auto it = m_internal_subscribers.begin(); it != m_internal_subscribers.end();++it)
        {
            if ((*it).second.first && !(*it).second.second)
            {
                auto handled = ((*it).second.first)(e);
                if (handled.has_value() && handled.value())
                {
                    someone_handled = true;
                }
                else
                {
                    (*it).second.second = true;
                }
            }
        }
        // now we can safely cleanup
        cleanup_subscriber();
        m_in_publish = false;
        return someone_handled;
    }

    bool                                                        m_in_publish = false;
    std::map<std::int64_t, internal_subscriber_t>               m_internal_subscribers;
    std::vector<std::pair<boost::uuids::uuid, subscriber_t>>    m_scheduler_subscribers;
};

// inline thread local subscriptions
template <class Event>
local_subscription<Event>& get_local_subscription_store_() 
{
    static thread_local local_subscription<Event> subs;
    return subs;
}

// every other scheduler we know of can add a function to be called within its thread context
inline std::vector<std::function<void(std::function<void()>)>>& get_other_schedulers_() 
{
    static thread_local std::vector< std::function<void(std::function<void()>)>> others;
    return others;
}

inline std::vector<std::function<void()>>& get_waiting_subscribes()
{
    static thread_local std::vector<std::function<void()>> waiting_subscribes;
    return waiting_subscribes;
}

template <class Sub, class Internal>
void subscribe_(Sub&& sub, Internal&& internal, boost::uuids::uuid scheduler_id, std::uint64_t token)
{
    using traits = boost::asynchronous::function_traits<Internal>;
    using arg0 = typename traits::template remove_ref_cv_arg_<0>::type;
    static_assert(traits::arity == 1, "callback should have only one argument");

    // remember subscribe
    boost::asynchronous::subscription::get_waiting_subscribes().push_back(
        [internal, scheduler_id]()
        {
            for (auto& other_scheduler : boost::asynchronous::subscription::get_other_schedulers_())
            {
                if (!!other_scheduler)
                {
                    other_scheduler([internal, scheduler_id]()
                        {
                            boost::asynchronous::subscription::get_local_subscription_store_<arg0>().subscribe_scheduler(internal, scheduler_id);
                        });
                }
            }
        }
    );

    // register subscriber
    boost::asynchronous::subscription::get_local_subscription_store_<arg0>().subscribe(std::move(sub), token);

    // register our internal scheduler to other schedulers (will call publish)
    for (auto& other_scheduler : boost::asynchronous::subscription::get_other_schedulers_())
    {
        if (!!other_scheduler)
        {        
            other_scheduler([internal, scheduler_id]()
            {
                    boost::asynchronous::subscription::get_local_subscription_store_<arg0>().subscribe_scheduler(internal, scheduler_id);
            });
        }
    }
}

template <class Event>
void unsubscribe_(std::uint64_t token, boost::uuids::uuid scheduler_id)
{
    // remove servant from list of internal subscribers
    bool empty = boost::asynchronous::subscription::get_local_subscription_store_<Event>().unsubscribe(token);
    if (empty)
    {
        // if it was the last servant subscribed to this event type for this scheduler
        // unsubscribe scheduler from other schedulers
        for (auto& other_scheduler : boost::asynchronous::subscription::get_other_schedulers_())
        {
            if (!!other_scheduler)
            {
                other_scheduler([scheduler_id]()
                    {
                        boost::asynchronous::subscription::get_local_subscription_store_<Event>().unsubscribe_scheduler(scheduler_id);
                    });
            }
        }
    }

}


template <class Event>
bool publish_(Event&& e)
{
    return boost::asynchronous::subscription::get_local_subscription_store_<std::remove_cv_t<std::remove_reference_t<Event>>>().publish(std::forward<Event>(e));
}

template <class Event>
bool publish_internal(Event&& e)
{
    return boost::asynchronous::subscription::get_local_subscription_store_<std::remove_cv_t<std::remove_reference_t<Event>>>().publish_internal(std::forward<Event>(e));
}

}}}
#endif // BOOST_ASYNCHRONOUS_NOTIFICATION_LOCAL_SUBSCRIPTION_HPP
