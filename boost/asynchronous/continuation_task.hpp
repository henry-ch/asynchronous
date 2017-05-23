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
#include <type_traits>
#include <exception>

#include <boost/asynchronous/expected.hpp>
#include <boost/asynchronous/scheduler/tss_scheduler.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/scheduler/detail/any_continuation.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/scheduler/detail/interrupt_state.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/client_request.hpp>

/*! \file continuation_task.h
    \brief provides the interface to creating continuations.

    Provides future-based continuations or callback continuations which are faster.
    Continuations support interruption and timeouts.
    Future continuations also allow interfacing with asynchronous libraries providing only futures.
    A continuation is always started using top_level_callback_continuation / top_level_continuation.
    create_continuation and create_callback_continuation create a continuation called from a task of a threadpool.
    create_continuation -> future continuation
    create_callback_continuation -> callback continuation
    _job is required if the job is not any_callable
*/

namespace boost { namespace asynchronous {

/*! \fn void create_continuation(OnDone&& on_done, Args&&... args)
    \brief Create a future-based continuation as sub-task of a top-level continuation.
    \param on_done. Functor called upon completion of sub-tasks. The functor signature is void (std::tuple<boost/std::future<T>...>) where T is the return type of the sub-tasks.
    \param args a variadic sequence of sub-tasks. All are posted except the last, which is immediately executed from within the caller context.
*/
template <class OnDone, typename... Args>
typename std::enable_if< !(boost::asynchronous::detail::is_future<Args...>::value ||
                           boost::asynchronous::detail::has_iterator_args<Args...>::value) ,void >::type
create_continuation(OnDone&& on_done, Args&&... args)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    typedef decltype(boost::asynchronous::detail::make_future_tuple(args...)) future_type;
    boost::asynchronous::detail::continuation<void,BOOST_ASYNCHRONOUS_DEFAULT_JOB,future_type> c (
                state,boost::asynchronous::detail::make_future_tuple(args...), std::chrono::milliseconds(0), std::forward<Args>(args)...);
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}

/*! \fn void create_continuation(OnDone&& on_done, boost/std::future<Args>&&... args)
    \brief Create a future-based continuation as sub-task of a top-level continuation using already created futures.
    \brief This makes it easier to interface with future-based libraries.
    \param on_done. Functor called upon completion of sub-tasks. The functor signature is void (std::tuple<boost/std::future<T>...>) where T is the return type of the sub-tasks.
    \param args a variadic sequence of futures.
*/
template <class OnDone, typename... Args>
typename std::enable_if< boost::asynchronous::detail::is_future<Args...>::value, void>::type
create_continuation(OnDone&& on_done, Args&&... args)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    typedef decltype(std::make_tuple(std::forward<Args >(args)...)) future_type;
    future_type sp(std::make_tuple( std::forward<Args >(args)...));

    boost::asynchronous::detail::continuation<void,BOOST_ASYNCHRONOUS_DEFAULT_JOB,future_type> c (state,std::move(sp), std::chrono::milliseconds(0));
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}

/*! \fn void create_continuation(OnDone&& on_done, Seq&& seq)
    \brief Create a future-based continuation as sub-task of a top-level continuation using a sequence of already created futures.
    \brief This makes it easier to interface with future-based libraries.
    \param on_done. Functor called upon completion of sub-tasks. The functor signature is void (std::tuple<boost/std::future<T>...>) where T is the return type of the sub-tasks.
    \param seq a sequence of futures.
*/
template <class OnDone, typename Seq>
typename std::enable_if< boost::asynchronous::has_iterator<Seq>::value ,void >::type
create_continuation(OnDone&& on_done, Seq&& seq)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    boost::asynchronous::detail::continuation_as_seq<void,BOOST_ASYNCHRONOUS_DEFAULT_JOB,Seq> c (state,std::chrono::milliseconds(0),std::forward<Seq>(seq));
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}

