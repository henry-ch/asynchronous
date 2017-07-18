// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2016
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_ANY_SHARED_SCHEDULER_PROXY_HPP
#define BOOST_ASYNC_ANY_SHARED_SCHEDULER_PROXY_HPP

#include <string>
#include <vector>

#include <boost/mpl/vector.hpp>
#include <memory>
#include <boost/thread/thread.hpp>

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/constructible.hpp>
#include <boost/type_erasure/relaxed.hpp>
#include <boost/type_erasure/any_cast.hpp>

#include <boost/asynchronous/detail/concept_members.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/diagnostics/any_loggable.hpp>
#include <boost/asynchronous/any_scheduler.hpp>
#include <boost/asynchronous/detail/any_pointer.hpp>
#include <boost/asynchronous/detail/any_interruptible.hpp>
#include <boost/asynchronous/queue/any_queue.hpp>
#include <boost/asynchronous/scheduler_diagnostics.hpp>

// any_shared_scheduler_proxy_concept is provided as a concept or interface (default)
// this type is what programmers will see: something representing the outer interface of a scheduler
// and keeping it and its threads active.
// The last any_shared_scheduler_proxy instance will destroy the scheduler and join its threads.
// It can be distributed everywhere in the code EXCEPT in one of its own threads, either directly or indirectly.
// This means a design with a cycle including a any_shared_scheduler_proxy might deadlock at shutdown.

namespace boost { namespace asynchronous
{
BOOST_MPL_HAS_XXX_TRAIT_DEF(self_proxy_creation)

#ifdef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
// for implementation use only
template <class JOB>
struct internal_scheduler_aspect_concept:
 ::boost::mpl::vector<
    boost::asynchronous::pointer<>,
    boost::type_erasure::same_type<boost::asynchronous::pointer<>::element_type,boost::type_erasure::_a >,        
    boost::asynchronous::has_get_queues<std::vector<boost::asynchronous::any_queue_ptr<JOB> >(),boost::type_erasure::_a>,
    boost::asynchronous::has_set_steal_from_queues<void(std::vector<boost::asynchronous::any_queue_ptr<JOB> > const&),boost::type_erasure::_a>,
    boost::type_erasure::relaxed,
    boost::type_erasure::copy_constructible<>
>{} ;
template <class T>
struct internal_scheduler_aspect
        : boost::type_erasure::any<boost::asynchronous::internal_scheduler_aspect_concept<T> >
{
    internal_scheduler_aspect():
        boost::type_erasure::any<boost::asynchronous::internal_scheduler_aspect_concept<T> > (){}

    template <class U>
    internal_scheduler_aspect(U const& u):
        boost::type_erasure::any<boost::asynchronous::internal_scheduler_aspect_concept<T> > (u){}    
};

template <class JOB>
struct any_shared_scheduler_proxy_concept:
 ::boost::mpl::vector<
    boost::asynchronous::pointer<>,
    boost::type_erasure::same_type<boost::asynchronous::pointer<>::element_type,boost::type_erasure::_a >,
    boost::asynchronous::has_post<void(JOB), const boost::type_erasure::_a>,
    boost::asynchronous::has_post<void(JOB, std::size_t), const boost::type_erasure::_a>,
    boost::asynchronous::has_interruptible_post<boost::asynchronous::any_interruptible(JOB), const boost::type_erasure::_a>,
    boost::asynchronous::has_interruptible_post<boost::asynchronous::any_interruptible(JOB, std::size_t),
                                             const boost::type_erasure::_a>,
    boost::asynchronous::has_thread_ids<std::vector<boost::thread::id>(), const boost::type_erasure::_a>,
    boost::asynchronous::has_get_weak_scheduler<boost::asynchronous::any_weak_scheduler<JOB>(), const boost::type_erasure::_a>,
    boost::asynchronous::has_is_valid<bool(), const boost::type_erasure::_a>,
    boost::asynchronous::has_get_queue_size<std::vector<std::size_t>(), const boost::type_erasure::_a>,
    boost::asynchronous::has_reset<void()>,
    boost::asynchronous::has_clear_diagnostics<void(), boost::type_erasure::_a>,
#ifndef BOOST_NO_RVALUE_REFERENCES
    boost::asynchronous::has_get_diagnostics<boost::asynchronous::scheduler_diagnostics(std::size_t),
                                          const boost::type_erasure::_a>,
#endif
    boost::asynchronous::has_get_diagnostics<boost::asynchronous::scheduler_diagnostics(),
                                          const boost::type_erasure::_a>,
    boost::asynchronous::has_get_internal_scheduler_aspect<boost::asynchronous::internal_scheduler_aspect<JOB>(), boost::type_erasure::_a>,
    boost::asynchronous::has_set_name<void(std::string const&), boost::type_erasure::_a>,
    boost::asynchronous::has_get_name<std::string(), const boost::type_erasure::_a>,
    boost::asynchronous::has_processor_bind<void(std::vector<std::tuple<unsigned int,unsigned int>>), boost::type_erasure::_a>,
    boost::asynchronous::has_execute_in_all_threads<std::vector<std::future<void>>(boost::asynchronous::any_callable), boost::type_erasure::_a>,
    boost::type_erasure::relaxed,
    boost::type_erasure::copy_constructible<>
>{} ;

template <class T = BOOST_ASYNCHRONOUS_DEFAULT_JOB>
struct any_shared_scheduler_proxy_ptr: boost::type_erasure::any<boost::asynchronous::any_shared_scheduler_proxy_concept<T> >
{
    typedef T job_type;
    any_shared_scheduler_proxy_ptr():
        boost::type_erasure::any<boost::asynchronous::any_shared_scheduler_proxy_concept<T> > (){}

