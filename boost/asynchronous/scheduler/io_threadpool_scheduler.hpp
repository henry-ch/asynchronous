// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef  BOOST_ASYNCHRONOUS_SCHEDULER_IO_THREADPOOL_SCHEDULER_HPP
#define BOOST_ASYNCHRONOUS_SCHEDULER_IO_THREADPOOL_SCHEDULER_HPP

#include <utility>
#include <vector>
#include <cstddef>
#include <set>
#include <numeric>
#include <functional>
#include <memory>

#include <boost/thread/thread.hpp>
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
#include <boost/asynchronous/scheduler/detail/execute_in_all_threads.hpp>

namespace boost { namespace asynchronous
{

template<class Q, class CPULoad =
#ifdef BOOST_ASYNCHRONOUS_NO_SAVING_CPU_LOAD
         boost::asynchronous::no_cpu_load_saving
#else
         boost::asynchronous::default_save_cpu_load<>
#endif
         >
class io_threadpool_scheduler: public boost::asynchronous::detail::single_queue_scheduler_policy<Q>
{
public:
    typedef boost::asynchronous::detail::single_queue_scheduler_policy<Q> base_type;
    typedef boost::asynchronous::io_threadpool_scheduler<Q,CPULoad> this_type;
    typedef Q queue_type;
    typedef typename Q::job_type job_type;
    typedef typename boost::asynchronous::job_traits<typename Q::job_type>::diagnostic_table_type diag_type;

    template<typename... Args>
    io_threadpool_scheduler(size_t min_number_of_workers, size_t max_number_of_workers, Args... args)
        : boost::asynchronous::detail::single_queue_scheduler_policy<Q>(std::make_shared<queue_type>(std::move(args)...))
        , m_data (std::make_shared<internal_data>(min_number_of_workers,max_number_of_workers, this->m_queue))
    {
        m_private_queues.reserve(max_number_of_workers);
        for (size_t i = 0; i< max_number_of_workers;++i)
        {
            m_private_queues.push_back(
                        std::make_shared<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> >());
        }
    }
    template<typename... Args>
    io_threadpool_scheduler(size_t min_number_of_workers, size_t max_number_of_workers,std::string const& name, Args... args)
        : boost::asynchronous::detail::single_queue_scheduler_policy<Q>(std::make_shared<queue_type>(std::move(args)...))
        , m_data (std::make_shared<internal_data>(min_number_of_workers,max_number_of_workers, this->m_queue))
        , m_name(name)
    {
        m_private_queues.reserve(max_number_of_workers);
        for (size_t i = 0; i< max_number_of_workers;++i)
        {
            m_private_queues.push_back(
                        std::make_shared<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> >());
        }
        set_name(name);
    }
    void constructor_done(std::weak_ptr<this_type> weak_self)
    {
        m_diagnostics = std::make_shared<diag_type>(m_data->m_max_number_of_workers);
        m_data->m_weak_self = weak_self;
        for (size_t i = 0; i< m_data->m_min_number_of_workers;++i)
        {
            std::promise<boost::thread*> new_thread_promise;
            std::shared_future<boost::thread*> fu = new_thread_promise.get_future();
            boost::mutex::scoped_lock lock(m_data->m_current_number_of_workers_mutex);
            ++m_data->m_current_number_of_workers;
            boost::thread* new_thread =
                    m_data->m_group->create_thread(
                        std::bind(&io_threadpool_scheduler::run_always,this->m_queue,
                                    m_private_queues[i],m_diagnostics,fu,weak_self,i));
            new_thread_promise.set_value(new_thread);
            m_data->m_thread_ids.insert(new_thread->get_id());
        }
    }
    std::vector<boost::asynchronous::any_queue_ptr<job_type> > get_queues()
    {
        // this scheduler doesn't give any queues for stealing
        return std::vector<boost::asynchronous::any_queue_ptr<job_type> >();
    }
    ~io_threadpool_scheduler()
    {
        for (size_t i = 0; i< m_data->m_min_number_of_workers;++i)
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
            // this task has to be executed lat => lowest prio
#ifndef BOOST_NO_RVALUE_REFERENCES
            boost::asynchronous::any_callable job(std::move(ttask));
            m_private_queues[i]->push(std::move(job),std::numeric_limits<std::size_t>::max());
#else
            m_private_queues[i]->push(boost::asynchronous::any_callable(ttask),std::numeric_limits<std::size_t>::max());
#endif
        }
        // join older threads
        m_data->cleanup_threads();
    }

