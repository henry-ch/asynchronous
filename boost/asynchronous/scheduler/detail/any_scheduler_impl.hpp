// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_SCHEDULER_ANY_SCHEDULER_IMPL_HPP
#define BOOST_ASYNC_SCHEDULER_ANY_SCHEDULER_IMPL_HPP

#include <string>
#include <vector>
#include <map>
#include <list>
#include <cstddef>

#include <boost/pointee.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/chrono/chrono.hpp>

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/constructible.hpp>
#include <boost/type_erasure/relaxed.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/callable.hpp>
#include <boost/type_erasure/deduced.hpp>

#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/queue/any_queue.hpp>
#include <boost/asynchronous/detail/any_pointer.hpp>
#include <boost/asynchronous/detail/any_joinable.hpp>
#include <boost/asynchronous/detail/concept_members.hpp>
#include <boost/asynchronous/detail/any_interruptible.hpp>

namespace boost { namespace asynchronous { namespace detail
{

// this defines the interface for a concrete implementation of a scheduler. Not the one the user sees, the implementation.
template <class JOB,class DiagItem>
struct any_scheduler_impl_concept :
 ::boost::mpl::vector<
    boost::asynchronous::pointer<>,
    boost::type_erasure::same_type<boost::asynchronous::pointer<>::element_type,boost::type_erasure::_a >,
    boost::type_erasure::relaxed,
    boost::type_erasure::typeid_<>,
#ifndef BOOST_NO_RVALUE_REFERENCES
    boost::asynchronous::has_add<void(JOB&&, std::size_t), boost::type_erasure::_a>,
    boost::asynchronous::has_interruptible_add<boost::asynchronous::any_interruptible(JOB&&, std::size_t),
                                            boost::type_erasure::_a>,
#else
    boost::asynchronous::has_add<void(JOB const&, std::size_t), boost::type_erasure::_a>,
    boost::asynchronous::has_interruptible_add<boost::asynchronous::any_interruptible(JOB const&, std::size_t),
                                            boost::type_erasure::_a>,
#endif
    boost::asynchronous::has_get_diagnostics<std::map<std::string,std::list<DiagItem> >(),const boost::type_erasure::_a>,
#ifndef BOOST_NO_RVALUE_REFERENCES
    boost::asynchronous::has_get_diagnostics<std::map<std::string,std::list<DiagItem> >(std::size_t),const boost::type_erasure::_a>,
#endif
    boost::asynchronous::has_get_worker<boost::asynchronous::any_joinable(),const boost::type_erasure::_a>,
    boost::asynchronous::has_thread_ids<std::vector<boost::thread::id>(), const boost::type_erasure::_a>,
    boost::asynchronous::has_get_queues<std::vector<boost::asynchronous::any_queue_ptr<JOB> >(),const boost::type_erasure::_a>,
    boost::asynchronous::has_set_steal_from_queues<void(std::vector<boost::asynchronous::any_queue_ptr<JOB> > const&),boost::type_erasure::_a>
> {};


template <class T, class DiagItem>
struct any_scheduler_impl: boost::type_erasure::any<boost::asynchronous::detail::any_scheduler_impl_concept<T,DiagItem> >
{
    typedef T job_type;
    template <class U>
    any_scheduler_impl(U const& u): boost::type_erasure::any<boost::asynchronous::detail::any_scheduler_impl_concept<T,DiagItem> > (u){}
    any_scheduler_impl(): boost::type_erasure::any<boost::asynchronous::detail::any_scheduler_impl_concept<T,DiagItem> > (){}

};

}}}
#endif // BOOST_ASYNC_SCHEDULER_ANY_SCHEDULER_IMPL_HPP
