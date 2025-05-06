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

#include <boost/asynchronous/detail/function_traits.hpp>

namespace boost { namespace asynchronous { namespace subscription
{
template <class Event>
struct local_subscription
{
    using subscriber_t = std::function<std::optional<bool>(Event const&)>;
    
    template <class Sub>
    void subscribe(Sub&& sub)
    {
        const bool was_empty = m_internal_subscribers.empty();
        m_internal_subscribers.push_back(std::forward<Sub>(sub));
        // inform scheduler if was empty
    }

    template <class Sub>
    void subscribe_scheduler(Sub&& sub)
    {
        m_scheduler_subscribers.push_back(std::forward<Sub>(sub));
    }

    // TODO value?
    template <class Ev>
    bool publish(Ev&& e)
    {
        // inform first other schedulers
        // to avoid event order inversion in case processing an event would send another event
        for (auto& sched_sub : m_scheduler_subscribers)
        {
            sched_sub(e);
        }
        bool someone_handled = false;
        for(auto it = m_internal_subscribers.begin(); it != m_internal_subscribers.end();)
        {
            auto handled = (*it)(e);
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

    std::vector<subscriber_t> m_internal_subscribers;
    std::vector<subscriber_t> m_scheduler_subscribers;
};

// inline thread local subscriptions
template <class Event>
inline thread_local local_subscription<Event> local_subscription_store_;    

// every other scheduler we know of can add a function to be called within its thread context
inline thread_local std::vector< std::function<void(std::function<void()>)>> other_schedulers_;


template <class Sub>
void subscribe_(Sub&& sub)
{
    using traits = boost::asynchronous::function_traits<Sub>;
    using arg0 = typename traits::template remove_ref_cv_arg_<0>::type;
    static_assert(traits::arity == 1, "callback should have only one argument");

    boost::asynchronous::subscription::local_subscription_store_<arg0>.subscribe(sub);

    for (auto& other_scheduler : boost::asynchronous::subscription::other_schedulers_)
    {
        if (!!other_scheduler)
        {        
            other_scheduler([sub]()mutable
            {
                boost::asynchronous::subscription::local_subscription_store_<arg0>.subscribe_scheduler(std::move(sub));
            });
        }
    }
}

template <class Sub>
void subscribe_scheduler_(Sub&& sub)
{
    using traits = boost::asynchronous::function_traits<Sub>;
    using arg0 = typename traits::template remove_ref_cv_arg_<0>::type;
    static_assert(traits::arity == 1, "callback should have only one argument");
    boost::asynchronous::subscription::local_subscription_store_<arg0>.subscribe_scheduler(std::forward<Sub>(sub));
}

template <class Event>
bool publish_(Event&& e)
{
    return boost::asynchronous::subscription::local_subscription_store_<Event>.publish(std::forward<Event>(e));
}



}}}
#endif // BOOST_ASYNCHRONOUS_NOTIFICATION_LOCAL_SUBSCRIPTION_HPP
