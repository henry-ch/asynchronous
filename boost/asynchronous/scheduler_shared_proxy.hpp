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

namespace detail
{
template<class S>
class scheduler_shared_proxy_impl :
#ifndef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
        public any_shared_scheduler_proxy_concept<typename S::job_type>, public internal_scheduler_aspect_concept<typename S::job_type>,
#endif
        public boost::enable_shared_from_this<scheduler_shared_proxy_impl<S> >
{
public:
    typedef S scheduler_type;
    typedef scheduler_shared_proxy_impl<S> this_type;
    typedef typename S::job_type job_type;

    bool is_valid() const
    {
        return (!!m_scheduler);
    }
    std::vector<std::size_t> get_queue_size() const
    {
        return m_scheduler->get_queue_size();
    }
#ifndef BOOST_NO_RVALUE_REFERENCES
    void post(job_type job) const
    {
        BOOST_ASSERT_MSG(m_scheduler,"scheduler_shared_proxy_impl::post has empty scheduler");
        move_post(std::move(job));
    }
    void post(job_type job,std::size_t priority) const
    {
        BOOST_ASSERT_MSG(m_scheduler,"scheduler_shared_proxy_impl::post has empty scheduler");
        move_post(std::move(job),priority);
    }
    void move_post(job_type job,std::size_t priority=0) const
    {
        m_scheduler->post(std::move(job),priority);
    }
    boost::asynchronous::any_interruptible interruptible_post(job_type job) const
    {
        BOOST_ASSERT_MSG(m_scheduler,"scheduler_shared_proxy_impl::interruptible_post has empty scheduler");
        return move_interruptible_post(std::move(job));
    }
    boost::asynchronous::any_interruptible interruptible_post(job_type job,std::size_t priority) const
    {
        BOOST_ASSERT_MSG(m_scheduler,"scheduler_shared_proxy_impl::interruptible_post has empty scheduler");
        return move_interruptible_post(std::move(job),priority);
    }
    boost::asynchronous::any_interruptible move_interruptible_post(job_type job,
                                                                std::size_t priority=0) const
    {
       return m_scheduler->interruptible_post(std::move(job),priority);
    }
#else
    void post(job_type job) const
    {
        BOOST_ASSERT_MSG(m_scheduler,"scheduler_shared_proxy_impl::post has empty scheduler");
        m_scheduler->post(job);
    }
    void post(job_type job,std::size_t priority) const
    {
        BOOST_ASSERT_MSG(m_scheduler,"scheduler_shared_proxy_impl::post has empty scheduler");
        m_scheduler->post(job,priority);
    }
    boost::asynchronous::any_interruptible interruptible_post(job_type job) const
    {
        BOOST_ASSERT_MSG(m_scheduler,"scheduler_shared_proxy_impl::interruptible_post has empty scheduler");
        return m_scheduler->interruptible_post(job);
    }
    boost::asynchronous::any_interruptible interruptible_post(job_type job,std::size_t priority) const
    {
        BOOST_ASSERT_MSG(m_scheduler,"scheduler_shared_proxy_impl::interruptible_post has empty scheduler");
        return m_scheduler->interruptible_post(job,priority);
    }
#endif

    std::vector<boost::thread::id> thread_ids()const
    {
        return m_scheduler->thread_ids();
    }

    boost::asynchronous::scheduler_diagnostics<job_type>
    get_diagnostics(std::size_t pos=0)const
    {
        return m_scheduler->get_diagnostics(pos);
    }
    void clear_diagnostics()
    {
        m_scheduler->clear_diagnostics();
    }
    boost::asynchronous::any_weak_scheduler<job_type> get_weak_scheduler() const
    {
        boost::asynchronous::detail::lockable_weak_scheduler<scheduler_type> w(m_scheduler);
        return boost::asynchronous::any_weak_scheduler<job_type>(std::move(w));
    }

    std::vector<boost::asynchronous::any_queue_ptr<job_type> > get_queues()
    {
        return m_scheduler->get_queues();
    }
    void set_steal_from_queues(std::vector<boost::asynchronous::any_queue_ptr<job_type> > const& queues)
    {
        m_scheduler->set_steal_from_queues(queues);
    }
    boost::asynchronous::internal_scheduler_aspect<job_type> get_internal_scheduler_aspect()
    {
#ifndef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
        boost::shared_ptr<this_type> t = this->shared_from_this();
        return boost::static_pointer_cast<
                boost::asynchronous::internal_scheduler_aspect_concept<job_type> > (t);
#else
        boost::asynchronous::internal_scheduler_aspect<job_type> a(this->shared_from_this());
        return a;
#endif
    }
    void set_name(std::string const& name)
    {
        m_scheduler->set_name(name);
    }

    ~scheduler_shared_proxy_impl()
    {
        // stop scheduler and block until joined
        //TODO post instead?
        boost::asynchronous::any_joinable worker = m_scheduler->get_worker();
        m_scheduler.reset();
        worker.join();
    }

#ifndef BOOST_NO_RVALUE_REFERENCES
    explicit scheduler_shared_proxy_impl(boost::shared_ptr<scheduler_type>&& scheduler)noexcept
        : m_scheduler(std::forward<boost::shared_ptr<scheduler_type>>(scheduler)){}
#else
    explicit scheduler_shared_proxy_impl(boost::shared_ptr<scheduler_type> scheduler): m_scheduler(scheduler){}
#endif

    // attribute
    boost::shared_ptr<scheduler_type> m_scheduler;
};
}

