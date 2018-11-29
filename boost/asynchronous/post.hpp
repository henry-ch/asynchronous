// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2016
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

// this file provides the posting workhorse functions in all their forms:
// post_future: posts a task to a pool, returns a future (similar to std::async without the blocking destructor).
// interruptible_post_future: same as above but returns a tuple of a future and an any_interruptible (for interrupting)
// post_callback: posts a task, call a callback when done (careful: the callback is called from any thread of the pool and must be thread-safe)
// or a trackable_servant's post_callback must be used. Returns nothing.
// interruptible_post_callback: same as above but returns an any_interruptible.


#ifndef BOOST_ASYNCHRON_POST_HPP
#define BOOST_ASYNCHRON_POST_HPP

#include <cstddef>

#ifndef BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE
#endif

#include <memory>
#include <type_traits>
#include <boost/mpl/has_xxx.hpp>

#include <boost/asynchronous/any_shared_scheduler_proxy.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/exceptions.hpp>
#include <boost/asynchronous/job_traits.hpp>
#include <boost/asynchronous/expected.hpp>
#include <boost/asynchronous/scheduler/detail/any_continuation.hpp>
#include <boost/asynchronous/scheduler/tss_scheduler.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/client_request.hpp>

namespace boost { namespace asynchronous
{
// Workaround for non-existing is_value in std:
template<typename R>
bool is_ready(std::future<R> const& f)
{
    return f.valid() && f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}
template<typename R>
bool is_ready(std::shared_future<R> const& f)
{
    return f.valid() && f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}
template<typename T>
bool is_ready(T const& f)
{
    return f.has_value() || f.has_exception();
}
//TODO move to own file in detail
template <class R, class Task,class Callback>
struct move_task_helper
{
    typedef R result_type;
    typedef Task task_type;

    move_task_helper(move_task_helper const& r)noexcept
        : m_callback(std::move(const_cast<move_task_helper&>(r).m_callback))
        , m_task(std::move(const_cast<move_task_helper&>(r).m_task))
    {
    }
    move_task_helper(Task t,Callback c)
        :m_callback(std::move(c))
        ,m_task(std::move(t))
    {
    }
    move_task_helper(move_task_helper && r)noexcept
        : m_callback(std::move(r.m_callback))
        , m_task(std::move(r.m_task))
    {
    }
    move_task_helper& operator= (move_task_helper&& r)noexcept
    {
        std::swap(m_callback,r.m_callback);
        std::swap(m_task,r.m_task);
    }
    void operator()(boost::asynchronous::expected<R> result_func)
    {
        m_callback(std::move(result_func));
    }
    Callback m_callback;
    // task is only here to be moved to worker and then back for destruction in originator thread
    Task m_task;
};

namespace detail
{
    template <class R,class F,class JOB, class OP,class Scheduler,class Enable=void>
    struct post_future_helper_base : public boost::asynchronous::job_traits<JOB>::diagnostic_type
    {
        post_future_helper_base():boost::asynchronous::job_traits<JOB>::diagnostic_type(){}

        post_future_helper_base(std::promise<R> p, F f)
            : boost::asynchronous::job_traits<JOB>::diagnostic_type(),
              m_promise(std::move(p)),m_func(std::move(f)) {}
        post_future_helper_base(post_future_helper_base&& rhs)noexcept
            : boost::asynchronous::job_traits<JOB>::diagnostic_type(),
              m_promise(std::move(rhs.m_promise)),m_func(std::move(rhs.m_func)) {}
        post_future_helper_base& operator= (post_future_helper_base&& rhs)noexcept
        {
            std::swap(m_promise,rhs.m_promise);
            std::swap(m_func,rhs.m_func);
            return *this;
        }

        post_future_helper_base(post_future_helper_base const& rhs)
            :boost::asynchronous::job_traits<JOB>::diagnostic_type()
            ,m_promise(std::move(const_cast<post_future_helper_base&>(rhs).m_promise)),m_func(std::move(const_cast<post_future_helper_base&>(rhs).m_func)){}
        void operator()()
        {
            try
            {
                OP()(m_promise,m_func);
            }
            catch(boost::thread_interrupted&)
            {
                boost::asynchronous::task_aborted_exception ta;
                m_promise.set_exception(std::make_exception_ptr(ta));this->set_failed();
            }
            catch(...){m_promise.set_exception(std::current_exception());this->set_failed();}
        }
        std::promise<R> m_promise;
        F m_func;
    };