/*! \fn void create_continuation_timeout(OnDone&& on_done, Duration const& d, Args&&... args)
    \brief Create a future-based continuation as sub-task of a top-level continuation.
    \brief A timeout for execution of sub-tasks is also provided.
    \param on_done. Functor called upon completion of sub-tasks. The functor signature is void (std::tuple<boost/std::future<T>...>) where T is the return type of the sub-tasks.
    \param d a std::chrono based duration
    \param args a variadic sequence of sub-tasks. All are posted except the last, which is immediately executed from within the caller context.
*/
template <class OnDone, class Duration, typename... Args>
typename std::enable_if< !( boost::asynchronous::detail::is_future<Args...>::value ||
                            boost::asynchronous::detail::has_iterator_args<Args...>::value)
                          ,void >::type
create_continuation_timeout(OnDone&& on_done, Duration const& d, Args&&... args)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    typedef decltype(boost::asynchronous::detail::make_future_tuple(args...)) future_type;
    boost::asynchronous::detail::continuation<void,BOOST_ASYNCHRONOUS_DEFAULT_JOB,future_type,Duration> c (
                state,boost::asynchronous::detail::make_future_tuple(args...), d, std::forward<Args>(args)...);
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}

/*! \fn void create_continuation_timeout(OnDone&& on_done, Duration const& d, boost/std::future<Args>&&... args)
    \brief Create a future-based continuation as sub-task of a top-level continuation using already created futures.
    \brief A timeout for execution of sub-tasks is also provided.
    \brief This makes it easier to interface with future-based libraries.
    \param on_done. Functor called upon completion of sub-tasks. The functor signature is void (std::tuple<boost/std::future<T>...>) where T is the return type of the sub-tasks.
    \param d a std::chrono based duration
    \param args a variadic sequence of futures.
*/
template <class OnDone, class Duration, typename... Args>
typename std::enable_if< boost::asynchronous::detail::is_future<Args...>::value ,void >::type
create_continuation_timeout(OnDone&& on_done, Duration const& d, Args&&... args)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    typedef decltype(std::make_tuple(std::forward<Args>(args)...)) future_type;
    future_type sp(std::make_tuple( std::forward<Args >(args)...));

    boost::asynchronous::detail::continuation<void,BOOST_ASYNCHRONOUS_DEFAULT_JOB,future_type,Duration> c (state,std::move(sp),d);
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}
/*! \fn void create_continuation_timeout(OnDone&& on_done, Duration const& d, Seq&& seq)
    \brief Create a future-based continuation as sub-task of a top-level continuation using a sequence of already created futures.
    \brief This makes it easier to interface with future-based libraries.
    \brief A timeout for execution of sub-tasks is also provided.
    \param on_done. Functor called upon completion of sub-tasks. The functor signature is void (std::tuple<boost/std::future<T>...>) where T is the return type of the sub-tasks.
    \param seq a sequence of futures.
*/
template <class OnDone, class Duration, typename Seq>
typename std::enable_if< boost::asynchronous::has_iterator<Seq>::value ,void >::type
create_continuation_timeout(OnDone&& on_done, Duration const& d, Seq&& seq)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    boost::asynchronous::detail::continuation_as_seq<void,BOOST_ASYNCHRONOUS_DEFAULT_JOB,Seq> c (state,d,std::forward<Seq>(seq));
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}

/*! \fn void create_continuation_job(OnDone&& on_done, boost/std::future<Args>&&... args)
    \brief Create a future-based continuation as sub-task of a top-level continuation using already created futures.
    \brief This makes it easier to interface with future-based libraries.
    \brief A Job type can be given as argument. This version must be used for loggable or serializable jobs.
    \param on_done. Functor called upon completion of sub-tasks. The functor signature is void (std::tuple<boost/std::future<T>...>) where T is the return type of the sub-tasks.
    \param args a variadic sequence of futures.
*/
template <typename Job, class OnDone, typename... Args>
typename std::enable_if< boost::asynchronous::detail::is_future<Args...>::value ,void >::type
create_continuation_job(OnDone&& on_done, Args&&... args)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    typedef decltype(std::make_tuple(std::forward<Args>(args)...)) future_type;
    future_type sp (std::make_tuple( std::forward<Args>(args)...));

    boost::asynchronous::detail::continuation<void,Job,future_type> c (state,std::move(sp), std::chrono::milliseconds(0));
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}