    //TODO move?
    boost::asynchronous::any_joinable get_worker()const
    {
        boost::mutex::scoped_lock lock(m_data->m_current_number_of_workers_mutex);
        m_data->m_joining = true;
        return boost::asynchronous::any_joinable (boost::asynchronous::detail::worker_wrap<boost::thread_group>(m_data->m_group));
    }

    std::vector<boost::thread::id> thread_ids()const
    {
        boost::mutex::scoped_lock lock(m_data->m_current_number_of_workers_mutex);
        return std::vector<boost::thread::id>(m_data->m_thread_ids.begin(),m_data->m_thread_ids.end());
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

    void try_create_thread()
    {
        boost::mutex::scoped_lock lock(m_data->m_current_number_of_workers_mutex);
        if ((m_data->m_current_number_of_workers+1 <= m_data->m_max_number_of_workers) &&
             this->m_data->calculate_queue_size() >1 &&
             !m_data->m_joining)
        {
            ++m_data->m_current_number_of_workers;
            std::promise<boost::thread*> new_thread_promise;
            std::shared_future<boost::thread*> fu = new_thread_promise.get_future();
            auto data = m_data;
            std::function<void(boost::thread*)> d = [data](boost::thread* p)mutable{data->thread_finished(p);};
            std::function<bool()> c = [data]()mutable{return data->test_worker_loop();};
            boost::thread* new_thread =
                     m_data->m_group->create_thread(std::bind(&io_threadpool_scheduler::run_once,this->m_queue,
                                                                m_private_queues[m_data->m_current_number_of_workers-1],
                                                                m_diagnostics,fu,m_data->m_weak_self,d,c,
                                                                m_data->m_current_number_of_workers-1,
                                                                m_name,
                                                                m_data->m_execute_in_all_threads));
            m_data->m_thread_ids.insert(new_thread->get_id());
            new_thread_promise.set_value(new_thread);
        }
    }
    // overwrite from base
#ifndef BOOST_NO_RVALUE_REFERENCES
    void post(typename queue_type::job_type job, std::size_t prio)
    {
        boost::asynchronous::job_traits<typename queue_type::job_type>::set_posted_time(job);
        this->m_queue->push(std::move(job),prio);
        // create a new thread if needed
        this->try_create_thread();
        // join older threads
        this->m_data->cleanup_threads();
    }
    void post(typename queue_type::job_type job)
    {
        post(std::move(job),0);
    }
    void post(boost::asynchronous::any_callable job,const std::string& name)
    {
        typename boost::asynchronous::job_traits<job_type>::wrapper_type w(std::move(job));
        w.set_name(name);
        post(std::move(w));
    }
    void post(boost::asynchronous::any_callable job,const std::string& name,std::size_t priority)
    {
        typename boost::asynchronous::job_traits<job_type>::wrapper_type w(std::move(job));
        w.set_name(name);
        post(std::move(w),priority);
    }
    boost::asynchronous::any_interruptible interruptible_post(typename queue_type::job_type job,
                                                          std::size_t prio)
    {
        std::shared_ptr<boost::asynchronous::detail::interrupt_state>
                state = std::make_shared<boost::asynchronous::detail::interrupt_state>();
        std::shared_ptr<std::promise<boost::thread*> > wpromise = std::make_shared<std::promise<boost::thread*> >();
        boost::asynchronous::job_traits<typename queue_type::job_type>::set_posted_time(job);
        boost::asynchronous::interruptible_job<typename queue_type::job_type,this_type>
                ijob(std::move(job),wpromise,state);

        this->m_queue->push(std::forward<typename queue_type::job_type>(ijob),prio);

        std::future<boost::thread*> fu = wpromise->get_future();
        boost::asynchronous::interrupt_helper interruptible(std::move(fu),state);

        // create a new thread if needed
        this->try_create_thread();
        // join older threads
        this->m_data->cleanup_threads();

        return boost::asynchronous::any_interruptible(interruptible);
    }
    boost::asynchronous::any_interruptible interruptible_post(typename queue_type::job_type job)
    {
        return interruptible_post(std::move(job),0);
    }
    boost::asynchronous::any_interruptible interruptible_post(boost::asynchronous::any_callable job,const std::string& name)
    {
        typename boost::asynchronous::job_traits<job_type>::wrapper_type w(std::move(job));
        w.set_name(name);
        return interruptible_post(std::move(w));
    }
    boost::asynchronous::any_interruptible interruptible_post(boost::asynchronous::any_callable job,const std::string& name,
                                                           std::size_t priority)
    {
        typename boost::asynchronous::job_traits<job_type>::wrapper_type w(std::move(job));
        w.set_name(name);
        return interruptible_post(std::move(w),priority);
    }
#else
    void post(typename queue_type::job_type& job, std::size_t prio=0)
    {
        boost::asynchronous::job_traits<typename queue_type::job_type>::set_posted_time(job);
        m_queue->push(job,prio);
        // create a new thread if needed
        this->try_create_thread();
        // join older threads
        this->m_data->cleanup_threads();
    }
    boost::asynchronous::any_interruptible interruptible_post(typename queue_type::job_type& job, std::size_t prio=0)
    {
        std::shared_ptr<boost::asynchronous::detail::interrupt_state>
                state = std::make_shared<boost::asynchronous::detail::interrupt_state>();
        std::shared_ptr<std::promise<boost::thread*> > wpromise = std::make_shared<std::promise<boost::thread*> >();
        boost::asynchronous::job_traits<typename queue_type::job_type>::set_posted_time(job);
        boost::asynchronous::interruptible_job<typename queue_type::job_type,this_type> ijob(job,wpromise,state);

        m_queue->push(ijob,prio);

        std::shared_future<boost::thread*> fu = wpromise->get_future();
        boost::asynchronous::interrupt_helper interruptible(fu,state);

        // create a new thread if needed
        this->try_create_thread();
        // join older threads
        m_data->cleanup_threads();

        return boost::asynchronous::any_interruptible(interruptible);
    }
#endif
    // end overwrite from base

    void set_name(std::string const& name)
    {
        boost::mutex::scoped_lock lock(m_data->m_current_number_of_workers_mutex);
        for (size_t i = 0; i< m_data->m_thread_ids.size();++i)
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
        std::size_t idsize=0;
        {
            boost::mutex::scoped_lock lock(m_data->m_current_number_of_workers_mutex);
            idsize = m_data->m_thread_ids.size();
        }
        // our thread (queue) index. 0 means "don't care" and is therefore not desirable)
        size_t t = 0;
        for(auto const& v : p)
        {
            for (size_t i = 0; i< std::get<1>(v) && (t< idsize);++i)
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
        std::size_t idsize=0;
        {
            boost::mutex::scoped_lock lock(m_data->m_current_number_of_workers_mutex);
            idsize = m_data->m_thread_ids.size();
            // remember waiting callable for newly created threads
            m_data->m_execute_in_all_threads.push_back(c);
        }
        // handle basic (always present) threads
        res.reserve(idsize);
        for (size_t i = 0; i< idsize;++i)
        {
            std::promise<void> p;
            auto fu = p.get_future();
            res.emplace_back(std::move(fu));
            boost::asynchronous::detail::execute_in_all_threads_task task(c,std::move(p));
            m_private_queues[i]->push(std::move(task),std::numeric_limits<std::size_t>::max());
        }
        return res;
    }

    // try to execute a job, return true
    static bool execute_one_job(std::shared_ptr<queue_type> const& queue,CPULoad* cpu_load,std::shared_ptr<diag_type> diagnostics,
                                std::list<boost::asynchronous::any_continuation>& waiting,size_t index)
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
                if (cpu_load)
                    cpu_load->popped_job();
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
    static bool run_loop(std::shared_ptr<queue_type> const& queue,
                         std::shared_ptr<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> > const& private_queue,
                         std::list<boost::asynchronous::any_continuation>& waiting,
                         std::shared_ptr<diag_type> diagnostics,CPULoad* cpu_load,
                         size_t index)
    {
        bool executed_job=false;
        {
            {
                executed_job = execute_one_job(queue,cpu_load,diagnostics,waiting,index);
                if (!executed_job)
                {
                    if (cpu_load)
                        cpu_load->loop_done_no_job();
                    // nothing for us to do, give up our time slice
                    boost::this_thread::yield();
                }
                // check for shutdown
                boost::asynchronous::any_callable djob;
                if (!!private_queue)
                    executed_job = private_queue->try_pop(djob);
                if (executed_job)
                {
                    djob();
                }
            } // job destroyed (for destruction useful)
            // check if we got an interruption job
            boost::this_thread::interruption_point();
        } // job destroyed (for destruction useful)
        return executed_job;
    }

    static void run_always(std::shared_ptr<queue_type> const& queue,
                           std::shared_ptr<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> > const& private_queue,
                           std::shared_ptr<diag_type> diagnostics,
                           std::shared_future<boost::thread*> self,
                           std::weak_ptr<this_type> this_,
                           size_t index)
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
                run_loop(queue,private_queue,waiting,diagnostics,&cpu_load,index);
            }
            catch(boost::asynchronous::detail::shutdown_exception&)
            {
                // we are done, execute jobs posted short before to the end, then shutdown
                while(execute_one_job(queue,&cpu_load,diagnostics,waiting,index));
                delete boost::asynchronous::detail::single_queue_scheduler_policy<Q>::m_self_thread.release();
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
            catch(...)
            {
                // TODO, user-defined error
            }
        }
    }
    static void run_once(std::shared_ptr<queue_type> const& queue,
                         std::shared_ptr<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> > const& /*private_queue*/,
                         std::shared_ptr<diag_type> diagnostics,
                         std::shared_future<boost::thread*> self,
                         std::weak_ptr<this_type> this_,
                         std::function<void(boost::thread*)> on_done,
                         std::function<bool()> check_continue,
                         size_t index,
                         std::string name,
                         std::vector<boost::asynchronous::any_callable> execute_in_all_threads_tasks)
    {
        boost::thread* t = self.get();
        boost::asynchronous::detail::single_queue_scheduler_policy<Q>::m_self_thread.reset(new thread_ptr_wrapper(t));
        // thread scheduler => tss
        boost::asynchronous::any_weak_scheduler<job_type> self_as_weak = boost::asynchronous::detail::lockable_weak_scheduler<this_type>(this_);
        boost::asynchronous::get_thread_scheduler<job_type>(self_as_weak,true);

        std::list<boost::asynchronous::any_continuation>& waiting =
                boost::asynchronous::get_continuations(std::list<boost::asynchronous::any_continuation>(),true);
        // set thread name
        boost::asynchronous::detail::set_name_task<typename Q::diagnostic_type> ntask(name);
        ntask();
        // execute tasks which were given to us and have to be executed in all threads
        for (auto& c : execute_in_all_threads_tasks)
        {
            c();
        }
        // we run once then end
        do
        {
            try
            {
                run_loop(queue,std::shared_ptr<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> >(),
                         waiting,diagnostics,nullptr,index);
            }
            catch(boost::asynchronous::detail::shutdown_exception&)
            {
                on_done(t);
                // we are done
                break;
            }
            catch(boost::thread_interrupted&)
            {
                // task interrupted, no problem, just continue
            }
            catch(std::exception&)
            {
                // TODO, user-defined error
            }
            catch(...)
            {
                // TODO, user-defined error
            }
        }
        while (check_continue());
        on_done(t);
    }

private:
    struct internal_data
    {
        internal_data(size_t min_number_of_workers, size_t max_number_of_workers, std::shared_ptr<queue_type> queue)
        : m_min_number_of_workers(min_number_of_workers <= max_number_of_workers ? min_number_of_workers:max_number_of_workers)
        , m_max_number_of_workers(min_number_of_workers <= max_number_of_workers ? max_number_of_workers:min_number_of_workers)
        , m_current_number_of_workers(0)
        , m_done_threads()
        , m_weak_self()
        , m_group(std::make_shared<boost::thread_group>())
        , m_queue(queue)
        {}

