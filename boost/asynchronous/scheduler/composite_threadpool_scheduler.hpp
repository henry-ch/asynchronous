// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_SCHEDULER_COMPOSITE_THREADPOOL_SCHEDULER_HPP
#define BOOST_ASYNC_SCHEDULER_COMPOSITE_THREADPOOL_SCHEDULER_HPP

#include <utility>
#include <vector>
#include <cstddef>
#include <atomic>

#ifndef BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE
#endif
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/tss.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <boost/asynchronous/scheduler/detail/exceptions.hpp>
#include <boost/asynchronous/detail/any_interruptible.hpp>
#include <boost/asynchronous/scheduler/detail/interruptible_job.hpp>
#include <boost/asynchronous/diagnostics/default_loggable_job.hpp>
#include <boost/asynchronous/job_traits.hpp>
#include <boost/asynchronous/detail/any_joinable.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/any_shared_scheduler_proxy.hpp>
#include <boost/asynchronous/queue/find_queue_position.hpp>
#include <boost/asynchronous/queue/any_queue.hpp>
#include <boost/asynchronous/queue/queue_converter.hpp>
#include <boost/asynchronous/any_scheduler.hpp>

namespace boost { namespace asynchronous
{
//TODO boost.parameter
template<class Job = boost::asynchronous::any_callable, 
         class FindPosition=boost::asynchronous::default_find_position< >,
         class Clock = boost::chrono::high_resolution_clock >
class composite_threadpool_scheduler: 
#ifdef BOOST_ASYNCHRONOUS_NO_TYPE_ERASURE
        public any_shared_scheduler_proxy_concept<Job>, public internal_scheduler_aspect_concept<Job>,
#endif        
        public FindPosition, public boost::enable_shared_from_this<composite_threadpool_scheduler<Job,FindPosition,Clock> >
{
public:
    typedef Job job_type;
    typedef composite_threadpool_scheduler<Job,FindPosition,Clock> this_type;

    composite_threadpool_scheduler(): FindPosition()
    {
        // just for default-init, use only if you are going to reset this object
    }

#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES
    template<typename... Args>
    composite_threadpool_scheduler(Args... args): FindPosition()
    {
        // we must get at least one scheduler
        BOOST_STATIC_ASSERT(sizeof...(args)>0);
        std::vector<std::vector<boost::asynchronous::any_queue_ptr<job_type> > > stealing_queues;
        ctor_queue_helper(m_subpools,stealing_queues,args...);
        ctor_pool_helper(stealing_queues,m_subpools);
    }

#endif

    ~composite_threadpool_scheduler()
    {
    }
    std::size_t get_queue_size() const
    {
        std::size_t res = 0;
        for (typename std::vector<subpool_type>::const_iterator it = m_subpools.begin(); it != m_subpools.end();++it)
        {
            res += (*it).get_queue_size();
        }
        return res;
    }
#ifndef BOOST_NO_RVALUE_REFERENCES
    void post(job_type job) const
    {
        post(std::move(job),0);
    }
    void post(job_type job,std::size_t priority) const
    {
        (m_subpools[this->find_position(priority,m_subpools.size())]).post(std::move(job),priority);
    }
    boost::asynchronous::any_interruptible interruptible_post(job_type job) const
    {
        return (m_subpools[this->find_position(0,m_subpools.size())]).interruptible_post(std::move(job));
    }
    boost::asynchronous::any_interruptible interruptible_post(job_type job,std::size_t priority) const
    {
        return (m_subpools[this->find_position(priority,m_subpools.size())]).interruptible_post(std::move(job));
    }
#else
//TODO
#endif
    bool is_valid()const
    {
        for (typename std::vector<subpool_type>::const_iterator it = m_subpools.begin(); it != m_subpools.end();++it)
        {
            if( !(*it).is_valid())
            {
                return false;
            }
        }
        return true;
    }
    
    std::vector<boost::thread::id> thread_ids()const
    {
        std::vector<boost::thread::id> ids;
        for (typename std::vector<subpool_type>::const_iterator it = m_subpools.begin(); it != m_subpools.end();++it)
        {
            std::vector<boost::thread::id> pids=(*it).thread_ids();
            ids.insert(ids.end(),pids.begin(),pids.end());
        }
        return ids;
    }
    void set_name(std::string const& name)
    {
        for (typename std::vector<subpool_type>::iterator it = m_subpools.begin(); it != m_subpools.end();++it)
        {
            (*it).set_name(name);
        }
    }
    std::map<std::string,
             std::list<typename boost::asynchronous::job_traits<job_type>::diagnostic_item_type > >
    get_diagnostics(std::size_t pos=0)const
    {
        return (m_subpools[this->find_position(pos,m_subpools.size())]).get_diagnostics(pos);
    }
    void clear_diagnostics()
    {
        for (typename std::vector<subpool_type>::iterator it = m_subpools.begin(); it != m_subpools.end();++it)
        {
            (*it).clear_diagnostics();
        }
    }
    
    boost::asynchronous::any_weak_scheduler<job_type> get_weak_scheduler() const
    {
        composite_lockable_weak_scheduler w(m_subpools);
        return boost::asynchronous::any_weak_scheduler<job_type>(w);
    }
    std::vector<boost::asynchronous::any_queue_ptr<job_type> > get_queues()
    {
        std::vector<boost::asynchronous::any_queue_ptr<job_type> > res;
        for (typename std::vector<subpool_type>::iterator it = m_subpools.begin(); it != m_subpools.end();++it)
        {
            std::vector<boost::asynchronous::any_queue_ptr<job_type> > queues = (*(*it).get_internal_scheduler_aspect()).get_queues();
            res.insert(res.end(),queues.begin(),queues.end());
        }
        return res;
    }
    void set_steal_from_queues(std::vector<boost::asynchronous::any_queue_ptr<job_type> > const& )
    {
        // composite of composite is not supported
    }
    boost::asynchronous::internal_scheduler_aspect<job_type> get_internal_scheduler_aspect()
    {
        boost::asynchronous::internal_scheduler_aspect<job_type> a(this->shared_from_this());
        return a;
    }
    boost::asynchronous::any_shared_scheduler_proxy<job_type,Clock> get_scheduler(std::size_t index)const
    {
        return m_subpools.at(index-1);
    }
private:
    typedef boost::asynchronous::any_shared_scheduler_proxy<Job,Clock> subpool_type;
    std::vector<subpool_type> m_subpools;
    
    template <class T,class S>
    typename ::boost::enable_if< boost::is_same<job_type, typename S::job_type>,void >::type
    add_scheduler_helper(T& t,S& s)
    {
        t.push_back(s);
    }
    template <class T,class S>
    typename ::boost::disable_if< boost::is_same<job_type, typename S::job_type>,void >::type
    add_scheduler_helper(T&,S&)
    {
    }
    template <class T,class S>
    typename ::boost::enable_if< boost::is_same<job_type, typename S::job_type>,void >::type
    add_queue_helper(T& t,S& s)
    {
        std::vector<boost::asynchronous::any_queue_ptr<job_type> > q = (*(s).get_internal_scheduler_aspect()).get_queues();
        t.push_back(q);
    }
    template <class T,class S>
    typename ::boost::disable_if< boost::is_same<job_type, typename S::job_type>,void >::type
    add_queue_helper(T& t,S& s)
    {
        std::vector<boost::asynchronous::any_queue_ptr<typename S::job_type> > q = (*(s).get_internal_scheduler_aspect()).get_queues();
        std::vector<boost::asynchronous::any_queue_ptr<job_type> > converted;
        for (auto aqueue : q)
        {
            converted.push_back(boost::make_shared<boost::asynchronous::queue_converter<job_type,typename S::job_type>>(aqueue));
        }
        t.push_back(converted);
    }

    template <typename T,typename Last>
    void ctor_queue_helper(T& t, std::vector<std::vector<boost::asynchronous::any_queue_ptr<job_type> > >& queues, Last& l)
    {
        add_scheduler_helper(t,l);
        add_queue_helper(queues,l);
    }

    template <typename T,typename... Tail, typename Front>
    void ctor_queue_helper(T& t,std::vector<std::vector<boost::asynchronous::any_queue_ptr<job_type> > >& queues, Front& front,Tail&... tail)
    {
        add_scheduler_helper(t,front);
        add_queue_helper(queues,front);
        ctor_queue_helper(t,queues,tail...);
    }
    void ctor_pool_helper(std::vector<std::vector<boost::asynchronous::any_queue_ptr<job_type> > > const& queues,std::vector<subpool_type>& subs)
    {   
        for (std::size_t i=0; i< subs.size(); ++i)
        {
            std::vector<boost::asynchronous::any_queue_ptr<job_type> > steal_from;
            for (std::size_t j=0; j< queues.size(); ++j)
            {
                if (i!=j)
                {                    
                    steal_from.insert(steal_from.end(),queues[j].begin(),queues[j].end());
                }
            }
            (*(subs[i]).get_internal_scheduler_aspect()).set_steal_from_queues(steal_from);
        }
    }
    // shared scheduler for use in the servant context
    // implements any_shared_scheduler_concept
    struct lockable_shared_scheduler : 
#ifdef BOOST_ASYNCHRONOUS_NO_TYPE_ERASURE
            public any_shared_scheduler_concept<job_type>,
#endif       
            public FindPosition
    {
        typedef typename this_type::job_type job_type;

        lockable_shared_scheduler(std::vector<boost::asynchronous::any_shared_scheduler<job_type> >&& schedulers)
            : m_schedulers(std::forward<std::vector<boost::asynchronous::any_shared_scheduler<job_type> > >(schedulers)){}
#ifndef BOOST_NO_RVALUE_REFERENCES
        void post(job_type&& job)
        {
            post(std::forward<job_type>(job),0);
        }
        void post(job_type&& job,std::size_t priority)
        {
            ((m_schedulers[this->find_position(priority,m_schedulers.size())])).post(std::forward<job_type>(job));
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
        boost::asynchronous::any_interruptible interruptible_post(job_type&& job)
        {
            return interruptible_post(std::forward<job_type>(job),0);
        }
        boost::asynchronous::any_interruptible interruptible_post(job_type&& job,std::size_t priority)
        {
            return (m_schedulers[this->find_position(priority,m_schedulers.size())]).interruptible_post(std::forward<job_type>(job));
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
//TODO
#endif
        std::vector<boost::thread::id> thread_ids()const
        {
            std::vector<boost::thread::id> ids;
            for (typename std::vector<boost::asynchronous::any_shared_scheduler<job_type> >::const_iterator it = m_schedulers.begin(); it != m_schedulers.end();++it)
            {
                std::vector<boost::thread::id> pids= (*it).thread_ids();
                ids.insert(ids.end(),pids.begin(),pids.end());
            }
            return ids;
        }
        bool is_valid()const
        {
            for (typename std::vector<boost::asynchronous::any_shared_scheduler<job_type> >::const_iterator it = m_schedulers.begin(); it != m_schedulers.end();++it)
            {
                if( !(*(*it)).is_valid())
                {
                    return false;
                }
            }
            return true;
        }
        std::size_t get_queue_size() const
        {
            std::size_t res = 0;
            for (typename std::vector<boost::asynchronous::any_shared_scheduler<job_type> >::const_iterator it = m_schedulers.begin(); it != m_schedulers.end();++it)
            {
                res += (*it).get_queue_size();
            }
            return res;
        }
        std::map<std::string,
                 std::list<typename boost::asynchronous::job_traits<job_type>::diagnostic_item_type > >
        get_diagnostics(std::size_t pos=0)const
        {
            return (m_schedulers[this->find_position(pos,m_schedulers.size())]).get_diagnostics(pos);
        }
        void clear_diagnostics()
        {
            for (typename std::vector<boost::asynchronous::any_shared_scheduler<job_type> >::iterator it = m_schedulers.begin(); it != m_schedulers.end();++it)
            {
                (*it).clear_diagnostics();
            }
        }
    private:
        std::vector<boost::asynchronous::any_shared_scheduler<job_type> > m_schedulers;
    };  
    // weak scheduler for use in the servant context
    // implements any_weak_scheduler_concept
    struct composite_lockable_weak_scheduler
    {
        composite_lockable_weak_scheduler(std::vector<subpool_type> const& schedulers)
        {
            m_schedulers.reserve(schedulers.size());
            for (typename std::vector<subpool_type>::const_iterator it = schedulers.begin(); it != schedulers.end(); ++it)
            {
                m_schedulers.push_back((*it).get_weak_scheduler());
            }
        }
        any_shared_scheduler<job_type> lock()const
        {
            std::vector<boost::asynchronous::any_shared_scheduler<job_type> > locked_schedulers;
            locked_schedulers.reserve(m_schedulers.size());
            for (typename std::vector<boost::asynchronous::any_weak_scheduler<job_type> >::const_iterator it = m_schedulers.begin(); 
                 it != m_schedulers.end(); ++it)
            {
                locked_schedulers.push_back((*it).lock());
            }
            boost::shared_ptr<lockable_shared_scheduler> s = boost::make_shared<lockable_shared_scheduler>(std::move(locked_schedulers));
            any_shared_scheduler_ptr<job_type> pscheduler(s);
            return any_shared_scheduler<job_type>(pscheduler);
        }
    private:
        std::vector<boost::asynchronous::any_weak_scheduler<job_type> > m_schedulers;
    };
};
template<class Job, class FindPosition,class Clock>
boost::asynchronous::any_shared_scheduler_proxy<Job>
create_shared_scheduler_proxy(composite_threadpool_scheduler<Job,FindPosition,Clock>* scheduler)
{
#ifdef BOOST_ASYNCHRONOUS_NO_TYPE_ERASURE
    boost::asynchronous::any_shared_scheduler_proxy<Job> pcomposite(scheduler);
#else
    boost::shared_ptr<boost::asynchronous::composite_threadpool_scheduler<Job> > sp(scheduler);
    boost::asynchronous::any_shared_scheduler_proxy_ptr<Job> pcomposite = sp;
#endif
    boost::asynchronous::any_shared_scheduler_proxy<Job> composite(pcomposite);
    return composite;
}
}}

#endif // BOOST_ASYNC_SCHEDULER_COMPOSITE_THREADPOOL_SCHEDULER_HPP