/*! \fn void create_continuation_job(OnDone&& on_done, Args&&... args)
    \brief Create a future-based continuation as sub-task of a top-level continuation.
    \brief A Job type can be given as argument. This version must be used for loggable or serializable jobs.
    \param on_done. Functor called upon completion of sub-tasks. The functor signature is void (std::tuple<std::future<T>...>) where T is the return type of the sub-tasks.
    \param args a variadic sequence of sub-tasks. All are posted except the last, which is immediately executed from within the caller context.
*/
template <typename Job, class OnDone, typename... Args>
typename std::enable_if< !(boost::asynchronous::detail::is_future<Args...>::value ||
                           boost::asynchronous::detail::has_iterator_args<Args...>::value)
                          ,void >::type
create_continuation_job(OnDone&& on_done, Args&&... args)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    typedef decltype(boost::asynchronous::detail::make_future_tuple(args...)) future_type;
    boost::asynchronous::detail::continuation<void,Job,future_type> c (
                state,boost::asynchronous::detail::make_future_tuple(args...), std::chrono::milliseconds(0), std::forward<Args>(args)...);
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}

/*! \fn void create_continuation(OnDone&& on_done, Seq&& seq)
    \brief Create a future-based continuation as sub-task of a top-level continuation using a sequence of already created futures.
    \brief This makes it easier to interface with future-based libraries.
    \brief A Job type can be given as argument. This version must be used for loggable or serializable jobs.
    \param on_done. Functor called upon completion of sub-tasks. The functor signature is void (std::tuple<boost/std::future<T>...>) where T is the return type of the sub-tasks.
    \param seq a sequence of futures.
*/
template <typename Job, class OnDone, typename Seq>
typename std::enable_if< boost::asynchronous::has_iterator<Seq>::value ,void >::type
create_continuation_job(OnDone&& on_done, Seq&& seq)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    boost::asynchronous::detail::continuation_as_seq<void,Job,Seq> c (state, std::chrono::milliseconds(0),std::forward<Seq>(seq));
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}

/*! \fn void create_continuation_job_timeout(OnDone&& on_done, Duration const& d, boost/std::future<Args>&&... args)
    \brief Create a future-based continuation as sub-task of a top-level continuation using already created futures.
    \brief A timeout for execution of sub-tasks is also provided.
    \brief A Job type can be given as argument. This version must be used for loggable or serializable jobs.
    \brief This makes it easier to interface with future-based libraries.
    \param on_done. Functor called upon completion of sub-tasks. The functor signature is void (std::tuple<boost/std::future<T>...>) where T is the return type of the sub-tasks.
    \param d a std::chrono based duration
    \param args a variadic sequence of futures.
*/
template <typename Job, class OnDone, class Duration, typename... Args>
typename std::enable_if< boost::asynchronous::detail::is_future<Args...>::value ,void >::type
create_continuation_job_timeout(OnDone&& on_done, Duration const& d, Args&&... args)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    typedef decltype(std::make_tuple(std::forward<Args>(args)...)) future_type;
    future_type sp (std::make_tuple( std::forward<Args>(args)...));

    boost::asynchronous::detail::continuation<void,Job,future_type> c (state,std::move(sp), d);
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}

/*! \fn void create_continuation_job_timeout(OnDone&& on_done, Duration const& d, Args&&... args)
    \brief Create a future-based continuation as sub-task of a top-level continuation.
    \brief A timeout for execution of sub-tasks is also provided.
    \brief A Job type can be given as argument. This version must be used for loggable or serializable jobs.
    \param on_done. Functor called upon completion of sub-tasks. The functor signature is void (std::tuple<boost/std::future<T>...>) where T is the return type of the sub-tasks.
    \param d a std::chrono based duration
    \param args a variadic sequence of sub-tasks. All are posted except the last, which is immediately executed from within the caller context.
*/
template <typename Job, class OnDone, class Duration, typename... Args>
typename std::enable_if<!(boost::asynchronous::detail::is_future<Args...>::value ||
                        boost::asynchronous::detail::has_iterator_args<Args...>::value)
                        ,void >::type
