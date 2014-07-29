// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_CONTINUATION_TASK_HPP
#define BOOST_ASYNCHRONOUS_CONTINUATION_TASK_HPP

#include <string>
#include <sstream>
#include <functional>
#include <boost/asynchronous/scheduler/tss_scheduler.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/scheduler/detail/any_continuation.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/scheduler/detail/interrupt_state.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/client_request.hpp>

namespace boost { namespace asynchronous {

// what has to be set when a task is ready
template <class Return>
struct continuation_result
{
public:
    continuation_result(boost::shared_ptr<boost::promise<Return> > p,std::function<void()> f):m_promise(p),m_done_func(f){}
    continuation_result(continuation_result&& rhs)noexcept
        : m_promise(std::move(rhs.m_promise))
        , m_done_func(std::move(rhs.m_done_func))
    {}
    continuation_result(continuation_result const& rhs)noexcept
        : m_promise(rhs.m_promise)
        , m_done_func(rhs.m_done_func)
    {}
    continuation_result& operator= (continuation_result&& rhs)noexcept
    {
        std::swap(m_promise,rhs.m_promise);
        std::swap(m_done_func,rhs.m_done_func);
        return *this;
    }
    continuation_result& operator= (continuation_result const& rhs)noexcept
    {
        m_promise = rhs.m_promise;
        m_done_func = rhs.m_done_func;
        return *this;
    }
    void set_value(Return const& val)const
    {
        m_promise->set_value(val);
        // inform caller if any
        if (m_done_func)
            (m_done_func)();
    }
    void emplace_value(Return&& val)const
    {
        m_promise->set_value(std::forward<Return>(val));
        // inform caller if any
        if (m_done_func)
            (m_done_func)();
    }
    void set_exception(boost::exception_ptr p)const
    {
        m_promise->set_exception(p);
        // inform caller if any
        if (m_done_func)
            (m_done_func)();
    }

private:
    boost::shared_ptr<boost::promise<Return> > m_promise;
    std::function<void()> m_done_func;
};
template <>
struct continuation_result<void>
{
public:
    continuation_result(boost::shared_ptr<boost::promise<void> > p,std::function<void()> f):m_promise(p),m_done_func(f){}
    continuation_result(continuation_result&& rhs)noexcept
        : m_promise(std::move(rhs.m_promise))
        , m_done_func(std::move(rhs.m_done_func))
    {}
    continuation_result(continuation_result const& rhs)noexcept
        : m_promise(rhs.m_promise)
        , m_done_func(rhs.m_done_func)
    {}
    continuation_result& operator= (continuation_result&& rhs)noexcept
    {
        std::swap(m_promise,rhs.m_promise);
        std::swap(m_done_func,rhs.m_done_func);
        return *this;
    }
    continuation_result& operator= (continuation_result const& rhs)noexcept
    {
        m_promise = rhs.m_promise;
        m_done_func = rhs.m_done_func;
        return *this;
    }
    void set_value()const
    {
        m_promise->set_value();
        // inform caller if any
        if (m_done_func)
            (m_done_func)();
    }
    void emplace_value()const
    {
        m_promise->set_value();
        // inform caller if any
        if (m_done_func)
            (m_done_func)();
    }
    void set_exception(boost::exception_ptr p)const
    {
        m_promise->set_exception(p);
        // inform caller if any
        if (m_done_func)
            (m_done_func)();
    }

private:
    boost::shared_ptr<boost::promise<void> > m_promise;
    std::function<void()> m_done_func;
};

// the base class of continuation tasks Provides typedefs and hides promise.
template <class Return>
struct continuation_task
{
public:
    typedef Return res_type;

