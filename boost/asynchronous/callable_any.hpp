// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_CALLABLE_ANY_HPP
#define BOOST_ASYNC_CALLABLE_ANY_HPP

#include <boost/mpl/vector.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/constructible.hpp>
#include <boost/type_erasure/relaxed.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/member.hpp>
#include <boost/type_erasure/callable.hpp>

// the basic and minimum job type of every scheduler
// The minimum a job has to do is to be callable: void task()

namespace boost { namespace asynchronous
{

typedef ::boost::mpl::vector<
    boost::type_erasure::callable<void()>,
    boost::type_erasure::relaxed,
    boost::type_erasure::copy_constructible<>,
    boost::type_erasure::typeid_<>
> any_callable_concept;
typedef boost::type_erasure::any<any_callable_concept> any_callable_helper;

struct any_callable
{
    any_callable():m_inner(){}
    template <class T>
    any_callable(T t):m_inner(std::move(t)){}
    any_callable(any_callable&& rhs)noexcept
        : m_inner(std::move(rhs.m_inner))
    {}
    any_callable& operator=(any_callable&& rhs)
    {
        std::swap(m_inner,rhs.m_inner);
        return *this;
    }
    any_callable(any_callable const& rhs)noexcept
        : m_inner(std::move(const_cast<any_callable&>(rhs).m_inner))
    {}
    any_callable& operator=(any_callable const& rhs)
    {
        m_inner= std::move(const_cast<any_callable&>(rhs).m_inner);
        return *this;
    }
    void operator()()
    {
        m_inner();
    }
private:
    boost::asynchronous::any_callable_helper m_inner;
};

}} // boost::async

#ifndef BOOST_ASYNCHRONOUS_DEFAULT_JOB
#define BOOST_ASYNCHRONOUS_DEFAULT_JOB boost::asynchronous::any_callable
#endif

#endif /* BOOST_ASYNC_CALLABLE_ANY_HPP */
