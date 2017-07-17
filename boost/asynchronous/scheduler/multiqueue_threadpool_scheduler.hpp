// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_SCHEDULER_MULTIQUEUE_THREADPOOL_SCHEDULER_HPP
#define BOOST_ASYNC_SCHEDULER_MULTIQUEUE_THREADPOOL_SCHEDULER_HPP

#include <utility>
#include <vector>
#include <cstddef>
#include <memory>
#include <functional>
#include <future>

#include <boost/thread/thread.hpp>
#include <boost/thread/tss.hpp>


#include <boost/asynchronous/scheduler/detail/scheduler_helpers.hpp>
#include <boost/asynchronous/scheduler/detail/exceptions.hpp>
#include <boost/asynchronous/detail/any_interruptible.hpp>
#include <boost/asynchronous/scheduler/detail/interruptible_job.hpp>
#include <boost/asynchronous/diagnostics/default_loggable_job.hpp>
#include <boost/asynchronous/job_traits.hpp>
#include <boost/asynchronous/scheduler/detail/job_diagnostic_closer.hpp>
#include <boost/asynchronous/queue/find_queue_position.hpp>
#include <boost/asynchronous/queue/any_queue.hpp>
#include <boost/asynchronous/scheduler/detail/multi_queue_scheduler_policy.hpp>
#include <boost/asynchronous/detail/any_joinable.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler/tss_scheduler.hpp>
#include <boost/asynchronous/scheduler/cpu_load_policies.hpp>

#include <boost/asynchronous/scheduler/detail/lockable_weak_scheduler.hpp>
#include <boost/asynchronous/scheduler/detail/any_continuation.hpp>
#include <boost/asynchronous/scheduler/detail/execute_in_all_threads.hpp>


namespace boost { namespace asynchronous
{

//TODO boost.parameter
template<class Q,
         class FindPosition=boost::asynchronous::default_find_position< >,
         class CPULoad =
#ifdef BOOST_ASYNCHRONOUS_NO_SAVING_CPU_LOAD
         boost::asynchronous::no_cpu_load_saving
#else
         boost::asynchronous::default_save_cpu_load<>
#endif
         >
class multiqueue_threadpool_scheduler: public boost::asynchronous::detail::multi_queue_scheduler_policy<Q,FindPosition>                                     
{
public:
    typedef Q queue_type;
    typedef typename Q::job_type job_type;
    typedef typename boost::asynchronous::job_traits<typename Q::job_type>::diagnostic_table_type diag_type;
    typedef multiqueue_threadpool_scheduler<Q,FindPosition,CPULoad> this_type;

    template<typename... Args>
    multiqueue_threadpool_scheduler(size_t number_of_workers, Args... args)
        : boost::asynchronous::detail::multi_queue_scheduler_policy<Q,FindPosition>(number_of_workers,std::move(args)...)
        , m_number_of_workers(number_of_workers)
    {
        m_private_queues.reserve(number_of_workers);
        for (size_t i = 0; i< number_of_workers;++i)
        {
            m_private_queues.push_back(
                        std::make_shared<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> >());
        }
    }
    template<typename... Args>
    multiqueue_threadpool_scheduler(size_t number_of_workers,std::string const& name, Args... args)
        : boost::asynchronous::detail::multi_queue_scheduler_policy<Q,FindPosition>(number_of_workers,std::move(args)...)
        , m_number_of_workers(number_of_workers)
        , m_name(name)
    {
        m_private_queues.reserve(number_of_workers);
        for (size_t i = 0; i< number_of_workers;++i)
        {
            m_private_queues.push_back(
                        std::make_shared<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> >());
        }
        set_name(name);
    }
    void constructor_done(std::weak_ptr<this_type> weak_self)
    {
        m_diagnostics = std::make_shared<diag_type>(m_number_of_workers);
        m_thread_ids.reserve(m_number_of_workers);
        m_group.reset(new boost::thread_group);
        for (size_t i = 0; i< m_number_of_workers;++i)
        {
            std::promise<boost::thread*> new_thread_promise;
            std::shared_future<boost::thread*> fu = new_thread_promise.get_future();
            boost::thread* new_thread =
                    m_group->create_thread(std::bind(&multiqueue_threadpool_scheduler::run,this->m_queues,
                                                       m_private_queues[i],i,m_diagnostics,fu,weak_self));
            new_thread_promise.set_value(new_thread);
            m_thread_ids.push_back(new_thread->get_id());
        }
    }

