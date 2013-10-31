// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_SCHEDULER_SINGLE_QUEUE_SCHEDULER_POLICY_HPP
#define BOOST_ASYNC_SCHEDULER_SINGLE_QUEUE_SCHEDULER_POLICY_HPP

#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/tss.hpp>

#include <boost/asynchronous/queue/any_queue.hpp>
#include <boost/asynchronous/scheduler/detail/interruptible_job.hpp>
#include <boost/asynchronous/any_scheduler.hpp>

namespace boost { namespace asynchronous { namespace detail
{
template<class Q>
class single_queue_scheduler_policy: 
#ifdef BOOST_ASYNCHRONOUS_NO_TYPE_ERASURE
        public any_shared_scheduler_concept<typename Q::job_type>,
#endif          
        private boost::noncopyable
{
public:
    typedef Q queue_type;
    typedef typename Q::job_type job_type;
    typedef single_queue_scheduler_policy<Q> this_type;
    
#ifndef BOOST_NO_RVALUE_REFERENCES
    void post(typename queue_type::job_type && job, std::size_t prio)
    {
        boost::asynchronous::job_traits<typename queue_type::job_type>::set_posted_time(job);
        m_queue->push(std::forward<typename queue_type::job_type>(job),prio);
    }
    void post(typename queue_type::job_type && job)
    {
        post(std::forward<typename queue_type::job_type>(job),0);
    }
//    void post(boost::asynchronous::any_callable&& job,const std::string& name)
//    {
//        typename boost::asynchronous::job_traits<job_type>::wrapper_type w(std::forward<boost::asynchronous::any_callable>(job));
//        w.set_name(name);
//        post(std::move(w));
//    }
//    void post(boost::asynchronous::any_callable&& job,const std::string& name,std::size_t priority)
//    {
//        typename boost::asynchronous::job_traits<job_type>::wrapper_type w(std::forward<boost::asynchronous::any_callable>(job));
//        w.set_name(name);
//        post(std::move(w),priority);
//    }
    
    boost::asynchronous::any_interruptible interruptible_post(typename queue_type::job_type && job,
                                                          std::size_t prio)
    {
        boost::shared_ptr<boost::asynchronous::detail::interrupt_state>
                state = boost::make_shared<boost::asynchronous::detail::interrupt_state>();
        boost::shared_ptr<boost::promise<boost::thread*> > wpromise = boost::make_shared<boost::promise<boost::thread*> >();
        boost::asynchronous::job_traits<typename queue_type::job_type>::set_posted_time(job);
        boost::asynchronous::interruptible_job<typename queue_type::job_type,this_type>
                ijob(std::forward<typename queue_type::job_type>(job),wpromise,state);

        m_queue->push(std::forward<typename queue_type::job_type>(ijob),prio);

        boost::future<boost::thread*> fu = wpromise->get_future();
        boost::asynchronous::interrupt_helper interruptible(std::move(fu),state);

        return boost::asynchronous::any_interruptible(interruptible);
    }
    boost::asynchronous::any_interruptible interruptible_post(typename queue_type::job_type && job)
    {
        return interruptible_post(std::forward<typename queue_type::job_type>(job),0);
    }
//    boost::asynchronous::any_interruptible interruptible_post(boost::asynchronous::any_callable&& job,const std::string& name)
//    {
//        typename boost::asynchronous::job_traits<job_type>::wrapper_type w(std::forward<boost::asynchronous::any_callable>(job));
//        w.set_name(name);
//        return interruptible_post(std::move(w));
//    }
//    boost::asynchronous::any_interruptible interruptible_post(boost::asynchronous::any_callable&& job,const std::string& name,
//                                                           std::size_t priority)
//    {
//        typename boost::asynchronous::job_traits<job_type>::wrapper_type w(std::forward<boost::asynchronous::any_callable>(job));
//        w.set_name(name);
//        return interruptible_post(std::move(w),priority);
//    }
#else
    void post(typename queue_type::job_type& job, std::size_t prio=0)
    {
        boost::asynchronous::job_traits<typename queue_type::job_type>::set_posted_time(job);
        m_queue->push(job,prio);
    }
//    void post(boost::asynchronous::any_callable&& job,const std::string& name)
//    {
//        typename boost::asynchronous::job_traits<job_type>::wrapper_type w(job);
//        w.set_name(name);
//        post(w);
//    }
//    void post(boost::asynchronous::any_callable&& job,const std::string& name,std::size_t priority)
//    {
//        typename boost::asynchronous::job_traits<job_type>::wrapper_type w(job);
//        w.set_name(name);
//        post(w,priority);
//    }
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

        return boost::asynchronous::any_interruptible(interruptible);
    }
//    boost::asynchronous::any_interruptible interruptible_post(boost::asynchronous::any_callable&& job,const std::string& name)
//    {
//        typename boost::asynchronous::job_traits<job_type>::wrapper_type w(job);
//        w.set_name(name);
//        return interruptible_post(w);
//    }
//    boost::asynchronous::any_interruptible interruptible_post(boost::asynchronous::any_callable&& job,const std::string& name,
//                                                           std::size_t priority)
//    {
//        typename boost::asynchronous::job_traits<job_type>::wrapper_type w(job);
//        w.set_name(name);
//        return interruptible_post(w,priority);
//    }
#endif
    std::vector<boost::asynchronous::any_queue_ptr<job_type> > get_queues()const
    {
        // this scheduler lends its queue for stealing
        std::vector<boost::asynchronous::any_queue_ptr<job_type> > res;
        res.reserve(1);
        res.push_back(m_queue);
        return res;
    }

    static boost::thread_specific_ptr<thread_ptr_wrapper> m_self_thread;
    
protected:

#ifndef BOOST_NO_RVALUE_REFERENCES
    single_queue_scheduler_policy(boost::shared_ptr<queue_type>&& queue)
        : m_queue(std::forward<boost::shared_ptr<queue_type> >(queue))
    {
    }
#endif
#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES
    template<typename... Args>
    single_queue_scheduler_policy(Args... args): m_queue(boost::make_shared<queue_type>(args...))
    {
    }
#endif
    single_queue_scheduler_policy()
    {
        m_queue = boost::make_shared<queue_type>();
    }
    
    boost::shared_ptr<queue_type> m_queue;
};

template<class Q>
boost::thread_specific_ptr<thread_ptr_wrapper>
single_queue_scheduler_policy<Q>::m_self_thread;

}}}
#endif // BOOST_ASYNC_SCHEDULER_SINGLE_QUEUE_SCHEDULER_POLICY_HPP
