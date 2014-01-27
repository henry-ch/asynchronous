// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_DIAGNOSTICS_DEFAULT_LOGGABLE_JOB_HPP
#define BOOST_ASYNC_DIAGNOSTICS_DEFAULT_LOGGABLE_JOB_HPP

#include <string>
#include <boost/chrono/chrono.hpp>
#include <boost/asynchronous/diagnostics/diagnostic_item.hpp>

namespace boost { namespace asynchronous
{

template<class Clock = boost::chrono::high_resolution_clock>
class default_loggable_job
{
public:
    default_loggable_job(default_loggable_job const&)=default;
    default_loggable_job(default_loggable_job&&)=default;
    // our diagnostic item
    typedef boost::asynchronous::diagnostic_item<Clock> diagnostic_item_type;
    default_loggable_job(std::string const& name="")
        : m_name(name)
        , m_posted(Clock::time_point::min())
        , m_started(Clock::time_point::min())
        , m_finished(Clock::time_point::min())
        , m_interrupted(false)
    {}
    void set_name(std::string const& name)
    {
        m_name = name;
    }
    #ifndef BOOST_NO_RVALUE_REFERENCES
    void set_name(std::string&& name)
    {
        m_name = std::forward<std::string>(name);
    }
    #endif
    std::string get_name() const
    {
       return m_name;
    }
    void set_posted_time()
    {
        m_posted = Clock::now();
    }
    void set_started_time()
    {
        m_started = Clock::now();
    }
    void set_finished_time()
    {
        m_finished = Clock::now();
    }
    void set_interrupted(bool is_interrupted)
    {
        m_interrupted = is_interrupted;
    }

    boost::asynchronous::diagnostic_item<Clock> get_diagnostic_item()const
    {
        return boost::asynchronous::diagnostic_item<Clock>(m_posted,m_started,m_finished,m_interrupted);
    }
private:
    std::string m_name;
    typename Clock::time_point m_posted;
    typename Clock::time_point m_started;
    typename Clock::time_point m_finished;
    bool                       m_interrupted;
};

}} // boost::asynchron

#endif // BOOST_ASYNC_DIAGNOSTICS_DEFAULT_LOGGABLE_JOB_HPP
