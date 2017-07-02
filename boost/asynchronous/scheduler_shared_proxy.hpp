// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2016
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

// This file provides scheduler_shared_proxy, which one usually does not use directly. This is the implementation of
// the any_shared_scheduler_proxy concept, the concept of a proxy to any scheduler.
// One uses make_shared_scheduler_proxy<desired pool>(pool constructor arguments)
// the scheduler proxy can be shared wherever desired EXCEPT in one of its own threads, directly or indirectly.
// a scheduler proxy can be posted to, or its diagnostics can be read.

#ifndef BOOST_ASYNC_SCHEDULER_SHARED_PROXY_HPP
#define BOOST_ASYNC_SCHEDULER_SHARED_PROXY_HPP

#include <utility>
// TODO ?
#include <map>
#include<list>
#include <string>
#include <limits>

#include <memory>


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
        public std::enable_shared_from_this<scheduler_shared_proxy_impl<S> >
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
    std::vector<std::size_t> get_max_queue_size() const
    {
        return m_scheduler->get_max_queue_size();
    }
    void reset_max_queue_size()
    {
        m_scheduler->reset_max_queue_size();
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

    boost::asynchronous::scheduler_diagnostics
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
        std::shared_ptr<this_type> t = this->shared_from_this();
        return std::static_pointer_cast<
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
    std::string get_name()const
    {
        return m_scheduler->get_name();
    }
    void processor_bind(std::vector<std::tuple<unsigned int,unsigned int>> p)
    {
        m_scheduler->processor_bind(std::move(p));
    }
    ~scheduler_shared_proxy_impl()
    {
        // stop scheduler and block until joined
        //TODO post instead?
        boost::asynchronous::any_joinable worker = m_scheduler->get_worker();
        m_scheduler.reset();
        // no interruption could go good here
        boost::this_thread::disable_interruption di;
        worker.join();
    }

#ifndef BOOST_NO_RVALUE_REFERENCES
    explicit scheduler_shared_proxy_impl(std::shared_ptr<scheduler_type>&& scheduler)noexcept
        : m_scheduler(std::forward<std::shared_ptr<scheduler_type>>(scheduler)){}
#else
    explicit scheduler_shared_proxy_impl(std::shared_ptr<scheduler_type> scheduler): m_scheduler(scheduler){}
#endif

    // attribute
    std::shared_ptr<scheduler_type> m_scheduler;
};
}

