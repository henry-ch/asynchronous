// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRON_SCHEDULER_INTERRUPTIBLE_JOB_HPP
#define BOOST_ASYNCHRON_SCHEDULER_INTERRUPTIBLE_JOB_HPP

#ifndef BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE
#endif

#include <boost/thread/future.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/asynchronous/scheduler/detail/interrupt_state.hpp>
#include <boost/asynchronous/job_traits.hpp>
#include <boost/asynchronous/scheduler/tss_scheduler.hpp>

namespace boost { namespace asynchronous
{

struct thread_ptr_wrapper
{
    thread_ptr_wrapper(boost::thread* t):m_thread(t){}
    boost::thread* m_thread;
};

struct interrupt_helper
{
    interrupt_helper(boost::future<boost::thread*>&& worker,
                     boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state)
        : m_worker(worker.share())
        , m_state(state)
    {}
    void interrupt()
    {
        m_state->interrupt(m_worker);
    }
private:
    boost::shared_future<boost::thread*>  m_worker;
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state>    m_state;
};
template <class Job,class Scheduler>
struct interruptible_job : public boost::asynchronous::job_traits<Job>::diagnostic_type
{
    interruptible_job(interruptible_job&& )=default;
    interruptible_job(interruptible_job const& )=default;
    interruptible_job& operator= (interruptible_job&& )=default;
    interruptible_job& operator= (interruptible_job const& )=default;

#ifndef BOOST_NO_RVALUE_REFERENCES
    interruptible_job(Job&& job,
#else
    interruptible_job(Job job,
#endif
                      boost::shared_ptr<boost::promise<boost::thread*> > worker_promise,
                      boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state)
        : m_worker(worker_promise)
        , m_state(state)
#ifndef BOOST_NO_RVALUE_REFERENCES
        , m_job(std::forward<Job>(job))
#else
        , m_job(job)
#endif
    {
        // we'll consider we are interrupted unless we have a chance to finish
        this->set_interrupted(true);
        // copy what we need from contained job
        // TODO something better...
        this->set_name(boost::asynchronous::job_traits<Job>::get_name(job));
        this->set_posted_time();
    }

    void operator()()
    {
        // save state to tss (valid until next task)
        boost::asynchronous::get_interrupt_state<>(m_state,true);
        // if already interrupted, we don't need to start
        if (m_state->is_interrupted())
        {
            return;
        }
        m_worker->set_value(Scheduler::m_self_thread.get()->m_thread);
        m_state->run();
        // if interrupted in this very short time, we don't need to start
        if (m_state->is_interrupted())
        {
            return;
        }
        m_job();
        // ok we were not interrupted
        this->set_interrupted(false);
        m_state->complete();
    }
    template<class Archive>
    void serialize(Archive & , const unsigned int /* version */){/* not implemented */}
    std::string get_task_name()const
    {
        // not implemented
        return "";
    }

    boost::shared_ptr<boost::promise<boost::thread*> >                  m_worker;
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state>     m_state;
    Job                                                                 m_job;
};

}}
#endif // BOOST_ASYNCHRON_SCHEDULER_INTERRUPTIBLE_JOB_HPP