create_continuation_job_timeout(OnDone&& on_done, Duration const& d, Args&&... args)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    typedef decltype(boost::asynchronous::detail::make_future_tuple(args...)) future_type;
    boost::asynchronous::detail::continuation<void,Job,future_type> c (
                state,boost::asynchronous::detail::make_future_tuple(args...), d, std::forward<Args>(args)...);
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}

/*! \fn void create_continuation_job_timeout(OnDone&& on_done, Duration const& d, Seq&& seq)
    \brief Create a future-based continuation as sub-task of a top-level continuation using a sequence of already created futures.
    \brief This makes it easier to interface with future-based libraries.
    \brief A timeout for execution of sub-tasks is also provided.
    \brief A Job type can be given as argument. This version must be used for loggable or serializable jobs.
    \param on_done. Functor called upon completion of sub-tasks. The functor signature is void (std::tuple<boost/std::future<T>...>) where T is the return type of the sub-tasks.
    \param seq a sequence of futures.
*/
template <typename Job, class OnDone, class Duration, typename Seq>
typename std::enable_if<boost::asynchronous::has_iterator<Seq>::value ,void >::type
create_continuation_job_timeout(OnDone&& on_done, Duration const& d, Seq&& seq)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    boost::asynchronous::detail::continuation_as_seq<void,Job,Seq> c (state, d,std::forward<Seq>(seq));
    c.on_done(std::forward<OnDone>(on_done));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}

/*! \fn continuation<Return,BOOST_ASYNCHRONOUS_DEFAULT_JOB> top_level_continuation(FirstTask&& t)
    \brief Creates the first continuation in the serie.
    \brief This function has to be called from a task / lambda executed from within a threadpool.
    \brief It instructs the library not to call a callback / set a future until the wrapped task completes.
    \brief Concretely it means the task is seen as completed when the continuation_result provided by boost::asynchronous::continuation_task is set.
    \param t a boost::asynchronous::continuation_task derived task.
*/
template <class Return, class FirstTask>
boost::asynchronous::detail::continuation<Return,BOOST_ASYNCHRONOUS_DEFAULT_JOB> top_level_continuation(FirstTask&& t)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    return boost::asynchronous::detail::continuation<Return,BOOST_ASYNCHRONOUS_DEFAULT_JOB>(state,
                                                     std::make_tuple(t.get_future()), std::chrono::milliseconds(0),std::forward<FirstTask>(t));
}
/*! \fn continuation<Return,BOOST_ASYNCHRONOUS_DEFAULT_JOB> top_level_continuation(FirstTask&& t)
    \brief Creates the first continuation in the serie.
    \brief This function has to be called from a task / lambda executed from within a threadpool.
    \brief It instructs the library not to call a callback / set a future until the wrapped task completes.
    \brief Concretely it means the task is seen as completed when the continuation_result provided by boost::asynchronous::continuation_task is set.
    \brief A Job type can be given as argument. This version must be used for loggable or serializable jobs.
    \param t a boost::asynchronous::continuation_task derived task.
*/
template <class Return, typename Job, class FirstTask>
boost::asynchronous::detail::continuation<Return,Job> top_level_continuation_job(FirstTask&& t)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    return boost::asynchronous::detail::continuation<Return,Job>(state,
                                                     std::make_tuple(t.get_future()), std::chrono::milliseconds(0), std::forward<FirstTask>(t));
}