    continuation_task(const std::string& name=""):m_promise(boost::make_shared<boost::promise<Return>>()),m_name(name){}
    continuation_task(continuation_task&& rhs)noexcept
        : m_promise(std::move(rhs.m_promise))
        , m_name(std::move(rhs.m_name))
        , m_done_func(std::move(rhs.m_done_func))
    {}
    continuation_task(continuation_task const& rhs)noexcept
        : m_promise(rhs.m_promise)
        , m_name(rhs.m_name)
        , m_done_func(rhs.m_done_func)
    {}
    continuation_task& operator= (continuation_task&& rhs)noexcept
    {
        std::swap(m_promise,rhs.m_promise);
        std::swap(m_name,rhs.m_name);
        std::swap(m_done_func,rhs.m_done_func);
        return *this;
    }
    continuation_task& operator= (continuation_task const& rhs)noexcept
    {
        m_promise = rhs.m_promise;
        m_name = rhs.m_name;
        m_done_func = rhs.m_done_func;
        return *this;
    }

    boost::future<Return> get_future()const
    {
        return m_promise->get_future();
    }

    boost::shared_ptr<boost::promise<Return> > get_promise()const
    {
        return m_promise;
    }
    std::string get_name()const
    {
        return m_name;
    }

    boost::asynchronous::continuation_result<Return> this_task_result()const
    {
        return continuation_result<Return>(m_promise,m_done_func);
    }
    // called in case task is stolen by some client and only the result is returned
    template <class Archive,class InternalArchive>
    void as_result(Archive & ar, const unsigned int /*version*/)
    {
        boost::asynchronous::tcp::client_request::message_payload payload;
        ar >> payload;
        if (!payload.m_has_exception)
        {
            std::istringstream archive_stream(payload.m_data);
            InternalArchive archive(archive_stream);

            Return res;
            archive >> res;
            get_promise()->set_value(res);
        }
        else
        {
            get_promise()->set_exception(boost::copy_exception(payload.m_exception));
        }
    }
    void set_done_func(std::function<void()> f)
    {
        m_done_func=std::move(f);
    }
private:
    boost::shared_ptr<boost::promise<Return> > m_promise;
    std::string m_name;
    std::function<void()> m_done_func;
};
template <>
struct continuation_task<void>
{
public:
    typedef void res_type;

    continuation_task(continuation_task&& rhs)noexcept
        : m_promise(std::move(rhs.m_promise))
        , m_name(std::move(rhs.m_name))
        , m_done_func(std::move(rhs.m_done_func))
    {}
    continuation_task(continuation_task const& rhs)noexcept
        : m_promise(rhs.m_promise)
        , m_name(rhs.m_name)
        , m_done_func(rhs.m_done_func)
    {}
    continuation_task& operator= (continuation_task&& rhs)noexcept
    {
        std::swap(m_promise,rhs.m_promise);
        std::swap(m_name,rhs.m_name);
        std::swap(m_done_func,rhs.m_done_func);
        return *this;
    }
    continuation_task& operator= (continuation_task const& rhs)noexcept
    {
        m_promise = rhs.m_promise;
        m_name = rhs.m_name;
        m_done_func = rhs.m_done_func;
        return *this;
    }

    continuation_task(const std::string& name=""):m_promise(boost::make_shared<boost::promise<void>>()),m_name(name){}

    boost::future<void> get_future()const
    {
        return m_promise->get_future();
    }

    boost::shared_ptr<boost::promise<void> > get_promise()const
    {
        return m_promise;
    }
    std::string get_name()const
    {
        return m_name;
    }

    boost::asynchronous::continuation_result<void> this_task_result()const
    {
        return continuation_result<void>(m_promise,m_done_func);
    }
    // called in case task is stolen by some client and only the result is returned
    template <class Archive,class InternalArchive>
    void as_result(Archive & ar, const unsigned int /*version*/)
    {
        boost::asynchronous::tcp::client_request::message_payload payload;
        ar >> payload;
        if (!payload.m_has_exception)
        {
            get_promise()->set_value();
        }
        else
        {
            get_promise()->set_exception(boost::copy_exception(payload.m_exception));
        }
    }
    void set_done_func(std::function<void()> f)
    {
        m_done_func=std::move(f);
    }

private:
    boost::shared_ptr<boost::promise<void> > m_promise;
    std::string m_name;
    std::function<void()> m_done_func;
};
// inside a task, create a continuation handling any number of subtasks
template <class OnDone, typename... Args>
typename boost::disable_if< typename boost::mpl::or_<
                            typename boost::asynchronous::detail::has_future_args<Args...>::type ,
                            typename boost::asynchronous::detail::has_iterator_args<Args...>::type >
                          ,void >::type
create_continuation(OnDone&& on_done, Args&&... args)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    typedef decltype(boost::asynchronous::detail::make_future_tuple(args...)) future_type;
    boost::asynchronous::detail::callback_continuation<void,boost::asynchronous::any_callable,future_type> c (
                state,boost::asynchronous::detail::make_future_tuple(args...), boost::chrono::milliseconds(0), std::forward<Args>(args)...);
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}
template <class OnDone, typename... Args>
typename boost::enable_if< typename boost::asynchronous::detail::has_future_args<boost::future<Args>...>::type ,void >::type
create_continuation(OnDone&& on_done, boost::future<Args>&&... args)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    typedef decltype(std::make_tuple(std::forward<boost::future<Args> >(args)...)) future_type;
    future_type sp(std::make_tuple( std::forward<boost::future<Args> >(args)...));