    template <class U>
    any_shared_scheduler_proxy_ptr(U const& u):
        boost::type_erasure::any<boost::asynchronous::any_shared_scheduler_proxy_concept<T> > (u){}
};
template <class JOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB>
class any_shared_scheduler_proxy
{
public:
    typedef JOB job_type;

    any_shared_scheduler_proxy(any_shared_scheduler_proxy_ptr<JOB> ptr):my_ptr(ptr){}
    any_shared_scheduler_proxy():my_ptr(){}
    any_shared_scheduler_proxy(any_shared_scheduler_proxy const& other):my_ptr(other.my_ptr){}

    void reset()
    {
        my_ptr.reset();
    }
    //TODO check is_valid
    void post(JOB job) const
    {
        (*my_ptr).post(std::move(job));
    }
    void post(JOB job, std::size_t priority) const
    {
        (*my_ptr).post(std::move(job),priority);
    }
    boost::asynchronous::any_interruptible interruptible_post(JOB job) const
    {
        return (*my_ptr).interruptible_post(std::move(job));
    }
    boost::asynchronous::any_interruptible interruptible_post(JOB job, std::size_t priority) const
    {
        return (*my_ptr).interruptible_post(std::move(job),priority);
    }
    std::vector<boost::thread::id> thread_ids() const
    {
        return (*my_ptr).thread_ids();
    }
    boost::asynchronous::any_weak_scheduler<JOB> get_weak_scheduler() const
    {
        return (*my_ptr).get_weak_scheduler();
    }
    bool is_valid() const
    {
        return (*my_ptr).is_valid();
    }
    std::vector<std::size_t> get_queue_size()const
    {
        return (*my_ptr).get_queue_size();
    }
    std::vector<std::size_t> get_max_queue_size()const
    {
        return (*my_ptr).get_max_queue_size();
    }
    void reset_max_queue_size()
    {
        (*my_ptr).reset_max_queue_size();
    }
    boost::asynchronous::scheduler_diagnostics get_diagnostics(std::size_t prio=0)const
    {
        return (*my_ptr).get_diagnostics(prio);
    }
    void clear_diagnostics()
    {
        (*my_ptr).clear_diagnostics();
    }
    boost::asynchronous::internal_scheduler_aspect<JOB> get_internal_scheduler_aspect() const
    {
        return (*my_ptr).get_internal_scheduler_aspect();
    }
    void set_name(std::string const& name)
    {
        (*my_ptr).set_name(name);
    }
    std::string get_name()const
    {
        return (*my_ptr).get_name(name);
    }
    void processor_bind(std::vector<std::tuple<unsigned int,unsigned int>> p)
    {
        (*my_ptr).processor_bind(std::move(p));
    }
    std::vector<std::future<void>> execute_in_all_threads(boost::asynchronous::any_callable c)
    {
        return (*my_ptr).execute_in_all_threads(std::move(c));
    }
private:
    any_shared_scheduler_proxy_ptr<JOB> my_ptr;
};
#else
template <class JOB>
struct internal_scheduler_aspect_concept
{
    virtual std::vector<boost::asynchronous::any_queue_ptr<JOB> > get_queues()=0;
    virtual void set_steal_from_queues(std::vector<boost::asynchronous::any_queue_ptr<JOB> > const&) =0;
};
template <class T>
struct internal_scheduler_aspect
        : std::shared_ptr<boost::asynchronous::internal_scheduler_aspect_concept<T> >
{
    internal_scheduler_aspect():
        std::shared_ptr<boost::asynchronous::internal_scheduler_aspect_concept<T> > (){}

    template <class U>
    internal_scheduler_aspect(U const& u):
        std::shared_ptr<boost::asynchronous::internal_scheduler_aspect_concept<T> > (u){}
};

/*!
 * \class any_shared_scheduler_proxy_concept
 * This class is the interface class of a scheduler proxy implementation. Currently, only scheduler_shared_proxy
 * inherits this interface. As template argument, the job type is optional.
 * Default job type is BOOST_ASYNCHRONOUS_DEFAULT_JOB, which can be overwritten but defaults to any_callable.
 */
template <class JOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB>
struct any_shared_scheduler_proxy_concept
{
    /*!
     * \brief virtual destructor
     */
    virtual ~any_shared_scheduler_proxy_concept<JOB>(){}