/*! \fn void create_callback_continuation(OnDone&& on_done, Args&&... args)
    \brief Create a callback-based continuation as sub-task of a top-level continuation.
    \param on_done. Functor called upon completion of sub-tasks. The functor signature is void (std::tuple<boost::expected<T>...>) where T is the return type of the sub-tasks.
    \param args a variadic sequence of sub-tasks. All are posted except the last, which is immediately executed from within the caller context.
*/
template <class OnDone, typename... Args>
void create_callback_continuation(OnDone on_done, Args&&... args)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    typedef decltype(boost::asynchronous::detail::make_expected_tuple(args...)) future_type;
    boost::asynchronous::detail::callback_continuation<void,BOOST_ASYNCHRONOUS_DEFAULT_JOB,future_type> c
            (state,boost::asynchronous::detail::make_expected_tuple(args...), std::chrono::milliseconds(0),
             std::move(on_done),boost::asynchronous::continuation_post_policy::post_all_but_one,std::forward<Args>(args)...);
    // no need of registration as no timeout checking
}
template <class OnDone, typename FutureType, typename... Args>
void create_callback_continuation(OnDone on_done, FutureType expected_tuple, std::tuple<Args...> args)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    boost::asynchronous::detail::callback_continuation<void,BOOST_ASYNCHRONOUS_DEFAULT_JOB,FutureType> c
            (state,std::move(expected_tuple), std::chrono::milliseconds(0),
             std::move(on_done),boost::asynchronous::continuation_post_policy::post_all_but_one,std::move(args));
    // no need of registration as no timeout checking
}

/*! \fn void create_callback_continuation(OnDone&& on_done, std::vector<Args> args)
    \brief Create a callback-based continuation as sub-task of a top-level continuation.
    \param on_done. Functor called upon completion of sub-tasks. The functor signature is void (std::tuple<boost::expected<T>...>) where T is the return type of the sub-tasks.
    \param args a vector of sub-tasks. All are posted except the last, which is immediately executed from within the caller context.
*/
template <class OnDone, typename Args>
void create_callback_continuation(OnDone on_done, std::vector<Args> args)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    boost::asynchronous::detail::callback_continuation_as_seq<typename Args::return_type,BOOST_ASYNCHRONOUS_DEFAULT_JOB> c (
                state,std::chrono::milliseconds(0),
                std::move(on_done),std::move(args));
    // no need of registration as no timeout checking
}

/*! \fn void create_callback_continuation_job(OnDone&& on_done, Args&&... args)
    \brief Create a callback-based continuation as sub-task of a top-level continuation.
    \brief A Job type can be given as argument. This version must be used for loggable or serializable jobs.
    \param on_done. Functor called upon completion of sub-tasks. The functor signature is void (std::tuple<boost::expected<T>...>) where T is the return type of the sub-tasks.
    \param args a variadic sequence of sub-tasks. All are posted except the last, which is immediately executed from within the caller context.
*/
template <typename Job, class OnDone, typename... Args>
void create_callback_continuation_job(OnDone on_done, Args&&... args)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    typedef decltype(boost::asynchronous::detail::make_expected_tuple(args...)) future_type;
    boost::asynchronous::detail::callback_continuation<void,Job,future_type>
            c (state,boost::asynchronous::detail::make_expected_tuple(args...), std::chrono::milliseconds(0),
               std::move(on_done),boost::asynchronous::continuation_post_policy::post_all_but_one,std::forward<Args>(args)...);
    // no need of registration as no timeout checking
}
template <typename Job, class OnDone, typename FutureType, typename... Args>
void create_callback_continuation_job(OnDone on_done, FutureType expected_tuple, std::tuple<Args...> args)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    boost::asynchronous::detail::callback_continuation<void,Job,FutureType>
            c (state,std::move(expected_tuple), std::chrono::milliseconds(0),
               std::move(on_done),boost::asynchronous::continuation_post_policy::post_all_but_one,std::move(args));
    // no need of registration as no timeout checking
}

