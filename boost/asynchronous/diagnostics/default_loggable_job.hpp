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

class default_loggable_job
{
public:
    typedef boost::chrono::high_resolution_clock Clock;

    default_loggable_job(default_loggable_job const&)=default;
    default_loggable_job(default_loggable_job&&)=default;
    // our diagnostic item
    typedef boost::asynchronous::diagnostic_item diagnostic_item_type;
    default_loggable_job(std::string const& name="")
        : m_name(name)
        , m_posted(Clock::time_point::min())
        , m_started(Clock::time_point::min())
        , m_finished(Clock::time_point::min())
        , m_interrupted(false)
        , m_failed(false)
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
    void set_failed()
    {
        m_failed = true;
    }
    bool get_failed()const
    {
        return m_failed;
    }
    void set_finished_time()
    {
        m_finished = Clock::now();
    }
    void set_interrupted(bool is_interrupted)
    {
        m_interrupted = is_interrupted;
    }

    boost::asynchronous::diagnostic_item get_diagnostic_item()const
    {
        return boost::asynchronous::diagnostic_item(m_posted,m_started,m_finished,m_interrupted,m_failed);
    }
private:
    std::string m_name;
    typename Clock::time_point m_posted;
    typename Clock::time_point m_started;
    typename Clock::time_point m_finished;
    bool                       m_interrupted;
    bool                       m_failed;
};

//same as default_loggable_job but with a vtable for get_failed support
class default_loggable_job_extended
{
public:
    typedef boost::chrono::high_resolution_clock Clock;

    default_loggable_job_extended(default_loggable_job_extended const&)=default;
    default_loggable_job_extended(default_loggable_job_extended&&)=default;
    // our diagnostic item
    typedef boost::asynchronous::diagnostic_item diagnostic_item_type;
    default_loggable_job_extended(std::string const& name="")
        : m_name(name)
        , m_posted(Clock::time_point::min())
        , m_started(Clock::time_point::min())
        , m_finished(Clock::time_point::min())
        , m_interrupted(false)
        , m_failed(false)
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
    void set_failed()
    {
        m_failed = true;
    }
    // TODO better
    virtual ~default_loggable_job_extended(){}
    virtual bool get_failed()const
    {
        return m_failed;
    }
    void set_finished_time()
    {
        m_finished = Clock::now();
    }
    void set_interrupted(bool is_interrupted)
    {
        m_interrupted = is_interrupted;
    }

    boost::asynchronous::diagnostic_item get_diagnostic_item()const
    {
        return boost::asynchronous::diagnostic_item(m_posted,m_started,m_finished,m_interrupted,get_failed());
    }
private:
    std::string m_name;
    typename Clock::time_point m_posted;
    typename Clock::time_point m_started;
    typename Clock::time_point m_finished;
    bool                       m_interrupted;
    bool                       m_failed;
};

}} // boost::asynchron

#endif // BOOST_ASYNC_DIAGNOSTICS_DEFAULT_LOGGABLE_JOB_HPP
