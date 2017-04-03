// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2017
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRON_TRACKABLE_SERVANT_HPP
#define BOOST_ASYNCHRON_TRACKABLE_SERVANT_HPP

#include <cstddef>

#include <boost/asynchronous/detail/function_traits.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/scheduler/tss_scheduler.hpp>
#include <boost/asynchronous/detail/move_bind.hpp>
#include <boost/system/error_code.hpp>
#include <boost/signals2/signal.hpp>


namespace boost { namespace asynchronous
{
//TODO in detail
struct track{};

// simple class for post and callback management
// hides threadpool and weak scheduler, adds automatic trackability for callbacks and tasks
// inherit from it to get functionality
/*!
 * \class trackable_servant
 * This is the basic class for all objects acting as servants and needing more than basic serialization of calls.
 * Offers safe passing of tasks to threadpools and running callbacks in the servant thread.
 * It also provides a very useful make_safe_callback, which wraps any functor, ensuring this functor will always be called
 * in the servant thread, and checking for life issues (no call if servant is no more alive).
 * This safe functor can be called at any time, from any thread.
 */
template <class JOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB,class WJOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB>
class trackable_servant
{
public:
    typedef int requires_weak_scheduler;
    /*!
     * \brief Constructor
     * \brief Constructs a trackable_servant from a weak scheduler(any_weak_scheduler), passed by a servant_proxy and a
     * \brief threadpool (any_shared_scheduler_proxy). The type of each is that of the trackable_servant template parameters
     * \brief (JOB for the weak scheduler, WJOB for the threadpool).
     * \brief The weak scheduler is the scheduler of the thread where the servant lives.
     * \brief If no threadpool is passed, no post_callback or post_future is possible and will fail.
     */
    trackable_servant(boost::asynchronous::any_weak_scheduler<JOB> const& s,
                      boost::asynchronous::any_shared_scheduler_proxy<WJOB> w=boost::asynchronous::any_shared_scheduler_proxy<WJOB>())
        : m_tracking(boost::make_shared<boost::asynchronous::track>())
        , m_scheduler(s)
        , m_worker(w)
    {}
    /*!
     * \brief Constructor
     * \brief Constructs a trackable_servant from a weak scheduler(any_weak_scheduler), already known, and a
     * \brief threadpool (any_shared_scheduler_proxy). The type of each is that of the trackable_servant template parameters
     * \brief (JOB for the weak scheduler, WJOB for the threadpool).
     * \brief The weak scheduler is the scheduler of the thread where the servant lives.
     * \brief If no threadpool is passed, no post_callback or post_future is possible and will fail.
     * \brief This constructor is for cases where a servant is created by another servant within the same thread. In this case, the scheduler
     * \brief where this servant lives is already known.
     */
    trackable_servant(boost::asynchronous::any_shared_scheduler_proxy<WJOB> w=boost::asynchronous::any_shared_scheduler_proxy<WJOB>())
        : m_tracking(boost::make_shared<boost::asynchronous::track>())
        , m_scheduler(boost::asynchronous::get_thread_scheduler<JOB>())
        , m_worker(w)
    {}

    // copy-ctor and operator= are needed for correct tracking
    /*!
     * \brief Copy-Constructor. Does not throw
     */
    trackable_servant(trackable_servant const& rhs) noexcept
        : m_tracking(boost::make_shared<boost::asynchronous::track>())
        , m_scheduler(rhs.m_scheduler)
        , m_worker(rhs.m_worker)
    {
    }

    /*!
     * \brief Move-Constructor. Does not throw
     */
    trackable_servant(trackable_servant&& rhs) noexcept
        : m_tracking(boost::make_shared<boost::asynchronous::track>())
        , m_scheduler(std::move(rhs.m_scheduler))
        , m_worker(std::move(rhs.m_worker))
    {
    }
    /*!
     * \brief Destructor.
     */
    ~trackable_servant()
    {
    }

    /*!
     * \brief Assignment operator. Does not throw
     */
    trackable_servant& operator= (trackable_servant const& rhs) noexcept
    {
        if (this != &rhs)
        {
            m_tracking = boost::make_shared<boost::asynchronous::track>();
            m_scheduler = rhs.m_scheduler;
            m_worker = rhs.m_worker;
        }
        return *this;
    }

