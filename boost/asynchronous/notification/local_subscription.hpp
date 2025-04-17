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
        const bool was_empty = m_subscribers.empty();
        m_subscribers.push_back(std::forward<Sub>(sub));
        // inform scheduler if was empty
    }

    template <class Ev>
    bool publish(Ev&& e)
    {
        bool someone_handled = false;
        for(auto it = m_subscribers.begin(); it != m_subscribers.end();)
        {
            auto handled = (*it)(e);
            if(handled.has_value() && handled.value())
            {
                ++it;
                someone_handled = true;
            }
            else
            {
                it = m_subscribers.erase(it);
                // add here unsusbscribe to schedulers if no subscribers
            }
        }
        return someone_handled;
    }

    std::vector<subscriber_t> m_subscribers;
};

template <class Event>
inline thread_local local_subscription<Event> local_subscription_store_;    

template <class Sub>
void subscribe_(Sub&& sub)
{
    using traits = boost::asynchronous::function_traits<Sub>;
    using arg0 = typename traits::template remove_ref_cv_arg_<0>::type;
    static_assert(traits::arity == 1, "callback should have only one argument");

    local_subscription_store_<arg0>.subscribe(std::forward<Sub>(sub));
}

template <class Event>
bool publish_(Event&& e)
{
    return local_subscription_store_<Event>.publish(std::forward<Event>(e));
}

}}}
#endif // BOOST_ASYNCHRONOUS_NOTIFICATION_LOCAL_SUBSCRIPTION_HPP