    // version for serializable tasks but not continuation tasks
    // TODO a bit better than checking R...
    template <class R,class F,class JOB, class OP,class Scheduler>
    struct post_future_helper_base<R,F,JOB,OP,Scheduler,
            typename std::enable_if<
                boost::asynchronous::detail::is_serializable<F>::value &&
                !std::is_same<R,void>::value >::type >
            : public boost::asynchronous::job_traits<JOB>::diagnostic_type
    {
        post_future_helper_base():boost::asynchronous::job_traits<JOB>::diagnostic_type(){}

        post_future_helper_base(std::promise<R> p, F f)
            : boost::asynchronous::job_traits<JOB>::diagnostic_type(),
              m_promise(std::move(p)),m_func(std::move(f)) {}
        post_future_helper_base(post_future_helper_base&& rhs)noexcept
            : boost::asynchronous::job_traits<JOB>::diagnostic_type(),
              m_promise(std::move(rhs.m_promise)),m_func(std::move(rhs.m_func)) {}

        post_future_helper_base& operator= (post_future_helper_base&& rhs)noexcept
        {
            std::swap(m_promise,rhs.m_promise);
            std::swap(m_func,rhs.m_func);
            return *this;
        }


        post_future_helper_base(post_future_helper_base const& rhs)
            :boost::asynchronous::job_traits<JOB>::diagnostic_type()
            ,m_promise(std::move(const_cast<post_future_helper_base&>(rhs).m_promise))
            ,m_func(std::move(const_cast<post_future_helper_base&>(rhs).m_func)){}

        template <class Archive>
        void save(Archive & ar, const unsigned int /*version*/)const
        {
            ar & m_func;
        }
        template <class Archive>
        void load(Archive & ar, const unsigned int /*version*/)
        {
            boost::asynchronous::tcp::client_request::message_payload payload;
            ar >> payload;
            if (!payload.m_has_exception)
            {
                std::istringstream archive_stream(std::move(payload.m_data));
                typedef typename Scheduler::job_type jobtype;
                typename jobtype::iarchive archive(archive_stream);
                R res;
                archive >> res;
                m_promise.set_value(std::move(res));
            }
            else
            {
                m_promise.set_exception(std::make_exception_ptr(payload.m_exception));
            }
        }
        std::string get_task_name()const
        {
            return m_func.get_task_name();
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()
        void operator()()
        {
            try
            {
                OP()(m_promise,m_func);
            }
            catch(boost::thread_interrupted&)
            {
                boost::asynchronous::task_aborted_exception ta;
                m_promise.set_exception(std::make_exception_ptr(ta));this->set_failed();
            }
            catch(...){m_promise.set_exception(std::current_exception());this->set_failed();}
        }
        std::promise<R> m_promise;
        F m_func;
    };
    // version for tasks which are serializable AND continuation tasks
    // TODO a bit better than checking R...
    template <class R,class F,class JOB, class OP,class Scheduler>
    struct post_future_helper_base<R,F,JOB,OP,Scheduler,
            typename std::enable_if<
                boost::asynchronous::detail::is_serializable<F>::value &&
                std::is_same<R,void>::value >::type >
            : public boost::asynchronous::job_traits<JOB>::diagnostic_type
    {
        post_future_helper_base():boost::asynchronous::job_traits<JOB>::diagnostic_type(){}

        post_future_helper_base(std::promise<R> p, F f)
            : boost::asynchronous::job_traits<JOB>::diagnostic_type(),
              m_promise(std::move(p)),m_func(std::move(f)) {}
        post_future_helper_base(post_future_helper_base&& rhs)noexcept
            : boost::asynchronous::job_traits<JOB>::diagnostic_type(),
              m_promise(std::move(rhs.m_promise)),m_func(std::move(rhs.m_func)) {}
        post_future_helper_base& operator= (post_future_helper_base&& rhs)noexcept
        {
            std::swap(m_promise,rhs.m_promise);
            std::swap(m_func,rhs.m_func);
            return *this;
        }

        post_future_helper_base(post_future_helper_base const& rhs)
            :boost::asynchronous::job_traits<JOB>::diagnostic_type()
            ,m_promise(std::move(const_cast<post_future_helper_base&>(rhs).m_promise))
            ,m_func(std::move(const_cast<post_future_helper_base&>(rhs).m_func)){}

        template <class Archive>
        void save(Archive & ar, const unsigned int /*version*/)const
        {
            ar & m_func;
        }
        template <class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            m_func.template as_result<typename Scheduler::job_type::iarchive,Archive>(ar,version);
        }
        std::string get_task_name()const
        {
            return m_func.get_task_name();
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()
        void operator()()
        {
            try
            {
                OP()(m_promise,m_func);
            }
            catch(boost::thread_interrupted&)
            {
                boost::asynchronous::task_aborted_exception ta;
                m_promise.set_exception(std::make_exception_ptr(ta));this->set_failed();
            }
            catch(...){m_promise.set_exception(std::current_exception());this->set_failed();}
        }
        std::promise<R> m_promise;
        F m_func;
    };

    // version for continuations
    template <class R,class F,class JOB,class Enable=void>
    struct post_future_helper_continuation : public boost::asynchronous::job_traits<JOB>::diagnostic_type
    {
        post_future_helper_continuation():boost::asynchronous::job_traits<JOB>::diagnostic_type(){}

        post_future_helper_continuation(std::promise<R> p, F f)
            : boost::asynchronous::job_traits<JOB>::diagnostic_type(),
              m_promise(std::move(p)),m_func(std::move(f)) {}
        post_future_helper_continuation(post_future_helper_continuation&& rhs)noexcept
            : boost::asynchronous::job_traits<JOB>::diagnostic_type(),
              m_promise(std::move(rhs.m_promise)),m_func(std::move(rhs.m_func)) {}
        post_future_helper_continuation& operator= (post_future_helper_continuation&& rhs)noexcept
        {
            std::swap(m_promise,rhs.m_promise);
            std::swap(m_func,rhs.m_func);
            return *this;
        }

        post_future_helper_continuation(post_future_helper_continuation const& rhs)
            :boost::asynchronous::job_traits<JOB>::diagnostic_type()
            ,m_promise(std::move(const_cast<post_future_helper_continuation&>(rhs).m_promise)),m_func(std::move(const_cast<post_future_helper_continuation&>(rhs).m_func)){}

        // to move our promise into continuation
        struct promise_move_helper
        {
            promise_move_helper(std::promise<R> p):m_promise(std::move(p)){}
            promise_move_helper(promise_move_helper&& rhs)noexcept
                : m_promise(std::move(rhs.m_promise))
            {}
            promise_move_helper& operator= (promise_move_helper&& rhs)noexcept
            {
                std::swap(m_promise,rhs.m_promise);
                return *this;
            }
            promise_move_helper(promise_move_helper const& rhs)noexcept
                : m_promise(std::move(const_cast<promise_move_helper&>(rhs).m_promise))
            {}
            promise_move_helper& operator= (promise_move_helper const& rhs)noexcept
            {
                std::swap(m_promise,const_cast<promise_move_helper&>(rhs).m_promise);
                return *this;
            }
            template <class Res>
            void operator()(std::tuple<Res>&& continuation_res)
            {
                if(!boost::asynchronous::is_ready(std::get<0>(continuation_res)))
                {
                    boost::asynchronous::task_aborted_exception ta;
                    m_promise.set_exception(std::make_exception_ptr(ta));
                }
                else
                {
                    try
                    {
                        m_promise.set_value(std::move(std::get<0>(continuation_res).get()));
                    }
                    catch(...)
                    {
                        m_promise.set_exception(std::current_exception());
                    }
                }
            }
            std::promise<R> m_promise;
        };

        void operator()()
        {
            try
            {
                auto cont = m_func();
                cont.on_done(promise_move_helper(std::move(m_promise)));
                boost::asynchronous::any_continuation ac(std::move(cont));
                boost::asynchronous::get_continuations().emplace_front(std::move(ac));
            }
            catch(...)
            {
                m_promise.set_exception(std::current_exception());
                this->set_failed();
            }
        }
        std::promise<R> m_promise;
        F m_func;
    };

    // version for void continuations
    template <class R,class F,class JOB>
    struct post_future_helper_continuation<R,F,JOB,typename std::enable_if<std::is_same<R,void>::value>::type>
            : public boost::asynchronous::job_traits<JOB>::diagnostic_type
    {
        post_future_helper_continuation():boost::asynchronous::job_traits<JOB>::diagnostic_type(){}

        post_future_helper_continuation(std::promise<R> p, F f)
            : boost::asynchronous::job_traits<JOB>::diagnostic_type(),
              m_promise(std::move(p)),m_func(std::move(f)) {}
        post_future_helper_continuation(post_future_helper_continuation&& rhs)noexcept
            : boost::asynchronous::job_traits<JOB>::diagnostic_type(),
              m_promise(std::move(rhs.m_promise)),m_func(std::move(rhs.m_func)) {}
        post_future_helper_continuation& operator= (post_future_helper_continuation&& rhs)noexcept
        {
            std::swap(m_promise,rhs.m_promise);
            std::swap(m_func,rhs.m_func);
            return *this;
        }

        post_future_helper_continuation(post_future_helper_continuation const& rhs)
            :boost::asynchronous::job_traits<JOB>::diagnostic_type()
            ,m_promise(std::move(const_cast<post_future_helper_continuation&>(rhs).m_promise)),m_func(std::move(const_cast<post_future_helper_continuation&>(rhs).m_func)){}

        // to move our promise into continuation
        struct promise_move_helper
        {
            promise_move_helper(std::promise<R> p):m_promise(std::move(p)){}
            promise_move_helper(promise_move_helper&& rhs)noexcept
                : m_promise(std::move(rhs.m_promise))
            {}
            promise_move_helper& operator= (promise_move_helper&& rhs)noexcept
            {
                std::swap(m_promise,rhs.m_promise);
                return *this;
            }
            promise_move_helper(promise_move_helper const& rhs)noexcept
                : m_promise(std::move(const_cast<promise_move_helper&>(rhs).m_promise))
            {}
            promise_move_helper& operator= (promise_move_helper const& rhs)noexcept
            {
                std::swap(m_promise,const_cast<promise_move_helper&>(rhs).m_promise);
                return *this;
            }
            template <class Res>
            void operator()(std::tuple<Res>&& continuation_res)
            {
                if(!boost::asynchronous::is_ready(std::get<0>(continuation_res)))
                {
                    boost::asynchronous::task_aborted_exception ta;
                    m_promise.set_exception(std::make_exception_ptr(ta));
                }
                else
                {
                    try
                    {
                        m_promise.set_value();
                    }
                    catch(...)
                    {
                        m_promise.set_exception(std::current_exception());
                    }
                }
            }
            std::promise<R> m_promise;
        };

        void operator()()
        {
            try
            {
                auto cont = m_func();
                cont.on_done(promise_move_helper(std::move(m_promise)));
                boost::asynchronous::any_continuation ac(std::move(cont));
                boost::asynchronous::get_continuations().emplace_front(std::move(ac));
            }
            catch(...)
            {
                m_promise.set_exception(std::current_exception());
                this->set_failed();
            }
        }
        std::promise<R> m_promise;
        F m_func;
    };

    // version for continuations and serializable non-void tasks (we consider void tasks as useless for serialization)
    template <class R,class F,class JOB>
    struct post_future_helper_continuation<R,F,JOB,
                                          typename std::enable_if<boost::asynchronous::detail::is_serializable<F>::value>::type>
            : public boost::asynchronous::job_traits<JOB>::diagnostic_type
    {
        post_future_helper_continuation():boost::asynchronous::job_traits<JOB>::diagnostic_type(){}

        post_future_helper_continuation(std::promise<R> p, F f)
            : boost::asynchronous::job_traits<JOB>::diagnostic_type(),
              m_promise(std::move(p)),m_func(std::move(f)) {}
        post_future_helper_continuation(post_future_helper_continuation&& rhs)noexcept
            : boost::asynchronous::job_traits<JOB>::diagnostic_type(),
              m_promise(std::move(rhs.m_promise)),m_func(std::move(rhs.m_func)) {}
        post_future_helper_continuation& operator= (post_future_helper_continuation&& rhs)noexcept
        {
            std::swap(m_promise,rhs.m_promise);
            std::swap(m_func,rhs.m_func);
            return *this;
        }

        post_future_helper_continuation(post_future_helper_continuation const& rhs)
            :boost::asynchronous::job_traits<JOB>::diagnostic_type()
            ,m_promise(std::move(const_cast<post_future_helper_continuation&>(rhs).m_promise)),m_func(std::move(const_cast<post_future_helper_continuation&>(rhs).m_func)){}

        // to move our promise into continuation
        struct promise_move_helper
        {
            promise_move_helper(std::promise<R> p):m_promise(std::move(p)){}
            promise_move_helper(promise_move_helper&& rhs)noexcept
                : m_promise(std::move(rhs.m_promise))
            {}
            promise_move_helper& operator= (promise_move_helper&& rhs)noexcept
            {
                std::swap(m_promise,rhs.m_promise);
                return *this;
            }
            promise_move_helper(promise_move_helper const& rhs)noexcept
                : m_promise(std::move(const_cast<promise_move_helper&>(rhs).m_promise))
            {}
            promise_move_helper& operator= (promise_move_helper const& rhs)noexcept
            {
                std::swap(m_promise,const_cast<promise_move_helper&>(rhs).m_promise);
                return *this;
            }
            template <class Res>
            void operator()(std::tuple<Res>&& continuation_res)
            {
                if(!std::get<0>(continuation_res).has_value())
                {
                    boost::asynchronous::task_aborted_exception ta;
                    m_promise.set_exception(std::make_exception_ptr(ta));
                }
                else
                {
                    try
                    {
                        m_promise.set_value(std::move(std::get<0>(continuation_res).get()));
                    }
                    catch(...)
                    {
                        m_promise.set_exception(std::current_exception());
                    }
                }
            }
            std::promise<R> m_promise;
        };

        void operator()()
        {
            try
            {
                auto cont = m_func();
                cont.on_done(promise_move_helper(std::move(m_promise)));
                boost::asynchronous::any_continuation ac(std::move(cont));
                boost::asynchronous::get_continuations().emplace_front(std::move(ac));
            }
            catch(...)
            {
                m_promise.set_exception(std::current_exception());
                this->set_failed();
            }
        }
        template <class Archive>
        void save(Archive & ar, const unsigned int /*version*/)const
        {
            ar & m_func;
        }
        template <class Archive>
        void load(Archive & ar, const unsigned int /*version*/)
        {
            boost::asynchronous::tcp::client_request::message_payload payload;
            ar >> payload;
            if (!payload.m_has_exception)
            {
                std::istringstream archive_stream(std::move(payload.m_data));
                typename JOB::iarchive archive(archive_stream);
                R res;
                archive >> res;
                m_promise.set_value(std::move(res));
            }
            else
            {
                m_promise.set_exception(std::make_exception_ptr(payload.m_exception));
                this->set_failed();
            }
        }
        std::string get_task_name()const
        {
            return m_func.get_task_name();
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()

        std::promise<R> m_promise;
        F m_func;
    };
}

template <class F, class S>
#ifndef BOOST_NO_RVALUE_REFERENCES
auto post_future(S const& scheduler, F func,
#else
auto post_future(S const& scheduler, F const& func,
#endif
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                 const std::string& task_name, std::size_t prio,
                 typename std::enable_if< !(std::is_same<void,decltype(func())>::value ||
                                              boost::asynchronous::detail::has_is_continuation_task<decltype(func())>::value)>::type* =0)
#else
                 const std::string& task_name="", std::size_t prio=0,
                 typename std::enable_if< !(std::is_same<void,decltype(func())>::value ||
                                              boost::asynchronous::detail::has_is_continuation_task<decltype(func())>::value)>::type* =0)
#endif
    -> std::future<decltype(func())>
{
    using promise_type = std::promise<decltype(func())>;
    promise_type p;
    std::future<decltype(func())> fu(p.get_future());

    struct post_helper
    {
        void operator()(promise_type& sp, F& f)const
        {
            sp.set_value(f());
        }
    };

#ifndef BOOST_NO_RVALUE_REFERENCES
    detail::post_future_helper_base<decltype(func()),F,typename S::job_type,post_helper,S> fct (std::move(p),std::move(func));
    typename boost::asynchronous::job_traits<typename S::job_type>::wrapper_type w(std::move(fct));
    w.set_name(task_name);
    scheduler.post(std::move(w),prio);
#else
    detail::post_future_helper_base<decltype(func()),F,typename S::job_type,post_helper,S> fct (p,func);
    if (task_name.empty())
    {
        scheduler.post(fct,prio);
    }
    else
    {
        scheduler.post_log(fct,task_name,prio);
    }
#endif
    return std::move(fu);
}

template <class F, class S>
#ifndef BOOST_NO_RVALUE_REFERENCES
auto post_future(S const& scheduler, F func,
#else
auto post_future(S const& scheduler, F const& func,
#endif
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                 const std::string& task_name, std::size_t prio,
                 typename std::enable_if<
                                     std::is_same<void,decltype(func())>::value &&
                                    !boost::asynchronous::detail::has_is_continuation_task<decltype(func())>::value>::type* =0)
#else
                 const std::string& task_name="", std::size_t prio=0,
                 typename std::enable_if<
                                     std::is_same<void,decltype(func())>::value &&
                                    !boost::asynchronous::detail::has_is_continuation_task<decltype(func())>::value>::type* =0)
#endif
    -> std::future<void>
{
    std::promise<void> p;
    std::future<void> fu(p.get_future());

    struct post_helper
    {
        void operator()(std::promise<void>& sp, F& f)const
        {
            f();
            sp.set_value();
        }
    };

#ifndef BOOST_NO_RVALUE_REFERENCES
    detail::post_future_helper_base<decltype(func()),F,typename S::job_type,post_helper,S> fct (std::move(p),std::move(func));
    typename boost::asynchronous::job_traits<typename S::job_type>::wrapper_type w(std::move(fct));
    w.set_name(task_name);
    scheduler.post(std::move(w),prio);
#else
    detail::post_future_helper_base<decltype(func()),F,typename S::job_type,post_helper,S> fct (p,func);
    if (task_name.empty())
    {
        scheduler.post(fct,prio);
    }
    else
    {
        scheduler.post_log(fct,task_name,prio);
    }
#endif

    return fu;
}

template <class F, class S>
#ifndef BOOST_NO_RVALUE_REFERENCES
auto post_future(S const& scheduler, F func,
#else
auto post_future(S const& scheduler, F const& func,
#endif
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                 const std::string& task_name, std::size_t prio, typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<decltype(func())>::value>::type* = 0)
#else
                 const std::string& task_name="", std::size_t prio=0, typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<decltype(func())>::value>::type* =0)
#endif
#ifndef _MSC_VER
 -> std::future<typename decltype(func())::return_type>
#endif

{
    std::promise<typename decltype(func())::return_type> p;
    std::future<typename decltype(func())::return_type> fu(p.get_future());

    detail::post_future_helper_continuation<typename decltype(func())::return_type,F,typename S::job_type> fct
            (std::move(p),std::move(func));
    typename boost::asynchronous::job_traits<typename S::job_type>::wrapper_type w(std::move(fct));
    w.set_name(task_name);
    scheduler.post(std::move(w),prio);

    return std::move(fu);
}

template <class F, class S>
#ifndef BOOST_NO_RVALUE_REFERENCES
auto interruptible_post_future(S const& scheduler, F func,
#else
auto interruptible_post_future(S const& scheduler, F const& func,
#endif
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                 const std::string& task_name, std::size_t prio,
                 typename std::enable_if<
                    !(std::is_same<void, decltype(func())>::value ||
                    boost::asynchronous::detail::has_is_continuation_task<decltype(func())>::value) >::type* = 0)
#else
                 const std::string& task_name="", std::size_t prio=0,
                 typename std::enable_if<
                    !(std::is_same<void, decltype(func())>::value ||
                    boost::asynchronous::detail::has_is_continuation_task<decltype(func())>::value) >::type* = 0)
#endif
    -> std::tuple<std::future<decltype(func())>,boost::asynchronous::any_interruptible >
{
    using promise_type = std::promise<decltype(func())>;
    promise_type p;
    std::future<decltype(func())> fu(p.get_future());

    struct post_helper
    {
        void operator()(promise_type& sp, F& f)const
        {
            sp.set_value(f());
        }
    };

    detail::post_future_helper_base<decltype(func()),F,typename S::job_type,post_helper,S> fct (std::move(p),std::move(func));
    typename boost::asynchronous::job_traits<typename S::job_type>::wrapper_type w(std::move(fct));
    w.set_name(task_name);
    return std::make_tuple(std::move(fu),scheduler.interruptible_post(std::move(w),prio));
}
template <class F, class S>
#ifndef BOOST_NO_RVALUE_REFERENCES
auto interruptible_post_future(S const& scheduler, F func,
#else
auto interruptible_post_future(S const& scheduler, F const& func,
#endif
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                 const std::string& task_name, std::size_t prio, 
                 typename std::enable_if<
                   std::is_same<void, decltype(func())>::value &&
                  !boost::asynchronous::detail::has_is_continuation_task<decltype(func())>::value> ::type* = 0)
#else
                 const std::string& task_name="", std::size_t prio=0,
                 typename std::enable_if<
                     std::is_same<void, decltype(func())>::value &&
                    !boost::asynchronous::detail::has_is_continuation_task<decltype(func())>::value> ::type* = 0)
#endif
    -> std::tuple<std::future<void>,boost::asynchronous::any_interruptible >
{
    std::promise<void> p;
    std::future<void> fu(p.get_future());

    struct post_helper
    {
        void operator()(std::promise<void>& sp, F& f)const
        {
            f();
            sp.set_value();
        }
    };

    detail::post_future_helper_base<decltype(func()),F,typename S::job_type,post_helper,S> fct (std::move(p),std::move(func));
    typename boost::asynchronous::job_traits<typename S::job_type>::wrapper_type w(std::move(fct));
    w.set_name(task_name);
    return std::make_tuple(std::move(fu),scheduler.interruptible_post(std::move(w),prio));
}

template <class F, class S>
#ifndef BOOST_NO_RVALUE_REFERENCES
auto interruptible_post_future(S const& scheduler, F func,
#else
auto interruptible_post_future(S const& scheduler, F const& func,
#endif
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    const std::string& task_name, std::size_t prio, typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<decltype(func())>::value>::type* = 0)
#else
const std::string& task_name = "", std::size_t prio = 0, typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<decltype(func())>::value>::type* = 0)
#endif
#ifndef _MSC_VER
    ->std::tuple<std::future<typename decltype(func())::return_type>, boost::asynchronous::any_interruptible >
#endif

{
    std::promise<typename decltype(func())::return_type> p;
    std::future<typename decltype(func())::return_type> fu(p.get_future());

	detail::post_future_helper_continuation<typename decltype(func())::return_type, F, typename S::job_type> fct
		(std::move(p), std::move(func));
	typename boost::asynchronous::job_traits<typename S::job_type>::wrapper_type w(std::move(fct));
    w.set_name(task_name);

	return std::make_tuple(std::move(fu),scheduler.interruptible_post(std::move(w), prio));
}

namespace detail
{
    template <class Work,class Sched,class PostSched,class OP,class Callback,class Enable=void>
    struct post_callback_helper_base : public boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type
    {
#ifndef BOOST_NO_RVALUE_REFERENCES
        post_callback_helper_base(Work&& w, Sched const& s, const std::string& task_name="",
                                  std::size_t cb_prio=0)
            : m_work(std::forward<Work>(w)),m_scheduler(s),m_task_name(task_name),m_cb_prio(cb_prio) {}
        post_callback_helper_base(post_callback_helper_base&& rhs)noexcept
            : m_work(std::move(rhs.m_work)),m_scheduler(rhs.m_scheduler)
            , m_task_name(std::move(rhs.m_task_name)),m_cb_prio(rhs.m_cb_prio) {}
        post_callback_helper_base& operator= (post_callback_helper_base&& rhs)noexcept
        {
            std::swap(m_work,rhs.m_work);
            std::swap(m_scheduler,rhs.m_scheduler);
            std::swap(m_task_name,rhs.m_task_name);
            m_cb_prio = rhs.m_cb_prio;
            return *this;
        }
#endif
        post_callback_helper_base(post_callback_helper_base const& rhs)
            : boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type(),
              m_work(rhs.m_work),m_scheduler(rhs.m_scheduler),m_task_name(rhs.m_task_name),m_cb_prio(rhs.m_cb_prio){}
        post_callback_helper_base(Work const& w, Sched const& s, const std::string& task_name="",std::size_t cb_prio=0)
            : boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type(),
              m_work(w),m_scheduler(s),m_task_name(task_name),m_cb_prio(cb_prio){}

        struct callback_fct : public boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type
        {            
            callback_fct(Work const& w,boost::asynchronous::expected<typename Work::result_type> fu)
                :boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type()
                ,m_work(w)
                ,m_fu(std::move(fu)){}
            // TODO type_erasure problem?
            callback_fct(callback_fct&& rhs)noexcept :m_work(std::move(rhs.m_work)),m_fu(std::move(rhs.m_fu)){}
            callback_fct& operator= (callback_fct&& rhs)noexcept
            {
                std::swap(m_work,rhs.m_work);
                std::swap(m_fu,rhs.m_fu);
                return *this;
            }
            callback_fct(callback_fct const& rhs)noexcept
                :boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type()
                ,m_work(std::move((const_cast<callback_fct&>(rhs)).m_work))
                ,m_fu(std::move((const_cast<callback_fct&>(rhs)).m_fu)){}
            callback_fct& operator= (callback_fct const& rhs)noexcept
            {
                std::swap(m_work,rhs.m_work);
                std::swap(m_fu,rhs.m_fu);
                return *this;
            }
            
            void operator()()
            {
                m_work(std::move(m_fu));
            }
            Work m_work;
            boost::asynchronous::expected<typename Work::result_type> m_fu;
        };

        void operator()()
        {
            boost::asynchronous::expected<typename Work::result_type> ex;
            // TODO check not empty
            try
            {
                // call task
                OP()(m_work,ex);
            }
            catch(boost::asynchronous::task_aborted_exception& e){ex.set_exception(std::make_exception_ptr(e));}
            catch(...){ex.set_exception(std::current_exception());this->set_failed();}
            try
            {
                auto shared_scheduler = m_scheduler.lock();
                if (!shared_scheduler.is_valid())
                    // no need to do any work as there is no way to callback
                    return;
                // call callback
                callback_fct cb(std::move(m_work),std::move(ex));
                Callback()(shared_scheduler,std::move(cb),m_task_name,m_cb_prio);
            }
            catch(...){/* TODO */}
        }
        Work m_work;
        Sched m_scheduler;
        std::string m_task_name;
        std::size_t m_cb_prio;
    };
    struct cb_helper
    {
        //TODO static?
        template <class S, class Callback>
        void operator()(S const& s, Callback&& cb,std::string const& name,std::size_t cb_prio)const
        {
            typename boost::asynchronous::job_traits<typename S::job_type>::wrapper_type w(std::forward<Callback>(cb));
            w.set_name(name);
            s.post(std::move(w),cb_prio);
        }
    };
    // version for serializable tasks
    template <class Work,class Sched,class PostSched,class OP,class Callback>
    struct post_callback_helper_base<Work,Sched,PostSched,OP,Callback,typename std::enable_if<boost::asynchronous::detail::is_serializable<typename Work::task_type>::value >::type>
            : public boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type
    {
#ifndef BOOST_NO_RVALUE_REFERENCES
        post_callback_helper_base(Work&& w, Sched const& s, const std::string& task_name="",
                                  std::size_t cb_prio=0)
            : m_work(std::forward<Work>(w)),m_scheduler(s),m_task_name(task_name),m_cb_prio(cb_prio)
        {
        }
        post_callback_helper_base(post_callback_helper_base&& rhs)noexcept
            : m_work(std::move(rhs.m_work)),m_scheduler(rhs.m_scheduler)
            , m_task_name(std::move(rhs.m_task_name)),m_cb_prio(rhs.m_cb_prio) {}
        post_callback_helper_base& operator= (post_callback_helper_base&& rhs)noexcept
        {
            std::swap(m_work,rhs.m_work);
            std::swap(m_scheduler,rhs.m_scheduler);
            std::swap(m_task_name,rhs.m_task_name);
            m_cb_prio = rhs.m_cb_prio;
            return *this;
        }
#endif
        post_callback_helper_base(post_callback_helper_base const& rhs)
            : boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type(),
              m_work(rhs.m_work),m_scheduler(rhs.m_scheduler),m_task_name(rhs.m_task_name),m_cb_prio(rhs.m_cb_prio){}
        post_callback_helper_base(Work const& w, Sched const& s, const std::string& task_name="",std::size_t cb_prio=0)
            : boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type(),
              m_work(w),m_scheduler(s),m_task_name(task_name),m_cb_prio(cb_prio)
        {
        }

        struct callback_fct : public boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type
        {            
            callback_fct(Work const& w,boost::asynchronous::expected<typename Work::result_type> fu)
                :boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type()
                ,m_work(w)
                ,m_fu(std::move(fu)){}
            callback_fct(callback_fct&& rhs)noexcept:m_work(std::move(rhs.m_work)),m_fu(std::move(rhs.m_fu)){}
            callback_fct& operator= (callback_fct&& rhs)noexcept
            {
                std::swap(m_work,rhs.m_work);
                std::swap(m_fu,rhs.m_fu);
                return *this;
            }
            callback_fct(callback_fct const& rhs)noexcept
                :boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type()
                ,m_work(std::move((const_cast<callback_fct&>(rhs)).m_work))
                ,m_fu(std::move((const_cast<callback_fct&>(rhs)).m_fu)){}
            callback_fct& operator= (callback_fct const& rhs)noexcept
            {
                std::swap(m_work,rhs.m_work);
                std::swap(m_fu,rhs.m_fu);
                return *this;
            }
            void operator()()
            {
                m_work(std::move(m_fu));
            }
            Work m_work;
            boost::asynchronous::expected<typename Work::result_type> m_fu;
        };
        std::string get_task_name()const
        {
            return (m_work.m_task).get_task_name();
        }
        template <class Archive>
        void save(Archive & ar, const unsigned int /*version*/)const
        {
            ar & m_work.m_task;
        }
        template <class Archive>
        void load(Archive & ar, const unsigned int /*version*/)
        {
            boost::asynchronous::tcp::client_request::message_payload payload;
            ar >> payload;
            boost::asynchronous::expected<typename Work::result_type> ex;
            if (!payload.m_has_exception)
            {
                std::istringstream archive_stream(std::move(payload.m_data));
                typedef typename PostSched::job_type jobtype;
                typename jobtype::iarchive archive(archive_stream);

                typename Work::result_type res;
                archive >> res;
                ex.set_value(std::move(res));
            }
            else
            {
                ex.set_exception(std::make_exception_ptr(payload.m_exception));
            }
            callback_ready(std::move(ex));
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()
        void operator()()
        {
            boost::asynchronous::expected<typename Work::result_type> ex;
            // TODO check not empty
            try
            {
                // call task
                OP()(m_work,ex);
            }
            catch(boost::asynchronous::task_aborted_exception& e){ex.set_exception(std::make_exception_ptr(e));}
            catch(...){ex.set_exception(std::current_exception());this->set_failed();}
            try
            {
                auto shared_scheduler = m_scheduler.lock();
                if (!shared_scheduler.is_valid())
                    // no need to do any work as there is no way to callback
                    return;
                // call callback
                callback_fct cb(std::move(m_work),std::move(ex));
                Callback()(shared_scheduler,std::move(cb),m_task_name,m_cb_prio);
            }
            catch(...){/* TODO */}
        }

        void callback_ready(boost::asynchronous::expected<typename Work::result_type> work_fu)
        {
            try
            {
                auto shared_scheduler = m_scheduler.lock();
                if (!shared_scheduler.is_valid())
                    // no need to do any work as there is no way to callback
                    return;
                // call callback
                callback_fct cb(std::move(m_work),std::move(work_fu));
                Callback()(shared_scheduler,std::move(cb),m_task_name,m_cb_prio);
            }
            catch(...){/* TODO */}
        }
        Work m_work;
        Sched m_scheduler;
        std::string m_task_name;
        std::size_t m_cb_prio;
    };
    template <class Ret,class Sched,class Func,class Work,class F1,class F2,class CallbackFct,class Callback,class EnablePostHelper=void>
    struct post_helper_continuation
    {
        void operator()(std::string const& task_name,std::size_t cb_prio, Sched scheduler,
                        move_task_helper<typename Func::return_type,F1,F2>& work)const
        {
            auto cont = work.m_task();
            typedef decltype(cont.get_continuation_args()) cont_return_type;
            cont.on_done([work,scheduler,task_name,cb_prio]
                         (std::tuple<cont_return_type> continuation_res)
                {
                    try
                    {
                        auto shared_scheduler = scheduler.lock();
                        if (!shared_scheduler.is_valid())
                            // no need to do any work as there is no way to callback
                            return;
                        // call callback
                        boost::asynchronous::expected<typename Work::result_type> ex;
                        if(!boost::asynchronous::is_ready(std::get<0>(continuation_res)))
                        {
                            boost::asynchronous::task_aborted_exception ta;
                            ex.set_exception(std::make_exception_ptr(ta));
                        }
                        else
                        {
                            try
                            {
                                ex.set_value(std::move(std::get<0>(continuation_res).get()));
                            }
                            catch(...)
                            {
                                ex.set_exception(std::current_exception());
                            }
                        }

                        CallbackFct cb(std::move(work),std::move(ex));
                        Callback()(shared_scheduler,std::move(cb),task_name,cb_prio);
                    }
                    catch(...){/* TODO */}
                }
            );
            boost::asynchronous::any_continuation ac(std::move(cont));
            boost::asynchronous::get_continuations().emplace_front(std::move(ac));
        }
    };
    template <class Ret,class Sched,class Func,class Work,class F1,class F2,class CallbackFct,class Callback>
    struct post_helper_continuation<Ret,Sched,Func,Work,F1,F2,CallbackFct,Callback,typename std::enable_if<std::is_same<Ret,void>::value >::type>
    {
        void operator()(std::string const& task_name,std::size_t cb_prio, Sched scheduler,
                        move_task_helper<typename Func::return_type,F1,F2>& work)const
        {
            auto cont = work.m_task();
            typedef decltype(cont.get_continuation_args()) cont_return_type;
            cont.on_done([work,scheduler,task_name,cb_prio]
                         (std::tuple<cont_return_type>&& continuation_res)
                {
                    try
                    {
                        auto shared_scheduler = scheduler.lock();
                        if (!shared_scheduler.is_valid())
                            // no need to do any work as there is no way to callback
                            return;
                        // call callback
                        boost::asynchronous::expected<typename Work::result_type> ex;
                        if(!boost::asynchronous::is_ready(std::get<0>(continuation_res)))
                        {
                            boost::asynchronous::task_aborted_exception ta;
                            ex.set_exception(std::make_exception_ptr(ta));
                        }
                        else
                        {
                            try
                            {
                                // check for exception
                                std::get<0>(continuation_res).get();
                                ex.set_value();
                            }
                            catch(...)
                            {
                                ex.set_exception(std::current_exception());
                            }
                        }

                        CallbackFct cb(std::move(work),std::move(ex));
                        Callback()(shared_scheduler,std::move(cb),task_name,cb_prio);
                    }
                    catch(...){/* TODO */}
                }
            );
            boost::asynchronous::any_continuation ac(std::move(cont));
            boost::asynchronous::get_continuations().emplace_front(std::move(ac));
        }
    };

    //TODO no copy/paste
    template <class Func,class F1, class F2,class Work,class Sched,class PostSched,class Callback,class Enable=void>
    struct post_callback_helper_continuation : public boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type
    {
#ifndef BOOST_NO_RVALUE_REFERENCES
        post_callback_helper_continuation(Work&& w, Sched const& s, const std::string& task_name="",
                                  std::size_t cb_prio=0)
            : m_work(std::forward<Work>(w)),m_scheduler(s),m_task_name(task_name),m_cb_prio(cb_prio) {}
        post_callback_helper_continuation(post_callback_helper_continuation&& rhs)noexcept
            : m_work(std::move(rhs.m_work)),m_scheduler(rhs.m_scheduler)
            , m_task_name(std::move(rhs.m_task_name)),m_cb_prio(rhs.m_cb_prio) {}
        post_callback_helper_continuation& operator= (post_callback_helper_continuation&& rhs)noexcept
        {
            std::swap(m_work,rhs.m_work);
            std::swap(m_scheduler,rhs.m_scheduler);
            std::swap(m_task_name,rhs.m_task_name);
            m_cb_prio = rhs.m_cb_prio;
            return *this;
        }
#endif
        post_callback_helper_continuation(post_callback_helper_continuation const& rhs)
            : boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type(),
              m_work(rhs.m_work),m_scheduler(rhs.m_scheduler),m_task_name(rhs.m_task_name),m_cb_prio(rhs.m_cb_prio){}
        post_callback_helper_continuation(Work const& w, Sched const& s, const std::string& task_name="",std::size_t cb_prio=0)
            : boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type(),
              m_work(w),m_scheduler(s),m_task_name(task_name),m_cb_prio(cb_prio){}


        struct callback_fct : public boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type
        {
            callback_fct(Work const& w,boost::asynchronous::expected<typename Work::result_type> fu)
                :boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type()
                ,m_work(w)
                ,m_fu(std::move(fu)){}
            callback_fct(callback_fct const& rhs)noexcept
                :boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type()
                ,m_work(std::move((const_cast<callback_fct&>(rhs)).m_work))
                ,m_fu(std::move((const_cast<callback_fct&>(rhs)).m_fu)){}
            callback_fct& operator= (callback_fct const& rhs)noexcept
            {
                std::swap(m_work,rhs.m_work);
                std::swap(m_fu,rhs.m_fu);
                return *this;
            }
            void operator()()
            {
                m_work(std::move(m_fu));
            }
            Work m_work;
            boost::asynchronous::expected<typename Work::result_type> m_fu;
        };

        void operator()()
        {
            // TODO check not empty
            // call task
            boost::asynchronous::expected<typename Work::result_type> ex;
            try
            {
                post_helper_continuation<typename Work::result_type,Sched,Func,Work,F1,F2,callback_fct,Callback>()(m_task_name,m_cb_prio,m_scheduler,m_work);
            }
            catch(boost::asynchronous::task_aborted_exception& e)
            {                
                ex.set_exception(std::make_exception_ptr(e));
                callback_fct cb(std::move(m_work),std::move(ex));
                auto shared_scheduler = m_scheduler.lock();
                if (shared_scheduler.is_valid())
                {
                    Callback()(shared_scheduler,std::move(cb),m_task_name,m_cb_prio);
                }
                // if no valid scheduler, there is nobody to inform anyway
            }
            catch(...)
            {
                ex.set_exception(std::current_exception());this->set_failed();
                callback_fct cb(std::move(m_work),std::move(ex));
                auto shared_scheduler = m_scheduler.lock();
                if (shared_scheduler.is_valid())
                {
                    Callback()(shared_scheduler,std::move(cb),m_task_name,m_cb_prio);
                }
                // if no valid scheduler, there is nobody to inform anyway
            }
        }
        Work m_work;
        Sched m_scheduler;
        std::string m_task_name;
        std::size_t m_cb_prio;
    };
    //TODO no copy/paste
    template <class Func,class F1, class F2,class Work,class Sched,class PostSched,class Callback>
    struct post_callback_helper_continuation<Func,F1,F2,Work,Sched,PostSched,Callback,typename std::enable_if<boost::asynchronous::detail::is_serializable<typename Work::task_type>::value >::type>
            : public boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type
    {
#ifndef BOOST_NO_RVALUE_REFERENCES
        post_callback_helper_continuation(Work&& w, Sched const& s, const std::string& task_name="",
                                  std::size_t cb_prio=0)
            : m_work(std::forward<Work>(w)),m_scheduler(s),m_task_name(task_name),m_cb_prio(cb_prio) {}
        post_callback_helper_continuation(post_callback_helper_continuation&& rhs)noexcept
            : m_work(std::move(rhs.m_work)),m_scheduler(rhs.m_scheduler)
            , m_task_name(std::move(rhs.m_task_name)),m_cb_prio(rhs.m_cb_prio) {}
        post_callback_helper_continuation& operator= (post_callback_helper_continuation&& rhs)noexcept
        {
            std::swap(m_work,rhs.m_work);
            std::swap(m_scheduler,rhs.m_scheduler);
            std::swap(m_task_name,rhs.m_task_name);
            m_cb_prio = rhs.m_cb_prio;
            return *this;
        }
#endif
        post_callback_helper_continuation(post_callback_helper_continuation const& rhs)
            : boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type(),
              m_work(rhs.m_work),m_scheduler(rhs.m_scheduler),m_task_name(rhs.m_task_name),m_cb_prio(rhs.m_cb_prio){}
        post_callback_helper_continuation(Work const& w, Sched const& s, const std::string& task_name="",std::size_t cb_prio=0)
            : boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type(),
              m_work(w),m_scheduler(s),m_task_name(task_name),m_cb_prio(cb_prio){}

        struct callback_fct : public boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type
        {
            callback_fct(Work const& w,boost::asynchronous::expected<typename Work::result_type> fu)
                :boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type()
                ,m_work(w)
                ,m_fu(std::move(fu)){}
            callback_fct(callback_fct const& rhs)noexcept
                :boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type()
                ,m_work(std::move((const_cast<callback_fct&>(rhs)).m_work))
                ,m_fu(std::move((const_cast<callback_fct&>(rhs)).m_fu)){}
            callback_fct& operator= (callback_fct const& rhs)noexcept
            {
                std::swap(m_work,rhs.m_work);
                std::swap(m_fu,rhs.m_fu);
                return *this;
            }
            void operator()()
            {
                m_work(std::move(m_fu));
            }
            Work m_work;
            boost::asynchronous::expected<typename Work::result_type> m_fu;
        };
        std::string get_task_name()const
        {
            return (m_work.m_task).get_task_name();
        }
        template <class Archive>
        void save(Archive & ar, const unsigned int /*version*/)const
        {
            ar & m_work.m_task;
        }
        template <class Archive>
        void load(Archive & ar, const unsigned int /*version*/)
        {
            boost::asynchronous::tcp::client_request::message_payload payload;
            ar >> payload;
            boost::asynchronous::expected<typename Work::result_type> ex;
            if (!payload.m_has_exception)
            {
                std::istringstream archive_stream(std::move(payload.m_data));
                typedef typename PostSched::job_type jobtype;
                typename jobtype::iarchive archive(archive_stream);

                typename Work::result_type res;
                archive >> res;
                ex.set_value(std::move(res));
            }
            else
            {
                ex.set_exception(std::make_exception_ptr(payload.m_exception));
            }
            callback_ready(std::move(ex));
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()
        void callback_ready(boost::asynchronous::expected<typename Work::result_type> work_fu)
        {
            try
            {
                auto shared_scheduler = m_scheduler.lock();
                if (!shared_scheduler.is_valid())
                    // no need to do any work as there is no way to callback
                    return;
                // call callback
                callback_fct cb(std::move(m_work),std::move(work_fu));
                Callback()(shared_scheduler,std::move(cb),m_task_name,m_cb_prio);
            }
            catch(...){/* TODO */}
        }
        void operator()()
        {
            // TODO check not empty
            // call task
            boost::asynchronous::expected<typename Work::result_type> ex;
            try
            {
                post_helper_continuation<typename Work::result_type,Sched,Func,Work,F1,F2,callback_fct,Callback>()(m_task_name,m_cb_prio,m_scheduler,m_work);
            }
            catch(boost::asynchronous::task_aborted_exception& e)
            {
                ex.set_exception(std::make_exception_ptr(e));
                callback_fct cb(std::move(m_work),std::move(ex));
                auto shared_scheduler = m_scheduler.lock();
                if (shared_scheduler.is_valid())
                {
                    Callback()(shared_scheduler,std::move(cb),m_task_name,m_cb_prio);
                }
                // if no valid scheduler, there is nobody to inform anyway
            }
            catch(...)
            {
                ex.set_exception(std::current_exception());this->set_failed();
                callback_fct cb(std::move(m_work),std::move(ex));
                auto shared_scheduler = m_scheduler.lock();
                if (shared_scheduler.is_valid())
                {
                    Callback()(shared_scheduler,std::move(cb),m_task_name,m_cb_prio);
                }
                // if no valid scheduler, there is nobody to inform anyway
            }
        }
        Work m_work;
        Sched m_scheduler;
        std::string m_task_name;
        std::size_t m_cb_prio;
    };
}

template <class F1, class S1,class F2, class S2>
#ifndef BOOST_NO_RVALUE_REFERENCES
auto post_callback(S1 const& scheduler,F1 func,S2 const& weak_cb_scheduler,F2 cb_func,
#else
auto post_callback(S1 const& scheduler,F1 const& func,S2 const& weak_cb_scheduler,F2 const& cb_func,
#endif
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                   const std::string& task_name, std::size_t post_prio, std::size_t cb_prio)
#else
                   const std::string& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0)
#endif
    -> typename std::enable_if< std::is_same<void,decltype(func())>::value,void >::type
{
    // abstract away the return value of work functor
    struct post_helper
    {
        void operator()(move_task_helper<void,F1,F2>& work,boost::asynchronous::expected<void>&)const
        {
            work.m_task();
        }
    };
    move_task_helper<void,F1,F2> moved_work(std::move(func),std::move(cb_func));
    // create a task which calls passed task, then post the callback
    detail::post_callback_helper_base<move_task_helper<void,F1,F2>,S2,S1,post_helper,detail::cb_helper > task_then_cb (
                std::move(moved_work), weak_cb_scheduler,task_name,cb_prio);
    typename boost::asynchronous::job_traits<typename S1::job_type>::wrapper_type w(std::move(task_then_cb));
    w.set_name(task_name);
    scheduler.post(std::move(w),post_prio);
}

template <class F1, class S1,class F2, class S2>
#ifndef BOOST_NO_RVALUE_REFERENCES
auto post_callback(S1 const& scheduler,F1 func,S2 const& weak_cb_scheduler,F2 cb_func,
#else
auto post_callback(S1 const& scheduler,F1 const& func,S2 const& weak_cb_scheduler,F2 const& cb_func,
#endif
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                   const std::string& task_name, std::size_t post_prio, std::size_t cb_prio)
#else
                   const std::string& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0)
#endif
    -> typename std::enable_if<!std::is_same<void,decltype(func())>::value &&
                               ! boost::asynchronous::detail::has_is_continuation_task<decltype(func())>::value
                               ,void >::type
{
    // abstract away the return value of work functor
    using func_type = decltype(func());
    struct post_helper
    {
        void operator()(move_task_helper<func_type,F1,F2>& work,boost::asynchronous::expected<decltype(func())>& res)const
        { 
            res.set_value(std::move(work.m_task()));
        }
    };
    move_task_helper<decltype(func()),F1,F2> moved_work(std::move(func),std::move(cb_func));
    // create a task which calls passed task, then post the callback
    detail::post_callback_helper_base<move_task_helper<decltype(func()),F1,F2>,S2,S1,post_helper,detail::cb_helper > task_then_cb (
                std::move(moved_work), weak_cb_scheduler,task_name,cb_prio);
    typename boost::asynchronous::job_traits<typename S1::job_type>::wrapper_type w(std::move(task_then_cb));
    w.set_name(task_name);
    scheduler.post(std::move(w),post_prio);
}

// version for continuations
template <class F1, class S1,class F2, class S2>
#ifndef BOOST_NO_RVALUE_REFERENCES
auto post_callback(S1 const& scheduler,F1 func,S2 const& weak_cb_scheduler,F2 cb_func,
#else
auto post_callback(S1 const& scheduler,F1 const& func,S2 const& weak_cb_scheduler,F2 const& cb_func,
#endif
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                   const std::string& task_name, std::size_t post_prio, std::size_t cb_prio)
#else
                   const std::string& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0)
#endif
    -> typename std::enable_if< boost::asynchronous::detail::has_is_continuation_task<decltype(func())>::value,void >::type
{
    move_task_helper<typename decltype(func())::return_type,F1,F2> moved_work(std::move(func),std::move(cb_func));
    // create a task which calls passed task, then post the callback
    detail::post_callback_helper_continuation<decltype(func()),F1,F2,move_task_helper<typename decltype(func())::return_type,F1,F2>,S2,S1,detail::cb_helper > task_then_cb (
                std::move(moved_work), weak_cb_scheduler,task_name,cb_prio);
    typename boost::asynchronous::job_traits<typename S1::job_type>::wrapper_type w(std::move(task_then_cb));
    w.set_name(task_name);
    scheduler.post(std::move(w),post_prio);
}

//TODO macro...
template <class F1, class S1,class F2, class S2>
#ifndef BOOST_NO_RVALUE_REFERENCES
auto interruptible_post_callback(S1 const& scheduler,F1 func,S2 const& weak_cb_scheduler,F2 cb_func,
#else
auto interruptible_post_callback(S1 const& scheduler,F1 const& func,S2 const& weak_cb_scheduler,F2 const& cb_func,
#endif
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                                 const std::string& task_name, std::size_t post_prio, std::size_t cb_prio)
#else
                                 const std::string& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0)
#endif
    -> typename std::enable_if< std::is_same<void,decltype(func())>::value,boost::asynchronous::any_interruptible >::type
{
    // abstract away the return value of work functor
    struct post_helper
    {
        void operator()(move_task_helper<void,F1,F2>& work,boost::asynchronous::expected<void>&)const
        {
            work.m_task();
        }
    };
    move_task_helper<void,F1,F2> moved_work(std::move(func),std::move(cb_func));
    // create a task which calls passed task, then post the callback
    detail::post_callback_helper_base<move_task_helper<void,F1,F2>,S2,S1,post_helper,detail::cb_helper > task_then_cb (
                std::move(moved_work), weak_cb_scheduler,task_name,cb_prio);
    typename boost::asynchronous::job_traits<typename S1::job_type>::wrapper_type w(std::move(task_then_cb));
    w.set_name(task_name);
    return scheduler.interruptible_post(std::move(w),post_prio);
}

template <class F1, class S1,class F2, class S2>
#ifndef BOOST_NO_RVALUE_REFERENCES
auto interruptible_post_callback(S1 const& scheduler,F1 func,S2 const& weak_cb_scheduler,F2 cb_func,
#else
auto interruptible_post_callback(S1 const& scheduler,F1 const& func,S2 const& weak_cb_scheduler,F2 const& cb_func,
#endif
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                                 const std::string& task_name, std::size_t post_prio, std::size_t cb_prio)
#else
                                 const std::string& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0)
#endif
        -> typename std::enable_if< !std::is_same<void,decltype(func())>::value &&
                                    !boost::asynchronous::detail::has_is_continuation_task<decltype(func())>::value
                                   ,boost::asynchronous::any_interruptible >::type
{
    // abstract away the return value of work functor
    using func_type = decltype(func());
    struct post_helper
    {
        void operator()(move_task_helper<func_type,F1,F2>& work,boost::asynchronous::expected<decltype(func())>& res)const
        {  
            res.set_value(std::move(work.m_task()));
        }
    };

    move_task_helper<decltype(func()),F1,F2> moved_work(std::move(func),std::move(cb_func));
    // create a task which calls passed task, then post the callback
    detail::post_callback_helper_base<move_task_helper<decltype(func()),F1,F2>,S2,S1,post_helper,detail::cb_helper > task_then_cb (
                std::move(moved_work), weak_cb_scheduler,task_name,cb_prio);
    typename boost::asynchronous::job_traits<typename S1::job_type>::wrapper_type w(std::move(task_then_cb));
    w.set_name(task_name);
    return scheduler.interruptible_post(std::move(w),post_prio);
}

// version for continuations
template <class F1, class S1,class F2, class S2>
#ifndef BOOST_NO_RVALUE_REFERENCES
auto interruptible_post_callback(S1 const& scheduler,F1 func,S2 const& weak_cb_scheduler,F2 cb_func,
#else
auto interruptible_post_callback(S1 const& scheduler,F1 const& func,S2 const& weak_cb_scheduler,F2 const& cb_func,
#endif
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                   const std::string& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0)
#else
                   const std::string& task_name, std::size_t post_prio, std::size_t cb_prio)
#endif
    -> typename std::enable_if< boost::asynchronous::detail::has_is_continuation_task<decltype(func())>::value,boost::asynchronous::any_interruptible >::type
{
    move_task_helper<typename decltype(func())::return_type,F1,F2> moved_work(std::move(func),std::move(cb_func));
    // create a task which calls passed task, then post the callback
    detail::post_callback_helper_continuation<decltype(func()),F1,F2,move_task_helper<typename decltype(func())::return_type,F1,F2>,S2,S1,detail::cb_helper > task_then_cb (
                std::move(moved_work), weak_cb_scheduler,task_name,cb_prio);
    typename boost::asynchronous::job_traits<typename S1::job_type>::wrapper_type w(std::move(task_then_cb));
    w.set_name(task_name);
    return scheduler.interruptible_post(std::move(w),post_prio);
}

}}

#endif // BOOST_ASYNCHRON_POST_HPP
