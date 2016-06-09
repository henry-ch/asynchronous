// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2016
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
#include <boost/asynchronous/expected.hpp>
#include <boost/asynchronous/scheduler/tss_scheduler.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/scheduler/detail/any_continuation.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/scheduler/detail/interrupt_state.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/client_request.hpp>

// provides the interface to creating continuations.
// one finds future-based continuations (the slower type)
// or callback continuations (faster)
// future continuations support interruption and timeouts, callback continuations do not (yet)
// future continuations also allow interfacing with asynchronous libraries providing only futures.
// All continuations are called by providing first a functor called upon completion and a variadic sequence of tasks or a vector of
// futures / any_continuation_tasks (test in test_callback_continuation_of_sequences.cpp).
// a continuation called from another scheduler is always started using top_level_callback_continuation / top_level_continuation.
// create_continuation and create_callback_continuation create a continuation called from a task of a threadpool.
// create_continuation -> future continuation
// create_callback_continuation -> callback continuation
// add _job if the job is not any_callable

namespace boost { namespace asynchronous {

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
    boost::asynchronous::detail::continuation<void,BOOST_ASYNCHRONOUS_DEFAULT_JOB,future_type> c (
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

    boost::asynchronous::detail::continuation<void,BOOST_ASYNCHRONOUS_DEFAULT_JOB,future_type> c (state,std::move(sp), boost::chrono::milliseconds(0));
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

    boost::asynchronous::detail::continuation<void,BOOST_ASYNCHRONOUS_DEFAULT_JOB,future_type> c (state,std::move(sp), boost::chrono::milliseconds(0));
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}
// version with containers of futures
template <class OnDone, typename Seq>
typename boost::enable_if< typename boost::asynchronous::has_iterator<Seq>::type ,void >::type
create_continuation(OnDone&& on_done, Seq&& seq)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    boost::asynchronous::detail::continuation_as_seq<void,BOOST_ASYNCHRONOUS_DEFAULT_JOB,Seq> c (state,boost::chrono::milliseconds(0),std::forward<Seq>(seq));
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
    boost::asynchronous::detail::continuation<void,BOOST_ASYNCHRONOUS_DEFAULT_JOB,future_type,Duration> c (
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

    boost::asynchronous::detail::continuation<void,BOOST_ASYNCHRONOUS_DEFAULT_JOB,future_type,Duration> c (state,std::move(sp),d);
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}
// with containers of futures
template <class OnDone, class Duration, typename Seq>
typename boost::enable_if< typename boost::asynchronous::has_iterator<Seq>::type ,void >::type
create_continuation_timeout(OnDone&& on_done, Duration const& d, Seq&& seq)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    boost::asynchronous::detail::continuation_as_seq<void,BOOST_ASYNCHRONOUS_DEFAULT_JOB,Seq> c (state,d,std::forward<Seq>(seq));
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
typename boost::enable_if< typename boost::asynchronous::has_iterator<Seq>::type ,void >::type
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
typename boost::enable_if< typename boost::asynchronous::has_iterator<Seq>::type ,void >::type
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
boost::asynchronous::detail::continuation<Return,BOOST_ASYNCHRONOUS_DEFAULT_JOB> top_level_continuation(FirstTask&& t)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    return boost::asynchronous::detail::continuation<Return,BOOST_ASYNCHRONOUS_DEFAULT_JOB>(state,
                                                     std::make_tuple(t.get_future()), boost::chrono::milliseconds(0),std::forward<FirstTask>(t));
}
//  create the first continuation in the serie
template <class Return, typename Job, class FirstTask>
boost::asynchronous::detail::continuation<Return,Job> top_level_continuation_job(FirstTask&& t)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    return boost::asynchronous::detail::continuation<Return,Job>(state,
                                                     std::make_tuple(t.get_future()), boost::chrono::milliseconds(0), std::forward<FirstTask>(t));
}

