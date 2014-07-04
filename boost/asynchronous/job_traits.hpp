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

#include <boost/serialization/split_member.hpp>

#include <boost/asynchronous/diagnostics/any_loggable.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/diagnostics/default_loggable_job.hpp>
#include <boost/asynchronous/diagnostics/diagnostics_table.hpp>
#include <boost/chrono/chrono.hpp>
#include <boost/asynchronous/any_serializable.hpp>

BOOST_MPL_HAS_XXX_TRAIT_DEF(task_failed_handling)

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
    void set_failed()
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
    template <class Base, class Fct,class Enable=void>
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
    template <class Base, class Fct>
    struct base_job<Base,Fct,typename ::boost::enable_if<typename has_task_failed_handling<Fct>::type >::type> : public Base
    {
        struct non_loggable_helper : public Base
        {
            non_loggable_helper(boost::asynchronous::any_callable c)
                :m_callable(std::move(c))
            {}
            void operator()()/*const*/ //TODO
            {
                m_callable();
            }
            boost::asynchronous::any_callable m_callable;
        };

        template <class T>
        base_job(T c) : m_callable(std::move(c))
        {
        }
        base_job(boost::asynchronous::any_callable c) : m_callable(non_loggable_helper(std::move(c)))
        {
        }
        void operator()()/*const*/ //TODO
        {
            m_callable();
        }
        virtual bool get_failed()const
        {
            return m_callable.get_failed();
        }
        Fct m_callable;
    };
    template <class Base, class Fct,class Enable=void>
    struct serializable_base_job : public Base
    {
        #ifndef BOOST_NO_RVALUE_REFERENCES
        serializable_base_job(Fct&& c) : m_callable(std::forward<Fct>(c))
        #else
        serializable_base_job(Fct const& c) : m_callable(c)
        #endif
        {
        }
        void operator()()/*const*/ //TODO
        {
            m_callable();
        }

        template <class Archive>
        void save(Archive & ar, const unsigned int version)const
        {
            const_cast<Fct&>(m_callable).serialize(ar,version);
        }
        template <class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            m_callable.serialize(ar,version);
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()
        std::string get_task_name()const
        {
            return const_cast<Fct&>(m_callable).get_task_name();
        }
        Fct m_callable;
    };
    template <class Base, class Fct>
    struct serializable_base_job<Base,Fct,typename ::boost::enable_if<typename has_task_failed_handling<Fct>::type >::type> : public Base
    {
        #ifndef BOOST_NO_RVALUE_REFERENCES
        serializable_base_job(Fct&& c) : m_callable(std::forward<Fct>(c))
        #else
        serializable_base_job(Fct const& c) : m_callable(c)
        #endif
        {
        }
        void operator()()/*const*/ //TODO
        {
            m_callable();
        }
        virtual bool get_failed()const
        {
            return m_callable.get_failed();
        }
        template <class Archive>
        void save(Archive & ar, const unsigned int version)const
        {
            const_cast<Fct&>(m_callable).serialize(ar,version);
        }
        template <class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            m_callable.serialize(ar,version);
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()
        std::string get_task_name()const
        {
            return const_cast<Fct&>(m_callable).get_task_name();
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

    static bool get_failed(T const& )
    {
        return false;
    }
    static void set_posted_time(T&)
    {
    }
    static void set_started_time(T&)
    {
    }
    static void set_failed(T&)
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

    static bool get_failed(boost::asynchronous::any_callable const& )
    {
        return false;
    }
    static void set_posted_time(boost::asynchronous::any_callable& )
    {
    }
    static void set_started_time(boost::asynchronous::any_callable& )
    {
    }
    static void set_failed(boost::asynchronous::any_callable& )
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
    typedef typename boost::asynchronous::default_loggable_job_extended<
            typename boost::asynchronous::any_loggable<Clock>::clock_type >             diagnostic_type;
    typedef boost::asynchronous::detail::base_job<
            diagnostic_type,boost::asynchronous::any_loggable<Clock> >                  wrapper_type;

    typedef typename diagnostic_type::diagnostic_item_type                              diagnostic_item_type;
    typedef boost::asynchronous::diagnostics_table<
            std::string,diagnostic_item_type>                                           diagnostic_table_type;

    static bool get_failed(boost::asynchronous::any_loggable<Clock> const& job)
    {
        return job.get_failed();
    }
    static void set_posted_time(boost::asynchronous::any_loggable<Clock>& job)
    {
        job.set_posted_time();
    }
    static void set_started_time(boost::asynchronous::any_loggable<Clock>& job)
    {
        job.set_started_time();
    }
    static void set_failed(boost::asynchronous::any_loggable<Clock>& job)
    {
        job.set_failed();
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

template< >
struct job_traits< boost::asynchronous::any_serializable >
{
    typedef typename boost::asynchronous::default_loggable_job<
                                  boost::chrono::high_resolution_clock >                diagnostic_type;
    typedef boost::asynchronous::detail::serializable_base_job<
            diagnostic_type,boost::asynchronous::any_serializable >                     wrapper_type;

    typedef typename diagnostic_type::diagnostic_item_type                              diagnostic_item_type;
    typedef boost::asynchronous::diagnostics_table<
            std::string,diagnostic_item_type>                                           diagnostic_table_type;

    static bool get_failed(boost::asynchronous::any_serializable const& )
    {
        return false;
    }
    static void set_posted_time(boost::asynchronous::any_serializable& )
    {
    }
    static void set_started_time(boost::asynchronous::any_serializable& )
    {
    }
    static void set_failed(boost::asynchronous::any_serializable& )
    {
    }
    static void set_finished_time(boost::asynchronous::any_serializable& )
    {
    }
    static void set_name(boost::asynchronous::any_serializable& , std::string const& )
    {
    }
    static std::string get_name(boost::asynchronous::any_serializable& )
    {
      return "";
    }
    static diagnostic_item_type get_diagnostic_item(boost::asynchronous::any_serializable& )
    {
      return diagnostic_item_type();
    }
    static void set_interrupted(boost::asynchronous::any_serializable& , bool )
    {
    }
    template <class Diag>
    static void add_diagnostic(boost::asynchronous::any_serializable& ,Diag* )
    {
    }
};
}} // boost::asynchronous
#endif // BOOST_ASYNC_JOB_TRAITS_HPP
