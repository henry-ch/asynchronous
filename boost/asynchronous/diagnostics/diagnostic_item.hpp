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
#include <chrono>
#include <boost/thread/thread.hpp>

namespace boost { namespace asynchronous
{

class diagnostic_item
{
public:
    typedef std::chrono::high_resolution_clock Clock;

    diagnostic_item():m_posted(),m_started(),m_finished(){}
    diagnostic_item(Clock::time_point const& posted,
                    Clock::time_point const& started,
                    Clock::time_point const& finished,
                    bool interrupted,
                    bool failed,
                    boost::thread::id executing_thread_id)
        : m_posted(posted)
        , m_started(started)
        , m_finished(finished)
        , m_interrupted(interrupted)
        , m_failed(failed)
        , m_executing_thread_id(executing_thread_id)
    {}
    Clock::time_point get_posted_time() const
    {
        return m_posted;
    }
    Clock::time_point get_started_time() const
    {
        return m_started;
    }
    Clock::time_point get_finished_time() const
    {
        return m_finished;
    }
    bool is_interrupted() const
    {
        return m_interrupted;
    }
    bool is_failed() const
    {
        return m_failed;
    }
    boost::thread::id get_executing_thread_id() const
    {
        return m_executing_thread_id;
    }
private:
    Clock::time_point m_posted;
    Clock::time_point m_started;
    Clock::time_point m_finished;
    bool              m_interrupted;
    bool              m_failed;
    boost::thread::id m_executing_thread_id;
};

}} // boost::async
#endif // BOOST_ASYNC_DIAGNOSTICS_DIAGNOSTIC_ITEM_HPP