        // check queue size. Used to find out when temporary threads have to shutdown
        std::size_t calculate_queue_size()const
        {
             std::vector<std::size_t> vec_sizes = this->m_queue->get_queue_size();
             return std::accumulate(vec_sizes.begin(),vec_sizes.end(),0,[](std::size_t a, std::size_t b){return a+b;});
        }
        // checks if worker thread should continue
        // if queue not empty, continue
        bool test_worker_loop()
        {
           boost::mutex::scoped_lock lock(m_current_number_of_workers_mutex);
           bool res = (this->calculate_queue_size() != 0);//(m_current_number_of_workers > m_min_number_of_workers);
           /*if (res)
           {
               --m_current_number_of_workers;
           }*/
           return res && !m_joining;
        }
        void basic_thread_not_busy()
        {
            //TODO needed?
            // update number of workers
            //boost::mutex::scoped_lock lock(m_current_number_of_workers_mutex);
            //--m_current_number_of_workers;
        }

        void thread_finished(boost::thread* finished_thread)
        {
            // update number of workers
            {
                boost::mutex::scoped_lock lock(m_current_number_of_workers_mutex);
                --m_current_number_of_workers;
                m_thread_ids.erase(finished_thread->get_id());
            }
            // update set of finished threads
            {
                boost::mutex::scoped_lock lock(m_done_threads_mutex);
                m_done_threads.insert(finished_thread);
            }
        }
        // checks which threads are finished and can be removed from thread group and joined
        // this is called when a new job is posted or at pool destrution
        void cleanup_threads()
        {
            // look for finished threads
            std::set<boost::thread*> to_join_threads;
            {
                boost::mutex::scoped_lock lock(m_done_threads_mutex);
    #ifndef BOOST_NO_RVALUE_REFERENCES
                to_join_threads = std::move(m_done_threads);
    #else
                to_join_threads = m_done_threads;
    #endif
                m_done_threads.clear();
            }
            // remove from group and join
            boost::thread_group cleanup_group;
            {
                boost::mutex::scoped_lock lock(m_current_number_of_workers_mutex);
                for (std::set<boost::thread*>::iterator it = to_join_threads.begin(); it != to_join_threads.end();++it)
                {
                    m_group->remove_thread(*it);
                }
            }
            for (std::set<boost::thread*>::iterator it = to_join_threads.begin(); it != to_join_threads.end();++it)
            {
                cleanup_group.add_thread(*it);
            }
            // join. Not blocking as all these threads are finished
            cleanup_group.join_all();
        }

