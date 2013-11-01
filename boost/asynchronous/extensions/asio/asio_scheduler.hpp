// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRON_EXTENSIONS_ASIO_SCHEDULER_HPP
#define BOOST_ASYNCHRON_EXTENSIONS_ASIO_SCHEDULER_HPP

#include <vector>
#include <atomic>

#ifndef BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE
#endif
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/tss.hpp>

#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/scheduler/detail/scheduler_helpers.hpp>
#include <boost/asynchronous/job_traits.hpp>
#include <boost/asynchronous/detail/any_joinable.hpp>
#include <boost/asynchronous/queue/any_queue.hpp>
#include <boost/asynchronous/exceptions.hpp>
#include <boost/asynchronous/job_traits.hpp>
#include <boost/asynchronous/queue/find_queue_position.hpp>
#include <boost/asynchronous/scheduler/detail/interruptible_job.hpp>
#include <boost/asynchronous/extensions/asio/tss_asio.hpp>
#include <boost/asynchronous/any_scheduler.hpp>
#include <boost/asynchronous/scheduler/tss_scheduler.hpp>
#include <boost/asynchronous/scheduler/detail/lockable_weak_scheduler.hpp>
#include <boost/asynchronous/scheduler/detail/any_continuation.hpp>
#include <boost/asynchronous/scheduler/tss_scheduler.hpp>
#include <boost/asynchronous/scheduler/cpu_load_policies.hpp>

