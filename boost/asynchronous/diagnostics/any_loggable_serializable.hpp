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

struct any_loggable_serializable_concept :
        boost::mpl::vector<
            boost::asynchronous::any_loggable_concept,
            boost::asynchronous::any_serializable_concept
> {};

struct any_loggable_serializable: public boost::type_erasure::any<any_loggable_serializable_concept>
{
    typedef std::chrono::high_resolution_clock clock_type;
    typedef int task_failed_handling;

    any_loggable_serializable(){}
    template <class T>
    any_loggable_serializable(T t): boost::type_erasure::any<any_loggable_serializable_concept>(std::move(t)){}
#ifdef BOOST_ASYNCHRONOUS_USE_PORTABLE_BINARY_ARCHIVE
    typedef portable_binary_oarchive oarchive;
    typedef portable_binary_iarchive iarchive;
#else
    typedef boost::archive::text_oarchive oarchive;
    typedef boost::archive::text_iarchive iarchive;
#endif
};

template<>
struct job_traits< boost::asynchronous::any_loggable_serializable >
{
    typedef typename boost::asynchronous::default_loggable_job_extended             diagnostic_type;
    typedef boost::asynchronous::detail::serializable_base_job<
            diagnostic_type,boost::asynchronous::any_loggable_serializable >        wrapper_type;

    typedef typename diagnostic_type::diagnostic_item_type                          diagnostic_item_type;
    typedef boost::asynchronous::diagnostics_table<
            std::string,diagnostic_item_type>                                       diagnostic_table_type;

    static bool get_failed(boost::asynchronous::any_loggable_serializable const& job)
    {
        return job.get_failed();
    }
    static void set_posted_time(boost::asynchronous::any_loggable_serializable& job)
    {
        job.set_posted_time();
    }
    static void set_started_time(boost::asynchronous::any_loggable_serializable& job)
    {
        job.set_started_time();
    }
    static void set_failed(boost::asynchronous::any_loggable_serializable& job)
    {
        job.set_failed();
    }
    static void set_finished_time(boost::asynchronous::any_loggable_serializable& job)
    {
        job.set_finished_time();
    }
    static void set_executing_thread_id(boost::asynchronous::any_loggable_serializable& job, boost::thread::id const& id)
    {
        job.set_executing_thread_id(id);
    }
    static void set_name(boost::asynchronous::any_loggable_serializable& job, std::string const& name)
    {
        job.set_name(name);
    }
    static std::string get_name(boost::asynchronous::any_loggable_serializable& job)
    {
        return job.get_name();
    }
    static diagnostic_item_type get_diagnostic_item(boost::asynchronous::any_loggable_serializable& job)
    {
        return job.get_diagnostic_item();
    }
    static void set_interrupted(boost::asynchronous::any_loggable_serializable& job, bool is_interrupted)
    {
        job.set_interrupted(is_interrupted);
    }
    template <class Diag>
    static void add_diagnostic(boost::asynchronous::any_loggable_serializable& job,Diag* diag)
    {
        diag->add(job.get_name(),job.get_diagnostic_item());
    }
    template <class Diag>
    static void add_current_diagnostic(size_t index,boost::asynchronous::any_loggable_serializable& job,Diag* diag)
    {
        diag->set_current(index,job.get_name(),job.get_diagnostic_item());
    }
    template <class Diag>
    static void reset_current_diagnostic(size_t index,Diag* diag)
    {
        diag->reset_current(index);
    }
};

}}

#endif // BOOST_ASYNCHRONOUS_ANY_LOGGABLE_SERIALIZABLE_HPP
