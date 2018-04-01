#ifndef BOOST_ASYNCHRON_SCHEDULER_TSS_SCHEDULERHPP
#define BOOST_ASYNCHRON_SCHEDULER_TSS_SCHEDULERHPP

#include <list>

#include <boost/thread/tss.hpp>

#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/any_scheduler.hpp>
#include <boost/asynchronous/scheduler/detail/any_continuation.hpp>
#include <boost/asynchronous/scheduler/detail/interrupt_state.hpp>
#include <boost/asynchronous/job_traits.hpp>

namespace boost { namespace asynchronous
{
template <class Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB >
struct tss_any_weak_scheduler_wrapper
{
    tss_any_weak_scheduler_wrapper(boost::asynchronous::any_weak_scheduler<Job> s):m_scheduler(s){}
    boost::asynchronous::any_weak_scheduler<Job> m_scheduler;
};

template <class Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB >
boost::asynchronous::any_weak_scheduler<Job> get_thread_scheduler(boost::asynchronous::any_weak_scheduler<Job> wscheduler= boost::asynchronous::any_weak_scheduler<Job>(), bool reset=false )
{
    static boost::thread_specific_ptr<boost::asynchronous::tss_any_weak_scheduler_wrapper<Job> > s_scheduler;
    if (reset)
    {
        s_scheduler.reset(new boost::asynchronous::tss_any_weak_scheduler_wrapper<Job>(wscheduler));
    }
    if (!s_scheduler.get())
        return boost::asynchronous::any_weak_scheduler<Job>();
    return s_scheduler.get()->m_scheduler;
}

struct tss_any_continuation_wrapper
{
    tss_any_continuation_wrapper(std::list<boost::asynchronous::any_continuation>&& c)
        :m_continuations(std::forward<std::list<boost::asynchronous::any_continuation> >(c)){}

    std::list<boost::asynchronous::any_continuation> m_continuations;
};
template <class dummy = void >
std::list<boost::asynchronous::any_continuation>& get_continuations(
        std::list<boost::asynchronous::any_continuation>&& c= std::list<boost::asynchronous::any_continuation>(),
        bool reset=false )
{
    static boost::thread_specific_ptr<boost::asynchronous::tss_any_continuation_wrapper > s_continuations;
    if (reset)
    {
        s_continuations.reset(new boost::asynchronous::tss_any_continuation_wrapper(std::forward<std::list<boost::asynchronous::any_continuation> >(c)));
    }
    return s_continuations.get()->m_continuations;
}

struct tss_interrupt_state_wrapper
{
    tss_interrupt_state_wrapper(std::shared_ptr<boost::asynchronous::detail::interrupt_state> state)
        :m_state(state){}

    std::shared_ptr<boost::asynchronous::detail::interrupt_state> m_state;
};
template <class dummy = void >
std::shared_ptr<boost::asynchronous::detail::interrupt_state> get_interrupt_state(
        std::shared_ptr<boost::asynchronous::detail::interrupt_state> state= std::shared_ptr<boost::asynchronous::detail::interrupt_state>(), bool reset=false )
{
    static boost::thread_specific_ptr<boost::asynchronous::tss_interrupt_state_wrapper > s_state;
    if (reset)
    {
        s_state.reset(new boost::asynchronous::tss_interrupt_state_wrapper(state));
    }
    // state could be empty (no interrupt call from user, just return an empty pointer)
    if (s_state.get() == 0)
        return std::shared_ptr<boost::asynchronous::detail::interrupt_state>();
    return s_state.get()->m_state;
}

template <class dummy = void >
 std::size_t get_own_queue_index(std::size_t i=0,bool reset=false)
{
    static boost::thread_specific_ptr< std::size_t > s_index;
    if (reset)
    {
        s_index.reset(new std::size_t(i));
    }
    if (s_index.get() == 0)
        return 0;
    return *s_index.get();
}

 template <class Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB >
 struct tss_weak_diagnostics_wrapper
 {
     tss_weak_diagnostics_wrapper(std::weak_ptr<typename boost::asynchronous::job_traits<Job>::diagnostic_table_type> d)
         :m_diagnostics(d){}
     std::weak_ptr<typename boost::asynchronous::job_traits<Job>::diagnostic_table_type> m_diagnostics;
 };

 template <class Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB >
 std::weak_ptr<typename boost::asynchronous::job_traits<Job>::diagnostic_table_type>
 get_scheduler_diagnostics(std::weak_ptr<typename boost::asynchronous::job_traits<Job>::diagnostic_table_type> wdiags=
                                std::weak_ptr<typename boost::asynchronous::job_traits<Job>::diagnostic_table_type>(),
                           bool reset=false)
 {
     static boost::thread_specific_ptr<boost::asynchronous::tss_weak_diagnostics_wrapper<Job> > s_diagnostics;
     if (reset)
     {
         s_diagnostics.reset(new boost::asynchronous::tss_weak_diagnostics_wrapper<Job>(wdiags));
     }
     if (!s_diagnostics.get())
         return std::weak_ptr<typename boost::asynchronous::job_traits<Job>::diagnostic_table_type>();
     return s_diagnostics.get()->m_diagnostics;
 }

}}

#endif // BOOST_ASYNCHRON_SCHEDULER_TSS_SCHEDULERHPP