    /*!
     * \brief Move assignment operator. Does not throw
     */
    trackable_servant& operator= (trackable_servant&& rhs) noexcept
    {
        if (this != &rhs)
        {
            m_tracking = std::move(rhs.m_tracking);
            m_scheduler = std::move(rhs.m_scheduler);
            m_worker = std::move(rhs.m_worker);
        }
        return *this;
    }

    /*!
     * \brief Makes a callback, which posts if not the correct thread, and calls directly otherwise
     * \brief In any case, it will check if this object is still alive.
     * \param func a functor will will safely be executed
     * \param task_name which will be displayed in the diagnostic of the servant's scheduler.
     * \param prio The priority of the functor within the servant's scheduler.
     */
    template<class T>
    auto make_safe_callback(T func,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                                                     const std::string& task_name, std::size_t prio) const
#else
                                                     const std::string& task_name="", std::size_t prio=0) const
#endif
    -> decltype(boost::asynchronous::make_function(std::move(func)))
    {
        return this->make_safe_callback_helper(boost::asynchronous::make_function(std::move(func)),task_name,prio);
    }

    /*!
     * \brief Returns a functor checking if servant is still alive
     */
    std::function<bool()> make_check_alive_functor()const
    {
        boost::weak_ptr<track> tracking (m_tracking);
        return [tracking](){return !tracking.expired();};
    }

    /*!
     * \brief Makes it easier to use a timer service
     * \param t usually an asio_deadline_timer_proxy, which hides an asio timer behind an asynchronous proxy.
     * \param func timer callback functor
     * \param task_name which will be displayed in the diagnostic of the servant's scheduler.
     * \param post_prio The priority of the timer call within the timer's scheduler.
     * \param cb_prio The priority of the timer callback functor within the servant's scheduler.
     */
    template <class Timer, class F>
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    void async_wait(Timer& t, F func, std::string const& task_name,std::size_t post_prio, std::size_t cb_prio)const
#else
    void async_wait(Timer& t, F func, std::string const& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0)const
#endif
    {
        std::function<void(const ::boost::system::error_code&)> f = std::move(func);
        call_callback(t.get_proxy(),
                      t.unsafe_async_wait(make_safe_callback(std::move(f),task_name,cb_prio)),
                      // ignore async_wait callback functor., real callback is above
                      [](boost::asynchronous::expected<void> ){},
                      task_name, post_prio, cb_prio
                      );
    }

    /*!
     * \brief Makes it easier to use a timer service
     * \param t usually an asio_deadline_timer_proxy, which hides an asio timer behind an asynchronous proxy.
     * \param timer_duration the timer value
     * \param func timer callback functor
     * \param task_name which will be displayed in the diagnostic of the servant's scheduler.
     * \param post_prio The priority of the timer call within the timer's scheduler.
     * \param cb_prio The priority of the timer callback functor within the servant's scheduler.
     */
    template <class Timer, class Duration, class F>
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    void async_wait_duration(Timer& t, Duration timer_duration, F func, std::string const& task_name,std::size_t post_prio, std::size_t cb_prio)const
#else
    void async_wait_duration(Timer& t, Duration timer_duration, F func, std::string const& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0)const
#endif
    {
        std::function<void(const ::boost::system::error_code&)> f = std::move(func);
        call_callback(t.get_proxy(),
                      t.unsafe_async_wait(make_safe_callback(std::move(f),task_name,cb_prio),std::move(timer_duration)),
                      // ignore async_wait callback functor., real callback is above
                      [](boost::asynchronous::expected<void> ){},
                      task_name, post_prio, cb_prio
                      );
    }

    /*!
     * \brief Posts a task to the servant's threadpool scheduler and get a safe callback when the task is done.
     * \param func task functor
     * \param cb_func callback functor
     * \param task_name which will be displayed in the diagnostic of the servant's scheduler.
     * \param post_prio The priority of the posted task within the threadpool scheduler.
     * \param cb_prio The priority of the timer callback functor within the servant's scheduler.
     */
    template <class F1, class F2>
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    void post_callback(F1 func,F2 cb_func, std::string const& task_name, std::size_t post_prio, std::size_t cb_prio) const
#else
    void post_callback(F1 func,F2 cb_func, std::string const& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0) const    
#endif
    {
        // we want to log if possible
        boost::asynchronous::post_callback(m_worker,
                                        boost::asynchronous::check_alive_before_exec(std::move(func),m_tracking),
                                        m_scheduler,
                                        boost::asynchronous::check_alive(std::move(cb_func),m_tracking),
                                        task_name,
                                        post_prio,
                                        cb_prio);
    }

