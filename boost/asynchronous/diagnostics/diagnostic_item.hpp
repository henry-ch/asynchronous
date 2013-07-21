// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_DIAGNOSTICS_DIAGNOSTIC_ITEM_HPP
#define BOOST_ASYNC_DIAGNOSTICS_DIAGNOSTIC_ITEM_HPP

#include <string>
#include <boost/chrono/chrono.hpp>

namespace boost { namespace asynchronous
{

template<class Clock = boost::chrono::high_resolution_clock>
class diagnostic_item
{
public:
    diagnostic_item():m_posted(),m_started(),m_finished(){}
    diagnostic_item(typename Clock::time_point const& posted,
                    typename Clock::time_point const& started,
                    typename Clock::time_point const& finished,
                    bool interrupted)
        : m_posted(posted)
        , m_started(started)
        , m_finished(finished)
        , m_interrupted(interrupted)
    {}
    typename Clock::time_point get_posted_time() const
    {
        return m_posted;
    }
    typename Clock::time_point get_started_time() const
    {
        return m_started;
    }
    typename Clock::time_point get_finished_time() const
    {
        return m_finished;
    }
    bool is_interrupted() const
    {
        return m_interrupted;
    }
private:
    typename Clock::time_point m_posted;
    typename Clock::time_point m_started;
    typename Clock::time_point m_finished;
    bool                       m_interrupted;
};

}} // boost::async
#endif // BOOST_ASYNC_DIAGNOSTICS_DIAGNOSTIC_ITEM_HPP