// callback continuations
template <class OnDone, typename... Args>
void create_callback_continuation(OnDone on_done, Args&&... args)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    typedef decltype(boost::asynchronous::detail::make_expected_tuple(args...)) future_type;
    boost::asynchronous::detail::callback_continuation<void,BOOST_ASYNCHRONOUS_DEFAULT_JOB,future_type> c (
                state,boost::asynchronous::detail::make_expected_tuple(args...), boost::chrono::milliseconds(0),
                std::move(on_done),std::forward<Args>(args)...);
    // no need of registration as no timeout checking
}
template <class OnDone, typename FutureType, typename... Args>
void create_callback_continuation(OnDone on_done, FutureType expected_tuple, std::tuple<Args...> args)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    boost::asynchronous::detail::callback_continuation<void,BOOST_ASYNCHRONOUS_DEFAULT_JOB,FutureType> c (
                state,std::move(expected_tuple), boost::chrono::milliseconds(0),
                std::move(on_done),std::move(args));
    // no need of registration as no timeout checking
}
template <class OnDone, typename Args>
void create_callback_continuation(OnDone on_done, std::vector<Args> args)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    boost::asynchronous::detail::callback_continuation_as_seq<typename Args::return_type,BOOST_ASYNCHRONOUS_DEFAULT_JOB> c (
                state,boost::chrono::milliseconds(0),
                std::move(on_done),std::move(args));
    // no need of registration as no timeout checking
}
template <typename Job, class OnDone, typename... Args>
void create_callback_continuation_job(OnDone on_done, Args&&... args)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    typedef decltype(boost::asynchronous::detail::make_expected_tuple(args...)) future_type;
    boost::asynchronous::detail::callback_continuation<void,Job,future_type> c (
                state,boost::asynchronous::detail::make_expected_tuple(args...), boost::chrono::milliseconds(0),
                std::move(on_done),std::forward<Args>(args)...);
    // no need of registration as no timeout checking
}
template <typename Job, class OnDone, typename FutureType, typename... Args>
void create_callback_continuation_job(OnDone on_done, FutureType expected_tuple, std::tuple<Args...> args)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    boost::asynchronous::detail::callback_continuation<void,Job,FutureType> c (
                state,std::move(expected_tuple), boost::chrono::milliseconds(0),
                std::move(on_done),std::move(args));
    // no need of registration as no timeout checking
}
template <typename Job,class OnDone, typename Args>
void create_callback_continuation_job(OnDone on_done, std::vector<Args> args)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    boost::asynchronous::detail::callback_continuation_as_seq<typename Args::return_type,Job> c (
                state,boost::chrono::milliseconds(0),
                std::move(on_done),std::move(args));
    // no need of registration as no timeout checking
}
template <typename Job, class OnDone, class Duration, typename... Args>
void create_callback_continuation_job_timeout(OnDone on_done, Duration const& d, Args&&... args)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    typedef decltype(boost::asynchronous::detail::make_expected_tuple(args...)) future_type;
    boost::asynchronous::detail::callback_continuation<void,Job,future_type,Duration> c (
                state,boost::asynchronous::detail::make_expected_tuple(args...), d, std::move(on_done),std::forward<Args>(args)...);
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}
template <typename Job, class OnDone, class Duration, typename FutureType, typename... Args>
void create_callback_continuation_job_timeout(OnDone on_done, Duration const& d, FutureType expected_tuple, std::tuple<Args...> args)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    boost::asynchronous::detail::callback_continuation<void,Job,FutureType,Duration> c (
                state,std::move(expected_tuple), d, std::move(on_done),std::move(args));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}
template <typename Job,class OnDone, class Duration, typename Args>
void create_callback_continuation_job_timeout(OnDone on_done, Duration const& d, std::vector<Args> args)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    boost::asynchronous::detail::callback_continuation_as_seq<typename Args::return_type,Job> c (
                state,d,
                std::move(on_done),std::move(args));
    // no need of registration as no timeout checking
}

template <class Return, class FirstTask>
boost::asynchronous::detail::callback_continuation<Return,BOOST_ASYNCHRONOUS_DEFAULT_JOB> top_level_callback_continuation(FirstTask&& t)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    return boost::asynchronous::detail::callback_continuation<Return,BOOST_ASYNCHRONOUS_DEFAULT_JOB> (
                state,boost::asynchronous::detail::make_expected_tuple(t), boost::chrono::milliseconds(0),
                true,std::forward<FirstTask>(t));
}
template <class Return, typename Job, class FirstTask>
boost::asynchronous::detail::callback_continuation<Return,Job> top_level_callback_continuation_job(FirstTask&& t)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    return boost::asynchronous::detail::callback_continuation<Return,Job>(
                state,boost::asynchronous::detail::make_expected_tuple(t), boost::chrono::milliseconds(0),
                true,std::forward<FirstTask>(t));
}
}}

#endif // BOOST_ASYNCHRONOUS_CONTINUATION_TASK_HPP
