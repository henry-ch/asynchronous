// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_SCHEDULER_SHARED_PROXY_HPP
#define BOOST_ASYNC_SCHEDULER_SHARED_PROXY_HPP

#include <utility>
// TODO ?
#include <map>
#include<list>
#include <string>
#include <limits>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <boost/asynchronous/any_shared_scheduler_proxy.hpp>
#include <boost/asynchronous/job_traits.hpp>
#include <boost/asynchronous/detail/any_joinable.hpp>
#include <boost/asynchronous/scheduler/detail/lockable_weak_scheduler.hpp>

namespace boost { namespace asynchronous
{

// scheduler proxy for use outside the active object world
// implements any_shared_scheduler_proxy
template<class S>
class scheduler_shared_proxy : 
#ifdef BOOST_ASYNCHRONOUS_NO_TYPE_ERASURE
        public any_shared_scheduler_proxy_concept<typename S::job_type>, public internal_scheduler_aspect_concept<typename S::job_type>,
#endif
        public boost::enable_shared_from_this<scheduler_shared_proxy<S> >
{
public:
    typedef S scheduler_type;
    typedef scheduler_shared_proxy<S> this_type;
    typedef typename S::job_type job_type;

    bool is_valid() const
    {
        return (!!m_scheduler);
    }
    std::size_t get_queue_size() const
    {
        return m_scheduler->get_queue_size();
    }
#ifndef BOOST_NO_RVALUE_REFERENCES
    void post(job_type job) const
    {
        BOOST_ASSERT_MSG(m_scheduler,"scheduler_shared_proxy::post has empty scheduler");
        move_post(std::move(job));
    }
    void post(job_type job,std::size_t priority) const
    {
        BOOST_ASSERT_MSG(m_scheduler,"scheduler_shared_proxy::post has empty scheduler");
        move_post(std::move(job),priority);
    }
    void move_post(job_type&& job,std::size_t priority=0) const
    {
        m_scheduler->post(std::forward<job_type>(job),priority);
    }
    boost::asynchronous::any_interruptible interruptible_post(job_type job) const
    {
        BOOST_ASSERT_MSG(m_scheduler,"scheduler_shared_proxy::interruptible_post has empty scheduler");
        return move_interruptible_post(std::move(job));
    }
    boost::asynchronous::any_interruptible interruptible_post(job_type job,std::size_t priority) const
    {
        BOOST_ASSERT_MSG(m_scheduler,"scheduler_shared_proxy::interruptible_post has empty scheduler");
        return move_interruptible_post(std::move(job),priority);
    }
    boost::asynchronous::any_interruptible move_interruptible_post(job_type && job,
                                                                std::size_t priority=0) const
    {
       return m_scheduler->interruptible_post(std::forward<job_type>(job),priority);
    }
#else
    void post(job_type job) const
    {
        BOOST_ASSERT_MSG(m_scheduler,"scheduler_shared_proxy::post has empty scheduler");
        m_scheduler->post(job);
    }
    void post(job_type job,std::size_t priority) const
    {
        BOOST_ASSERT_MSG(m_scheduler,"scheduler_shared_proxy::post has empty scheduler");
        m_scheduler->post(job,priority);
    }
    boost::asynchronous::any_interruptible interruptible_post(job_type job) const
    {
        BOOST_ASSERT_MSG(m_scheduler,"scheduler_shared_proxy::interruptible_post has empty scheduler");
        return m_scheduler->interruptible_post(job);
    }
    boost::asynchronous::any_interruptible interruptible_post(job_type job,std::size_t priority) const
    {
        BOOST_ASSERT_MSG(m_scheduler,"scheduler_shared_proxy::interruptible_post has empty scheduler");
        return m_scheduler->interruptible_post(job,priority);
    }
#endif

    std::vector<boost::thread::id> thread_ids()const
    {
        return m_scheduler->thread_ids();
    }
    
    std::map<std::string,
             std::list<typename boost::asynchronous::job_traits<job_type>::diagnostic_item_type > >
    get_diagnostics(std::size_t pos=0)const
    {
        return m_scheduler->get_diagnostics(pos);
    }

    boost::asynchronous::any_weak_scheduler<job_type> get_weak_scheduler() const
    {
        boost::asynchronous::detail::lockable_weak_scheduler<scheduler_type> w(m_scheduler);
        return boost::asynchronous::any_weak_scheduler<job_type>(w);
    }
    
    std::vector<boost::asynchronous::any_queue_ptr<job_type> > get_queues() const
    {
        return m_scheduler->get_queues();
    }
    void set_steal_from_queues(std::vector<boost::asynchronous::any_queue_ptr<job_type> > const& queues)
    {
        m_scheduler->set_steal_from_queues(queues);
    }
    boost::asynchronous::internal_scheduler_aspect<job_type> get_internal_scheduler_aspect()
    {
#ifdef BOOST_ASYNCHRONOUS_NO_TYPE_ERASURE
        boost::shared_ptr<this_type> t = this->shared_from_this();
        return boost::static_pointer_cast<
                boost::asynchronous::internal_scheduler_aspect_concept<job_type> > (t);
#else        
        boost::asynchronous::internal_scheduler_aspect<job_type> a(this->shared_from_this());
        return a;
#endif
    }

    ~scheduler_shared_proxy()
    {
        // stop scheduler and block until joined
        //TODO post instead?
        boost::asynchronous::any_joinable worker = m_scheduler->get_worker();
        m_scheduler.reset();
        worker.join();
    }

private:
#ifndef BOOST_NO_RVALUE_REFERENCES
    explicit scheduler_shared_proxy(boost::shared_ptr<scheduler_type>&& scheduler): m_scheduler(scheduler){scheduler.reset();}
#else
    explicit scheduler_shared_proxy(boost::shared_ptr<scheduler_type> scheduler): m_scheduler(scheduler){}
#endif

    // attribute
    boost::shared_ptr<scheduler_type> m_scheduler;

    template <class Sched>
    friend boost::asynchronous::any_shared_scheduler_proxy<typename Sched::job_type>
    create_shared_scheduler_proxy(Sched *);
#ifndef BOOST_NO_RVALUE_REFERENCES  
    template <class Sched>
    friend boost::asynchronous::any_shared_scheduler_proxy<typename Sched::job_type>
    create_shared_scheduler_proxy(boost::shared_ptr<Sched>&&);
#endif
    template <class Sched>
    friend boost::asynchronous::any_shared_scheduler_proxy<typename Sched::job_type>
    create_shared_scheduler_proxy(boost::shared_ptr<Sched>);
};

template <class S>
boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type> create_shared_scheduler_proxy(S* scheduler)
{
    boost::shared_ptr<S> sps (scheduler);
    sps->constructor_done(sps);
#ifdef BOOST_ASYNCHRONOUS_NO_TYPE_ERASURE
    boost::asynchronous::any_shared_scheduler_proxy_ptr<typename S::job_type> p(new boost::asynchronous::scheduler_shared_proxy<S>(std::move(sps)));
    return boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type>(p);
#else
    //TODO solve friend problem
    boost::shared_ptr<boost::asynchronous::scheduler_shared_proxy<S> > p(new boost::asynchronous::scheduler_shared_proxy<S>(std::move(sps)));
    boost::asynchronous::any_shared_scheduler_proxy_ptr<typename S::job_type> ptr (p);
    return boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type>(ptr);
#endif
}

#ifndef BOOST_NO_RVALUE_REFERENCES
template <class S>
boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type> create_shared_scheduler_proxy(boost::shared_ptr<S>&& scheduler)
{
    scheduler->constructor_done(boost::weak_ptr<S>(scheduler));
#ifdef BOOST_ASYNCHRONOUS_NO_TYPE_ERASURE
    boost::asynchronous::any_shared_scheduler_proxy_ptr<typename S::job_type> p(new boost::asynchronous::scheduler_shared_proxy<S>(std::forward<boost::shared_ptr<S> >(scheduler)));
    return boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type>(p);
#else
    //TODO solve friend problem
    //TODO Clock
    boost::shared_ptr<boost::asynchronous::scheduler_shared_proxy<S> > p(new boost::asynchronous::scheduler_shared_proxy<S>(std::forward<boost::shared_ptr<S> >(scheduler)));
    boost::asynchronous::any_shared_scheduler_proxy_ptr<typename S::job_type> ptr (p);
    return boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type>(ptr);
#endif
}
#else
template <class S>
boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type> create_shared_scheduler_proxy(boost::shared_ptr<S> scheduler)
{
    scheduler->constructor_done(scheduler);
    //TODO solve friend problem
    boost::shared_ptr<boost::asynchronous::scheduler_shared_proxy<S> > p(new boost::asynchronous::scheduler_shared_proxy<S>(scheduler));
    boost::asynchronous::any_shared_scheduler_proxy_ptr<typename S::job_type> ptr (p);
    return boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type>(ptr);
}
#endif

}} // boost::async


#endif /* BOOST_ASYNC_SCHEDULER_WEAK_PROXY_HPP */
