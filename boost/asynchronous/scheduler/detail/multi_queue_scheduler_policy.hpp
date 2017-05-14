// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_SCHEDULER_MULTI_QUEUE_SCHEDULER_POLICY_HPP
#define BOOST_ASYNC_SCHEDULER_MULTI_QUEUE_SCHEDULER_POLICY_HPP

#include <vector>
#include <atomic>
#include <numeric>

#include <boost/noncopyable.hpp>
#include <memory>
#include <boost/thread/thread.hpp>
#include <boost/thread/tss.hpp>

#include <boost/asynchronous/queue/any_queue.hpp>
#include <boost/asynchronous/scheduler/detail/interruptible_job.hpp>
#include <boost/asynchronous/any_scheduler.hpp>

namespace boost { namespace asynchronous { namespace detail
{
template<class Q, class FindPosition>
class multi_queue_scheduler_policy: 
#ifndef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
        public any_shared_scheduler_concept<typename Q::job_type>,
#endif  
        private boost::noncopyable, public FindPosition
{
public:
    typedef Q queue_type;
    typedef typename Q::job_type job_type;
    typedef multi_queue_scheduler_policy<Q,FindPosition> this_type;
    
    std::vector<std::size_t> get_queue_size() const
    {
        std::size_t res = 0;
        for (typename std::vector<std::shared_ptr<queue_type> >::const_iterator it = m_queues.begin(); it != m_queues.end();++it)
        {
            auto vec = (*it)->get_queue_size();
            res += std::accumulate(vec.begin(),vec.end(),0,[](std::size_t rhs,std::size_t lhs){return rhs + lhs;});
        }
        std::vector<std::size_t> res_vec;
        res_vec.push_back(res);
        return res_vec;
    }

    std::vector<std::size_t> get_max_queue_size() const
    {
        std::size_t res = 0;
        for (typename std::vector<std::shared_ptr<queue_type> >::const_iterator it = m_queues.begin(); it != m_queues.end();++it)
        {
            auto vec = (*it)->get_max_queue_size();
            res += std::accumulate(vec.begin(),vec.end(),0,[](std::size_t rhs,std::size_t lhs){return std::max(rhs,lhs);});
        }
        std::vector<std::size_t> res_vec;
        res_vec.push_back(res);
        return res_vec;
    }
    void reset_max_queue_size()
    {
        for (typename std::vector<std::shared_ptr<queue_type> >::const_iterator it = m_queues.begin(); it != m_queues.end();++it)
        {
            (*it)->reset_max_queue_size();
        }
    }

    std::vector<boost::asynchronous::any_queue_ptr<job_type> > get_queues()
    {
        // this scheduler lends its queue for stealing
        std::vector<boost::asynchronous::any_queue_ptr<job_type> > res;
        res.reserve(m_queues.size());
        res.insert(res.end(),m_queues.begin(),m_queues.end());
        return res;
    }


    void post(typename queue_type::job_type job, std::size_t prio)
    {
        boost::asynchronous::job_traits<typename queue_type::job_type>::set_posted_time(job);
        if (prio == std::numeric_limits<std::size_t>::max())
        {
            // shutdown jobs have to be sent to all queues
            m_queues[m_next_shutdown_bucket.load()% m_queues.size()]->push(std::move(job),prio);
            ++m_next_shutdown_bucket;
        }
        else
        {
            m_queues[this->find_position(prio,m_queues.size())]->push(std::move(job),prio);
        }
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
        std::shared_ptr<boost::asynchronous::detail::interrupt_state>
                state = std::make_shared<boost::asynchronous::detail::interrupt_state>();
        std::shared_ptr<boost::promise<boost::thread*> > wpromise = std::make_shared<boost::promise<boost::thread*> >();
        boost::asynchronous::job_traits<typename queue_type::job_type>::set_posted_time(job);
        boost::asynchronous::interruptible_job<typename queue_type::job_type,this_type>
                ijob(std::move(job),wpromise,state);
        if (prio == std::numeric_limits<std::size_t>::max())
        {
            // shutdown jobs have to be sent to all queues
            m_queues[m_next_shutdown_bucket.load()% m_queues.size()]->push(std::move(ijob),prio);
            ++m_next_shutdown_bucket;
        }
        else
        {
            m_queues[this->find_position(prio,m_queues.size())]->push(std::move(ijob),prio);
        }

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
    
    static boost::thread_specific_ptr<thread_ptr_wrapper> m_self_thread;
    
protected:

    multi_queue_scheduler_policy(std::shared_ptr<queue_type>&& queues)
        : m_queues(std::forward<std::vector<std::shared_ptr<queue_type> > >(queues))
        , m_next_shutdown_bucket(0)
    {
    }
    template<typename... Args>
    multi_queue_scheduler_policy(size_t number_of_workers,Args... args)
         : m_next_shutdown_bucket(0)
    {
        m_queues.reserve(number_of_workers);
        for (size_t i = 0; i< number_of_workers;++i)
        {
            m_queues.push_back(std::make_shared<queue_type>(args...));
        }
    }
    multi_queue_scheduler_policy(size_t number_of_workers)
        : m_next_shutdown_bucket(0)
    {
        m_queues.reserve(number_of_workers);
        for (size_t i = 0; i< number_of_workers;++i)
        {
            m_queues.push_back(std::make_shared<queue_type>());
        }
    }
    
    std::vector<std::shared_ptr<queue_type> > m_queues;
    std::atomic<size_t> m_next_shutdown_bucket;
};

template<class Q,class FindPosition>
boost::thread_specific_ptr<thread_ptr_wrapper>
multi_queue_scheduler_policy<Q,FindPosition>::m_self_thread;

}}}
#endif // BOOST_ASYNC_SCHEDULER_MULTI_QUEUE_SCHEDULER_POLICY_HPP
