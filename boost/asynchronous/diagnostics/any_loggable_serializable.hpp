#ifndef BOOST_ASYNCHRONOUS_ANY_LOGGABLE_SERIALIZABLE_HPP
#define BOOST_ASYNCHRONOUS_ANY_LOGGABLE_SERIALIZABLE_HPP

#include <string>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/asynchronous/any_serializable.hpp>
#include <boost/asynchronous/diagnostics/any_loggable.hpp>
#include <boost/asynchronous/job_traits.hpp>

namespace boost { namespace asynchronous
{

template <class Clock>
struct any_loggable_serializable_concept :
        boost::mpl::vector<
            boost::asynchronous::any_loggable_concept<Clock>,
            boost::asynchronous::any_serializable_concept
> {};

template <class Clock = boost::chrono::high_resolution_clock>
struct any_loggable_serializable: public boost::type_erasure::any<any_loggable_serializable_concept<Clock>>
{
    typedef Clock clock_type;
    any_loggable_serializable(){}
    template <class T>
    any_loggable_serializable(T t): boost::type_erasure::any<any_loggable_serializable_concept<Clock>>(std::move(t)){}
#ifdef BOOST_ASYNCHRONOUS_USE_PORTABLE_BINARY_ARCHIVE
    typedef portable_binary_oarchive oarchive;
    typedef portable_binary_iarchive iarchive;
#else
    typedef boost::archive::text_oarchive oarchive;
    typedef boost::archive::text_iarchive iarchive;
#endif
};

template< class Clock >
struct job_traits< boost::asynchronous::any_loggable_serializable<Clock> >
{
    typedef typename boost::asynchronous::default_loggable_job<
                                  boost::chrono::high_resolution_clock >            diagnostic_type;
    typedef boost::asynchronous::detail::serializable_base_job<
            diagnostic_type,boost::asynchronous::any_loggable_serializable<Clock> >        wrapper_type;

    typedef typename diagnostic_type::diagnostic_item_type                          diagnostic_item_type;
    typedef boost::asynchronous::diagnostics_table<
            std::string,diagnostic_item_type>                                       diagnostic_table_type;

    static void set_posted_time(boost::asynchronous::any_loggable_serializable<Clock>& job)
    {
        job.set_posted_time();
    }
    static void set_started_time(boost::asynchronous::any_loggable_serializable<Clock>& job)
    {
        job.set_started_time();
    }
    static void set_finished_time(boost::asynchronous::any_loggable_serializable<Clock>& job)
    {
        job.set_finished_time();
    }
    static void set_name(boost::asynchronous::any_loggable_serializable<Clock>& job, std::string const& name)
    {
        job.set_name(name);
    }
    static std::string get_name(boost::asynchronous::any_loggable_serializable<Clock>& job)
    {
        return job.get_name();
    }
    static diagnostic_item_type get_diagnostic_item(boost::asynchronous::any_loggable_serializable<Clock>& job)
    {
        return job.get_diagnostic_item();
    }
    static void set_interrupted(boost::asynchronous::any_loggable_serializable<Clock>& job, bool is_interrupted)
    {
        job.set_interrupted(is_interrupted);
    }
    template <class Diag>
    static void add_diagnostic(boost::asynchronous::any_loggable_serializable<Clock>& job,Diag* diag)
    {
        diag->add(job.get_name(),job.get_diagnostic_item());
    }
};

}}

#endif // BOOST_ASYNCHRONOUS_ANY_LOGGABLE_SERIALIZABLE_HPP