    /*!
     * \brief posts a job into the scheduler queue
     * \param job passed by value
     */
    virtual void post(JOB) const =0;

    /*!
     * \brief posts a job into the scheduler queue with the given priority
     * \param job passed by value
     * \param priority. Depending on the scheduler, it can be queue or sub-pool composite_threadpool_scheduler for example). 0 means dont'care
     */
    virtual void post(JOB, std::size_t) const =0;

    /*!
     * \brief posts an interruptible job into the scheduler queue
     * \param job passed by value
     * \return any_interruptible. Can be used to interrupt the job
     */
    virtual boost::asynchronous::any_interruptible interruptible_post(JOB) const =0;

    /*!
     * \brief posts an interruptible job into the scheduler queue with the given priority
     * \param job passed by value
     * \param priority. Depending on the scheduler, it can be queue or sub-pool composite_threadpool_scheduler for example). 0 means dont'care
     * \return any_interruptible. Can be used to interrupt the job
     */
    virtual boost::asynchronous::any_interruptible interruptible_post(JOB, std::size_t) const =0;

    /*!
     * \brief returns the ids of the threads run by this scheduler
     * \return std::vector of thread ids
     */
    virtual std::vector<boost::thread::id> thread_ids() const =0;

    /*!
     * \brief returns a weak scheduler. A weak scheduler is to a shared scheduler what weak_ptr is to a shared_ptr
     * \return any_weak_scheduler
     */
    virtual boost::asynchronous::any_weak_scheduler<JOB> get_weak_scheduler() const = 0;

    /*!
     * \brief returns whether the scheduler proxy is a proxy to a valid scheduler
     * \return bool
     */
    virtual bool is_valid() const =0;

    /*!
     * \brief returns the number of waiting jobs in every queue
     * \return a std::vector containing the queue sizes
     */
    virtual std::vector<std::size_t> get_queue_size()const=0;

    /*!
     * \brief returns the max number of waiting jobs in every queue
     * \return a std::vector containing the max queue sizes
     */
    virtual std::vector<std::size_t> get_max_queue_size()const=0;

    /*!
     * \brief reset the max queue sizes
     */
    virtual void reset_max_queue_size()=0;

    /*!
     * \brief returns the diagnostics for this scheduler
     * \return a scheduler_diagnostics containing totals or current diagnostics
     */
    virtual boost::asynchronous::scheduler_diagnostics get_diagnostics(std::size_t =0)const =0;

    /*!
     * \brief reset the diagnostics
     */
    virtual void clear_diagnostics() =0;