    /*!
     * \brief Posts an interruptible task to the servant's threadpool scheduler and get a safe callback when the task is done.
     * \brief The task must be interruptible as defined by Boost Thread
     * \param func task functor
     * \param cb_func callback functor
     * \param task_name which will be displayed in the diagnostic of the servant's scheduler.
     * \param post_prio The priority of the posted task within the threadpool scheduler.
     * \param cb_prio The priority of the timer callback functor within the servant's scheduler.
     * \return any_interruptible which can be used to interrupt the task
     */
    template <class F1, class F2>
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    boost::asynchronous::any_interruptible interruptible_post_callback(F1 func,F2 cb_func, std::string const& task_name,
                                                                    std::size_t post_prio, std::size_t cb_prio)const
#else
    boost::asynchronous::any_interruptible interruptible_post_callback(F1 func,F2 cb_func, std::string const& task_name="",
                                                                    std::size_t post_prio=0, std::size_t cb_prio=0) const
#endif
    {
        return boost::asynchronous::interruptible_post_callback(
                                        m_worker,
                                        boost::asynchronous::check_alive_before_exec(std::move(func),m_tracking),
                                        m_scheduler,
                                        boost::asynchronous::check_alive(std::move(cb_func),m_tracking),
                                        task_name,
                                        post_prio,
                                        cb_prio);
    }

    /*!
     * \brief Posts a task to any threadpool scheduler and get a safe callback when the task is done.
     * \param func task functor
     * \param cb_func callback functor
     * \param task_name which will be displayed in the diagnostic of the servant's scheduler.
     * \param post_prio The priority of the posted task within the threadpool scheduler.
     * \param cb_prio The priority of the timer callback functor within the servant's scheduler.
     */
    template <class Worker,class F1, class F2>
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    void post_callback(Worker& wscheduler,F1 func,F2 cb_func, std::string const& task_name, std::size_t post_prio, std::size_t cb_prio) const
#else
    void post_callback(Worker& wscheduler,F1 func,F2 cb_func, std::string const& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0) const
#endif
    {
        // we want to log if possible
        boost::asynchronous::post_callback(wscheduler,
                                        boost::asynchronous::check_alive_before_exec(std::move(func),m_tracking),
                                        m_scheduler,
                                        boost::asynchronous::check_alive(std::move(cb_func),m_tracking),
                                        task_name,
                                        post_prio,
                                        cb_prio);
    }

    /*!
     * \brief Posts an interruptible task to any threadpool scheduler and get a safe callback when the task is done.
     * \brief The task must be interruptible as defined by Boost Thread
     * \param func task functor
     * \param cb_func callback functor
     * \param task_name which will be displayed in the diagnostic of the servant's scheduler.
     * \param post_prio The priority of the posted task within the threadpool scheduler.
     * \param cb_prio The priority of the timer callback functor within the servant's scheduler.
     * \return any_interruptible which can be used to interrupt the task
     */
    template <class Worker,class F1, class F2>
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    boost::asynchronous::any_interruptible interruptible_post_callback(Worker& wscheduler,F1 func,F2 cb_func, std::string const& task_name,
                                                                    std::size_t post_prio, std::size_t cb_prio)const
#else
    boost::asynchronous::any_interruptible interruptible_post_callback(Worker& wscheduler,F1 func,F2 cb_func, std::string const& task_name="",
                                                                    std::size_t post_prio=0, std::size_t cb_prio=0)const
#endif
    {
        return boost::asynchronous::interruptible_post_callback(
                                        wscheduler,
                                        boost::asynchronous::check_alive_before_exec(std::move(func),m_tracking),
                                        m_scheduler,
                                        boost::asynchronous::check_alive(std::move(cb_func),m_tracking),
                                        task_name,
                                        post_prio,
                                        cb_prio);
    }

