// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2025
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_NOTIFICATION_TOPICS_HPP
#define BOOST_ASYNCHRONOUS_NOTIFICATION_TOPICS_HPP

#include <functional>
#include <string>



namespace boost { namespace asynchronous { namespace subscription
{
    template<typename T>
    concept topic_concept = requires(T a)
    {
        { a.matches(a) } -> std::convertible_to<bool>;
        { a == a } -> std::convertible_to<bool>;
        { a != a } -> std::convertible_to<bool>;
    };

    // Default topic matching everything => same as no topic
    struct no_topic
    {
        constexpr bool matches(no_topic const&)const { return true; }

        inline constexpr bool operator == (no_topic const&) const{return true;}
        inline constexpr bool operator != (no_topic const&) const { return false; }
    };

    template <class T>
    struct exact_topic
    {
        inline constexpr bool operator == (auto const& t) const { return  t == m_topic; }
        inline constexpr bool operator != (auto const& t) const { return t != m_topic; }
        constexpr bool matches(auto const& t)const { return t == m_topic; }
        T m_topic;
    };

}}}
#endif // BOOST_ASYNCHRONOUS_NOTIFICATION_TOPICS_HPP