    /*!
     * \brief returns a reduced scheduler interface for internal needs
     */
    virtual boost::asynchronous::internal_scheduler_aspect<JOB> get_internal_scheduler_aspect() =0;

    /*!
     * \brief sets a name to this scheduler. On posix, this will set the name with prctl for every thread of this scheduler
     * \param name as string
     */
    virtual void set_name(std::string const&)=0;

    /*!
     * \brief returns the name of this scheduler
     * \return name as string
     */
    virtual std::string get_name()const =0;

    /*!
     * \brief binds threads of this scheduler to processors, starting with the given id from.
     * \brief thread 0 will be bound to from, thread 1 to from + 1, etc.
     * \param start id
     */
    virtual void processor_bind(std::vector<std::tuple<unsigned int,unsigned int>> /*from*/)=0;

    /*!
     * \brief Executes callable (no logging) in each thread of a scheduler.
     * \brief Useful to register something into the TLS of each thread.
     * \param c callable object
     * \return futures indicating when tasks have been executed
     */
    virtual std::vector<std::future<void>> execute_in_all_threads(boost::asynchronous::any_callable)=0;
};
template <class JOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB>
class any_shared_scheduler_proxy
{
public:
    typedef JOB job_type;

    any_shared_scheduler_proxy():my_ptr(){}
    any_shared_scheduler_proxy(any_shared_scheduler_proxy const& other):my_ptr(other.my_ptr){}
    template <class U>
    any_shared_scheduler_proxy(U const& u):
        my_ptr (u){}

    /*!
     * \brief resets this proxy and releases the scheduler if no other proxy references it
     */
    void reset()
    {
        my_ptr.reset();
    }

    /*!
     * \brief posts a job into the scheduler queue
     * \param job passed by value
     */
    void post(JOB job) const
    {
        (*my_ptr).post(std::move(job));
    }

    /*!
     * \brief posts a job into the scheduler queue with the given priority
     * \param job passed by value
     * \param priority. Depending on the scheduler, it can be queue or sub-pool composite_threadpool_scheduler for example). 0 means dont'care
     */
    void post(JOB job, std::size_t priority) const
    {
        (*my_ptr).post(std::move(job),priority);
    }

    /*!
     * \brief posts an interruptible job into the scheduler queue
     * \param job passed by value
     * \return any_interruptible. Can be used to interrupt the job
     */
    boost::asynchronous::any_interruptible interruptible_post(JOB job) const
    {
        return (*my_ptr).interruptible_post(std::move(job));
    }

    /*!
     * \brief posts an interruptible job into the scheduler queue with the given priority
     * \param job passed by value
     * \param priority. Depending on the scheduler, it can be queue or sub-pool composite_threadpool_scheduler for example). 0 means dont'care
     * \return any_interruptible. Can be used to interrupt the job
     */
    boost::asynchronous::any_interruptible interruptible_post(JOB job, std::size_t priority) const
    {
        return (*my_ptr).interruptible_post(std::move(job),priority);
    }

    /*!
     * \brief returns the ids of the threads run by this scheduler
     * \return std::vector of thread ids
     */
    std::vector<boost::thread::id> thread_ids() const
    {
        return (*my_ptr).thread_ids();
    }

    /*!
     * \brief returns a weak scheduler. A weak scheduler is to a shared scheduler what weak_ptr is to a shared_ptr
     * \return any_weak_scheduler
     */
    boost::asynchronous::any_weak_scheduler<JOB> get_weak_scheduler() const
    {
        return (*my_ptr).get_weak_scheduler();
    }

    /*!
     * \brief returns whether the scheduler proxy is a proxy to a valid scheduler
     * \return bool
     */
    bool is_valid() const
    {
        return !!my_ptr && (*my_ptr).is_valid();
    }

    /*!
     * \brief returns the number of waiting jobs in every queue
     * \return a std::vector containing the queue sizes
     */
    std::vector<std::size_t> get_queue_size()const
    {
        return (*my_ptr).get_queue_size();
    }

    /*!
     * \brief returns the max number of waiting jobs in every queue
     * \return a std::vector containing the max queue sizes
     */
    std::vector<std::size_t> get_max_queue_size()const
    {
        return (*my_ptr).get_max_queue_size();
    }