    /*!
     * \brief Posts a task to the servant's own scheduler.
     * \param func task functor
     * \param task_name which will be displayed in the diagnostic of the servant's scheduler.
     * \param post_prio The priority of the posted task within the threadpool scheduler.
     */
    template <class F1>
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    auto post_self(F1 func, std::string const& task_name, std::size_t post_prio)const
#else
    auto post_self(F1 func, std::string const& task_name="", std::size_t post_prio=0)const
#endif
        -> boost::future<typename boost::asynchronous::detail::get_return_type_if_possible_continuation<decltype(func())>::type>
    {
        boost::asynchronous::any_shared_scheduler<JOB> sched = m_scheduler.lock();
        if (sched.is_valid())
        {
            // we want to log if possible
            return boost::asynchronous::post_future(sched,
                                                    boost::asynchronous::check_alive_before_exec(std::move(func),m_tracking),
                                                    task_name,
                                                    post_prio);
        }
        // no valid scheduler, must be shutdown
        boost::promise<typename boost::asynchronous::detail::get_return_type_if_possible_continuation<decltype(func())>::type> p;
        p.set_exception(boost::asynchronous::task_aborted_exception());
        return p.get_future();
    }

    /*!
     * \brief Posts an interruptible task to the servant's own scheduler.
     * \param func task functor
     * \param task_name which will be displayed in the diagnostic of the servant's scheduler.
     * \param post_prio The priority of the posted task within the threadpool scheduler.
     * \return any_interruptible which can be used to interrupt the task
     */
    template <class F1>
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    auto interruptible_post_self(F1 func, std::string const& task_name,std::size_t post_prio)const
#else
    auto interruptible_post_self(F1 func, std::string const& task_name="",std::size_t post_prio=0)const
#endif
    -> std::tuple<boost::future<typename boost::asynchronous::detail::get_return_type_if_possible_continuation<decltype(func())>::type>,
                  boost::asynchronous::any_interruptible >
    {
        boost::asynchronous::any_shared_scheduler<JOB> sched = m_scheduler.lock();
        if (sched.is_valid())
        {
            // we want to log if possible
            return boost::asynchronous::interruptible_post_future(
                                            sched,
                                            boost::asynchronous::check_alive_before_exec(std::move(func),m_tracking),
                                            task_name,
                                            post_prio);
        }
        // no valid scheduler, must be shutdown
        boost::promise<typename boost::asynchronous::detail::get_return_type_if_possible_continuation<decltype(func())>::type> p;
        p.set_exception(boost::asynchronous::task_aborted_exception());
        return std::make_tuple(p.get_future(),boost::asynchronous::any_interruptible());
    }
    /*!
     * \brief Posts a task to the servant's threadpool scheduler and get a future corresponding to this task.
     * \param func task functor
     * \param task_name which will be displayed in the diagnostic of the servant's scheduler.
     * \param post_prio The priority of the posted task within the threadpool scheduler.
     * \return future<task result type>
     */
    template <class F1>
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    auto post_future(F1 func, std::string const& task_name, std::size_t post_prio)const
#else
    auto post_future(F1 func, std::string const& task_name="", std::size_t post_prio=0)const
#endif
        -> boost::future<typename boost::asynchronous::detail::get_return_type_if_possible_continuation<decltype(func())>::type>
    {
        // we want to log if possible
        return boost::asynchronous::post_future(
                                        m_worker,
                                        boost::asynchronous::check_alive_before_exec(std::move(func),m_tracking),
                                        task_name,
                                        post_prio);
    }

    /*!
     * \brief Posts an interruptible task to the servant's threadpool scheduler and get a future corresponding to this task.
     * \param func task functor
     * \param task_name which will be displayed in the diagnostic of the servant's scheduler.
     * \param post_prio The priority of the posted task within the threadpool scheduler.
     * \return tuple<future<task result type>,any_interruptible>
     */
    template <class F1>
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    auto interruptible_post_future(F1 func, std::string const& task_name,std::size_t post_prio)const

#else
    auto interruptible_post_future(F1 func, std::string const& task_name="",std::size_t post_prio=0)const
#endif
     -> std::tuple<boost::future<typename boost::asynchronous::detail::get_return_type_if_possible_continuation<decltype(func())>::type>,
                   boost::asynchronous::any_interruptible >
    {
        // we want to log if possible
        return boost::asynchronous::interruptible_post_future(
                                        m_worker,
                                        boost::asynchronous::check_alive_before_exec(std::move(func),m_tracking),
                                        task_name,
                                        post_prio);
    }

