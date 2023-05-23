// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRON_CHECKS_HPP
#define BOOST_ASYNCHRON_CHECKS_HPP

#include <type_traits>
#include <string>


#include <memory>

#include <boost/asynchronous/exceptions.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>

// call_if_alive provides a protection for trackable_servant: check if the servant is still alive before calling it
// (through a post_callback for example).
// call_if_alive_exec provides an optimization: execute a task posted by a servant only if the servant is still alive.
// This is done through use of a weak pointer to the servant.

namespace boost { namespace asynchronous
{
// TODO move to details
template <class Func, class T,class R>
struct call_if_alive
{
#ifndef BOOST_NO_RVALUE_REFERENCES
    call_if_alive(Func f, std::weak_ptr<T> tracked):m_wrapped(std::move(f)),m_tracked(tracked){}
#else
    call_if_alive(Func const& f, std::weak_ptr<T> tracked):m_wrapped(f),m_tracked(tracked){}
#endif
    call_if_alive(call_if_alive&& rhs)noexcept
        : m_wrapped(std::move(rhs.m_wrapped))
        , m_tracked(std::move(rhs.m_tracked))
    {}
    call_if_alive(call_if_alive const& rhs)noexcept
        : m_wrapped(std::move(const_cast<call_if_alive&>(rhs).m_wrapped))
        , m_tracked(std::move(const_cast<call_if_alive&>(rhs).m_tracked))
    {}
    call_if_alive& operator= (call_if_alive&& rhs)noexcept
    {
        std::swap(m_wrapped,rhs.m_wrapped);
        std::swap(m_tracked,rhs.m_tracked);
        return *this;
    }
    call_if_alive& operator= (call_if_alive const& rhs)noexcept
    {
        std::swap(m_wrapped,std::move(const_cast<call_if_alive&>(rhs).m_wrapped));
        std::swap(m_tracked,std::move(const_cast<call_if_alive&>(rhs).m_tracked));
        return *this;
    }
    template <typename... Arg>
    R operator()(Arg... arg)
    {
        // call only if tracked object is alive
        if (!m_tracked.expired())
            return m_wrapped(std::move(arg)...);
        return R();
    }

    Func m_wrapped;
    std::weak_ptr<T> m_tracked;
};
template <class Func, class T>
struct call_if_alive<Func,T,void>
{
#ifndef BOOST_NO_RVALUE_REFERENCES
    call_if_alive(Func f, std::weak_ptr<T> tracked):m_wrapped(std::move(f)),m_tracked(tracked){}
#else
    call_if_alive(Func const& f, std::weak_ptr<T> tracked):m_wrapped(f),m_tracked(tracked){}
#endif
    call_if_alive(call_if_alive&& rhs)noexcept
        : m_wrapped(std::move(rhs.m_wrapped))
        , m_tracked(std::move(rhs.m_tracked))
    {}
    call_if_alive(call_if_alive const& rhs)noexcept
        : m_wrapped(std::move(const_cast<call_if_alive&>(rhs).m_wrapped))
        , m_tracked(std::move(const_cast<call_if_alive&>(rhs).m_tracked))
    {}
    call_if_alive& operator= (call_if_alive&& rhs)noexcept
    {
        std::swap(m_wrapped,rhs.m_wrapped);
        std::swap(m_tracked,rhs.m_tracked);
        return *this;
    }
    call_if_alive& operator= (call_if_alive const& rhs)noexcept
    {
        std::swap(m_wrapped,std::move(const_cast<call_if_alive&>(rhs).m_wrapped));
        std::swap(m_tracked,std::move(const_cast<call_if_alive&>(rhs).m_tracked));
        return *this;
    }
    template <typename... Arg>
    void operator()(Arg... arg)
    {
        // call only if tracked object is alive
        if (!m_tracked.expired())
            m_wrapped(std::move(arg)...);
    }
    void operator()()
    {
        // call only if tracked object is alive
        if (!m_tracked.expired())
            m_wrapped();
    }
    Func m_wrapped;
    std::weak_ptr<T> m_tracked;
};

template <class Func, class T,class R,class Enable=void>
struct call_if_alive_exec
{
    typedef typename boost::asynchronous::detail::get_return_type<Func>::type return_type;
    // pretend we are a continuation as we do not know and return the correct type above
    // TODO a bit better...
    typedef int is_continuation_task;
#ifndef BOOST_NO_RVALUE_REFERENCES
    call_if_alive_exec(Func&& f, std::weak_ptr<T> tracked):m_wrapped(std::forward<Func>(f)),m_tracked(tracked){}
#else
    call_if_alive_exec(Func const& f, std::weak_ptr<T> tracked):m_wrapped(f),m_tracked(tracked){}
#endif
    call_if_alive_exec(call_if_alive_exec&& rhs)noexcept
        : m_wrapped(std::move(rhs.m_wrapped))
        , m_tracked(std::move(rhs.m_tracked))
    {}
    call_if_alive_exec(call_if_alive_exec const& rhs)noexcept
        : m_wrapped(std::move(const_cast<call_if_alive_exec&>(rhs).m_wrapped))
        , m_tracked(std::move(const_cast<call_if_alive_exec&>(rhs).m_tracked))
    {}
    call_if_alive_exec& operator= (call_if_alive_exec&& rhs)noexcept
    {
        std::swap(m_wrapped,rhs.m_wrapped);
        std::swap(m_tracked,rhs.m_tracked);
        return *this;
    }
    call_if_alive_exec& operator= (call_if_alive_exec const& rhs)noexcept
    {
        std::swap(m_wrapped,std::move(const_cast<call_if_alive_exec&>(rhs).m_wrapped));
        std::swap(m_tracked,std::move(const_cast<call_if_alive_exec&>(rhs).m_tracked));
        return *this;
    }
    R operator()()
    {
        // call only if tracked object is alive
        if (!m_tracked.expired())
        {
            return m_wrapped();
        }
        // throw exception, will be caught, then ignored
        ASYNCHRONOUS_THROW(boost::asynchronous::task_aborted_exception());
    }
    Func m_wrapped;
    std::weak_ptr<T> m_tracked;
};

#if defined BOOST_ASYNCHRONOUS_USE_PORTABLE_BINARY_ARCHIVE || defined BOOST_ASYNCHRONOUS_USE_BINARY_ARCHIVE || defined BOOST_ASYNCHRONOUS_USE_DEFAULT_ARCHIVE
template <class Func, class T,class R>
struct call_if_alive_exec<Func,T,R,typename std::enable_if<boost::asynchronous::detail::is_serializable<Func>::value >::type>
{
    typedef typename boost::asynchronous::detail::get_return_type<Func>::type return_type;
    // pretend we are a continuation as we do not know and return the correct type above
    // TODO a bit better...
    typedef int is_continuation_task;

#ifndef BOOST_NO_RVALUE_REFERENCES
    call_if_alive_exec(Func&& f, std::weak_ptr<T> tracked):m_wrapped(std::forward<Func>(f)),m_tracked(tracked){}
#else
    call_if_alive_exec(Func const& f, std::weak_ptr<T> tracked):m_wrapped(f),m_tracked(tracked){}
#endif
    call_if_alive_exec(call_if_alive_exec&& rhs)noexcept
        : m_wrapped(std::move(rhs.m_wrapped))
        , m_tracked(std::move(rhs.m_tracked))
    {}
    call_if_alive_exec(call_if_alive_exec const& rhs)noexcept
        : m_wrapped(std::move(const_cast<call_if_alive_exec&>(rhs).m_wrapped))
        , m_tracked(std::move(const_cast<call_if_alive_exec&>(rhs).m_tracked))
    {}
    call_if_alive_exec& operator= (call_if_alive_exec&& rhs)noexcept
    {
        std::swap(m_wrapped,rhs.m_wrapped);
        std::swap(m_tracked,rhs.m_tracked);
        return *this;
    }
    call_if_alive_exec& operator= (call_if_alive_exec const& rhs)noexcept
    {
        std::swap(m_wrapped,std::move(const_cast<call_if_alive_exec&>(rhs).m_wrapped));
        std::swap(m_tracked,std::move(const_cast<call_if_alive_exec&>(rhs).m_tracked));
        return *this;
    }
    R operator()()
    {
        // call only if tracked object is alive
        if (!m_tracked.expired())
        {
            return m_wrapped();
        }
        // throw exception, will be caught, then ignored
        ASYNCHRONOUS_THROW(boost::asynchronous::task_aborted_exception());
    }
    typedef int serializable_type;