/*! \fn void create_callback_continuation_job(OnDone&& on_done, std::vector<Args> args)
    \brief Create a callback-based continuation as sub-task of a top-level continuation.
    \brief A Job type can be given as argument. This version must be used for loggable or serializable jobs.
    \param on_done. Functor called upon completion of sub-tasks. The functor signature is void (std::tuple<boost::expected<T>...>) where T is the return type of the sub-tasks.
    \param args a vector of sub-tasks. All are posted except the last, which is immediately executed from within the caller context.
*/
template <typename Job,class OnDone, typename Args>
void create_callback_continuation_job(OnDone on_done, std::vector<Args> args)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    boost::asynchronous::detail::callback_continuation_as_seq<typename Args::return_type,Job> c (
                state,std::chrono::milliseconds(0),
                std::move(on_done),std::move(args));
    // no need of registration as no timeout checking
}

/*! \fn void create_callback_continuation_job_timeout(OnDone&& on_done, Duration const& d, Args&&... args)
    \brief Create a callback-based continuation as sub-task of a top-level continuation.
    \brief A timeout for execution of sub-tasks is also provided.
    \brief A Job type can be given as argument. This version must be used for loggable or serializable jobs.
    \param on_done. Functor called upon completion of sub-tasks. The functor signature is void (std::tuple<boost::expected<T>...>) where T is the return type of the sub-tasks.
    \param d a std::chrono based duration
    \param args a variadic sequence of sub-tasks. All are posted except the last, which is immediately executed from within the caller context.
*/
template <typename Job, class OnDone, class Duration, typename... Args>
void create_callback_continuation_job_timeout(OnDone on_done, Duration const& d, Args&&... args)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    typedef decltype(boost::asynchronous::detail::make_expected_tuple(args...)) future_type;
    boost::asynchronous::detail::callback_continuation<void,Job,future_type,Duration>
            c (state,boost::asynchronous::detail::make_expected_tuple(args...), d, std::move(on_done),boost::asynchronous::continuation_post_policy::post_all_but_one,
               std::forward<Args>(args)...);
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}
template <typename Job, class OnDone, class Duration, typename FutureType, typename... Args>
void create_callback_continuation_job_timeout(OnDone on_done, Duration const& d, FutureType expected_tuple, std::tuple<Args...> args)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    boost::asynchronous::detail::callback_continuation<void,Job,FutureType,Duration>
            c (state,std::move(expected_tuple), d, std::move(on_done),boost::asynchronous::continuation_post_policy::post_all_but_one,std::move(args));
    boost::asynchronous::any_continuation a(std::move(c));
    boost::asynchronous::get_continuations().emplace_front(std::move(a));
}

/*! \fn void create_callback_continuation_job_timeout(OnDone&& on_done, Duration const& d, std::vector<Args> args)
    \brief Create a callback-based continuation as sub-task of a top-level continuation.
    \brief A timeout for execution of sub-tasks is also provided.
    \brief A Job type can be given as argument. This version must be used for loggable or serializable jobs.
    \param on_done. Functor called upon completion of sub-tasks. The functor signature is void (std::tuple<boost::expected<T>...>) where T is the return type of the sub-tasks.
    \param d a std::chrono based duration
    \param args a vector of sub-tasks. All are posted except the last, which is immediately executed from within the caller context.
*/
template <typename Job,class OnDone, class Duration, typename Args>
void create_callback_continuation_job_timeout(OnDone on_done, Duration const& d, std::vector<Args> args)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    boost::asynchronous::detail::callback_continuation_as_seq<typename Args::return_type,Job> c (
                state,d,
                std::move(on_done),std::move(args));
    // no need of registration as no timeout checking
}

/*! \fn callback_continuation<Return,BOOST_ASYNCHRONOUS_DEFAULT_JOB> top_level_callback_continuation(FirstTask&& t)
    \brief Creates the first continuation in the serie.
    \brief This function has to be called from a task / lambda executed from within a threadpool.
    \brief It instructs the library not to call a callback / set a future until the wrapped task completes.
    \brief Concretely it means the task is seen as completed when the continuation_result provided by boost::asynchronous::continuation_task is set.
    \param t a boost::asynchronous::continuation_task derived task.
*/
template <class Return, class FirstTask>
boost::asynchronous::detail::callback_continuation<Return,BOOST_ASYNCHRONOUS_DEFAULT_JOB> top_level_callback_continuation(FirstTask&& t)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    return boost::asynchronous::detail::callback_continuation<Return,BOOST_ASYNCHRONOUS_DEFAULT_JOB> (
                state,boost::asynchronous::detail::make_expected_tuple(t), std::chrono::milliseconds(0),
                true,boost::asynchronous::continuation_post_policy::post_all_but_one,std::forward<FirstTask>(t));
}

