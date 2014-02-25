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

#ifndef BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE
#endif

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/assert.hpp>

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
class io_threadpool_scheduler: public boost::asynchronous::detail::single_queue_scheduler_policy<Q>
{
public:
    typedef boost::asynchronous::detail::single_queue_scheduler_policy<Q> base_type;
    typedef boost::asynchronous::io_threadpool_scheduler<Q,CPULoad> this_type;
    typedef Q queue_type;
    typedef typename Q::job_type job_type;
    typedef typename boost::asynchronous::job_traits<typename Q::job_type>::diagnostic_table_type diag_type;

#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES
    template<typename... Args>
    io_threadpool_scheduler(size_t min_number_of_workers, size_t max_number_of_workers, Args... args)
        : boost::asynchronous::detail::single_queue_scheduler_policy<Q>(boost::make_shared<queue_type>(args...))
        , m_data (boost::make_shared<internal_data>(min_number_of_workers,max_number_of_workers))
    {
        m_private_queues.reserve(max_number_of_workers);
        for (size_t i = 0; i< max_number_of_workers;++i)
        {
            m_private_queues.push_back(
                        boost::make_shared<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> >());
        }
    }
#endif
    io_threadpool_scheduler(size_t min_number_of_workers, size_t max_number_of_workers)
        : boost::asynchronous::detail::single_queue_scheduler_policy<Q>()
        , m_data (boost::make_shared<internal_data>(min_number_of_workers,max_number_of_workers))
    {
        m_private_queues.reserve(max_number_of_workers);
        for (size_t i = 0; i< max_number_of_workers;++i)
        {
            m_private_queues.push_back(
                        boost::make_shared<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> >());
        }
    }
    void constructor_done(boost::weak_ptr<this_type> weak_self)
    {
        m_diagnostics = boost::make_shared<diag_type>();
        m_data->m_weak_self = weak_self;
        for (size_t i = 0; i< m_data->m_min_number_of_workers;++i)
        {
            boost::promise<boost::thread*> new_thread_promise;
            boost::shared_future<boost::thread*> fu = new_thread_promise.get_future();
            boost::function<void()> d = boost::bind(&io_threadpool_scheduler::internal_data::basic_thread_not_busy,m_data);
            boost::thread* new_thread =
                    m_data->m_group->create_thread(
                        boost::bind(&io_threadpool_scheduler::run_always,this->m_queue,
                                    m_private_queues[i],m_diagnostics,fu,weak_self,d));
            new_thread_promise.set_value(new_thread);
            m_data->m_thread_ids.insert(new_thread->get_id());
        }
    }
    std::vector<boost::asynchronous::any_queue_ptr<job_type> > get_queues()const
    {
        // this scheduler doesn't give any queues for stealing
        return std::vector<boost::asynchronous::any_queue_ptr<job_type> >();
    }
    ~io_threadpool_scheduler()
    {
        for (size_t i = 0; i< m_data->m_thread_ids.size();++i)
        {
            boost::asynchronous::detail::default_termination_task<typename Q::diagnostic_type,boost::thread_group> ttask(m_data->m_group);
            // this task has to be executed lat => lowest prio
#ifndef BOOST_NO_RVALUE_REFERENCES
            boost::asynchronous::any_callable job(ttask);
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
        return boost::asynchronous::any_joinable (boost::asynchronous::detail::worker_wrap<boost::thread_group>(m_data->m_group));
    }

    std::vector<boost::thread::id> thread_ids()const
    {
        boost::mutex::scoped_lock lock(m_data->m_current_number_of_workers_mutex);
        return std::vector<boost::thread::id>(m_data->m_thread_ids.begin(),m_data->m_thread_ids.end());
    }
    std::map<std::string,
             std::list<typename boost::asynchronous::job_traits<typename queue_type::job_type>::diagnostic_item_type > >
    get_diagnostics(std::size_t =0)const
    {
        return m_diagnostics->get_map();
    }
    void set_steal_from_queues(std::vector<boost::asynchronous::any_queue_ptr<job_type> > const&)
    {
        // this scheduler does not steal
    }

    void try_create_thread()
    {
        boost::mutex::scoped_lock lock(m_data->m_current_number_of_workers_mutex);
        if ((m_data->m_current_number_of_workers+1 <= m_data->m_max_number_of_workers) &&
            (m_data->m_current_number_of_workers+1 > m_data->m_min_number_of_workers))
        {
            ++m_data->m_current_number_of_workers;
            lock.unlock();
            boost::promise<boost::thread*> new_thread_promise;
            boost::shared_future<boost::thread*> fu = new_thread_promise.get_future();
            boost::function<void(boost::thread*)> d = boost::bind(&io_threadpool_scheduler::internal_data::thread_finished,m_data,_1);
            boost::function<bool()> c = boost::bind(&io_threadpool_scheduler::internal_data::test_worker_loop,m_data);
            boost::thread* new_thread =
                     m_data->m_group->create_thread(boost::bind(&io_threadpool_scheduler::run_once,this->m_queue,
                                                                m_private_queues[m_data->m_current_number_of_workers-1],
                                                                m_diagnostics,fu,m_data->m_weak_self,d,c));
            lock.lock();
            m_data->m_thread_ids.insert(new_thread->get_id());
            new_thread_promise.set_value(new_thread);
        }
        else
        {
            ++m_data->m_current_number_of_workers;
        }
    }
    // overwrite from base
#ifndef BOOST_NO_RVALUE_REFERENCES
    void post(typename queue_type::job_type && job, std::size_t prio)
    {
        boost::asynchronous::job_traits<typename queue_type::job_type>::set_posted_time(job);
        this->m_queue->push(std::forward<typename queue_type::job_type>(job),prio);
        // create a new thread if needed
        this->try_create_thread();
        // join older threads
        this->m_data->cleanup_threads();
    }
    void post(typename queue_type::job_type && job)
    {
        post(std::forward<typename queue_type::job_type>(job),0);
    }
    void post(boost::asynchronous::any_callable&& job,const std::string& name)
    {
        typename boost::asynchronous::job_traits<job_type>::wrapper_type w(std::forward<boost::asynchronous::any_callable>(job));
        w.set_name(name);
        post(std::move(w));
    }
    void post(boost::asynchronous::any_callable&& job,const std::string& name,std::size_t priority)
    {
        typename boost::asynchronous::job_traits<job_type>::wrapper_type w(std::forward<boost::asynchronous::any_callable>(job));
        w.set_name(name);
        post(std::move(w),priority);
    }
    boost::asynchronous::any_interruptible interruptible_post(typename queue_type::job_type && job,
                                                          std::size_t prio)
    {
        boost::shared_ptr<boost::asynchronous::detail::interrupt_state>
                state = boost::make_shared<boost::asynchronous::detail::interrupt_state>();
        boost::shared_ptr<boost::promise<boost::thread*> > wpromise = boost::make_shared<boost::promise<boost::thread*> >();
        boost::asynchronous::job_traits<typename queue_type::job_type>::set_posted_time(job);
        boost::asynchronous::interruptible_job<typename queue_type::job_type,this_type>
                ijob(std::forward<typename queue_type::job_type>(job),wpromise,state);

        this->m_queue->push(std::forward<typename queue_type::job_type>(ijob),prio);

        boost::future<boost::thread*> fu = wpromise->get_future();
        boost::asynchronous::interrupt_helper interruptible(std::move(fu),state);

        // create a new thread if needed
        this->try_create_thread();
        // join older threads
        this->m_data->cleanup_threads();

        return boost::asynchronous::any_interruptible(interruptible);
    }
    boost::asynchronous::any_interruptible interruptible_post(typename queue_type::job_type && job)
    {
        return interruptible_post(std::forward<typename queue_type::job_type>(job),0);
    }
    boost::asynchronous::any_interruptible interruptible_post(boost::asynchronous::any_callable&& job,const std::string& name)
    {
        typename boost::asynchronous::job_traits<job_type>::wrapper_type w(std::forward<boost::asynchronous::any_callable>(job));
        w.set_name(name);
        return interruptible_post(std::move(w));
    }
    boost::asynchronous::any_interruptible interruptible_post(boost::asynchronous::any_callable&& job,const std::string& name,
                                                           std::size_t priority)
    {
        typename boost::asynchronous::job_traits<job_type>::wrapper_type w(std::forward<boost::asynchronous::any_callable>(job));
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
        boost::shared_ptr<boost::asynchronous::detail::interrupt_state>
                state = boost::make_shared<boost::asynchronous::detail::interrupt_state>();
        boost::shared_ptr<boost::promise<boost::thread*> > wpromise = boost::make_shared<boost::promise<boost::thread*> >();
        boost::asynchronous::job_traits<typename queue_type::job_type>::set_posted_time(job);
        boost::asynchronous::interruptible_job<typename queue_type::job_type,this_type> ijob(job,wpromise,state);

        m_queue->push(ijob,prio);

        boost::shared_future<boost::thread*> fu = wpromise->get_future();
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
        for (size_t i = 0; i< m_data->m_thread_ids.size();++i)
        {
            boost::asynchronous::detail::set_name_task<typename Q::diagnostic_type> ntask(name);
#ifndef BOOST_NO_RVALUE_REFERENCES
            boost::asynchronous::any_callable job(ntask);
            m_private_queues[i]->push(std::move(job),std::numeric_limits<std::size_t>::max());
#else
            m_private_queues[i]->push(boost::asynchronous::any_callable(ntask),std::numeric_limits<std::size_t>::max());
#endif
        }
    }

    static bool run_loop(boost::shared_ptr<queue_type> queue,
                         boost::shared_ptr<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> > private_queue,
                         std::deque<boost::asynchronous::any_continuation>& waiting,
                         boost::shared_ptr<diag_type> diagnostics,CPULoad* cpu_load)
    {
        bool executed_job=false;
        {
            // get a job
            //TODO rval ref?
            typename Q::job_type job;
            // try from queue
            bool popped = queue->try_pop(job);
            // did we manage to pop or steal?
            if (popped)
            {
                executed_job = true;
                if (cpu_load)
                    cpu_load->popped_job();
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
                // TODO thread count for continuations?
                // look for waiting tasks
                if (!waiting.empty())
                {
                    for (std::deque<boost::asynchronous::any_continuation>::iterator it = waiting.begin(); it != waiting.end();)
                    {
                        if ((*it).is_ready())
                        {
                            boost::asynchronous::any_continuation c = *it;
                            it = waiting.erase(it);
                            c();
                        }
                        else
                        {
                            ++it;
                        }
                    }
                }
                // check for shutdown
                boost::asynchronous::any_callable djob;
                popped = private_queue->try_pop(djob);
                if (popped)
                {
                    djob();
                }
                if (cpu_load)
                    cpu_load->loop_done_no_job();
                // nothing for us to do, give up our time slice
                boost::this_thread::yield();
            }
        } // job destroyed (for destruction useful)
        // check if we got an interruption job
        boost::this_thread::interruption_point();
        return executed_job;
    }

    static void run_always(boost::shared_ptr<queue_type> queue,
                           boost::shared_ptr<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> > private_queue,
                           boost::shared_ptr<diag_type> diagnostics,
                           boost::shared_future<boost::thread*> self,
                           boost::weak_ptr<this_type> this_,
                           boost::function<void()> on_done)
    {
        boost::thread* t = self.get();
        boost::asynchronous::detail::single_queue_scheduler_policy<Q>::m_self_thread.reset(new thread_ptr_wrapper(t));
        // thread scheduler => tss
        boost::asynchronous::any_weak_scheduler<job_type> self_as_weak = boost::asynchronous::detail::lockable_weak_scheduler<this_type>(this_);
        boost::asynchronous::get_thread_scheduler<job_type>(self_as_weak,true);

        std::deque<boost::asynchronous::any_continuation>& waiting =
                boost::asynchronous::get_continuations(std::deque<boost::asynchronous::any_continuation>(),true);

        CPULoad cpu_load;
        while(true)
        {
            try
            {
                bool executed_job = run_loop(queue,private_queue,waiting,diagnostics,&cpu_load);
                if (executed_job)
                    on_done();
            }
            catch(boost::asynchronous::detail::shutdown_exception&)
            {
                // we are done
                on_done();
                return;
            }
            catch(boost::thread_interrupted&)
            {
                // task interrupted, no problem, just continue
                on_done();
            }
            catch(std::exception&)
            {
                // TODO, user-defined error
                on_done();
            }
            catch(...)
            {
                // TODO, user-defined error
                on_done();
            }
        }
    }
    static void run_once(boost::shared_ptr<queue_type> queue,
                         boost::shared_ptr<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> > private_queue,
                         boost::shared_ptr<diag_type> diagnostics,
                         boost::shared_future<boost::thread*> self,
                         boost::weak_ptr<this_type> this_,
                         boost::function<void(boost::thread*)> on_done,
                         boost::function<bool()> check_continue)
    {
        boost::thread* t = self.get();
        boost::asynchronous::detail::single_queue_scheduler_policy<Q>::m_self_thread.reset(new thread_ptr_wrapper(t));
        // thread scheduler => tss
        boost::asynchronous::any_weak_scheduler<job_type> self_as_weak = boost::asynchronous::detail::lockable_weak_scheduler<this_type>(this_);
        boost::asynchronous::get_thread_scheduler<job_type>(self_as_weak,true);

        std::deque<boost::asynchronous::any_continuation>& waiting =
                boost::asynchronous::get_continuations(std::deque<boost::asynchronous::any_continuation>(),true);
        // we run once then end
        do
        {
            try
            {
                run_loop(queue,private_queue,waiting,diagnostics,0);
            }
            catch(boost::asynchronous::detail::shutdown_exception&)
            {
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
        internal_data(size_t min_number_of_workers, size_t max_number_of_workers)
        : m_min_number_of_workers(min_number_of_workers <= max_number_of_workers ? min_number_of_workers:max_number_of_workers)
        , m_max_number_of_workers(min_number_of_workers <= max_number_of_workers ? max_number_of_workers:min_number_of_workers)
        , m_current_number_of_workers(0)
        , m_done_threads()
        , m_weak_self()
        , m_group(boost::make_shared<boost::thread_group>())
        {}

        // checks if worker thread should continue
        bool test_worker_loop()
        {
           boost::mutex::scoped_lock lock(m_current_number_of_workers_mutex);
           bool res = (m_current_number_of_workers > m_min_number_of_workers);
           if (res)
               --m_current_number_of_workers;
           return res;
        }
        void basic_thread_not_busy()
        {
            // update number of workers
            boost::mutex::scoped_lock lock(m_current_number_of_workers_mutex);
            --m_current_number_of_workers;
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
            for (std::set<boost::thread*>::iterator it = to_join_threads.begin(); it != to_join_threads.end();++it)
            {
                m_group->remove_thread(*it);
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
        boost::weak_ptr<this_type> m_weak_self;
        boost::shared_ptr<boost::thread_group> m_group;
        // protects m_done_threads
        boost::mutex m_done_threads_mutex;
        // protects m_current_number_of_workers
        mutable boost::mutex m_current_number_of_workers_mutex;
    };
    boost::shared_ptr<internal_data> m_data;

    boost::shared_ptr<diag_type> m_diagnostics;
    std::vector<boost::shared_ptr<
                boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable>>> m_private_queues;
};

}} // boost::asynchronous::scheduler

#endif // BOOST_ASYNCHRONOUS_SCHEDULER_IO_THREADPOOL_SCHEDULER_HPP
