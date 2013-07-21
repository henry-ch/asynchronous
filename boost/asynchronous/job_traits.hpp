// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_JOB_TRAITS_HPP
#define BOOST_ASYNC_JOB_TRAITS_HPP

#include <boost/asynchronous/diagnostics/any_loggable.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/diagnostics/default_loggable_job.hpp>
#include <boost/asynchronous/diagnostics/diagnostics_table.hpp>
#include <boost/chrono/chrono.hpp>

namespace boost { namespace asynchronous
{

struct no_diagnostics
{
    no_diagnostics(std::string const& =""){}
    void set_name(std::string const& )
    {
        // ignore
    }
    std::string get_name()const
    {
        return "";
    }
    void set_posted_time()
    {
    }
    void set_started_time()
    {
    }
    void set_finished_time()
    {
    }
    void set_interrupted(bool)
    {
    }
    boost::asynchronous::diagnostic_item<> get_diagnostic_item()const
    {
        return boost::asynchronous::diagnostic_item<>();
    }
};

namespace detail
{
    template <class Base, class Fct>
    struct base_job : public Base
    {
        #ifndef BOOST_NO_RVALUE_REFERENCES
        base_job(Fct&& c) : m_callable(std::forward<Fct>(c))
        #else
        base_job(Fct const& c) : m_callable(c)
        #endif
        {
        }
        void operator()()/*const*/ //TODO
        {
            m_callable();
        }
        Fct m_callable;
    };
}

template < class T >
struct job_traits
{
    typedef boost::asynchronous::no_diagnostics                                        diagnostic_type;
    typedef boost::asynchronous::detail::base_job<
            diagnostic_type,boost::asynchronous::any_callable>                         wrapper_type;
    typedef no_diagnostics                                                          diagnostic_item_type;
    typedef no_diagnostics                                                          diagnostic_table_type;

    static void set_posted_time(T&)
    {
    }
    static void set_started_time(T&)
    {
    }
    static void set_finished_time(T&)
    {
    }
    static void set_name(T&, std::string const&)
    {
    }
    static std::string get_name(T&)
    {
        return "";
    }
    static diagnostic_item_type get_diagnostic_item(T&)
    {
        return diagnostic_item_type();
    }
    static void set_interrupted(T&, bool)
    {
    }
    template <class Diag>
    static void add_diagnostic(T&,Diag*)
    {
    }
};

template< >
struct job_traits< boost::asynchronous::any_callable >
{
    typedef typename boost::asynchronous::default_loggable_job<
                                  boost::chrono::high_resolution_clock >            diagnostic_type;
    typedef boost::asynchronous::detail::base_job<
            diagnostic_type,boost::asynchronous::any_callable >                        wrapper_type;

    typedef typename diagnostic_type::diagnostic_item_type                          diagnostic_item_type;
    typedef boost::asynchronous::diagnostics_table<
            std::string,diagnostic_item_type>                                       diagnostic_table_type;

    static void set_posted_time(boost::asynchronous::any_callable& )
    {
    }
    static void set_started_time(boost::asynchronous::any_callable& )
    {
    }
    static void set_finished_time(boost::asynchronous::any_callable& )
    {
    }
    static void set_name(boost::asynchronous::any_callable& , std::string const& )
    {
    }
    static std::string get_name(boost::asynchronous::any_callable& )
    {
      return "";
    }
    static diagnostic_item_type get_diagnostic_item(boost::asynchronous::any_callable& )
    {
      return diagnostic_item_type();
    }
    static void set_interrupted(boost::asynchronous::any_callable& , bool )
    {
    }
    template <class Diag>
    static void add_diagnostic(boost::asynchronous::any_callable& ,Diag* )
    {
    }
};

template< class Clock >
struct job_traits< boost::asynchronous::any_loggable<Clock> >
{
    typedef typename boost::asynchronous::default_loggable_job<
            typename boost::asynchronous::any_loggable<Clock>::clock_type >            diagnostic_type;
//    typedef boost::asynchronous::detail::base_job<
//            diagnostic_type,boost::asynchronous::any_loggable<Clock> >                 wrapper_type;
    typedef boost::asynchronous::detail::base_job<
            diagnostic_type,boost::asynchronous::any_callable >                 wrapper_type;

    typedef typename diagnostic_type::diagnostic_item_type                          diagnostic_item_type;
    typedef boost::asynchronous::diagnostics_table<
            std::string,diagnostic_item_type>                                       diagnostic_table_type;

    static void set_posted_time(boost::asynchronous::any_loggable<Clock>& job)
    {
        job.set_posted_time();
    }
    static void set_started_time(boost::asynchronous::any_loggable<Clock>& job)
    {
        job.set_started_time();
    }
    static void set_finished_time(boost::asynchronous::any_loggable<Clock>& job)
    {
        job.set_finished_time();
    }
    static void set_name(boost::asynchronous::any_loggable<Clock>& job, std::string const& name)
    {
        job.set_name(name);
    }
    static std::string get_name(boost::asynchronous::any_loggable<Clock>& job)
    {
        return job.get_name();
    }
    static diagnostic_item_type get_diagnostic_item(boost::asynchronous::any_loggable<Clock>& job)
    {
        return job.get_diagnostic_item();
    }
    static void set_interrupted(boost::asynchronous::any_loggable<Clock>& job, bool is_interrupted)
    {
        job.set_interrupted(is_interrupted);
    }
    template <class Diag>
    static void add_diagnostic(boost::asynchronous::any_loggable<Clock>& job,Diag* diag)
    {
        diag->add(job.get_name(),job.get_diagnostic_item());
    }
};


}} // boost::asynchron
#endif // BOOST_ASYNC_JOB_TRAITS_HPP
