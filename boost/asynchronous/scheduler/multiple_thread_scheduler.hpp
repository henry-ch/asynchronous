// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_SCHEDULER_MULTIPLE_THREAD_SCHEDULER_HPP
#define BOOST_ASYNCHRONOUS_SCHEDULER_MULTIPLE_THREAD_SCHEDULER_HPP

#include <utility>
#include <vector>
#include <cstddef>
#include <tuple>
#include <atomic>
#include <numeric>

#ifndef BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE
#endif

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/tss.hpp>

#include <boost/asynchronous/queue/any_queue.hpp>
#include <boost/asynchronous/scheduler/detail/interruptible_job.hpp>
#include <boost/asynchronous/scheduler/detail/scheduler_helpers.hpp>
#include <boost/asynchronous/scheduler/detail/exceptions.hpp>
#include <boost/asynchronous/detail/any_interruptible.hpp>
#include <boost/asynchronous/diagnostics/default_loggable_job.hpp>
#include <boost/asynchronous/job_traits.hpp>
#include <boost/asynchronous/scheduler/detail/job_diagnostic_closer.hpp>
#include <boost/asynchronous/detail/any_joinable.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler/tss_scheduler.hpp>
#include <boost/asynchronous/scheduler/detail/lockable_weak_scheduler.hpp>
#include <boost/asynchronous/scheduler/detail/any_continuation.hpp>
#include <boost/asynchronous/scheduler/cpu_load_policies.hpp>
#include <boost/asynchronous/any_scheduler.hpp>