        std::set<boost::thread::id> m_thread_ids;
        size_t m_min_number_of_workers;
        size_t m_max_number_of_workers;
        size_t m_current_number_of_workers;
        std::set<boost::thread*> m_done_threads;
        std::weak_ptr<this_type> m_weak_self;
        std::shared_ptr<boost::thread_group> m_group;
        std::shared_ptr<queue_type> m_queue;
        // remembers if the thread group has already been given for joining. If yes, no thread will be created
        bool m_joining=false;
        // pass them to any newly created thread
        std::vector<boost::asynchronous::any_callable> m_execute_in_all_threads;
        // protects m_done_threads
        boost::mutex m_done_threads_mutex;
        // protects m_current_number_of_workers
        mutable boost::mutex m_current_number_of_workers_mutex;
    };
    std::shared_ptr<internal_data> m_data;

    std::shared_ptr<diag_type> m_diagnostics;
    std::vector<std::shared_ptr<
                boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable>>> m_private_queues;
    std::function<void(boost::asynchronous::scheduler_diagnostics)> m_diagnostics_fct;
    const std::string m_name;
};

}} // boost::asynchronous::scheduler

#endif // BOOST_ASYNCHRONOUS_SCHEDULER_IO_THREADPOOL_SCHEDULER_HPP
