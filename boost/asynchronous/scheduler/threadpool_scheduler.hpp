// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_SCHEDULER_THREADPOOL_SCHEDULER_HPP
#define BOOST_ASYNC_SCHEDULER_THREADPOOL_SCHEDULER_HPP

#include <utility>
#include <vector>
#include <cstddef>

#ifndef BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE
#endif

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/mutex.hpp>

#include <boost/asynchronous/scheduler/detail/scheduler_helpers.hpp>
#include <boost/asynchronous/scheduler/detail/exceptions.hpp>
#include <boost/asynchronous/detail/any_interruptible.hpp>
#include <boost/asynchronous/diagnostics/default_loggable_job.hpp>
#include <boost/asynchronous/job_traits.hpp>
#include <boost/asynchronous/scheduler/detail/job_diagnostic_closer.hpp>
#include <boost/asynchronous/scheduler/detail/single_queue_scheduler_policy.hpp>
#include <boost/asynchronous/detail/any_joinable.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler/tss_scheduler.hpp>
#include <boost/asynchronous/scheduler/detail/lockable_weak_scheduler.hpp>
#include <boost/asynchronous/scheduler/detail/any_continuation.hpp>
#include <boost/asynchronous/scheduler/cpu_load_policies.hpp>

namespace boost { namespace asynchronous
{

template<class Q, class CPULoad =
#ifndef BOOST_ASYNCHRONOUS_SAVE_CPU_LOAD
         boost::asynchronous::no_cpu_load_saving
#else
         boost::asynchronous::default_save_cpu_load<>
#endif
         >
class threadpool_scheduler: public boost::asynchronous::detail::single_queue_scheduler_policy<Q>
{
public:
    typedef boost::asynchronous::threadpool_scheduler<Q,CPULoad> this_type;
    typedef Q queue_type;
    typedef typename Q::job_type job_type;
    typedef typename boost::asynchronous::job_traits<typename Q::job_type>::diagnostic_table_type diag_type;
    
#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES
    template<typename... Args>
    threadpool_scheduler(size_t number_of_workers, Args... args)
        : boost::asynchronous::detail::single_queue_scheduler_policy<Q>(boost::make_shared<queue_type>(std::move(args)...))
        , m_number_of_workers(number_of_workers)
    {
        m_private_queues.reserve(number_of_workers);
        for (size_t i = 0; i< number_of_workers;++i)
        {
            m_private_queues.push_back(
                        boost::make_shared<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> >());
        }
    }
#endif
    threadpool_scheduler(size_t number_of_workers)
        : boost::asynchronous::detail::single_queue_scheduler_policy<Q>()
        , m_number_of_workers(number_of_workers)
    {
        m_private_queues.reserve(number_of_workers);
        for (size_t i = 0; i< number_of_workers;++i)
        {
            m_private_queues.push_back(
                        boost::make_shared<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> >());
        }
    }
    void constructor_done(boost::weak_ptr<this_type> weak_self)
    {
        m_diagnostics = boost::make_shared<diag_type>();
        m_thread_ids.reserve(m_number_of_workers);
        m_group.reset(new boost::thread_group);
        for (size_t i = 0; i< m_number_of_workers;++i)
        {
            boost::promise<boost::thread*> new_thread_promise;
            boost::shared_future<boost::thread*> fu = new_thread_promise.get_future();
            boost::thread* new_thread =
                    m_group->create_thread(boost::bind(&threadpool_scheduler::run,this->m_queue,
                                                       m_private_queues[i],m_diagnostics,fu,weak_self));
            new_thread_promise.set_value(new_thread);
            m_thread_ids.push_back(new_thread->get_id());
        }
    }

    ~threadpool_scheduler()
    {
        for (size_t i = 0; i< m_thread_ids.size();++i)
        {
            boost::asynchronous::detail::default_termination_task<typename Q::diagnostic_type,boost::thread_group> ttask;
            // this task has to be executed lat => lowest prio
#ifndef BOOST_NO_RVALUE_REFERENCES
            boost::asynchronous::any_callable job(std::move(ttask));
            m_private_queues[i]->push(std::move(job),std::numeric_limits<std::size_t>::max());
#else
            m_private_queues[i]->push(boost::asynchronous::any_callable(ttask),std::numeric_limits<std::size_t>::max());
#endif
        }
    }

    //TODO move?
    boost::asynchronous::any_joinable get_worker()const
    {
        return boost::asynchronous::any_joinable (boost::asynchronous::detail::worker_wrap<boost::thread_group>(m_group));
    }

