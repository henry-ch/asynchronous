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
#include <memory>
#include <future>

#include <boost/thread/thread.hpp>
#include <boost/thread/tss.hpp>

#include <boost/asynchronous/queue/any_queue.hpp>
#include <boost/asynchronous/scheduler/detail/interruptible_job.hpp>
#include <boost/asynchronous/any_scheduler.hpp>

namespace boost { namespace asynchronous { namespace detail
{
template<class Q>
class single_queue_scheduler_policy: 
#ifndef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
        public any_shared_scheduler_concept<typename Q::job_type>
#endif          
{
public:
    typedef Q queue_type;
    typedef typename Q::job_type job_type;
    typedef single_queue_scheduler_policy<Q> this_type;
    
    single_queue_scheduler_policy(const single_queue_scheduler_policy&) = delete;
    single_queue_scheduler_policy& operator=(const single_queue_scheduler_policy&) = delete;
    std::vector<std::size_t> get_queue_size() const override
    {
        return m_queue->get_queue_size();
    }
    std::vector<std::size_t> get_max_queue_size() const override
    {
        return m_queue->get_max_queue_size();
    }
    void reset_max_queue_size() override
    {
        m_queue->reset_max_queue_size();
    }

#ifndef BOOST_NO_RVALUE_REFERENCES
    void post(typename queue_type::job_type job, std::size_t prio) override
    {
        boost::asynchronous::job_traits<typename queue_type::job_type>::set_posted_time(job);
        m_queue->push(std::move(job),prio);
    }
    void post(typename queue_type::job_type job) override
    {
        post(std::move(job),0);
    }
    
    boost::asynchronous::any_interruptible interruptible_post(typename queue_type::job_type job,
                                                          std::size_t prio) override
    {
        std::shared_ptr<boost::asynchronous::detail::interrupt_state>
                state = std::make_shared<boost::asynchronous::detail::interrupt_state>();
        std::shared_ptr<std::promise<boost::thread*> > wpromise = std::make_shared<std::promise<boost::thread*> >();
        boost::asynchronous::job_traits<typename queue_type::job_type>::set_posted_time(job);
        boost::asynchronous::interruptible_job<typename queue_type::job_type,this_type>
                ijob(std::move(job),wpromise,state);

        m_queue->push(std::move(ijob),prio);

        std::future<boost::thread*> fu = wpromise->get_future();
        boost::asynchronous::interrupt_helper interruptible(std::move(fu),state);

        return boost::asynchronous::any_interruptible(interruptible);
    }
    boost::asynchronous::any_interruptible interruptible_post(typename queue_type::job_type job) override
    {
        return interruptible_post(std::move(job),0);
    }
#else
    void post(typename queue_type::job_type& job, std::size_t prio=0) override
    {
        boost::asynchronous::job_traits<typename queue_type::job_type>::set_posted_time(job);
        m_queue->push(job,prio);
    }
    boost::asynchronous::any_interruptible interruptible_post(typename queue_type::job_type& job, std::size_t prio=0) override
    {
        std::shared_ptr<boost::asynchronous::detail::interrupt_state>
                state = std::make_shared<boost::asynchronous::detail::interrupt_state>();
        std::shared_ptr<std::promise<boost::thread*> > wpromise = std::make_shared<std::promise<boost::thread*> >();
        boost::asynchronous::job_traits<typename queue_type::job_type>::set_posted_time(job);
        boost::asynchronous::interruptible_job<typename queue_type::job_type,this_type> ijob(job,wpromise,state);

        m_queue->push(ijob,prio);

        std::shared_future<boost::thread*> fu = wpromise->get_future();
        boost::asynchronous::interrupt_helper interruptible(fu,state);

        return boost::asynchronous::any_interruptible(interruptible);
    }
#endif
    std::vector<boost::asynchronous::any_queue_ptr<job_type> > get_queues()
    {
        // this scheduler lends its queue for stealing
        std::vector<boost::asynchronous::any_queue_ptr<job_type> > res;
        res.reserve(1);
        res.push_back(m_queue);
        return res;
    }

    void enable_queue(std::size_t queue_prio, bool enable) override
    {
        m_queue->enable_queue(queue_prio,enable);
    }

    static boost::thread_specific_ptr<thread_ptr_wrapper> m_self_thread;
    
protected:

#ifndef BOOST_NO_RVALUE_REFERENCES
    single_queue_scheduler_policy(std::shared_ptr<queue_type>&& queue)
        : m_queue(std::forward<std::shared_ptr<queue_type> >(queue))
    {
    }
#endif
#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES
    template<typename... Args>
    single_queue_scheduler_policy(Args... args): m_queue(std::make_shared<queue_type>(std::move(args)...))
    {
    }
#endif
#ifndef _MSC_VER
    single_queue_scheduler_policy()
    {
        m_queue = std::make_shared<queue_type>();
    }
#endif

    std::shared_ptr<queue_type> m_queue;
};

template<class Q>
boost::thread_specific_ptr<thread_ptr_wrapper>
single_queue_scheduler_policy<Q>::m_self_thread;

}}}
#endif // BOOST_ASYNC_SCHEDULER_SINGLE_QUEUE_SCHEDULER_POLICY_HPP