    boost::asynchronous::detail::continuation<void,boost::asynchronous::any_callable,future_type> c (state,std::move(sp), boost::chrono::milliseconds(0));
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}
template <class OnDone, typename... Args>
typename boost::enable_if< typename boost::asynchronous::detail::has_future_args<boost::shared_future<Args>...>::type ,void >::type
create_continuation(OnDone&& on_done, boost::shared_future<Args>&&... args)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    typedef decltype(std::make_tuple(std::forward<boost::shared_future<Args> >(args)...)) future_type;
    future_type sp(std::make_tuple( std::forward<boost::shared_future<Args> >(args)...));

    boost::asynchronous::detail::continuation<void,boost::asynchronous::any_callable,future_type> c (state,std::move(sp), boost::chrono::milliseconds(0));
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}
// version with containers of futures
template <class OnDone, typename Seq>
typename boost::enable_if< typename has_iterator<Seq>::type ,void >::type
create_continuation(OnDone&& on_done, Seq&& seq)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    boost::asynchronous::detail::continuation_as_seq<void,boost::asynchronous::any_callable,Seq> c (state,boost::chrono::milliseconds(0),std::forward<Seq>(seq));
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}
// standard version as above but with timeout
template <class OnDone, class Duration, typename... Args>
typename boost::disable_if< typename boost::mpl::or_<
                            typename boost::asynchronous::detail::has_future_args<Args...>::type ,
                            typename boost::asynchronous::detail::has_iterator_args<Args...>::type >
                          ,void >::type
create_continuation_timeout(OnDone&& on_done, Duration const& d, Args&&... args)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    typedef decltype(boost::asynchronous::detail::make_future_tuple(args...)) future_type;
    boost::asynchronous::detail::continuation<void,boost::asynchronous::any_callable,future_type,Duration> c (
                state,boost::asynchronous::detail::make_future_tuple(args...), d, std::forward<Args>(args)...);
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}
template <class OnDone, class Duration, typename... Args>
typename boost::enable_if< typename boost::asynchronous::detail::has_future_args<boost::future<Args>...>::type ,void >::type
create_continuation_timeout(OnDone&& on_done, Duration const& d, boost::future<Args>&&... args)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    typedef decltype(std::make_tuple(std::forward<boost::future<Args> >(args)...)) future_type;
    future_type sp(std::make_tuple( std::forward<boost::future<Args> >(args)...));

    boost::asynchronous::detail::continuation<void,boost::asynchronous::any_callable,future_type,Duration> c (state,std::move(sp),d);
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}
// with containers of futures
template <class OnDone, class Duration, typename Seq>
typename boost::enable_if< typename has_iterator<Seq>::type ,void >::type
create_continuation_timeout(OnDone&& on_done, Duration const& d, Seq&& seq)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    boost::asynchronous::detail::continuation_as_seq<void,boost::asynchronous::any_callable,Seq> c (state,d,std::forward<Seq>(seq));
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}