// scheduler proxy for use outside the active object world
// implements any_shared_scheduler_proxy
template<class S>
class scheduler_shared_proxy
#ifndef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
       : public any_shared_scheduler_proxy_concept<typename S::job_type>, public internal_scheduler_aspect_concept<typename S::job_type>
#endif
{
public:
    typedef S scheduler_type;
    typedef scheduler_shared_proxy<S> this_type;
    typedef typename S::job_type job_type;

    bool is_valid() const
    {
        return m_impl->is_valid();
    }
    std::vector<std::size_t> get_queue_size() const
    {
        return m_impl->get_queue_size();
    }
#ifndef BOOST_NO_RVALUE_REFERENCES
    void post(job_type job) const
    {
        move_post(std::move(job));
    }
    void post(job_type job,std::size_t priority) const
    {
        move_post(std::move(job),priority);
    }
    void move_post(job_type job,std::size_t priority=0) const
    {
        m_impl->post(std::move(job),priority);
    }
    boost::asynchronous::any_interruptible interruptible_post(job_type job) const
    {
        return move_interruptible_post(std::move(job));
    }
    boost::asynchronous::any_interruptible interruptible_post(job_type job,std::size_t priority) const
    {
        return move_interruptible_post(std::move(job),priority);
    }
    boost::asynchronous::any_interruptible move_interruptible_post(job_type job,
                                                                std::size_t priority=0) const
    {
       return m_impl->interruptible_post(std::move(job),priority);
    }
#else
    void post(job_type job) const
    {
        m_impl->post(job);
    }
    void post(job_type job,std::size_t priority) const
    {
        m_impl->post(job,priority);
    }
    boost::asynchronous::any_interruptible interruptible_post(job_type job) const
    {
        return m_impl->interruptible_post(job);
    }
    boost::asynchronous::any_interruptible interruptible_post(job_type job,std::size_t priority) const
    {
        return m_impl->interruptible_post(job,priority);
    }
#endif

    std::vector<boost::thread::id> thread_ids()const
    {
        return m_impl->thread_ids();
    }
    
    boost::asynchronous::scheduler_diagnostics<job_type>
    get_diagnostics(std::size_t pos=0)const
    {
        return m_impl->get_diagnostics(pos);
    }
    void clear_diagnostics()
    {
        m_impl->clear_diagnostics();
    }
    boost::asynchronous::any_weak_scheduler<job_type> get_weak_scheduler() const
    {
        boost::asynchronous::detail::lockable_weak_scheduler<scheduler_type> w(m_impl->m_scheduler);
        return boost::asynchronous::any_weak_scheduler<job_type>(std::move(w));
    }
    
    std::vector<boost::asynchronous::any_queue_ptr<job_type> > get_queues()
    {
        return m_impl->get_queues();
    }
    void set_steal_from_queues(std::vector<boost::asynchronous::any_queue_ptr<job_type> > const& queues)
    {
        m_impl->set_steal_from_queues(queues);
    }
    boost::asynchronous::internal_scheduler_aspect<job_type> get_internal_scheduler_aspect()
    {
#ifndef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
        boost::shared_ptr<boost::asynchronous::detail::scheduler_shared_proxy_impl<S> > t = m_impl->shared_from_this();
        return boost::static_pointer_cast<
                boost::asynchronous::internal_scheduler_aspect_concept<job_type> > (std::move(t));
#else        
        boost::asynchronous::internal_scheduler_aspect<job_type> a(m_impl->shared_from_this());
        return a;
#endif
    }
    void set_name(std::string const& name)
    {
        m_impl->set_name(name);
    }

    ~scheduler_shared_proxy()
    {
    }

    scheduler_shared_proxy(scheduler_shared_proxy const& rhs)
        : m_impl(rhs.m_impl)
    {
    }
    scheduler_shared_proxy(scheduler_shared_proxy&& rhs) noexcept
        : m_impl(std::move(rhs.m_impl))
    {
    }
    scheduler_shared_proxy& operator= (scheduler_shared_proxy const& rhs)
    {
        m_impl = rhs.m_impl;
        return *this;
    }
    scheduler_shared_proxy& operator= (scheduler_shared_proxy&& rhs) noexcept
    {
        std::swap(m_impl,rhs.m_impl);
        return *this;
    }
private:
#ifndef BOOST_NO_RVALUE_REFERENCES
    explicit scheduler_shared_proxy(boost::shared_ptr<scheduler_type>&& scheduler)
        : m_impl(boost::make_shared<boost::asynchronous::detail::scheduler_shared_proxy_impl<S> >(
                     std::forward<boost::shared_ptr<scheduler_type>>(scheduler)))
    {
        scheduler.reset();
    }
#else
    explicit scheduler_shared_proxy(boost::shared_ptr<scheduler_type> scheduler)
        : m_impl(boost::make_shared<boost::asynchronous::detail::scheduler_shared_proxy_impl<S> >(scheduler)){}
#endif
    // attribute
    boost::shared_ptr<boost::asynchronous::detail::scheduler_shared_proxy_impl<S> > m_impl;

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

    template< class Sched, class... Args >
    friend
    typename boost::disable_if<has_self_proxy_creation<Sched>,boost::asynchronous::any_shared_scheduler_proxy<typename Sched::job_type> >::type
    make_shared_scheduler_proxy(Args && ... args);
};