// scheduler proxy for use outside the active object world
// implements any_shared_scheduler_proxy
/*!
 * \class scheduler_shared_proxy
 * This class acts as a proxy to a scheduler for use outside the scheduler world.
 * Implements any_shared_scheduler_proxy_concept.
 * Objects of this class are shareable. Sharing different instances of proxies to the same scheduler is thread-safe.
 */
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


    /*!
     * \brief returns whether the scheduler proxy is a proxy to a valid scheduler
     * \return bool
     */
    bool is_valid() const
    {
        return m_impl->is_valid();
    }

    /*!
     * \brief returns the number of waiting jobs in every queue
     * \return a std::vector containing the queue sizes
     */
    std::vector<std::size_t> get_queue_size() const
    {
        return m_impl->get_queue_size();
    }

    /*!
     * \brief returns the max number of waiting jobs in every queue
     * \return a std::vector containing the max queue sizes
     */
    std::vector<std::size_t> get_max_queue_size() const
    {
        return m_impl->get_max_queue_size();
    }

    /*!
     * \brief reset the max queue sizes
     */
    void reset_max_queue_size()
    {
        m_impl->reset_max_queue_size();
    }

    /*!
     * \brief posts a job into the scheduler queue
     * \param job passed by value
     */
    void post(job_type job) const
    {
        move_post(std::move(job));
    }

    /*!
     * \brief posts a job into the scheduler queue with the given priority
     * \param job passed by value
     * \param priority. Depending on the scheduler, it can be queue or sub-pool composite_threadpool_scheduler for example). 0 means dont'care
     */
    void post(job_type job,std::size_t priority) const
    {
        move_post(std::move(job),priority);
    }

    /*!
     * \brief posts an interruptible job into the scheduler queue
     * \param job passed by value
     * \return any_interruptible. Can be used to interrupt the job
     */
    boost::asynchronous::any_interruptible interruptible_post(job_type job) const
    {
        return move_interruptible_post(std::move(job));
    }

    /*!
     * \brief posts an interruptible job into the scheduler queue with the given priority
     * \param job passed by value
     * \param priority. Depending on the scheduler, it can be queue or sub-pool composite_threadpool_scheduler for example). 0 means dont'care
     * \return any_interruptible. Can be used to interrupt the job
     */
    boost::asynchronous::any_interruptible interruptible_post(job_type job,std::size_t priority) const
    {
        return move_interruptible_post(std::move(job),priority);
    }

    /*!
     * \brief returns the ids of the threads run by this scheduler
     * \return std::vector of thread ids
     */
    std::vector<boost::thread::id> thread_ids()const
    {
        return m_impl->thread_ids();
    }
    
    /*!
     * \brief returns the diagnostics for this scheduler
     * \return a scheduler_diagnostics containing totals or current diagnostics
     */
    boost::asynchronous::scheduler_diagnostics
    get_diagnostics(std::size_t pos=0)const
    {
        return m_impl->get_diagnostics(pos);
    }

    /*!
     * \brief reset the diagnostics
     */
    void clear_diagnostics()
    {
        m_impl->clear_diagnostics();
    }

    /*!
     * \brief returns a weak scheduler. A weak scheduler is to a shared scheduler what weak_ptr is to a shared_ptr
     * \return any_weak_scheduler
     */
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

    /*!
     * \brief returns a reduced scheduler interface for internal needs
     */
    boost::asynchronous::internal_scheduler_aspect<job_type> get_internal_scheduler_aspect()
    {
#ifndef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
        std::shared_ptr<boost::asynchronous::detail::scheduler_shared_proxy_impl<S> > t = m_impl->shared_from_this();
        return std::static_pointer_cast<
                boost::asynchronous::internal_scheduler_aspect_concept<job_type> > (std::move(t));
#else        
        boost::asynchronous::internal_scheduler_aspect<job_type> a(m_impl->shared_from_this());
        return a;
#endif
    }

    /*!
     * \brief sets a name to this scheduler. On posix, this will set the name with prctl for every thread of this scheduler
     * \param name as string
     */
    void set_name(std::string const& name)
    {
        m_impl->set_name(name);
    }

    /*!
     * \brief returns the name of this scheduler
     * \return name as string
     */
    std::string get_name()const
    {
        return m_impl->get_name();
    }

    /*!
     * \brief binds threads of this scheduler to processors, starting with the given id from.
     * \brief thread 0 will be bound to from, thread 1 to from + 1, etc.
     * \param start id
     */
    void processor_bind(std::vector<std::tuple<unsigned int,unsigned int>> p)
    {
        m_impl->processor_bind(std::move(p));
    }
    ~scheduler_shared_proxy()
    {
    }

    /*!
     * \brief copy constructor noexcept
     */
    scheduler_shared_proxy(scheduler_shared_proxy const& rhs)
        : m_impl(rhs.m_impl)
    {
    }

    /*!
     * \brief move constructor noexcept
     */
    scheduler_shared_proxy(scheduler_shared_proxy&& rhs) noexcept
        : m_impl(std::move(rhs.m_impl))
    {
    }

    /*!
     * \brief copy assignment operator noexcept
     */
    scheduler_shared_proxy& operator= (scheduler_shared_proxy const& rhs) noexcept
    {
        m_impl = rhs.m_impl;
        return *this;
    }

    /*!
     * \brief move assignment operator noexcept
     */
    scheduler_shared_proxy& operator= (scheduler_shared_proxy&& rhs) noexcept
    {
        std::swap(m_impl,rhs.m_impl);
        return *this;
    }
private:
    void move_post(job_type job,std::size_t priority=0) const
    {
        m_impl->post(std::move(job),priority);
    }
    boost::asynchronous::any_interruptible move_interruptible_post(job_type job,
                                                                std::size_t priority=0) const
    {
       return m_impl->interruptible_post(std::move(job),priority);
    }

#ifndef BOOST_NO_RVALUE_REFERENCES
    explicit scheduler_shared_proxy(std::shared_ptr<scheduler_type>&& scheduler)
        : m_impl(std::make_shared<boost::asynchronous::detail::scheduler_shared_proxy_impl<S> >(
                     std::forward<std::shared_ptr<scheduler_type>>(scheduler)))
    {
        scheduler.reset();
    }
#else
    explicit scheduler_shared_proxy(std::shared_ptr<scheduler_type> scheduler)
        : m_impl(std::make_shared<boost::asynchronous::detail::scheduler_shared_proxy_impl<S> >(scheduler)){}
#endif
    // attribute
    std::shared_ptr<boost::asynchronous::detail::scheduler_shared_proxy_impl<S> > m_impl;

    template <class Sched>
    friend boost::asynchronous::any_shared_scheduler_proxy<typename Sched::job_type>
    create_shared_scheduler_proxy(Sched *);
#ifndef BOOST_NO_RVALUE_REFERENCES  
    template <class Sched>
    friend boost::asynchronous::any_shared_scheduler_proxy<typename Sched::job_type>
    create_shared_scheduler_proxy(std::shared_ptr<Sched>&&);
#endif
    template <class Sched>
    friend boost::asynchronous::any_shared_scheduler_proxy<typename Sched::job_type>
    create_shared_scheduler_proxy(std::shared_ptr<Sched>);

    template< class Sched, class... Args >
    friend
    typename std::enable_if<!boost::asynchronous::has_self_proxy_creation<Sched>::value,
                            boost::asynchronous::any_shared_scheduler_proxy<typename Sched::job_type> >::type
    make_shared_scheduler_proxy(Args && ... args);
};