// versions with logging
template <typename Job, class OnDone, typename... Args>
typename boost::enable_if< typename boost::asynchronous::detail::has_future_args<boost::future<Args>...>::type ,void >::type
create_continuation_job(OnDone&& on_done, boost::future<Args>&&... args)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    typedef decltype(std::make_tuple(std::forward<boost::future<Args> >(args)...)) future_type;
    future_type sp (std::make_tuple( std::forward<boost::future<Args> >(args)...));

    boost::asynchronous::detail::continuation<void,Job,future_type> c (state,std::move(sp), boost::chrono::milliseconds(0));
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}


template <typename Job, class OnDone, typename... Args>
typename boost::disable_if< typename boost::mpl::or_<
                            typename boost::asynchronous::detail::has_future_args<Args...>::type ,
                            typename boost::asynchronous::detail::has_iterator_args<Args...>::type >
                          ,void >::type
create_continuation_job(OnDone&& on_done, Args&&... args)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    typedef decltype(boost::asynchronous::detail::make_future_tuple(args...)) future_type;
    boost::asynchronous::detail::continuation<void,Job,future_type> c (
                state,boost::asynchronous::detail::make_future_tuple(args...), boost::chrono::milliseconds(0), std::forward<Args>(args)...);
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}

// version with containers of futures
template <typename Job, class OnDone, typename Seq>
typename boost::enable_if< typename has_iterator<Seq>::type ,void >::type
create_continuation_job(OnDone&& on_done, Seq&& seq)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    boost::asynchronous::detail::continuation_as_seq<void,Job,Seq> c (state, boost::chrono::milliseconds(0),std::forward<Seq>(seq));
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}

// versions with logging and timeout
template <typename Job, class OnDone, class Duration, typename... Args>
typename boost::enable_if< typename boost::asynchronous::detail::has_future_args<boost::future<Args>...>::type ,void >::type
create_continuation_job_timeout(OnDone&& on_done, Duration const& d, boost::future<Args>&&... args)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    typedef decltype(std::make_tuple(std::forward<boost::future<Args> >(args)...)) future_type;
    future_type sp (std::make_tuple( std::forward<boost::future<Args> >(args)...));

    boost::asynchronous::detail::continuation<void,Job,future_type> c (state,std::move(sp), d);
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}


template <typename Job, class OnDone, class Duration, typename... Args>
typename boost::disable_if< typename boost::mpl::or_<
                            typename boost::asynchronous::detail::has_future_args<Args...>::type ,
                            typename boost::asynchronous::detail::has_iterator_args<Args...>::type >
                          ,void >::type
create_continuation_job_timeout(OnDone&& on_done, Duration const& d, Args&&... args)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    typedef decltype(boost::asynchronous::detail::make_future_tuple(args...)) future_type;
    boost::asynchronous::detail::continuation<void,Job,future_type> c (
                state,boost::asynchronous::detail::make_future_tuple(args...), d, std::forward<Args>(args)...);
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}

// version with containers of futures and timeout
template <typename Job, class OnDone, class Duration, typename Seq>
typename boost::enable_if< typename has_iterator<Seq>::type ,void >::type
create_continuation_job_timeout(OnDone&& on_done, Duration const& d, Seq&& seq)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    boost::asynchronous::detail::continuation_as_seq<void,Job,Seq> c (state, d,std::forward<Seq>(seq));
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}

// top level task
//  create the first continuation in the serie
template <class Return, class FirstTask>
boost::asynchronous::detail::continuation<Return,boost::asynchronous::any_callable> top_level_continuation(FirstTask&& t)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    return boost::asynchronous::detail::continuation<Return,boost::asynchronous::any_callable>(state,
                                                     std::make_tuple(t.get_future()), boost::chrono::milliseconds(0),std::forward<FirstTask>(t));
}
//  create the first continuation in the serie
template <class Return, typename Job, class FirstTask>
boost::asynchronous::detail::continuation<Return,Job> top_level_continuation_log(FirstTask&& t)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    return boost::asynchronous::detail::continuation<Return,Job>(state,
                                                     std::make_tuple(t.get_future()), boost::chrono::milliseconds(0), std::forward<FirstTask>(t));
}

}}

#endif // BOOST_ASYNCHRONOUS_CONTINUATION_TASK_HPP