    /*!
     * \brief Posts a task to any threadpool scheduler and get a future corresponding to this task.
     * \param func task functor
     * \param task_name which will be displayed in the diagnostic of the servant's scheduler.
     * \param post_prio The priority of the posted task within the threadpool scheduler.
     * \return future<task result type>
     */
    template <class Worker,class F1>
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    auto post_future(Worker& wscheduler,F1 func, std::string const& task_name, std::size_t post_prio)const
#else
    auto post_future(Worker& wscheduler,F1 func, std::string const& task_name="", std::size_t post_prio=0)const
#endif
        -> boost::future<typename boost::asynchronous::detail::get_return_type_if_possible_continuation<decltype(func())>::type>
    {
        // we want to log if possible
        return boost::asynchronous::post_future(
                                        wscheduler,
                                        boost::asynchronous::check_alive_before_exec(std::move(func),m_tracking),
                                        task_name,
                                        post_prio);
    }

    /*!
     * \brief Posts an interruptible task to any threadpool scheduler and get a future corresponding to this task.
     * \param func task functor
     * \param task_name which will be displayed in the diagnostic of the servant's scheduler.
     * \param post_prio The priority of the posted task within the threadpool scheduler.
     * \return tuple<future<task result type>,any_interruptible>
     */
    template <class Worker,class F1>
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    auto interruptible_post_future(Worker& wscheduler,F1 func, std::string const& task_name,std::size_t post_prio)const
#else
    auto interruptible_post_future(Worker& wscheduler,F1 func, std::string const& task_name="",std::size_t post_prio=0)const
#endif
     -> std::tuple<boost::future<typename boost::asynchronous::detail::get_return_type_if_possible_continuation<decltype(func())>::type>,
                   boost::asynchronous::any_interruptible >
    {
        // we want to log if possible
        return boost::asynchronous::interruptible_post_future(
                                        wscheduler,
                                        boost::asynchronous::check_alive_before_exec(std::move(func),m_tracking),
                                        task_name,
                                        post_prio);
    }
    /*!
     * \brief Posts a task to any scheduler and get a callback corresponding to this task.
     * \brief This is to be used with BOOST_ASYNC_UNSAFE_MEMBER provided by a Servant Proxy.
     * \brief Unsafe means that the call to this Servant Proxy member must be protected through call_callback.
     * \param s Scheduler where func executes
     * \param func task functor
     * \param task_name which will be displayed in the diagnostic of the servant's scheduler.
     * \param post_prio The priority of the posted task within the threadpool scheduler.
     * \return void
     */
    template <class CallerSched,class F1, class F2>
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    void call_callback(CallerSched s, F1 func,F2 cb_func, std::string const& task_name, std::size_t post_prio, std::size_t cb_prio)const
#else
    void call_callback(CallerSched s, F1 func,F2 cb_func, std::string const& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0)const
#endif
    {
        // we want to log if possible
        boost::asynchronous::post_callback(s,
                                        boost::asynchronous::check_alive_before_exec(std::move(func),m_tracking),
                                        m_scheduler,
                                        boost::asynchronous::check_alive(std::move(cb_func),m_tracking),
                                        task_name,
                                        post_prio,
                                        cb_prio);
    }

    /*!
     * \brief Posts an interruptible task to any scheduler and get a callback corresponding to this task.
     * \brief This is to be used with BOOST_ASYNC_UNSAFE_MEMBER provided by a Servant Proxy.
     * \brief Unsafe means that the call to this Servant Proxy member must be protected through call_callback.
     * \param s Scheduler where func executes
     * \param func task functor
     * \param task_name which will be displayed in the diagnostic of the servant's scheduler.
     * \param post_prio The priority of the posted task within the threadpool scheduler.
     * \return boost::asynchronous::any_interruptible to be used for interruption
     */
    template <class CallerSched,class F1, class F2>
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    boost::asynchronous::any_interruptible interruptible_call_callback(CallerSched s, F1 func,F2 cb_func, std::string const& task_name,
                                                                    std::size_t post_prio, std::size_t cb_prio)const
#else
    boost::asynchronous::any_interruptible interruptible_call_callback(CallerSched s, F1 func,F2 cb_func, std::string const& task_name="",
                                                                    std::size_t post_prio=0, std::size_t cb_prio=0)const
#endif
    {
        return boost::asynchronous::interruptible_post_callback(
                                        s,
                                        boost::asynchronous::check_alive_before_exec(std::move(func),m_tracking),
                                        m_scheduler,
                                        boost::asynchronous::check_alive(std::move(cb_func),m_tracking),
                                        task_name,
                                        post_prio,
                                        cb_prio);
    }