template <class S>
boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type> create_shared_scheduler_proxy(S* scheduler)
{
    boost::shared_ptr<S> sps (scheduler);
    sps->constructor_done(sps);
#ifndef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
    return boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type>(new boost::asynchronous::scheduler_shared_proxy<S>(std::move(sps)));
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
#ifndef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
    return boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type>(new boost::asynchronous::scheduler_shared_proxy<S>(std::forward<boost::shared_ptr<S> >(scheduler)));
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

template< class S, class... Args >
typename boost::disable_if<has_self_proxy_creation<S>,boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type> >::type
make_shared_scheduler_proxy(Args && ... args)
{
    auto sps = boost::make_shared<S>(std::forward<Args>(args)...);
    sps->constructor_done(sps);
#ifndef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
    return boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type>(new boost::asynchronous::scheduler_shared_proxy<S>(std::move(sps)));
#else
    //TODO solve friend problem
    boost::shared_ptr<boost::asynchronous::scheduler_shared_proxy<S> > p(boost::make_shared<boost::asynchronous::scheduler_shared_proxy<S>>(std::move(sps)));
    boost::asynchronous::any_shared_scheduler_proxy_ptr<typename S::job_type> ptr (p);
    return boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type>(ptr);
#endif
}

#ifndef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
template<class JOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB,class Clock = boost::chrono::high_resolution_clock>
class scheduler_weak_proxy
{
public:
    typedef JOB job_type;

    scheduler_weak_proxy(boost::asynchronous::any_shared_scheduler_proxy<job_type,Clock>& impl)
        : m_impl(impl.my_ptr)
        {}

    scheduler_weak_proxy():m_impl(){}
    any_shared_scheduler_proxy<job_type,Clock> lock() const
    {
        if (m_impl.expired())
        {
            return any_shared_scheduler_proxy<job_type,Clock>();
        }
        boost::shared_ptr<boost::asynchronous::any_shared_scheduler_proxy_concept<job_type,Clock>> shared_impl = m_impl.lock();
        return boost::asynchronous::any_shared_scheduler_proxy<job_type,Clock>(shared_impl);
    }
    scheduler_weak_proxy<job_type,Clock>& operator= (scheduler_weak_proxy<job_type,Clock> const& rhs)
    {
        m_impl = rhs.m_impl;
        return *this;
    }
    scheduler_weak_proxy<job_type,Clock>& operator= (boost::asynchronous::any_shared_scheduler_proxy<job_type,Clock> const& rhs)
    {
        m_impl = rhs.my_ptr;
        return *this;
    }
private:
    boost::weak_ptr<boost::asynchronous::any_shared_scheduler_proxy_concept<job_type,Clock>> m_impl;

};
#endif
}} // boost::asynchronous


#endif /* BOOST_ASYNC_SCHEDULER_SHARED_PROXY_HPP */