template <class S>
boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type> create_shared_scheduler_proxy(S* scheduler)
{
    std::shared_ptr<S> sps (scheduler);
    sps->constructor_done(sps);
#ifndef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
    return boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type>(new boost::asynchronous::scheduler_shared_proxy<S>(std::move(sps)));
#else
    //TODO solve friend problem
    std::shared_ptr<boost::asynchronous::scheduler_shared_proxy<S> > p(new boost::asynchronous::scheduler_shared_proxy<S>(std::move(sps)));
    boost::asynchronous::any_shared_scheduler_proxy_ptr<typename S::job_type> ptr (p);
    return boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type>(ptr);
#endif
}

#ifndef BOOST_NO_RVALUE_REFERENCES
template <class S>
boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type> create_shared_scheduler_proxy(std::shared_ptr<S>&& scheduler)
{
    scheduler->constructor_done(std::weak_ptr<S>(scheduler));
#ifndef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
    return boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type>(new boost::asynchronous::scheduler_shared_proxy<S>(std::forward<std::shared_ptr<S> >(scheduler)));
#else
    //TODO solve friend problem
    //TODO Clock
    std::shared_ptr<boost::asynchronous::scheduler_shared_proxy<S> > p(new boost::asynchronous::scheduler_shared_proxy<S>(std::forward<std::shared_ptr<S> >(scheduler)));
    boost::asynchronous::any_shared_scheduler_proxy_ptr<typename S::job_type> ptr (p);
    return boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type>(ptr);
#endif
}
#else
template <class S>
boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type> create_shared_scheduler_proxy(std::shared_ptr<S> scheduler)
{
    scheduler->constructor_done(scheduler);
    //TODO solve friend problem
    std::shared_ptr<boost::asynchronous::scheduler_shared_proxy<S> > p(new boost::asynchronous::scheduler_shared_proxy<S>(scheduler));
    boost::asynchronous::any_shared_scheduler_proxy_ptr<typename S::job_type> ptr (p);
    return boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type>(ptr);
}
#endif

/*!
 * \brief creates a proxy to a given scheduler, forward passed arguments to it.
 * \brief example: make_shared_scheduler_proxy<
 * \brief            threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(3)
 * \brief will create a threadpool scheduler using a lockfree queue and 3 threads and pass it to a proxy,
 * \brief represented by a any_shared_scheduler_proxy
 * \tparam args the arguments passed to the scheduler upon creation
 * \return any_shared_scheduler_proxy<job>, job type being given by the queue, default BOOST_ASYNCHRONOUS_DEFAULT_JOB (any_callable by default)
 */
template< class S, class... Args >
typename std::enable_if<!boost::asynchronous::has_self_proxy_creation<S>::value,
                           boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type> >::type
make_shared_scheduler_proxy(Args && ... args)
{
    auto sps = std::make_shared<S>(std::forward<Args>(args)...);
    sps->constructor_done(sps);
#ifndef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
    return boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type>(new boost::asynchronous::scheduler_shared_proxy<S>(std::move(sps)));
#else
    //TODO solve friend problem
    std::shared_ptr<boost::asynchronous::scheduler_shared_proxy<S> > p(std::make_shared<boost::asynchronous::scheduler_shared_proxy<S>>(std::move(sps)));
    boost::asynchronous::any_shared_scheduler_proxy_ptr<typename S::job_type> ptr (p);
    return boost::asynchronous::any_shared_scheduler_proxy<typename S::job_type>(ptr);
#endif
}

#ifndef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
template<class JOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB>
class scheduler_weak_proxy
{
public:
    typedef JOB job_type;

    scheduler_weak_proxy(boost::asynchronous::any_shared_scheduler_proxy<job_type>& impl)
        : m_impl(impl.my_ptr)
        {}

    scheduler_weak_proxy():m_impl(){}
    any_shared_scheduler_proxy<job_type> lock() const
    {
        if (m_impl.expired())
        {
            return any_shared_scheduler_proxy<job_type>();
        }
        std::shared_ptr<boost::asynchronous::any_shared_scheduler_proxy_concept<job_type>> shared_impl = m_impl.lock();
        return boost::asynchronous::any_shared_scheduler_proxy<job_type>(shared_impl);
    }
    scheduler_weak_proxy<job_type>& operator= (scheduler_weak_proxy<job_type> const& rhs)
    {
        m_impl = rhs.m_impl;
        return *this;
    }
    scheduler_weak_proxy<job_type>& operator= (boost::asynchronous::any_shared_scheduler_proxy<job_type> const& rhs)
    {
        m_impl = rhs.my_ptr;
        return *this;
    }
private:
    std::weak_ptr<boost::asynchronous::any_shared_scheduler_proxy_concept<job_type>> m_impl;

};
#endif
}} // boost::asynchronous


#endif /* BOOST_ASYNC_SCHEDULER_SHARED_PROXY_HPP */