    ~multiqueue_threadpool_scheduler()
    {
        for (size_t i = 0; i< m_number_of_workers;++i)
        {
            auto fct = m_diagnostics_fct;
            auto diag = m_diagnostics;
            auto l = [fct,diag]() mutable
            {
                if (fct)
                    fct(boost::asynchronous::scheduler_diagnostics(diag->get_map(),diag->get_current()));
            };
            boost::asynchronous::detail::default_termination_task<typename Q::diagnostic_type,boost::thread_group>
                    ttask(std::move(l));
            // this task has to be executed last => lowest prio
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

    boost::asynchronous::scheduler_diagnostics
    get_diagnostics(std::size_t =0)const
    {
        return boost::asynchronous::scheduler_diagnostics(m_diagnostics->get_map(),m_diagnostics->get_current());
    }
    void clear_diagnostics()
    {
        m_diagnostics->clear();
    }
    void register_diagnostics_functor(std::function<void(boost::asynchronous::scheduler_diagnostics)> fct,
                                      boost::asynchronous::register_diagnostics_type =
                                                    boost::asynchronous::register_diagnostics_type())
    {
        m_diagnostics_fct = fct;
    }
    void set_steal_from_queues(std::vector<boost::asynchronous::any_queue_ptr<job_type> > const&)
    {
        // this scheduler does not steal
    }
    void set_name(std::string const& name)
    {
        for (size_t i = 0; i< m_number_of_workers;++i)
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
    std::string get_name()const
    {
        return m_name;
    }
    void processor_bind(std::vector<std::tuple<unsigned int/*first core*/,unsigned int/*number of threads*/>> p)
    {
        // our thread (queue) index. 0 means "don't care" and is therefore not desirable)
        size_t t = 0;
        for(auto const& v : p)
        {
            for (size_t i = 0; i<= std::get<1>(v) && (t < m_number_of_workers);++i)
            {
                boost::asynchronous::detail::processor_bind_task task(std::get<0>(v)+i);
                boost::asynchronous::any_callable job(std::move(task));
                m_private_queues[t++]->push(std::move(job),std::numeric_limits<std::size_t>::max());
            }
        }
    }
    std::vector<std::future<void>> execute_in_all_threads(boost::asynchronous::any_callable c)
    {
        std::vector<std::future<void>> res;
        res.reserve(m_number_of_workers);
        for (size_t i = 0; i< m_number_of_workers;++i)
        {
            std::promise<void> p;
            auto fu = p.get_future();
            res.emplace_back(std::move(fu));
            boost::asynchronous::detail::execute_in_all_threads_task task(std::move(c),std::move(p));
            m_private_queues[i]->push(std::move(task),std::numeric_limits<std::size_t>::max());
        }
        return res;
    }

    // try to execute a job, return true
    static bool execute_one_job(std::vector<std::shared_ptr<queue_type> > const& queues,size_t index,CPULoad& cpu_load,std::shared_ptr<diag_type> diagnostics,
                                std::list<boost::asynchronous::any_continuation>& waiting)
    {
        bool popped = false;
        // get a job
        typename Q::job_type job;
        try
        {            
            popped = queues[index]->try_pop(job);
            if (!popped)
            {
                // ok we have nothing to do, maybe we can steal some work?
                for (std::size_t i=1; i< queues.size(); ++i)
                {
                    if (index >= i)
                    {
                        popped = queues[(index-i)%queues.size()]->try_steal(job);
                    }
                    else
                    {
                        popped = queues[queues.size()-(i-index)]->try_steal(job);
                    }
                    if (popped)
                        break;
                }
            }
            // did we manage to pop or steal?
            if (popped)
            {
                cpu_load.popped_job();
                // log time
                boost::asynchronous::job_traits<typename Q::job_type>::set_started_time(job);
                // log thread
                boost::asynchronous::job_traits<typename Q::job_type>::set_executing_thread_id(job,boost::this_thread::get_id());
                // log current
                boost::asynchronous::job_traits<typename Q::job_type>::add_current_diagnostic(index,job,diagnostics.get());
                // execute job
                job();
                boost::asynchronous::job_traits<typename Q::job_type>::reset_current_diagnostic(index,diagnostics.get());
                boost::asynchronous::job_traits<typename Q::job_type>::set_finished_time(job);
                boost::asynchronous::job_traits<typename Q::job_type>::add_diagnostic(job,diagnostics.get());
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
            if (popped)
            {
                boost::asynchronous::job_traits<typename Q::job_type>::set_failed(job);
                boost::asynchronous::job_traits<typename Q::job_type>::set_finished_time(job);
                boost::asynchronous::job_traits<typename Q::job_type>::add_diagnostic(job,diagnostics.get());
                boost::asynchronous::job_traits<typename Q::job_type>::reset_current_diagnostic(index,diagnostics.get());
            }
        }
        return popped;
    }

    static void run(std::vector<std::shared_ptr<queue_type> > const& queues,
                    std::shared_ptr<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> > const& private_queue,
                    size_t index,std::shared_ptr<diag_type> diagnostics,
                    std::shared_future<boost::thread*> self,
                    std::weak_ptr<this_type> this_)
    {
        boost::thread* t = self.get();
        boost::asynchronous::detail::multi_queue_scheduler_policy<Q,FindPosition>::m_self_thread.reset(new thread_ptr_wrapper(t));
        // thread scheduler => tss
        boost::asynchronous::any_weak_scheduler<job_type> self_as_weak = boost::asynchronous::detail::lockable_weak_scheduler<this_type>(this_);
        boost::asynchronous::get_thread_scheduler<job_type>(self_as_weak,true);
        boost::asynchronous::get_own_queue_index<>(index+1,true);

        std::list<boost::asynchronous::any_continuation>& waiting =
                boost::asynchronous::get_continuations(std::list<boost::asynchronous::any_continuation>(),true);
        CPULoad cpu_load;
        while(true)
        {
            try
            {
                {
                    bool popped = execute_one_job(queues,index,cpu_load,diagnostics,waiting);
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
                while(execute_one_job(queues,index,cpu_load,diagnostics,waiting));
                delete boost::asynchronous::detail::multi_queue_scheduler_policy<Q,FindPosition>::m_self_thread.release();
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
    size_t m_number_of_workers;
    std::shared_ptr<boost::thread_group> m_group;
    std::vector<boost::thread::id> m_thread_ids;
    std::shared_ptr<diag_type> m_diagnostics;
    std::vector<std::shared_ptr<
                boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable>>> m_private_queues;
    std::function<void(boost::asynchronous::scheduler_diagnostics)> m_diagnostics_fct;
    const std::string m_name;
};

}} // boost::async::scheduler

#endif // BOOST_ASYNC_SCHEDULER_MULTIQUEUE_THREADPOOL_SCHEDULER_HPP
