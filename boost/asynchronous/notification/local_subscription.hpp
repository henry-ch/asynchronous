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

#include <boost/uuid/uuid.hpp>

#include <boost/asynchronous/detail/function_traits.hpp>
#include <boost/asynchronous/notification/topics.hpp>

namespace boost { namespace asynchronous { namespace subscription
{
inline std::map<std::uint64_t, std::function<void()>>& get_waiting_subscribes()
{
    static thread_local std::map<std::uint64_t, std::function<void()>> waiting_subscribes;
    return waiting_subscribes;
}


template <class Event, class Topic>
struct local_subscription
{
    using subscriber_t = std::function<std::optional<bool>(Event const&)>;
    
    using internal_subscriber_t = std::tuple<
        subscriber_t, // subscriber
        Topic, // topic for this object 
        bool>; // true: to be removed 

    using scheduler_subscriber_t = std::tuple<
        boost::uuids::uuid, // scheduler uuid
        Topic, // topic entry for this scheduler
        subscriber_t>; //callback

    void cleanup_subscriber()
    {
        for (auto it = m_internal_subscribers.begin(); it != m_internal_subscribers.end(); ) 
        {
            if (std::get<bool>((*it).second))
            {
                // remove waiting subscription
                boost::asynchronous::subscription::get_waiting_subscribes().erase((*it).first);
                it = m_internal_subscribers.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    template <class Sub, class Topic>
    void subscribe(Sub&& sub, std::int64_t token, Topic const& topic)
    {
        m_internal_subscribers.insert_or_assign(token, std::make_tuple(std::forward<Sub>(sub),topic, false));
    }

    template <class Sub>
    void subscribe_scheduler(Sub&& sub, boost::uuids::uuid scheduler_id, Topic const& topic)
    {
        // do not register double entries
        auto it = std::find_if(m_scheduler_subscribers.begin(), m_scheduler_subscribers.end(),
            [&](const auto& s)
            {
                return (scheduler_id == std::get<boost::uuids::uuid>(s)) && (topic == std::get<Topic>(s));
            });

        if (it == m_scheduler_subscribers.end())
        {
            m_scheduler_subscribers.emplace_back(std::make_tuple(std::move(scheduler_id), topic, std::forward<Sub>(sub)));
        }
        else
        {
            // replace entry
            *it = std::make_tuple(std::move(scheduler_id), topic, std::forward<Sub>(sub));
        }
    }

    // unsubscribes servant, return true if no more servant
    bool unsubscribe(std::int64_t token, Topic const& topic)
    {
        if (m_publish_counter > 0)
        {
            // only mark
            auto it = m_internal_subscribers.find(token);
            if (it != m_internal_subscribers.end() && std::get<Topic>((*it).second) == topic)
            {
                std::get<bool>((*it).second) = true; // mark as removeable
            }
        }
        else
        {
            auto it = m_internal_subscribers.find(token);
            if (it != m_internal_subscribers.end() && std::get<Topic>((*it).second) == topic)
            {
                // we can immediately remove
                m_internal_subscribers.erase(it);
                // remove waiting subscription
                boost::asynchronous::subscription::get_waiting_subscribes().erase(token);
            }
        }
        // something left? As only marked as removeable, real removal will need a publish
        // return true (no more subscriber) if m_internal_subscribers is empty or has no more this topic
        return std::count_if(m_internal_subscribers.begin(), m_internal_subscribers.end(),
            [&](auto const& i)
            {
                return std::get<Topic>(i.second) == topic;
            }) == 0;
    }

    void unsubscribe_scheduler(boost::uuids::uuid scheduler_id, Topic const& topic)
    {
        m_deleted_scheduler_subscribers.emplace_back(std::make_tuple(std::move(scheduler_id), std::optional<Topic>{topic}));
    }

    void unsubscribe_scheduler_all_topics(boost::uuids::uuid scheduler_id)
    {
        m_deleted_scheduler_subscribers.emplace_back(std::move(scheduler_id), std::nullopt);
    }
    
    void cleanup_deleted_schedulers()
    {
        m_scheduler_subscribers.erase(
            std::remove_if(m_scheduler_subscribers.begin(), m_scheduler_subscribers.end(),
                [&](auto const& sub) 
                {
                    // cleanup scheduler if:
                    return std::find_if(m_deleted_scheduler_subscribers.begin(), m_deleted_scheduler_subscribers.end(),
                        [sub_id = std::get<boost::uuids::uuid>(sub), sub_topic = std::get<Topic>(sub)]
                        (auto const& deleted) 
                        {
                            return 
                                // match uid and
                                (std::get<boost::uuids::uuid>(deleted) == sub_id) 
                                // either cleanup for any topic (no topic set)
                              &&(!std::get<std::optional<Topic>>(deleted)
                                // or cleanup given topic
                                || (std::get<std::optional<Topic>>(deleted).value() == sub_topic));
                        }
                    ) != m_deleted_scheduler_subscribers.end();
                }),
            m_scheduler_subscribers.end());
        m_deleted_scheduler_subscribers.clear();
    }

    template <class Ev>
    bool publish(Ev&& e, Topic const& other_topic)
    {
        // inform first other schedulers
        // to avoid event order inversion in case processing an event would send another event
        // external schedulers execute publishing asynchronously and therefore are not counted as handling the event
        for (auto& sched_sub : m_scheduler_subscribers)
        {
            if (std::get<Topic>(sched_sub).matches(other_topic))
            {
                std::get<subscriber_t>(sched_sub)(e);
            }
        }
        bool res = publish_internal(std::forward<Ev>(e), other_topic);
        cleanup_deleted_schedulers();
        return res;
    }

    template <class Ev>
    bool publish_internal(Ev&& e, Topic const& other_topic)
    {
        ++m_publish_counter;
        bool someone_handled = false;
        for (auto it = m_internal_subscribers.begin(); it != m_internal_subscribers.end();++it)
        {
            if (std::get<subscriber_t>((*it).second) && !std::get<bool>((*it).second) && std::get<Topic>((*it).second).matches(other_topic))
            {
                auto handled = (std::get<subscriber_t>((*it).second))(e);
                if (handled.has_value() && handled.value())
                {
                    someone_handled = true;
                }
                else
                {
                    std::get<bool>((*it).second) = true;
                }
            }
        }
        --m_publish_counter;
        if(m_publish_counter == 0)
        {
            // now we can safely cleanup
            cleanup_subscriber();
        }


        return someone_handled;
    }

    std::int16_t                                                        m_publish_counter = 0;
    std::map<std::int64_t, internal_subscriber_t>                       m_internal_subscribers;
    std::vector<scheduler_subscriber_t>                                 m_scheduler_subscribers;
    // in order not to invalidate iterators during publish, remember deleted ids and cleanup later
    std::vector< std::tuple<boost::uuids::uuid, std::optional<Topic>>>  m_deleted_scheduler_subscribers;
};

// inline thread local subscriptions
template <class Event, class Topic= boost::asynchronous::subscription::no_topic>
local_subscription<Event, Topic>& get_local_subscription_store_() 
{
    static thread_local local_subscription<Event, Topic> subs;
    return subs;
}

// every other scheduler we know of can add a function to be called within its thread context
inline std::vector<std::function<void(std::function<void()>)>>& get_other_schedulers_() 
{
    static thread_local std::vector< std::function<void(std::function<void()>)>> others;
    return others;
}

template <class Sub, class Internal, class Topic>
void subscribe(Sub&& sub, Internal&& internal, boost::uuids::uuid scheduler_id, std::uint64_t token, Topic const& topic)
{
    using traits = boost::asynchronous::function_traits<Internal>;
    using arg0 = typename traits::template remove_ref_cv_arg_<0>::type;
    static_assert(traits::arity == 1, "callback should have only one argument");

    // remember subscribe
    boost::asynchronous::subscription::get_waiting_subscribes()[token] =
        [internal, scheduler_id, topic]()
        {
            for (auto& other_scheduler : boost::asynchronous::subscription::get_other_schedulers_())
            {
                if (!!other_scheduler)
                {
                    other_scheduler([internal, scheduler_id, topic]()
                        {
                            boost::asynchronous::subscription::get_local_subscription_store_<arg0, Topic>().subscribe_scheduler(internal, scheduler_id, topic);
                        });
                }
            }
        };

    // register subscriber
    boost::asynchronous::subscription::get_local_subscription_store_<arg0, Topic>().subscribe(std::move(sub), token, topic);

    // register our internal scheduler to other schedulers (will call publish)
    for (auto& other_scheduler : boost::asynchronous::subscription::get_other_schedulers_())
    {
        if (!!other_scheduler)
        {        
            other_scheduler([internal, scheduler_id, topic]()
            {
                    boost::asynchronous::subscription::get_local_subscription_store_<arg0, Topic>().subscribe_scheduler(internal, scheduler_id, topic);
            });
        }
    }
}

template <class Event, class Topic>
void unsubscribe(std::uint64_t token, boost::uuids::uuid scheduler_id, Topic const& topic)
{
    // remove servant from list of internal subscribers
    bool empty = boost::asynchronous::subscription::get_local_subscription_store_<Event, Topic>().unsubscribe(token, topic);
    if (empty)
    {
        // if it was the last servant subscribed to this event type for this scheduler
        // unsubscribe scheduler from other schedulers
        for (auto& other_scheduler : boost::asynchronous::subscription::get_other_schedulers_())
        {
            if (!!other_scheduler)
            {
                other_scheduler([scheduler_id, topic]()
                    {
                        // unsubscribe only for this topic
                        boost::asynchronous::subscription::get_local_subscription_store_<Event, Topic>().unsubscribe_scheduler(scheduler_id, topic);
                    });
            }
        }
    }

}

template <class Event>
bool publish(Event&& e)
{
    return boost::asynchronous::subscription::get_local_subscription_store_<
        std::remove_cv_t<std::remove_reference_t<Event>>, boost::asynchronous::subscription::no_topic>().publish(std::forward<Event>(e), boost::asynchronous::subscription::no_topic{});
}

template <class Event, class Topic>
requires boost::asynchronous::subscription::topic_concept<Topic>
bool publish(Event&& e, Topic const& topic)
{
    return boost::asynchronous::subscription::get_local_subscription_store_<std::remove_cv_t<std::remove_reference_t<Event>>, Topic>().publish(std::forward<Event>(e), topic);
}

template <class Event, class Topic>
bool publish_internal(Event&& e, Topic const& topic)
{
    return boost::asynchronous::subscription::get_local_subscription_store_<std::remove_cv_t<std::remove_reference_t<Event>>, Topic>().publish_internal(std::forward<Event>(e), topic);
}

}}}
#endif // BOOST_ASYNCHRONOUS_NOTIFICATION_LOCAL_SUBSCRIPTION_HPP
