// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_QUEUE_ANY_QUEUE_HPP
#define BOOST_ASYNC_QUEUE_ANY_QUEUE_HPP

#include <boost/mpl/vector.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/constructible.hpp>
#include <boost/type_erasure/relaxed.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/member.hpp>

#include <boost/asynchronous/detail/any_pointer.hpp>
#include <boost/asynchronous/detail/concept_members.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/queue/queue_base.hpp>

namespace boost { namespace asynchronous
{
#ifdef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
template <class JOB>
struct any_queue_ptr_concept:
  ::boost::mpl::vector<
    boost::asynchronous::pointer<>,
    boost::type_erasure::same_type<boost::asynchronous::pointer<>::element_type,boost::type_erasure::_a >,
#ifndef BOOST_NO_RVALUE_REFERENCES
    boost::asynchronous::has_push<void(JOB&&,std::size_t), boost::type_erasure::_a>,
    boost::asynchronous::has_push<void(JOB&&), boost::type_erasure::_a>,
#else
    boost::asynchronous::has_push<void(JOB const&,std::size_t), boost::type_erasure::_a>,
    boost::asynchronous::has_push<void(JOB const&), boost::type_erasure::_a>,
#endif
    boost::asynchronous::has_pop<JOB(), boost::type_erasure::_a>,
    boost::asynchronous::has_try_pop<bool(JOB&), boost::type_erasure::_a>,
    boost::asynchronous::has_try_steal<bool(JOB&), boost::type_erasure::_a>,
    boost::asynchronous::has_get_queue_size<std::vector<std::size_t>(), const boost::type_erasure::_a>,
    boost::type_erasure::relaxed,
    boost::type_erasure::copy_constructible<>,
    boost::type_erasure::typeid_<>
> {};

template <class JOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB>
struct any_queue_ptr: public boost::type_erasure::any<boost::asynchronous::any_queue_ptr_concept<JOB> >
                , public boost::asynchronous::queue_base<JOB>
{
    template <class U>
    any_queue_ptr(U const& u): boost::type_erasure::any<boost::asynchronous::any_queue_ptr_concept<JOB> > (u){}
    any_queue_ptr(): boost::type_erasure::any<boost::asynchronous::any_queue_ptr_concept<JOB> > (){}
};

template <class JOB>
struct any_queue_concept:
  ::boost::mpl::vector<
#ifndef BOOST_NO_RVALUE_REFERENCES
    boost::asynchronous::has_push<void(JOB&&,std::size_t)>,
    boost::asynchronous::has_push<void(JOB&&)>,
#else
    boost::asynchronous::has_push<void(JOB const&,std::size_t)>,
    boost::asynchronous::has_push<void(JOB const&)>,
#endif
    boost::asynchronous::has_pop<JOB()>,
    boost::asynchronous::has_try_pop<bool(JOB&)>,
    boost::asynchronous::has_get_queue_size<std::vector<std::size_t>()>,
    boost::type_erasure::relaxed,
    boost::type_erasure::copy_constructible<>,
    boost::type_erasure::typeid_<>
> {};

template <class JOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB>
struct any_queue: public boost::type_erasure::any<boost::asynchronous::any_queue_concept<JOB> >
                , public boost::asynchronous::queue_base<JOB>
{
    template <class U>
    any_queue(U const& u): boost::type_erasure::any<boost::asynchronous::any_queue_concept<JOB> > (u){}
    any_queue(): boost::type_erasure::any<boost::asynchronous::any_queue_concept<JOB> > (){}
};
#else
template <class JOB>
struct any_queue_concept
{
    virtual ~any_queue_concept<JOB>(){}
    virtual void push(JOB&&,std::size_t)=0;
    virtual void push(JOB&&)=0;
    virtual JOB pop()=0;
    virtual bool try_pop(JOB&)=0;
    virtual bool try_steal(JOB&)=0;
    virtual std::vector<std::size_t> get_queue_size()const=0;
    virtual std::vector<std::size_t> get_max_queue_size() const=0;
    virtual void reset_max_queue_size()=0;
};
template <class JOB>
struct any_queue_ptr: boost::shared_ptr<boost::asynchronous::any_queue_concept<JOB> >
{
    any_queue_ptr():
        boost::shared_ptr<boost::asynchronous::any_queue_concept<JOB> > (){}

    template <class U>
    any_queue_ptr(U const& u):
        boost::shared_ptr<boost::asynchronous::any_queue_concept<JOB> > (u){}
};

#endif
}}
#endif // BOOST_ASYNC_QUEUE_ANY_QUEUE_HPP
