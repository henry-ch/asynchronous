// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
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
#include <boost/shared_ptr.hpp>
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

namespace boost { namespace asynchronous
{
#ifndef BOOST_ASYNCHRONOUS_NO_TYPE_ERASURE
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

template <class JOB,class Clock>
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
    boost::asynchronous::has_get_queue_size<std::size_t(), const boost::type_erasure::_a>,
    boost::asynchronous::has_reset<void()>,
    boost::asynchronous::has_clear_diagnostics<void(), boost::type_erasure::_a>,
#ifndef BOOST_NO_RVALUE_REFERENCES
    boost::asynchronous::has_get_diagnostics<std::map<std::string,
                                                   std::list<boost::asynchronous::diagnostic_item<Clock> > >(std::size_t),
                                          const boost::type_erasure::_a>,
#endif
    boost::asynchronous::has_get_diagnostics<std::map<std::string,
                                                   std::list<boost::asynchronous::diagnostic_item<Clock> > >(),
                                          const boost::type_erasure::_a>,
    boost::asynchronous::has_get_internal_scheduler_aspect<boost::asynchronous::internal_scheduler_aspect<JOB>(), boost::type_erasure::_a>,
    boost::asynchronous::has_set_name<void(std::string const&), boost::type_erasure::_a>,
    boost::type_erasure::relaxed,
    boost::type_erasure::copy_constructible<>
>{} ;

template <class T = BOOST_ASYNCHRONOUS_DEFAULT_JOB,class Clock = boost::chrono::high_resolution_clock>
struct any_shared_scheduler_proxy_ptr: boost::type_erasure::any<boost::asynchronous::any_shared_scheduler_proxy_concept<T,Clock> >
{
    typedef T job_type;
    typedef Clock clock_type;
    any_shared_scheduler_proxy_ptr():
        boost::type_erasure::any<boost::asynchronous::any_shared_scheduler_proxy_concept<T,Clock> > (){}

    template <class U>
    any_shared_scheduler_proxy_ptr(U const& u):
        boost::type_erasure::any<boost::asynchronous::any_shared_scheduler_proxy_concept<T,Clock> > (u){}
};
template <class JOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB,class Clock = boost::chrono::high_resolution_clock>
class any_shared_scheduler_proxy
{
public:
    typedef JOB job_type;
    typedef Clock clock_type;

    any_shared_scheduler_proxy(any_shared_scheduler_proxy_ptr<JOB,Clock> ptr):my_ptr(ptr){}
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
    std::size_t get_queue_size()const
    {
        return (*my_ptr).get_queue_size();
    }
    std::map<std::string,std::list<boost::asynchronous::diagnostic_item<Clock> > > get_diagnostics(std::size_t prio=0)const
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
private:
    any_shared_scheduler_proxy_ptr<JOB,Clock> my_ptr;
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
        : boost::shared_ptr<boost::asynchronous::internal_scheduler_aspect_concept<T> >
{
    internal_scheduler_aspect():
        boost::shared_ptr<boost::asynchronous::internal_scheduler_aspect_concept<T> > (){}

    template <class U>
    internal_scheduler_aspect(U const& u):
        boost::shared_ptr<boost::asynchronous::internal_scheduler_aspect_concept<T> > (u){}
};

template <class JOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB,class Clock = boost::chrono::high_resolution_clock>
struct any_shared_scheduler_proxy_concept
{
    virtual ~any_shared_scheduler_proxy_concept<JOB,Clock>(){}

    virtual void post(JOB) const =0;
    virtual void post(JOB, std::size_t) const =0;
    virtual boost::asynchronous::any_interruptible interruptible_post(JOB) const =0;
    virtual boost::asynchronous::any_interruptible interruptible_post(JOB, std::size_t) const =0;
    virtual std::vector<boost::thread::id> thread_ids() const =0;
    virtual boost::asynchronous::any_weak_scheduler<JOB> get_weak_scheduler() const = 0;
    virtual bool is_valid() const =0;
    virtual std::size_t get_queue_size()const=0;
    virtual std::map<std::string,std::list<boost::asynchronous::diagnostic_item<Clock> > > get_diagnostics(std::size_t =0)const =0;
    virtual void clear_diagnostics() =0;
    virtual boost::asynchronous::internal_scheduler_aspect<JOB> get_internal_scheduler_aspect() =0;
    virtual void set_name(std::string const&)=0;
};
template <class JOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB,class Clock = boost::chrono::high_resolution_clock>
class any_shared_scheduler_proxy
{
public:
    typedef JOB job_type;
    typedef Clock clock_type;

    any_shared_scheduler_proxy():my_ptr(){}
    any_shared_scheduler_proxy(any_shared_scheduler_proxy const& other):my_ptr(other.my_ptr){}
    template <class U>
    any_shared_scheduler_proxy(U const& u):
        my_ptr (u){}

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
        return !!my_ptr && (*my_ptr).is_valid();
    }
    std::size_t get_queue_size()const
    {
        return (*my_ptr).get_queue_size();
    }
    std::map<std::string,std::list<boost::asynchronous::diagnostic_item<Clock> > > get_diagnostics(std::size_t prio=0)const
    {
        return (*my_ptr).get_diagnostics(prio);
    }
    void clear_diagnostics()
    {
        (*my_ptr).clear_diagnostics();
    }
    boost::asynchronous::internal_scheduler_aspect<JOB> get_internal_scheduler_aspect()
    {
        return (*my_ptr).get_internal_scheduler_aspect();
    }
    void set_name(std::string const& name)
    {
        (*my_ptr).set_name(name);
    }

private:
    template <class J,class C>
    friend class scheduler_weak_proxy;

    boost::shared_ptr<boost::asynchronous::any_shared_scheduler_proxy_concept<JOB,Clock> > my_ptr;
};

// weak pointer to any_shared_scheduler_proxy concept
template <class JOB,class Clock>
struct any_weak_scheduler_proxy_concept :
 ::boost::mpl::vector<
    boost::type_erasure::relaxed,
    boost::type_erasure::copy_constructible<>,
    boost::type_erasure::typeid_<>,
    boost::asynchronous::has_lock<any_shared_scheduler_proxy<JOB,Clock>(), const boost::type_erasure::_self>
> {};
template <class T = BOOST_ASYNCHRONOUS_DEFAULT_JOB, class Clock = boost::chrono::high_resolution_clock>
struct any_weak_scheduler_proxy: boost::type_erasure::any<boost::asynchronous::any_weak_scheduler_proxy_concept<T,Clock> >
{
    typedef T job_type;
    typedef Clock clock_type;
    template <class U>
    any_weak_scheduler_proxy(U const& u)
        : boost::type_erasure::any<boost::asynchronous::any_weak_scheduler_proxy_concept<T,Clock> > (u){}
    any_weak_scheduler_proxy()
        : boost::type_erasure::any<boost::asynchronous::any_weak_scheduler_proxy_concept<T,Clock> > (){}
};

#endif
}} // boost::asynchronous


#endif // BOOST_ASYNC_ANY_SHARED_SCHEDULER_PROXY_HPP