/*! \fn callback_continuation<Return,BOOST_ASYNCHRONOUS_DEFAULT_JOB> top_level_callback_continuation(FirstTask&& t)
    \brief Creates the first continuation in the serie.
    \brief This function has to be called from a task / lambda executed from within a threadpool.
    \brief It instructs the library not to call a callback / set a future until the wrapped task completes.
    \brief Concretely it means the task is seen as completed when the continuation_result provided by boost::asynchronous::continuation_task is set.
    \brief A Job type can be given as argument. This version must be used for loggable or serializable jobs.
    \param t a boost::asynchronous::continuation_task derived task.
*/
template <class Return, typename Job, class ... Args>
boost::asynchronous::detail::callback_continuation<Return,Job> top_level_callback_continuation_job(Args&&... args)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    return boost::asynchronous::detail::callback_continuation<Return,Job>(
                state,boost::asynchronous::detail::make_expected_tuple(args...), std::chrono::milliseconds(0),
                true,boost::asynchronous::continuation_post_policy::post_all_but_one,std::forward<Args>(args)...);
}

/*! \fn callback_continuation<Return,BOOST_ASYNCHRONOUS_DEFAULT_JOB> top_level_callback_continuation_force_post(FirstTask&& t)
    \brief Creates the first continuation in the serie.
    \brief This function has to be called from a task / lambda executed from within a threadpool.
    \brief It instructs the library not to call a callback / set a future until the wrapped task completes.
    \brief Concretely it means the task is seen as completed when the continuation_result provided by boost::asynchronous::continuation_task is set.
    \brief This version forces posting of all tasks, including the last one passed to create_callback_continuation_XXX versions
    \param t a boost::asynchronous::continuation_task derived task.
*/
template <class Return, class FirstTask>
boost::asynchronous::detail::callback_continuation<Return,BOOST_ASYNCHRONOUS_DEFAULT_JOB> top_level_callback_continuation_force_post(FirstTask&& t)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    return boost::asynchronous::detail::callback_continuation<Return,BOOST_ASYNCHRONOUS_DEFAULT_JOB>
            (state,boost::asynchronous::detail::make_expected_tuple(t), std::chrono::milliseconds(0),
             true,boost::asynchronous::continuation_post_policy::post_all,std::forward<FirstTask>(t));
}

/*! \fn callback_continuation<Return,BOOST_ASYNCHRONOUS_DEFAULT_JOB> top_level_callback_continuation_force_post(FirstTask&& t)
    \brief Creates the first continuation in the serie.
    \brief This function has to be called from a task / lambda executed from within a threadpool.
    \brief It instructs the library not to call a callback / set a future until the wrapped task completes.
    \brief Concretely it means the task is seen as completed when the continuation_result provided by boost::asynchronous::continuation_task is set.
    \brief This version forces posting of all tasks, including the last one passed to create_callback_continuation_XXX versions
    \brief A variadic number of tasks can be passed to avoid having to write a first task which just passed through.
    \param args boost::asynchronous::continuation_task derived tasks.
*/
template <class Return, typename Job, class ... Args>
boost::asynchronous::detail::callback_continuation<Return,Job> top_level_callback_continuation_job_force_post(Args&&... args)
{
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    return boost::asynchronous::detail::callback_continuation<Return,Job>
            (state,boost::asynchronous::detail::make_expected_tuple(args...), std::chrono::milliseconds(0),
             true,boost::asynchronous::continuation_post_policy::post_all,std::forward<Args>(args)...);
}

}}

#endif // BOOST_ASYNCHRONOUS_CONTINUATION_TASK_HPP