    /*!
     * \brief reset the max queue sizes
     */
    void reset_max_queue_size()
    {
        (*my_ptr).reset_max_queue_size();
    }

    /*!
     * \brief returns the diagnostics for this scheduler
     * \return a scheduler_diagnostics containing totals or current diagnostics
     */
    boost::asynchronous::scheduler_diagnostics get_diagnostics(std::size_t prio=0)const
    {
        return (*my_ptr).get_diagnostics(prio);
    }

    /*!
     * \brief reset the diagnostics
     */
    void clear_diagnostics()
    {
        (*my_ptr).clear_diagnostics();
    }

    /*!
     * \brief returns a reduced scheduler interface for internal needs
     */
    boost::asynchronous::internal_scheduler_aspect<JOB> get_internal_scheduler_aspect()
    {
        return (*my_ptr).get_internal_scheduler_aspect();
    }

    /*!
     * \brief sets a name to this scheduler. On posix, this will set the name with prctl for every thread of this scheduler
     * \param name as string
     */
    void set_name(std::string const& name)
    {
        (*my_ptr).set_name(name);
    }

    /*!
     * \brief returns the name of this scheduler
     * \return name as string
     */
    std::string get_name()const
    {
        return (*my_ptr).get_name();
    }

    /*!
     * \brief binds threads of this scheduler to processors, starting with the given id from.
     * \brief thread 0 will be bound to from, thread 1 to from + 1, etc.
     * \param start id
     */
    void processor_bind(std::vector<std::tuple<unsigned int,unsigned int>> p)
    {
        (*my_ptr).processor_bind(std::move(p));
    }

    /*!
     * \brief Executes callable (no logging) in each thread of a scheduler.
     * \brief Useful to register something into the TLS of each thread.
     * \param c callable object
     * \return futures indicating when tasks have been executed
     */
    std::vector<std::future<void>> execute_in_all_threads(boost::asynchronous::any_callable c)
    {
        return (*my_ptr).execute_in_all_threads(std::move(c));
    }
private:
    template <class J>
    friend class scheduler_weak_proxy;

    std::shared_ptr<boost::asynchronous::any_shared_scheduler_proxy_concept<JOB> > my_ptr;
};

/*!
 * \brief any_shared_scheduler_proxy equal operator. Uses thread ids of the schedulers for comparison.
 */
template <class JOB>
inline bool operator==(const any_shared_scheduler_proxy<JOB>& lhs, const any_shared_scheduler_proxy<JOB>& rhs)
{
    return lhs.thread_ids() == rhs.thread_ids();
}

/*!
 * \brief any_shared_scheduler_proxy not equal operator. Uses thread ids of the schedulers for comparison.
 */
template <class JOB>
inline bool operator!=(const any_shared_scheduler_proxy<JOB>& lhs, const any_shared_scheduler_proxy<JOB>& rhs)
{
    return lhs.thread_ids() != rhs.thread_ids();
}

// weak pointer to any_shared_scheduler_proxy concept
template <class JOB>
struct any_weak_scheduler_proxy_concept :
 ::boost::mpl::vector<
    boost::type_erasure::relaxed,
    boost::type_erasure::copy_constructible<>,
    boost::type_erasure::typeid_<>,
    boost::asynchronous::has_lock<any_shared_scheduler_proxy<JOB>(), const boost::type_erasure::_self>
> {};
template <class T = BOOST_ASYNCHRONOUS_DEFAULT_JOB>
struct any_weak_scheduler_proxy: boost::type_erasure::any<boost::asynchronous::any_weak_scheduler_proxy_concept<T> >
{
    typedef T job_type;
    template <class U>
    any_weak_scheduler_proxy(U const& u)
        : boost::type_erasure::any<boost::asynchronous::any_weak_scheduler_proxy_concept<T> > (u){}
    any_weak_scheduler_proxy()
        : boost::type_erasure::any<boost::asynchronous::any_weak_scheduler_proxy_concept<T> > (){}
};

#endif
}} // boost::asynchronous


#endif // BOOST_ASYNC_ANY_SHARED_SCHEDULER_PROXY_HPP