    template <class Archive>
    void save(Archive & ar, const unsigned int version)const
    {
        const_cast<Func&>(m_wrapped).serialize(ar,version);
    }
    template <class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        m_wrapped.serialize(ar,version);
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
    std::string get_task_name()const
    {
        return m_wrapped.get_task_name();
    }
    Func m_wrapped;
    std::weak_ptr<T> m_tracked;
};
#endif

#ifndef BOOST_NO_RVALUE_REFERENCES
template <class F, class T,class R=void>
call_if_alive<F,T,R> check_alive(F func, std::shared_ptr<T> tracked)
{
    call_if_alive<F,T,R> wrapped(std::move(func),std::weak_ptr<T>(tracked));
    return wrapped;
}
template <class F, class T,class R=void>
call_if_alive<F,T,R> check_alive(F func, std::weak_ptr<T> tracked)
{
    call_if_alive<F,T,R> wrapped(std::move(func),tracked);
    return wrapped;
}
#else
template <class F, class T,class R=void>
call_if_alive<F,T,R> check_alive(F const& func, std::shared_ptr<T> tracked)
{
    call_if_alive<F,T,R> wrapped(func,std::weak_ptr<T>(tracked));
    return wrapped;
}
template <class F, class T,class R=void>
call_if_alive<F,T,R> check_alive(F const& func, std::weak_ptr<T> tracked)
{
    call_if_alive<F,T,R> wrapped(func,tracked);
    return wrapped;
}
#endif


#ifndef BOOST_NO_RVALUE_REFERENCES
template <class F, class T>
auto check_alive_before_exec(F func, std::shared_ptr<T> tracked) -> call_if_alive_exec<F,T,decltype(func())>
{
    call_if_alive_exec<F,T,decltype(func())> wrapped(std::move(func),std::weak_ptr<T>(tracked));
    return wrapped;
}
template <class F, class T>
auto check_alive_before_exec(F func, std::weak_ptr<T> tracked) -> call_if_alive_exec<F,T,decltype(func())>
{
    call_if_alive_exec<F,T,decltype(func())> wrapped(std::move(func),tracked);
    return std::move(wrapped);
}
#else
template <class F, class T>
auto check_alive_before_exec(F const& func, std::shared_ptr<T> tracked) -> call_if_alive_exec<F,T,decltype(func())>
{
    call_if_alive_exec<F,T,decltype(func())> wrapped(func,std::weak_ptr<T>(tracked));
    return wrapped;
}
template <class F, class T>
auto check_alive_before_exec(F const& func, std::weak_ptr<T> tracked) -> call_if_alive_exec<F,T,decltype(func())>
{
    call_if_alive_exec<F,T,decltype(func())> wrapped(func,tracked);
    return wrapped;
}
#endif


}}
#endif // BOOST_ASYNCHRON_CHECKS_HPP