namespace boost { namespace asynchronous
{

template<class Q, class CPULoad =
#ifdef BOOST_ASYNCHRONOUS_NO_SAVING_CPU_LOAD
         boost::asynchronous::no_cpu_load_saving
#else
         boost::asynchronous::default_save_cpu_load<>
#endif
         >
class multiple_thread_scheduler:
#ifndef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
                public any_shared_scheduler_concept<typename Q::job_type>
#endif
{
public:
    typedef boost::asynchronous::multiple_thread_scheduler<Q,CPULoad> this_type;
    typedef Q queue_type;
    typedef typename Q::job_type job_type;
    typedef typename boost::asynchronous::job_traits<typename Q::job_type>::diagnostic_table_type diag_type;
    struct job_queues
    {
        job_queues(bool f, boost::shared_ptr<queue_type> q)
            : m_taken(f), m_queues(q)
        {}

        job_queues(job_queues&& rhs)
            : m_taken(rhs.m_taken.load())
            , m_queues(std::move(rhs.m_queues))
        {}
        std::atomic_bool m_taken;
        boost::shared_ptr<queue_type> m_queues;
    };

    template<typename... Args>
    multiple_thread_scheduler(size_t number_of_workers, size_t max_number_of_clients, Args... args)
        : m_max_number_of_clients(max_number_of_clients)
        , m_number_of_workers(number_of_workers)
    {
        m_private_queues.reserve(number_of_workers);
        m_queues = boost::make_shared<std::vector<job_queues>>();
        for (std::size_t i = 0; i < max_number_of_clients ; ++i)
        {
            m_queues->emplace_back(false,boost::make_shared<queue_type>(args...));
        }
        for (size_t i = 0; i< number_of_workers;++i)
        {
            m_private_queues.push_back(
                        boost::make_shared<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> >());
        }
    }
    template<typename... Args>
    multiple_thread_scheduler(size_t number_of_workers, size_t max_number_of_clients, std::string const& name, Args... args)
        : m_max_number_of_clients(max_number_of_clients)
        , m_number_of_workers(number_of_workers)
        , m_name(name)
    {
        m_private_queues.reserve(number_of_workers);
        m_queues = boost::make_shared<std::vector<job_queues>>();
        for (std::size_t i = 0; i < max_number_of_clients ; ++i)
        {
            m_queues->emplace_back(false,boost::make_shared<queue_type>(args...));
        }
        for (size_t i = 0; i< number_of_workers;++i)
        {
            m_private_queues.push_back(
                        boost::make_shared<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> >());
        }
        set_name(name);
    }
    void constructor_done(boost::weak_ptr<this_type> weak_self)
    {
        m_diagnostics = boost::make_shared<diag_type>(m_number_of_workers);
        m_thread_ids.reserve(m_number_of_workers);
        m_group.reset(new boost::thread_group);
        for (size_t i = 0; i< m_number_of_workers;++i)
        {
            boost::promise<boost::thread*> new_thread_promise;
            boost::shared_future<boost::thread*> fu = new_thread_promise.get_future();
            boost::thread* new_thread =
                    m_group->create_thread(boost::bind(&multiple_thread_scheduler::run,this->m_queues,
                                                       m_private_queues[i],m_diagnostics,fu,weak_self,i));
            new_thread_promise.set_value(new_thread);
            m_thread_ids.push_back(new_thread->get_id());
        }
    }

    ~multiple_thread_scheduler()
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
            // this task has to be executed lat => lowest prio
            boost::asynchronous::any_callable job(std::move(ttask));
            m_private_queues[i]->push(std::move(job),std::numeric_limits<std::size_t>::max());
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
    void processor_bind(unsigned int p)
    {
        for (size_t i = 0; i< m_number_of_workers;++i)
        {
            boost::asynchronous::detail::processor_bind_task task(p+i);
            boost::asynchronous::any_callable job(std::move(task));
            m_private_queues[i]->push(std::move(job),std::numeric_limits<std::size_t>::max());
        }
    }
    void register_diagnostics_functor(std::function<void(boost::asynchronous::scheduler_diagnostics)> fct,
                                      boost::asynchronous::register_diagnostics_type =
                                                    boost::asynchronous::register_diagnostics_type())
    {
        m_diagnostics_fct = fct;
    }

    std::vector<std::size_t> get_queue_size() const
    {
        std::size_t res = 0;
        for (auto const& q : (*m_queues))
        {
            auto vec = q.m_queues->get_queue_size();
            res += std::accumulate(vec.begin(),vec.end(),0,[](std::size_t rhs,std::size_t lhs){return rhs + lhs;});
        }
        std::vector<std::size_t> res_vec;
        res_vec.push_back(res);
        return res_vec;
    }
    std::vector<std::size_t> get_max_queue_size() const
    {
        std::size_t res = 0;
        for (auto const& q : (*m_queues))
        {
            auto vec = q.m_queues->get_max_queue_size();
            res += std::accumulate(vec.begin(),vec.end(),0,[](std::size_t rhs,std::size_t lhs){return std::max(rhs,lhs);});
        }
        std::vector<std::size_t> res_vec;
        res_vec.push_back(res);
        return res_vec;
    }

    void reset_max_queue_size()
    {
        for (auto const& q : (*m_queues))
        {
            q.m_queues->reset_max_queue_size();
        }
    }

    std::vector<boost::asynchronous::any_queue_ptr<job_type> > get_queues()
    {
        // this scheduler doesn't give any queues for stealing
        return std::vector<boost::asynchronous::any_queue_ptr<job_type> >();
    }
    void post(typename queue_type::job_type job, std::size_t prio)
    {
        std::size_t servant_index = (prio / 100000);
        boost::asynchronous::job_traits<typename queue_type::job_type>::set_posted_time(job);
        if ((servant_index >= m_queues->size()))
        {
            // this queue is not existing
            assert(false);
        }
        (*m_queues)[servant_index].m_queues->push(std::move(job),prio % 100000);
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
    boost::asynchronous::any_interruptible interruptible_post(typename queue_type::job_type job,std::size_t prio)
    {
        boost::shared_ptr<boost::asynchronous::detail::interrupt_state>
                state = boost::make_shared<boost::asynchronous::detail::interrupt_state>();
        boost::shared_ptr<boost::promise<boost::thread*> > wpromise = boost::make_shared<boost::promise<boost::thread*> >();
        boost::asynchronous::job_traits<typename queue_type::job_type>::set_posted_time(job);
        boost::asynchronous::interruptible_job<typename queue_type::job_type,this_type>
                ijob(std::move(job),wpromise,state);
        if (prio >= m_queues->size())
        {
            // this queue is not existing
            assert(false);
        }
        (*m_queues)[prio].m_queues->push(std::move(ijob),prio);

        boost::future<boost::thread*> fu = wpromise->get_future();
        boost::asynchronous::interrupt_helper interruptible(std::move(fu),state);

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
    // try to execute a job, return true
    static bool execute_one_job(std::vector<job_queues>* queues,size_t index,size_t& current_servant,CPULoad& cpu_load,boost::shared_ptr<diag_type> diagnostics,
                                std::list<boost::asynchronous::any_continuation>& waiting)
    {
        bool popped = false;
        // get a job
        typename Q::job_type job;
        std::size_t taken_index = 0;
        try
        {
            auto s = queues->size();
            for (std::size_t i = 0 ; i < s ; ++i)
            {
                current_servant = (current_servant+1)%s;
                // used slot, check if we can take it
                bool expected = false;
                if ((*queues)[current_servant].m_taken.compare_exchange_strong(expected,true))
                {
                    // ok we have it
                    popped = ((*queues)[current_servant].m_queues)->try_pop(job);
                    if (popped)
                    {
                        taken_index = current_servant;
                    }
                    else
                    {
                        (*queues)[current_servant].m_taken = false;
                    }
                }
                if (popped)
                    break;
            }
            // did we manage to pop or steal?
            if (popped)
            {
                cpu_load.popped_job();
                // log time
                boost::asynchronous::job_traits<typename Q::job_type>::set_started_time(job);
                // log current
                boost::asynchronous::job_traits<typename Q::job_type>::add_current_diagnostic(index,job,diagnostics.get());
                // execute job
                job();
                (*queues)[taken_index].m_taken = false;
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
            if (popped)
            {
                (*queues)[taken_index].m_taken = false;
            }
        }
        catch(std::exception&)
        {
            if (popped)
            {
                (*queues)[taken_index].m_taken = false;
                boost::asynchronous::job_traits<typename Q::job_type>::set_failed(job);
                boost::asynchronous::job_traits<typename Q::job_type>::set_finished_time(job);
                boost::asynchronous::job_traits<typename Q::job_type>::add_diagnostic(job,diagnostics.get());
                boost::asynchronous::job_traits<typename Q::job_type>::reset_current_diagnostic(index,diagnostics.get());
            }
        }
        return popped;
    }

    static void run(boost::shared_ptr<std::vector<job_queues>> queues,
                    boost::shared_ptr<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> > const& private_queue,
                    boost::shared_ptr<diag_type> diagnostics,
                    boost::shared_future<boost::thread*> self,
                    boost::weak_ptr<this_type> this_,
                    size_t index)
    {
        boost::thread* t = self.get();
        this_type::m_self_thread.reset(new thread_ptr_wrapper(t));
        // thread scheduler => tss
        boost::asynchronous::any_weak_scheduler<job_type> self_as_weak = boost::asynchronous::detail::lockable_weak_scheduler<this_type>(this_);
        boost::asynchronous::get_thread_scheduler<job_type>(self_as_weak,true);

        std::list<boost::asynchronous::any_continuation>& waiting =
                boost::asynchronous::get_continuations(std::list<boost::asynchronous::any_continuation>(),true);

        CPULoad cpu_load;
        size_t current_servant=0;
        while(true)
        {
            try
            {

                {
                    bool popped = execute_one_job(queues.get(),index,current_servant,cpu_load,diagnostics,waiting);
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
                while(execute_one_job(queues.get(),index,current_servant,cpu_load,diagnostics,waiting));
                delete this_type::m_self_thread.release();
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

    static boost::thread_specific_ptr<thread_ptr_wrapper> m_self_thread;

private:
    boost::shared_ptr<std::vector<job_queues>> m_queues;
    size_t m_max_number_of_clients;
    boost::shared_ptr<boost::thread_group> m_group;
    std::vector<boost::thread::id> m_thread_ids;
    boost::shared_ptr<diag_type> m_diagnostics;
    std::vector<boost::shared_ptr<
                boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable>>> m_private_queues;
    size_t m_number_of_workers;
    std::function<void(boost::asynchronous::scheduler_diagnostics)> m_diagnostics_fct;
    const std::string m_name;
};

template<class Q,class CPULoad>
boost::thread_specific_ptr<thread_ptr_wrapper>
multiple_thread_scheduler<Q,CPULoad>::m_self_thread;

}} // boost::asynchronous


#endif // BOOST_ASYNCHRONOUS_SCHEDULER_MULTIPLE_THREAD_SCHEDULER_HPP
