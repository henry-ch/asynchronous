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

#include <boost/asynchronous/detail/function_traits.hpp>

namespace boost { namespace asynchronous { namespace subscription
{
template <class Event>
struct local_subscription
{
    using subscriber_t = std::function<std::optional<bool>(Event const&)>;
    
    template <class Sub>
    void subscribe(Sub&& sub, std::uint64_t token)
    {
        m_internal_subscribers.insert_or_assign(token, std::forward<Sub>(sub));
    }

    template <class Sub>
    void subscribe_scheduler(Sub&& sub, std::vector<boost::thread::id> scheduler_thread_ids)
    {
        auto it = std::find_if(m_scheduler_subscribers.begin(), m_scheduler_subscribers.end(), [&](auto const& sub) {return scheduler_thread_ids == sub.first; });
        if (it == m_scheduler_subscribers.end())
        {
            m_scheduler_subscribers.emplace_back(std::make_pair(std::move(scheduler_thread_ids), std::forward<Sub>(sub)));
        }
        else
        {
            *it = std::make_pair(std::move(scheduler_thread_ids), std::forward<Sub>(sub));
        }
    }

    // unsubscribes servant, return true if no more servant
    bool unsubscribe(std::uint64_t token)
    {
        auto it = m_internal_subscribers.find(token);
        if (it != m_internal_subscribers.end())
        {
            m_internal_subscribers.erase(it);
        }
        // something left?
        return m_internal_subscribers.empty();
    }

    void unsubscribe_scheduler(std::vector<boost::thread::id> scheduler_thread_ids)
    {
        m_scheduler_subscribers.erase(
            std::remove_if(m_scheduler_subscribers.begin(), m_scheduler_subscribers.end(),
                [&](auto const& sub) {return sub.first == scheduler_thread_ids; }),
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
        bool someone_handled = false;
        for(auto it = m_internal_subscribers.begin(); it != m_internal_subscribers.end();)
        {
            auto handled = ((*it).second)(e);
            if(handled.has_value() && handled.value())
            {
                ++it;
                someone_handled = true;
            }
            else
            {
                it = m_internal_subscribers.erase(it);
                // add here unsusbscribe to schedulers if no subscribers
            }
        }
        return someone_handled;
    }

    std::map<std::uint64_t, subscriber_t> m_internal_subscribers;
    std::vector<std::pair<std::vector<boost::thread::id>, subscriber_t>> m_scheduler_subscribers;
};

// inline thread local subscriptions
template <class Event>
inline thread_local local_subscription<Event> local_subscription_store_;    

// every other scheduler we know of can add a function to be called within its thread context
inline thread_local std::vector< std::function<void(std::function<void()>)>> other_schedulers_;


template <class Sub>
void subscribe_(Sub&& sub, std::vector<boost::thread::id> scheduler_thread_ids, std::uint64_t token)
{
    using traits = boost::asynchronous::function_traits<Sub>;
    using arg0 = typename traits::template remove_ref_cv_arg_<0>::type;
    static_assert(traits::arity == 1, "callback should have only one argument");

    boost::asynchronous::subscription::local_subscription_store_<arg0>.subscribe(sub, token);

    for (auto& other_scheduler : boost::asynchronous::subscription::other_schedulers_)
    {
        if (!!other_scheduler)
        {        
            other_scheduler([sub, scheduler_thread_ids]()mutable
            {
                boost::asynchronous::subscription::local_subscription_store_<arg0>.subscribe_scheduler(std::move(sub),std::move(scheduler_thread_ids));
            });
        }
    }
}

template <class Event>
void unsubscribe_(std::uint64_t token, std::vector<boost::thread::id> scheduler_thread_ids)
{
    // remove servant from list of internal subscribers
    bool empty = boost::asynchronous::subscription::local_subscription_store_<Event>.unsubscribe(token);
    if (empty)
    {
        // if it was the last servant subscribed to this event type for this scheduler
        // unsubscribe scheduler from other schedulers
        for (auto& other_scheduler : boost::asynchronous::subscription::other_schedulers_)
        {
            if (!!other_scheduler)
            {
                other_scheduler([scheduler_thread_ids]()mutable
                    {
                        boost::asynchronous::subscription::local_subscription_store_<Event>.unsubscribe_scheduler(std::move(scheduler_thread_ids));
                    });
            }
        }
    }

}


template <class Event>
bool publish_(Event&& e)
{
    return boost::asynchronous::subscription::local_subscription_store_<std::remove_cv_t<std::remove_reference_t<Event>>>.publish(std::forward<Event>(e));
}



}}}
#endif // BOOST_ASYNCHRONOUS_NOTIFICATION_LOCAL_SUBSCRIPTION_HPP