    std::vector<boost::thread::id> thread_ids()const
    {
        return m_thread_ids;
    }
    std::map<std::string,
             std::list<typename boost::asynchronous::job_traits<typename queue_type::job_type>::diagnostic_item_type > >
    get_diagnostics(std::size_t =0)const
    {
        return m_diagnostics->get_map();
    }
    void clear_diagnostics()
    {
        m_diagnostics->clear();
    }
    void set_steal_from_queues(std::vector<boost::asynchronous::any_queue_ptr<job_type> > const&)
    {
        // this scheduler does not steal
    }
    void set_name(std::string const& name)
    {
        for (size_t i = 0; i< m_thread_ids.size();++i)
        {
            boost::asynchronous::detail::set_name_task<typename Q::diagnostic_type> ntask(name);
#ifndef BOOST_NO_RVALUE_REFERENCES
            boost::asynchronous::any_callable job(std::move(ntask));
            m_private_queues[i]->push(std::move(job),std::numeric_limits<std::size_t>::max());
#else
            m_private_queues[i]->push(boost::asynchronous::any_callable(ntask),std::numeric_limits<std::size_t>::max());
#endif
        }
    }
    // try to execute a job, return true
    static bool execute_one_job(boost::shared_ptr<queue_type> queue,CPULoad& cpu_load,boost::shared_ptr<diag_type> diagnostics,
                                std::list<boost::asynchronous::any_continuation>& waiting)
    {
        bool popped = false;
        // get a job
        typename Q::job_type job;
        try
        {

            // try from queue
            popped = queue->try_pop(job);
            // did we manage to pop or steal?
            if (popped)
            {
                cpu_load.popped_job();
                // automatic closing of log
                boost::asynchronous::detail::job_diagnostic_closer<typename Q::job_type,diag_type> closer
                        (&job,diagnostics.get());
                // log time
                boost::asynchronous::job_traits<typename Q::job_type>::set_started_time(job);
                // execute job
                job();
            }
            else
            {
                // look for waiting tasks
                if (!waiting.empty())
                {
                    for (std::list<boost::asynchronous::any_continuation>::iterator it = waiting.begin(); it != waiting.end();)
                    {
                        if ((*it).is_ready())
                        {
                            boost::asynchronous::any_continuation c = std::move(*it);
                            it = waiting.erase(it);
                            c();
                        }
                        else
                        {
                            ++it;
                        }
                    }
                }
            }
        }
        catch(boost::thread_interrupted&)
        {
            // task interrupted, no problem, just continue
        }
        catch(std::exception&)
        {
            boost::asynchronous::job_traits<typename Q::job_type>::set_failed(job);
        }
        return popped;
    }

    static void run(boost::shared_ptr<queue_type> queue,
                    boost::shared_ptr<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> > private_queue,
                    boost::shared_ptr<diag_type> diagnostics,
                    boost::shared_future<boost::thread*> self,
                    boost::weak_ptr<this_type> this_)
    {
        boost::thread* t = self.get();
        boost::asynchronous::detail::single_queue_scheduler_policy<Q>::m_self_thread.reset(new thread_ptr_wrapper(t));
        // thread scheduler => tss
        boost::asynchronous::any_weak_scheduler<job_type> self_as_weak = boost::asynchronous::detail::lockable_weak_scheduler<this_type>(this_);
        boost::asynchronous::get_thread_scheduler<job_type>(self_as_weak,true);

        std::list<boost::asynchronous::any_continuation>& waiting =
                boost::asynchronous::get_continuations(std::list<boost::asynchronous::any_continuation>(),true);

        CPULoad cpu_load;
        while(true)
        {
            try
            {
                {
                    bool popped = execute_one_job(queue,cpu_load,diagnostics,waiting);
                    if (!popped)
                    {
                        cpu_load.loop_done_no_job();
                        // nothing for us to do, give up our time slice
                        boost::this_thread::yield();
                    }
                    // check for shutdown
                    boost::asynchronous::any_callable djob;
                    popped = private_queue->try_pop(djob);
                    if (popped)
                    {
                        djob();
                    }
                } // job destroyed (for destruction useful)
                // check if we got an interruption job
                boost::this_thread::interruption_point();
            }
            catch(boost::asynchronous::detail::shutdown_exception&)
            {
                // we are done, execute jobs posted short before to the end, then shutdown
                while(execute_one_job(queue,cpu_load,diagnostics,waiting));
                return;
            }
            catch(boost::thread_interrupted&)
            {
                // task interrupted, no problem, just continue
            }
            catch(std::exception&)
            {
                // TODO, user-defined error
            }
        }
    }
private:
    boost::shared_ptr<boost::thread_group> m_group;
    std::vector<boost::thread::id> m_thread_ids;
    boost::shared_ptr<diag_type> m_diagnostics;
    std::vector<boost::shared_ptr<
                boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable>>> m_private_queues;
    size_t m_number_of_workers;
};

}} // boost::asynchronous::scheduler


#endif // BOOST_ASYNC_SCHEDULER_THREADPOOL_SCHEDULER_HPP