    // connect to a signal from any thread
    template <class SlotFct, class Signal>
    void safe_slot(Signal& signal, SlotFct slot,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    const std::string& task_name, std::size_t prio)
#else
    const std::string& task_name="", std::size_t prio=0)
#endif
    {
        signal.connect(typename Signal::slot_type(make_safe_callback(std::move(slot),task_name,prio)).track(m_tracking));
    }

    /*!
     * \brief Returns the worker threadpool used by this servant.
     * \return any_shared_scheduler_proxy<WJOB> hiding the threadpool type. WJOB is the Job type of the threadpool.
     */
    boost::asynchronous::any_shared_scheduler_proxy<WJOB> const& get_worker()const
    {
        return m_worker;
    }

    /*!
     * \brief Replaces the worker threadpool used by this servant.
     * \param w any_shared_scheduler_proxy<WJOB> hiding the threadpool type. WJOB is the Job type of the threadpool.
     */
    void set_worker(boost::asynchronous::any_shared_scheduler_proxy<WJOB> w)
    {
        m_worker=w;
    }

    /*!
     * \brief Sets the worker threadpool used by this servant.
     * \brief Warning. This is not made to replace the scheduler, which would be a race, but to set it in some corner cases.
     * \param s boost::asynchronous::any_weak_scheduler<JOB> hiding the scheduler type. JOB is the Job type of the servant.
     */
    void set_scheduler(boost::asynchronous::any_weak_scheduler<JOB> s)
    {
        m_scheduler=s;
    }

    /*!
     * \brief Returns the servant scheduler as a weak scheduler.
     * \return boost::asynchronous::any_weak_scheduler<JOB>. JOB is the Job type of the servant scheduler.
     */
    boost::asynchronous::any_weak_scheduler<JOB> const& get_scheduler()const
    {
        return m_scheduler;
    }

private:
    template<typename... Args>
    std::function<void(Args... )> make_safe_callback_helper(std::function<void(Args... )> func,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                                                     const std::string& task_name, std::size_t prio) const
#else
                                                     const std::string& task_name="", std::size_t prio=0) const
#endif
    {
        boost::weak_ptr<track> tracking (m_tracking);
        boost::asynchronous::any_weak_scheduler<JOB> wscheduler = get_scheduler();
        //TODO functor with move
        boost::shared_ptr<std::function<void(Args... )>> func_ptr =
                boost::make_shared<std::function<void(Args... )>>(std::move(func));
        std::function<void(Args...)> res = [func_ptr,tracking,wscheduler,task_name,prio](Args... as)mutable
        {
            boost::asynchronous::any_shared_scheduler<JOB> sched = wscheduler.lock();
            if (sched.is_valid())
            {
                std::vector<boost::thread::id> ids = sched.thread_ids();
                if (ids.size() == 1 && (ids[0] == boost::this_thread::get_id()))
                {
                    // our thread, call if servant alive
                   boost::asynchronous::move_bind( boost::asynchronous::check_alive([func_ptr](Args... args){(*func_ptr)(std::move(args)...);},tracking),
                                                   std::move(as)...)();
                }
                else
                {
                    // not in our thread, post
                    boost::asynchronous::post_future(sched,
                                                     boost::asynchronous::move_bind( boost::asynchronous::check_alive([func_ptr](Args... args){(*func_ptr)(std::move(args)...);},tracking),
                                                                                     std::move(as)...),
                                                     task_name,prio);
                }
            }
        };
        return res;
    }

protected:
    // tracking object for callbacks / tasks
    boost::shared_ptr<track> m_tracking;
private:
    // scheduler where we are living
    boost::asynchronous::any_weak_scheduler<JOB> m_scheduler;
    // our worker pool
    boost::asynchronous::any_shared_scheduler_proxy<WJOB> m_worker;

};

}}

#endif // BOOST_ASYNCHRON_TRACKABLE_SERVANT_HPP
