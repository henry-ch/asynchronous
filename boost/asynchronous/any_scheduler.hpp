// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_ANY_SCHEDULER_HPP
#define BOOST_ASYNC_ANY_SCHEDULER_HPP

#include <string>
#include <map>
#include <list>
#include <cstddef>

#include <boost/pointee.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/chrono/chrono.hpp>
#include <boost/thread/thread.hpp>

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/constructible.hpp>
#include <boost/type_erasure/relaxed.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/callable.hpp>
#include <boost/type_erasure/deduced.hpp>

#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/diagnostics/any_loggable.hpp>
#include <boost/asynchronous/detail/any_pointer.hpp>
#include <boost/asynchronous/detail/concept_members.hpp>
#include <boost/asynchronous/detail/any_interruptible.hpp>
#include <boost/asynchronous/scheduler_diagnostics.hpp>

namespace boost { namespace asynchronous
{
#ifdef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
template <class JOB,class Clock>
struct any_shared_scheduler_concept :
 ::boost::mpl::vector<
    boost::asynchronous::pointer<>,
    boost::type_erasure::same_type<boost::asynchronous::pointer<>::element_type,boost::type_erasure::_a >,
    boost::type_erasure::relaxed,
    boost::type_erasure::typeid_<>,
#ifndef BOOST_NO_RVALUE_REFERENCES
    boost::asynchronous::has_post<void(JOB&&), boost::type_erasure::_a>,
    boost::asynchronous::has_post<void(JOB&&, std::size_t), boost::type_erasure::_a>,
    boost::asynchronous::has_interruptible_post<boost::asynchronous::any_interruptible(JOB), boost::type_erasure::_a>,
    boost::asynchronous::has_interruptible_post<boost::asynchronous::any_interruptible(JOB, std::size_t),
                                             boost::type_erasure::_a>,
#else
    boost::asynchronous::has_post<void(JOB), boost::type_erasure::_a>,
    boost::asynchronous::has_post<void(JOB, std::size_t), boost::type_erasure::_a>,
    boost::asynchronous::has_interruptible_post<boost::asynchronous::any_interruptible(JOB), boost::type_erasure::_a>,
    boost::asynchronous::has_interruptible_post<boost::asynchronous::any_interruptible(JOB, std::size_t),
                                             boost::type_erasure::_a>,
#endif
    boost::asynchronous::has_thread_ids<std::vector<boost::thread::id>(), const boost::type_erasure::_a>,
#ifndef BOOST_NO_RVALUE_REFERENCES
    boost::asynchronous::has_get_diagnostics<boost::asynchronous::scheduler_diagnostics<JOB>(std::size_t),
                                      const boost::type_erasure::_a>,
#endif
    boost::asynchronous::has_clear_diagnostics<void(), boost::type_erasure::_a>,
    boost::asynchronous::has_get_queue_size<std::vector<std::size_t>(), const boost::type_erasure::_a>,
    boost::asynchronous::has_get_diagnostics<boost::asynchronous::scheduler_diagnostics<JOB>(),
                                          const boost::type_erasure::_a>
> {};

template <class T = BOOST_ASYNCHRONOUS_DEFAULT_JOB, class Clock = boost::chrono::high_resolution_clock >
struct any_shared_scheduler_ptr: boost::type_erasure::any<boost::asynchronous::any_shared_scheduler_concept<T,Clock> >
{
    typedef T job_type;
    typedef Clock clock_type;
    template <class U>
    any_shared_scheduler_ptr(U const& u): boost::type_erasure::any<boost::asynchronous::any_shared_scheduler_concept<T,Clock> > (u){}
    any_shared_scheduler_ptr(): boost::type_erasure::any<boost::asynchronous::any_shared_scheduler_concept<T,Clock> > (){}

};
#else
template <class JOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB,class Clock = boost::chrono::high_resolution_clock>
struct any_shared_scheduler_concept
{
    virtual ~any_shared_scheduler_concept<JOB,Clock>(){}
    virtual void post(JOB) =0;
    virtual void post(JOB, std::size_t) =0;
    virtual boost::asynchronous::any_interruptible interruptible_post(JOB) =0;
    virtual boost::asynchronous::any_interruptible interruptible_post(JOB, std::size_t) =0;
    
    virtual std::vector<boost::thread::id> thread_ids() const =0;
    virtual std::vector<std::size_t> get_queue_size()const=0;
    virtual boost::asynchronous::scheduler_diagnostics<JOB> get_diagnostics(std::size_t =0)const =0;
    virtual void clear_diagnostics() =0;
};
template <class T = BOOST_ASYNCHRONOUS_DEFAULT_JOB,class Clock = boost::chrono::high_resolution_clock>
struct any_shared_scheduler_ptr: boost::shared_ptr<boost::asynchronous::any_shared_scheduler_concept<T,Clock> >
{
    typedef T job_type;
    typedef Clock clock_type;
    any_shared_scheduler_ptr():
        boost::shared_ptr<boost::asynchronous::any_shared_scheduler_concept<T,Clock> > (){}

    template <class U>
    any_shared_scheduler_ptr(U const& u):
        boost::shared_ptr<boost::asynchronous::any_shared_scheduler_concept<T,Clock> > (u){}
};
#endif

template <class JOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB,class Clock = boost::chrono::high_resolution_clock>
class any_shared_scheduler
{
public:
    typedef JOB job_type;
    typedef Clock clock_type;

    any_shared_scheduler(any_shared_scheduler_ptr<JOB,Clock> ptr):my_ptr(ptr){}
    any_shared_scheduler():my_ptr(){}
    any_shared_scheduler(any_shared_scheduler const& other):my_ptr(other.my_ptr){}

    void reset()
    {
        my_ptr.reset();
    }
    bool is_valid()const
    {
#ifndef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
        return !!my_ptr;
#else
        return my_ptr.is_valid();
#endif
    }
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
    std::vector<std::size_t> get_queue_size()const
    {
        return (*my_ptr).get_queue_size();
    }
    boost::asynchronous::scheduler_diagnostics<JOB> get_diagnostics(std::size_t prio=0)const
    {
        return (*my_ptr).get_diagnostics(prio);
    }
    void clear_diagnostics()
    {
        (*my_ptr).clear_diagnostics();
    }

private:
    any_shared_scheduler_ptr<JOB,Clock> my_ptr;
};

template <class JOB,class Clock>
struct any_weak_scheduler_concept :
 ::boost::mpl::vector<
    boost::type_erasure::relaxed,
    boost::type_erasure::copy_constructible<>,
    boost::type_erasure::typeid_<>,
    boost::asynchronous::has_lock<any_shared_scheduler<JOB>(), const boost::type_erasure::_self>
> {};
template <class T = BOOST_ASYNCHRONOUS_DEFAULT_JOB, class Clock = boost::chrono::high_resolution_clock>
struct any_weak_scheduler: boost::type_erasure::any<boost::asynchronous::any_weak_scheduler_concept<T,Clock> >
{
    typedef T job_type;
    typedef Clock clock_type;
    template <class U>
    any_weak_scheduler(U const& u): boost::type_erasure::any<boost::asynchronous::any_weak_scheduler_concept<T,Clock> > (u){}
    any_weak_scheduler(): boost::type_erasure::any<boost::asynchronous::any_weak_scheduler_concept<T,Clock> > (){}
};
}} // boost::async

#endif // BOOST_ASYNC_ANY_SCHEDULER_HPP