// partial implementation of any_shared_scheduler_impl using boost::asio
namespace boost { namespace asynchronous
{

template<class FindPosition=boost::asynchronous::default_find_position<boost::asynchronous::sequential_push_policy > ,
         class CPULoad =
#ifndef BOOST_ASYNCHRONOUS_SAVE_CPU_LOAD
         boost::asynchronous::no_cpu_load_saving
#else
         boost::asynchronous::default_save_cpu_load<>
#endif
         >
class asio_scheduler : 
#ifdef BOOST_ASYNCHRONOUS_NO_TYPE_ERASURE
        public any_shared_scheduler_concept<boost::asynchronous::any_callable>,
#endif          
        public FindPosition
{
public:
    typedef boost::asynchronous::any_callable job_type;
    typedef asio_scheduler<FindPosition,CPULoad> this_type;
    
    asio_scheduler(size_t number_of_workers=1,bool immediate=true)
        : m_next_shutdown_bucket(0)
        , m_number_of_workers(number_of_workers)
        , m_immediate(immediate)
    {
    }
    void init(size_t number_of_workers,std::vector<boost::asynchronous::any_queue_ptr<job_type> > const& others,boost::weak_ptr<this_type> weak_self)
    {
        m_works.reserve(number_of_workers);
        m_ioservices.reserve(number_of_workers);
        m_group.reset(new boost::thread_group);
        std::vector<boost::shared_future<void> > all_threads_started;
        all_threads_started.reserve(number_of_workers);
        for (size_t i=0; i<number_of_workers; ++i)
        {
            // create worker
            boost::shared_ptr<boost::asio::io_service> ioservice = boost::make_shared<boost::asio::io_service>();
            m_ioservices.push_back(ioservice);
            m_works.push_back(boost::make_shared<boost::asio::io_service::work>(*ioservice));
        }
        for (size_t i=0; i<number_of_workers; ++i)
        {
            // a worker gets the other io_services, so that he can try to steal
            std::vector<boost::shared_ptr<boost::asio::io_service > > other_ioservices(m_ioservices);
            other_ioservices.erase(other_ioservices.begin()+i);
            // create thread and add to group
            boost::promise<boost::thread*> new_thread_promise;
            boost::shared_future<boost::thread*> fu = new_thread_promise.get_future();
            boost::shared_ptr<boost::promise<void> > p(new boost::promise<void>);
            boost::thread* new_thread =
                    m_group->create_thread(boost::bind(&asio_scheduler::run,m_ioservices[i],fu,p,other_ioservices,others,weak_self));
            all_threads_started.push_back(p->get_future());
            new_thread_promise.set_value(new_thread);

            m_thread_ids.push_back(new_thread->get_id());
        }
        // wait for all threads to be started
        boost::wait_for_all(all_threads_started.begin(),all_threads_started.end());
    }
    void constructor_done(boost::weak_ptr<this_type> weak_self)
    {
        m_weak_self = weak_self;
        if (m_immediate)
            init(m_number_of_workers,std::vector<boost::asynchronous::any_queue_ptr<job_type> >(),m_weak_self);
    }
    ~asio_scheduler()
    {
        for (size_t i = 0; i< m_ioservices.size();++i)
        {
            boost::asynchronous::detail::default_termination_task<typename boost::asynchronous::job_traits<job_type>::diagnostic_type,boost::thread_group> ttask(m_group);
            // this task has to be executed last => lowest prio
#ifndef BOOST_NO_RVALUE_REFERENCES
            job_type job(ttask);
            this->post(std::move(job),std::numeric_limits<std::size_t>::max());
#else
            this->post(job_type(ttask),std::numeric_limits<std::size_t>::max());
#endif
        }
        m_works.clear();
    }
    
    boost::asynchronous::any_joinable get_worker()const
    {
        return boost::asynchronous::any_joinable (boost::asynchronous::detail::worker_wrap<boost::thread_group>(m_group));
    }
    
    std::vector<boost::thread::id> thread_ids()const
    {
        return m_thread_ids;
    }
    
    std::map<std::string,
             std::list<typename boost::asynchronous::job_traits<job_type>::diagnostic_item_type > >
    get_diagnostics(std::size_t =0)const
    {
        // TODO if possible
        return std::map<std::string,
                std::list<typename boost::asynchronous::job_traits<job_type>::diagnostic_item_type > >();
    }
    void set_steal_from_queues(std::vector<boost::asynchronous::any_queue_ptr<job_type> > const& others)
    {
        // this scheduler steals if offered
        init(m_number_of_workers,others,m_weak_self);
    }

#ifndef BOOST_NO_RVALUE_REFERENCES    
    void post(job_type && job, std::size_t prio)
    {
        boost::asynchronous::job_traits<job_type>::set_posted_time(job);
        if (prio == std::numeric_limits<std::size_t>::max())
        {
            std::size_t pos = m_next_shutdown_bucket.load()% m_ioservices.size();
            m_ioservices[pos]->post(std::forward<job_type>(job));
            ++m_next_shutdown_bucket;
        }
        else
        {
            std::size_t pos = this->find_position(prio,m_ioservices.size());
            m_ioservices[pos]->post(std::forward<job_type>(job));
        }
    }
    void post(job_type && job)
    {
        post(std::forward<job_type>(job),0);
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
    boost::asynchronous::any_interruptible interruptible_post(job_type && job,
                                                          std::size_t prio)
    {
        boost::shared_ptr<boost::asynchronous::detail::interrupt_state>
                state = boost::make_shared<boost::asynchronous::detail::interrupt_state>();
        // write to tss in case the task requires it
        boost::asynchronous::get_interrupt_state<>(state,true);

        boost::shared_ptr<boost::promise<boost::thread*> > wpromise = boost::make_shared<boost::promise<boost::thread*> >();
        boost::asynchronous::job_traits<job_type>::set_posted_time(job);
        boost::asynchronous::interruptible_job<job_type,this_type>
                ijob(std::forward<job_type>(job),wpromise,state);
        
        boost::asynchronous::job_traits<job_type>::set_posted_time(job);
        if (prio == std::numeric_limits<std::size_t>::max())
        {
            std::size_t pos = m_next_shutdown_bucket.load()% m_ioservices.size();
            m_ioservices[pos]->post(std::forward<job_type>(ijob));
            ++m_next_shutdown_bucket;
        }
        else
        {
            std::size_t pos = this->find_position(prio,m_ioservices.size());
            m_ioservices[pos]->post(std::forward<job_type>(ijob));
        }
        
        boost::future<boost::thread*> fu = wpromise->get_future();
        boost::asynchronous::interrupt_helper interruptible(std::move(fu),state);

        return boost::asynchronous::any_interruptible(interruptible);
    }
    boost::asynchronous::any_interruptible interruptible_post(job_type && job)
    {
        return interruptible_post(std::forward<job_type>(job),0);
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
    void post(job_type& job, std::size_t prio=0)
    {
        boost::asynchronous::job_traits<job_type>::set_posted_time(job);
        if (prio == std::numeric_limits<std::size_t>::max())
        {
            std::size_t pos = m_next_shutdown_bucket.load()% m_ioservices.size();
            m_ioservices[pos]->post(job);
            ++m_next_shutdown_bucket;
        }
        else
        {
            std::size_t pos = this->find_position(prio,m_ioservices.size());
            m_ioservices[pos]->post(job);
        }
    }
    void post(boost::asynchronous::any_callable&& job,const std::string& name)
    {
        typename boost::asynchronous::job_traits<job_type>::wrapper_type w(job);
        w.set_name(name);
        post(w);
    }
    void post(boost::asynchronous::any_callable&& job,const std::string& name,std::size_t priority)
    {
        typename boost::asynchronous::job_traits<job_type>::wrapper_type w(job);
        w.set_name(name);
        post(w,priority);
    }  
    boost::asynchronous::any_interruptible interruptible_post(job_type& job, std::size_t prio=0)
    {
        boost::shared_ptr<boost::asynchronous::detail::interrupt_state>
                state = boost::make_shared<boost::asynchronous::detail::interrupt_state>();
        boost::shared_ptr<boost::promise<boost::thread*> > wpromise = boost::make_shared<boost::promise<boost::thread*> >();
        boost::asynchronous::job_traits<job_type>::set_posted_time(job);
        boost::asynchronous::interruptible_job<job_type,this_type>
                ijob(job,wpromise,state);
        
        boost::asynchronous::job_traits<job_type>::set_posted_time(ijob);
        if (prio == std::numeric_limits<std::size_t>::max())
        {
            std::size_t pos = m_next_shutdown_bucket.load()% m_ioservices.size();
            m_ioservices[pos]->post(ijob);
            ++m_next_shutdown_bucket;
        }
        else
        {
            std::size_t pos = this->find_position(prio,m_ioservices.size());
            m_ioservices[pos]->post(ijob);
        }
        
        boost::shared_future<boost::thread*> fu = wpromise->get_future();
        boost::asynchronous::interrupt_helper interruptible(fu,state);

        return boost::asynchronous::any_interruptible(interruptible);
    }
    boost::asynchronous::any_interruptible interruptible_post(boost::asynchronous::any_callable&& job,const std::string& name)
    {
        typename boost::asynchronous::job_traits<job_type>::wrapper_type w(job);
        w.set_name(name);
        return interruptible_post(w);
    }
    boost::asynchronous::any_interruptible interruptible_post(boost::asynchronous::any_callable&& job,const std::string& name,
                                                           std::size_t priority)
    {
        typename boost::asynchronous::job_traits<job_type>::wrapper_type w(job);
        w.set_name(name);
        return interruptible_post(w,priority);
    }  
#endif
    std::vector<boost::asynchronous::any_queue_ptr<job_type> > get_queues()const
    {
        // this scheduler doesn't give any queues for stealing
        return std::vector<boost::asynchronous::any_queue_ptr<job_type> >();
    }    
    // TLS
    static boost::thread_specific_ptr<thread_ptr_wrapper> m_self_thread;
    
private:
    static void run(boost::shared_ptr<boost::asio::io_service> ioservice,boost::shared_future<boost::thread*> self,
                    boost::shared_ptr<boost::promise<void> > started,
                    std::vector<boost::shared_ptr<boost::asio::io_service > > other_ioservices,
                    std::vector<boost::asynchronous::any_queue_ptr<job_type> > other_queues,
                    boost::weak_ptr<this_type> this_)
    {
        boost::thread* t = self.get();
        m_self_thread.reset(new thread_ptr_wrapper(t));
        started->set_value();
        get_io_service<>(ioservice.get());
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
                bool popped = (ioservice->poll_one() != 0);
                for (std::size_t i = 0; i< other_ioservices.size();++i)
                {
                    popped = (other_ioservices[i]->poll_one() != 0);
                    if (popped)
                    {
                        cpu_load.popped_job();
                        break;
                    }
                }
                bool stolen=false;
                if (!popped)
                {
                    job_type job;
                    // ok we have nothing to do, maybe we can steal some work from other pools?
                    for (std::size_t i=0; i< other_queues.size(); ++i)
                    {
                        stolen = (*other_queues[i]).try_steal(job);
                        if (stolen)
                            break;
                    }
                    if (stolen)
                    {
                        cpu_load.popped_job();
                        // execute stolen job
                        job();
                    }
                }
                if (!popped && !stolen)
                {
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
                    cpu_load.loop_done_no_job();
                    // nothing for us to do, give up our time slice
                    boost::this_thread::yield();
                }
            }
            catch(boost::asynchronous::detail::shutdown_exception&)
            {
                // we are done
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
    
    
    std::atomic<size_t> m_next_shutdown_bucket;
    size_t m_number_of_workers;
    std::vector<boost::shared_ptr<boost::asio::io_service::work> > m_works;
    std::vector<boost::shared_ptr<boost::asio::io_service > > m_ioservices;
    boost::shared_ptr<boost::thread_group> m_group;
    std::vector<boost::thread::id> m_thread_ids;
    boost::weak_ptr<this_type> m_weak_self;
    bool m_immediate;
};

template<class FindPosition,class CPULoad>
boost::thread_specific_ptr<thread_ptr_wrapper>
asio_scheduler<FindPosition,CPULoad>::m_self_thread;

}}
#endif // BOOST_ASYNCHRON_EXTENSIONS_ASIO_SCHEDULER_HPP
