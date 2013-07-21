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

#include <boost/type_traits/is_same.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/exception/all.hpp>

#include <boost/asynchronous/exceptions.hpp>

namespace boost { namespace asynchronous
{
// TODO move to details
template <class Func, class T,class R>
struct call_if_alive
{
#ifndef BOOST_NO_RVALUE_REFERENCES
    call_if_alive(Func&& f, boost::weak_ptr<T> tracked):m_wrapped(std::forward<Func>(f)),m_tracked(tracked){}
#else
    call_if_alive(Func const& f, boost::weak_ptr<T> tracked):m_wrapped(f),m_tracked(tracked){}
#endif
    template <typename... Arg>
    R operator()(Arg... arg)
    {
        // call only if tracked object is alive
        if (!m_tracked.expired())
            return m_wrapped(arg...);
        return R();
    }

    Func m_wrapped;
    boost::weak_ptr<T> m_tracked;
};
template <class Func, class T>
struct call_if_alive<Func,T,void>
{
#ifndef BOOST_NO_RVALUE_REFERENCES
    call_if_alive(Func&& f, boost::weak_ptr<T> tracked):m_wrapped(std::forward<Func>(f)),m_tracked(tracked){}
#else
    call_if_alive(Func const& f, boost::weak_ptr<T> tracked):m_wrapped(f),m_tracked(tracked){}
#endif
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
    boost::weak_ptr<T> m_tracked;
};

template <class Func, class T,class R>
struct call_if_alive_exec
{
#ifndef BOOST_NO_RVALUE_REFERENCES
    call_if_alive_exec(Func&& f, boost::weak_ptr<T> tracked):m_wrapped(std::forward<Func>(f)),m_tracked(tracked){}
#else
    call_if_alive_exec(Func const& f, boost::weak_ptr<T> tracked):m_wrapped(f),m_tracked(tracked){}
#endif

    R operator()()
    {
        // call only if tracked object is alive
        if (!m_tracked.expired())
        {
            return m_wrapped();
        }
        // throw exception, will be caught, then ignored
        boost::throw_exception(boost::asynchronous::task_aborted_exception());
    }
    Func m_wrapped;
    boost::weak_ptr<T> m_tracked;
};

#ifndef BOOST_NO_RVALUE_REFERENCES
template <class F, class T,class R=void>
call_if_alive<F,T,R> check_alive(F&& func, boost::shared_ptr<T> tracked)
{
    call_if_alive<F,T,R> wrapped(std::forward<F>(func),boost::weak_ptr<T>(tracked));
    return std::move(wrapped);
}
template <class F, class T,class R=void>
call_if_alive<F,T,R> check_alive(F&& func, boost::weak_ptr<T> tracked)
{
    call_if_alive<F,T,R> wrapped(std::forward<F>(func),tracked);
    return std::move(wrapped);
}
#else
template <class F, class T,class R=void>
call_if_alive<F,T,R> check_alive(F const& func, boost::shared_ptr<T> tracked)
{
    call_if_alive<F,T,R> wrapped(func,boost::weak_ptr<T>(tracked));
    return wrapped;
}
template <class F, class T,class R=void>
call_if_alive<F,T,R> check_alive(F const& func, boost::weak_ptr<T> tracked)
{
    call_if_alive<F,T,R> wrapped(func,tracked);
    return wrapped;
}
#endif


#ifndef BOOST_NO_RVALUE_REFERENCES
template <class F, class T>
auto check_alive_before_exec(F&& func, boost::shared_ptr<T> tracked) -> call_if_alive_exec<F,T,decltype(func())>
{
    call_if_alive_exec<F,T,decltype(func())> wrapped(std::forward<F>(func),boost::weak_ptr<T>(tracked));
    return std::move(wrapped);
}
template <class F, class T>
auto check_alive_before_exec(F&& func, boost::weak_ptr<T> tracked) -> call_if_alive_exec<F,T,decltype(func())>
{
    call_if_alive_exec<F,T,decltype(func())> wrapped(std::forward<F>(func),tracked);
    return std::move(wrapped);
}
#else
template <class F, class T>
auto check_alive_before_exec(F const& func, boost::shared_ptr<T> tracked) -> call_if_alive_exec<F,T,decltype(func())>
{
    call_if_alive_exec<F,T,decltype(func())> wrapped(func,boost::weak_ptr<T>(tracked));
    return wrapped;
}
template <class F, class T>
auto check_alive_before_exec(F const& func, boost::weak_ptr<T> tracked) -> call_if_alive_exec<F,T,decltype(func())>
{
    call_if_alive_exec<F,T,decltype(func())> wrapped(func,tracked);
    return wrapped;
}
#endif


}}
#endif // BOOST_ASYNCHRON_CHECKS_HPP
