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
#include <string_view>



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
        constexpr bool matches(exact_topic const& t)const { return t.m_topic == m_topic; }
        constexpr bool matches(T const& t)const { return t == m_topic; }
        T m_topic;
    };

    template <char Separator = '/', char Wildcard = '.'>
    struct channel_topic
    {
        channel_topic(std::string const& channel)
            :m_topic(channel)
        {
        }
        channel_topic(std::string&& channel)
            : m_topic(std::move(channel))
        {
        }

        inline constexpr bool operator == (auto const& t) const { return  t == m_topic; }
        inline constexpr bool operator != (auto const& t) const { return t != m_topic; }
        constexpr bool matches(std::string const& t)const 
        { 
            return matches(t, m_topic); 
        }
        constexpr bool matches(channel_topic const& t)const
        {
            return matches(t.m_topic, m_topic);
        }
    private:
        static std::string_view trim_trailing_slash(std::string_view path) 
        {
            while (!path.empty() && path.back() == Separator)
            {
                path.remove_suffix(1);
            }
            return path;
        }
        static std::string_view next_token(std::string_view& path) {
            if (path.empty()) return {};

            auto pos = path.find(Separator);
            std::string_view token = (pos == std::string_view::npos)
                ? path
                : path.substr(0, pos);

            path.remove_prefix(std::min(token.size() + 1, path.size()));
            return token;
        }

        static bool matches(std::string_view input, std::string_view current) 
        {
            input = trim_trailing_slash(input);
            current = trim_trailing_slash(current);

            std::string_view input_view = input;
            std::string_view current_view = current;

            while (!current_view.empty()) 
            {
                auto input_token = next_token(input_view);
                auto current_token = next_token(current_view);

                if (input_token.empty()) 
                {
                    // input path is shorter than own
                    return false;
                }

                if (!((current_token.starts_with(Wildcard) && current_token.size() == 1) || current_token == input_token))
                {
                    return false;
                }
            }
            // input fully matched; allow exact match or descendant
            return current_view.empty() || current_view.front() == Separator;
        }
        std::string m_topic;
    };

}}}
#endif // BOOST_ASYNCHRONOUS_NOTIFICATION_TOPICS_HPP
