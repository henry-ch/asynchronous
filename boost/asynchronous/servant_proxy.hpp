// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2016
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

// servant_proxy is the proxy to every servant, trackable or not.
// its job is to protect a thread-unsafe object from other threads by serializing calls to it (see Active Object Pattern / Proxy).
// It also takes responsibility of creating the servant. It is a shareable object, the last instance will destroy the servant safely.
// This file also provides the following macros for use withing servant_proxy:
// BOOST_ASYNC_FUTURE_MEMBER(member [,priority]): calls the desired member of the servant, returns a future<return type of member>
// BOOST_ASYNC_FUTURE_MEMBER_LOG(member, taskname [,priority]): as above but will be logged with this name if the job type supports it.
// BOOST_ASYNC_POST_MEMBER(member [,priority]): calls the desired member of the servant, returns nothing
// BOOST_ASYNC_POST_MEMBER_LOG(member, taskname [,priority]): as above but will be logged with this name if the job type supports it.
// more exotic:
// BOOST_ASYNC_MEMBER_UNSAFE_CALLBACK(member [,priority]): calls the desired member of the servant, takes as first argument a callback
// Useful when a servant wants to call a member of another servant and being a trackable_servant, needs no future but a callback.
// UNSAFE means the calling servant must provide safety itself, at best using make_safe_callback.
// BOOST_ASYNC_MEMBER_UNSAFE_CALLBACK_LOG(member, taskname [,priority]): as above but will be logged with this name if the job type supports it.
// Tested in test_servant_proxy_unsafe_callback.cpp.

#ifndef BOOST_ASYNC_SERVANT_PROXY_H
#define BOOST_ASYNC_SERVANT_PROXY_H

#include <functional>
#include <utility>
#include <exception>
#include <cstddef>
#include <type_traits>

#ifndef BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE
#endif
#include  <boost/preprocessor/facilities/overload.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits/has_trivial_constructor.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <boost/preprocessor/facilities/overload.hpp>

#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/job_traits.hpp>
#include <boost/asynchronous/detail/move_bind.hpp>


namespace boost { namespace asynchronous
{        
BOOST_MPL_HAS_XXX_TRAIT_DEF(simple_ctor)
BOOST_MPL_HAS_XXX_TRAIT_DEF(simple_dtor)
BOOST_MPL_HAS_XXX_TRAIT_DEF(requires_weak_scheduler)
BOOST_MPL_HAS_XXX_TRAIT_DEF(servant_type)

#ifndef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
#define BOOST_ASYNC_POST_MEMBER_1(funcname)                                                                                     \
    template <typename... Args>                                                                                                 \
    void funcname(Args... args)const                                                                                            \
    {                                                                                                                           \
        auto servant = this->m_servant;                                                                                         \
        std::size_t p = 100000 * this->m_offset_id;                                                                             \
        this->post(typename boost::asynchronous::job_traits<callable_type>::wrapper_type(boost::asynchronous::any_callable      \
        (boost::asynchronous::move_bind([servant](Args... as){servant->funcname(std::move(as)...);},std::move(args)...))),p);   \
    }
#endif

#define BOOST_ASYNC_POST_MEMBER_2(funcname,prio)                                                                                \
    template <typename... Args>                                                                                                 \
    void funcname(Args... args)const                                                                                            \
    {                                                                                                                           \
        auto servant = this->m_servant;                                                                                         \
        std::size_t p = prio + 100000 * this->m_offset_id;                                                                      \
        this->post(typename boost::asynchronous::job_traits<callable_type>::wrapper_type(boost::asynchronous::any_callable      \
        (boost::asynchronous::move_bind([servant](Args... as){servant->funcname(std::move(as)...);},std::move(args)...))),p);   \
    }

#define BOOST_ASYNC_POST_MEMBER(...)                                                                            \
    BOOST_PP_OVERLOAD(BOOST_ASYNC_POST_MEMBER_,__VA_ARGS__)(__VA_ARGS__)

#ifndef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
#define BOOST_ASYNC_POST_MEMBER_LOG_2(funcname,taskname)                                                                                    \
    template <typename... Args>                                                                                                             \
    void funcname(Args... args)const                                                                                                        \
    {                                                                                                                                       \
        auto servant = this->m_servant;                                                                                                     \
        std::size_t prio = 100000 * this->m_offset_id;                                                                                      \
        typename boost::asynchronous::job_traits<callable_type>::wrapper_type  a(boost::asynchronous::any_callable                          \
                        (boost::asynchronous::move_bind([servant](Args... as){servant->funcname(std::move(as)...);},std::move(args)...)));  \
        a.set_name(taskname);                                                                                                               \
        this->post(std::move(a),prio);                                                                                                      \
    }
#endif

#define BOOST_ASYNC_POST_MEMBER_LOG_3(funcname,taskname,prio)                                                                               \
    template <typename... Args>                                                                                                             \
    void funcname(Args... args)const                                                                                                        \
    {                                                                                                                                       \
        auto servant = this->m_servant;                                                                                                     \
        std::size_t p = prio + 100000 * this->m_offset_id;                                                                                  \
        typename boost::asynchronous::job_traits<callable_type>::wrapper_type  a(boost::asynchronous::any_callable                          \
                        (boost::asynchronous::move_bind([servant](Args... as){servant->funcname(std::move(as)...);},std::move(args)...)));  \
        a.set_name(taskname);                                                                                                               \
        this->post(std::move(a),p);                                                                                                         \
    }

#define BOOST_ASYNC_POST_MEMBER_LOG(...)                                                                        \
    BOOST_PP_OVERLOAD(BOOST_ASYNC_POST_MEMBER_LOG_,__VA_ARGS__)(__VA_ARGS__)

#ifndef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
#ifdef BOOST_NO_CXX14_RETURN_TYPE_DEDUCTION
#define BOOST_ASYNC_FUTURE_MEMBER_LOG_2(funcname,taskname)                                                                                                  \
    template <typename... Args>                                                                                                                             \
    auto funcname(Args... args)const                                                                                                                        \
        -> boost::future<decltype(boost::shared_ptr<servant_type>()->funcname(std::move(args)...))>                                                         \
    {                                                                                                                                                       \
        auto servant = this->m_servant;                                                                                                                     \
        std::size_t prio = 100000 * this->m_offset_id;                                                                                                      \
        return boost::asynchronous::post_future(this->m_proxy,                                                                                              \
                boost::asynchronous::move_bind([servant](Args... as)                                                                                        \
                                    {return servant->funcname(std::move(as)...);                                                                            \
                                    },std::move(args)...),taskname,prio);                                                                                   \
    }
#else
#define BOOST_ASYNC_FUTURE_MEMBER_LOG_2(funcname,taskname)                                                                                                  \
    template <typename... Args>                                                                                                                             \
    auto funcname(Args... args)const                                                                                                                        \
    {                                                                                                                                                       \
        auto servant = this->m_servant;                                                                                                                     \
        std::size_t prio = 100000 * this->m_offset_id;                                                                                                      \
        return boost::asynchronous::post_future(this->m_proxy,                                                                                              \
                boost::asynchronous::move_bind([servant](Args... as)                                                                                        \
                                    {return servant->funcname(std::move(as)...);                                                                            \
                                    },std::move(args)...),taskname,prio);                                                                                   \
    }
#endif
#endif

#ifdef BOOST_NO_CXX14_RETURN_TYPE_DEDUCTION
#define BOOST_ASYNC_FUTURE_MEMBER_LOG_3(funcname,taskname,prio)                                                                                         \
    template <typename... Args>                                                                                                                         \
    auto funcname(Args... args)const                                                                                                                    \
        -> boost::future<decltype(boost::shared_ptr<servant_type>()->funcname(std::move(args)...))>                                                     \
    {                                                                                                                                                   \
        auto servant = this->m_servant;                                                                                                                 \
        std::size_t p = prio + 100000 * this->m_offset_id;                                                                                              \
        return boost::asynchronous::post_future(this->m_proxy,                                                                                          \
                boost::asynchronous::move_bind([servant](Args... as)                                                                                    \
                                    {return servant->funcname(std::move(as)...);                                                                        \
                                    },std::move(args)...),taskname,p);                                                                                  \
    }
#else
#define BOOST_ASYNC_FUTURE_MEMBER_LOG_3(funcname,taskname,prio)                                                                                         \
    template <typename... Args>                                                                                                                         \
    auto funcname(Args... args)const                                                                                                                    \
    {                                                                                                                                                   \
        auto servant = this->m_servant;                                                                                                                 \
        std::size_t p = prio + 100000 * this->m_offset_id;                                                                                              \
        return boost::asynchronous::post_future(this->m_proxy,                                                                                          \
                boost::asynchronous::move_bind([servant](Args... as)                                                                                    \
                                    {return servant->funcname(std::move(as)...);                                                                        \
                                    },std::move(args)...),taskname,p);                                                                                  \
    }
#endif
#define BOOST_ASYNC_FUTURE_MEMBER_LOG(...)                                                                      \
    BOOST_PP_OVERLOAD(BOOST_ASYNC_FUTURE_MEMBER_LOG_,__VA_ARGS__)(__VA_ARGS__)

#ifndef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
#ifdef BOOST_NO_CXX14_RETURN_TYPE_DEDUCTION
#define BOOST_ASYNC_FUTURE_MEMBER_1(funcname)                                                                                                           \
    template <typename... Args>                                                                                                                         \
    auto funcname(Args... args)const                                                                                                                    \
        -> boost::future<decltype(boost::shared_ptr<servant_type>()->funcname(std::move(args)...))>                                                     \
    {                                                                                                                                                   \
        auto servant = this->m_servant;                                                                                                                 \
        std::size_t prio = 100000 * this->m_offset_id;                                                                                                  \
        return boost::asynchronous::post_future(this->m_proxy,                                                                                          \
                boost::asynchronous::move_bind([servant](Args... as)                                                                                    \
                                    {return servant->funcname(std::move(as)...);                                                                        \
                                    },std::move(args)...),"",prio);                                                                                     \
    }
#else
#define BOOST_ASYNC_FUTURE_MEMBER_1(funcname)                                                                                                           \
    template <typename... Args>                                                                                                                         \
    auto funcname(Args... args)const                                                                                                                    \
    {                                                                                                                                                   \
        auto servant = this->m_servant;                                                                                                                 \
        std::size_t prio = 100000 * this->m_offset_id;                                                                                                  \
        return boost::asynchronous::post_future(this->m_proxy,                                                                                          \
                boost::asynchronous::move_bind([servant](Args... as)                                                                                    \
                                    {return servant->funcname(std::move(as)...);                                                                        \
                                    },std::move(args)...),"",prio);                                                                                     \
    }
#endif
#endif

#ifdef BOOST_NO_CXX14_RETURN_TYPE_DEDUCTION
#define BOOST_ASYNC_FUTURE_MEMBER_2(funcname,prio)                                                                                                      \
    template <typename... Args>                                                                                                                         \
    auto funcname(Args... args)const                                                                                                                    \
        -> boost::future<decltype(boost::shared_ptr<servant_type>()->funcname(std::move(args)...))>                                                     \
    {                                                                                                                                                   \
        auto servant = this->m_servant;                                                                                                                 \
        std::size_t p = prio + 100000 * this->m_offset_id;                                                                                              \
        return boost::asynchronous::post_future(this->m_proxy,                                                                                          \
                boost::asynchronous::move_bind([servant](Args... as)                                                                                    \
                                    {return servant->funcname(std::move(as)...);                                                                        \
                                    },std::move(args)...),"",p);                                                                                        \
    }
#else
#define BOOST_ASYNC_FUTURE_MEMBER_2(funcname,prio)                                                                                                      \
    template <typename... Args>                                                                                                                         \
    auto funcname(Args... args)const                                                                                                                    \
    {                                                                                                                                                   \
        auto servant = this->m_servant;                                                                                                                 \
        std::size_t p = prio + 100000 * this->m_offset_id;                                                                                              \
        return boost::asynchronous::post_future(this->m_proxy,                                                                                          \
                boost::asynchronous::move_bind([servant](Args... as)                                                                                    \
                                    {return servant->funcname(std::move(as)...);                                                                        \
                                    },std::move(args)...),"",p);                                                                                        \
    }
#endif
#define BOOST_ASYNC_FUTURE_MEMBER(...)                                                                          \
    BOOST_PP_OVERLOAD(BOOST_ASYNC_FUTURE_MEMBER_,__VA_ARGS__)(__VA_ARGS__)

#ifndef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
#define BOOST_ASYNC_MEMBER_UNSAFE_CALLBACK_1(funcname)                                                                                              \
    template <typename F,typename... Args>                                                                                                          \
    auto funcname(F&& cb_func, Args... args)const                                                                                                   \
    -> typename std::enable_if<!std::is_same<decltype(boost::shared_ptr<servant_type>()->funcname(std::move(args)...)),void>::value,void>::type   \
    {                                                                                                                                               \
        struct workaround_gcc{typedef decltype(boost::shared_ptr<servant_type>()->funcname(std::move(args)...)) servant_return;};                   \
        auto servant = this->m_servant;                                                                                                             \
        std::size_t prio = 100000 * this->m_offset_id;                                                                                              \
        using servant_return = typename workaround_gcc::servant_return;                                                                             \
        boost::asynchronous::post_future(this->m_proxy,                                                                                             \
                boost::asynchronous::move_bind([servant,cb_func](Args... as)                                                                        \
                                    {try{ cb_func(boost::asynchronous::expected<servant_return>(servant->funcname(std::move(as)...)));}             \
                                     catch(std::exception& e){cb_func(boost::asynchronous::expected<servant_return>(boost::copy_exception(e)));}    \
                                     catch(...){cb_func(boost::asynchronous::expected<servant_return>(boost::current_exception()));}                \
                                    },std::move(args)...),"",prio);                                                                                 \
    }                                                                                                                                               \
    template <typename F,typename... Args>                                                                                                          \
    auto funcname(F&& cb_func, Args... args)const                                                                                                   \
    -> typename std::enable_if<std::is_same<decltype(boost::shared_ptr<servant_type>()->funcname(std::move(args)...)),void>::value,void>::type         \
    {                                                                                                                                               \
        auto servant = this->m_servant;                                                                                                             \
        std::size_t prio = 100000 * this->m_offset_id;                                                                                              \
        boost::asynchronous::post_future(this->m_proxy,                                                                                             \
                boost::asynchronous::move_bind([servant,cb_func](Args... as)                                                                        \
                                    {try{servant->funcname(std::move(as)...); cb_func(boost::asynchronous::expected<void>());}                      \
                                     catch(std::exception& e){cb_func(boost::asynchronous::expected<void>(boost::copy_exception(e)));}              \
                                     catch(...){cb_func(boost::asynchronous::expected<void>(boost::current_exception()));}                          \
                                    },std::move(args)...),"",prio);                                                                                 \
    }

#endif
#define BOOST_ASYNC_MEMBER_UNSAFE_CALLBACK_2(funcname,prio)                                                                                         \
    template <typename F,typename... Args>                                                                                                          \
    auto funcname(F&& cb_func, Args... args)const                                                                                                   \
    -> typename std::enable_if<!std::is_same<decltype(boost::shared_ptr<servant_type>()->funcname(std::move(args)...)),void>::value,void>::type   \
    {                                                                                                                                               \
        struct workaround_gcc{typedef decltype(boost::shared_ptr<servant_type>()->funcname(std::move(args)...)) servant_return;};                   \
        auto servant = this->m_servant;                                                                                                             \
        std::size_t p = prio + 100000 * this->m_offset_id;                                                                                          \
        using servant_return = typename workaround_gcc::servant_return;                                                                             \
        boost::asynchronous::post_future(this->m_proxy,                                                                                             \
                boost::asynchronous::move_bind([servant,cb_func](Args... as)                                                                        \
                                    {try{ cb_func(boost::asynchronous::expected<servant_return>(servant->funcname(std::move(as)...)));}             \
                                     catch(std::exception& e){cb_func(boost::asynchronous::expected<servant_return>(boost::copy_exception(e)));}    \
                                     catch(...){cb_func(boost::asynchronous::expected<servant_return>(boost::current_exception()));}                \
                                    },std::move(args)...),"",p);                                                                                    \
    }                                                                                                                                               \
    template <typename F,typename... Args>                                                                                                          \
    auto funcname(F&& cb_func, Args... args)const                                                                                                   \
    -> typename std::enable_if<std::is_same<decltype(boost::shared_ptr<servant_type>()->funcname(std::move(args)...)),void>::value,void>::type         \
    {                                                                                                                                               \
        auto servant = this->m_servant;                                                                                                             \
        std::size_t p = prio + 100000 * this->m_offset_id;                                                                                          \
        boost::asynchronous::post_future(this->m_proxy,                                                                                             \
                boost::asynchronous::move_bind([servant,cb_func](Args... as)                                                                        \
                                    {try{servant->funcname(std::move(as)...); cb_func(boost::asynchronous::expected<void>());}                      \
                                     catch(std::exception& e){cb_func(boost::asynchronous::expected<void>(boost::copy_exception(e)));}              \
                                     catch(...){cb_func(boost::asynchronous::expected<void>(boost::current_exception()));}                          \
                                    },std::move(args)...),"",p);                                                                                 \
    }

#define BOOST_ASYNC_MEMBER_UNSAFE_CALLBACK(...)                                                                          \
    BOOST_PP_OVERLOAD(BOOST_ASYNC_MEMBER_UNSAFE_CALLBACK_,__VA_ARGS__)(__VA_ARGS__)


#ifndef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
#define BOOST_ASYNC_MEMBER_UNSAFE_CALLBACK_LOG_2(funcname,taskname)                                                                                 \
    template <typename F,typename... Args>                                                                                                          \
    auto funcname(F&& cb_func, Args... args)const                                                                                                   \
    -> typename std::enable_if<!std::is_same<decltype(boost::shared_ptr<servant_type>()->funcname(std::move(args)...)),void>::value,void>::type   \
    {                                                                                                                                               \
        struct workaround_gcc{typedef decltype(boost::shared_ptr<servant_type>()->funcname(std::move(args)...)) servant_return;};                   \
        auto servant = this->m_servant;                                                                                                             \
        std::size_t prio = 100000 * this->m_offset_id;                                                                                              \
        using servant_return = typename workaround_gcc::servant_return;                                                                             \
        boost::asynchronous::post_future(this->m_proxy,                                                                                             \
                boost::asynchronous::move_bind([servant,cb_func](Args... as)                                                                        \
                                    {try{ cb_func(boost::asynchronous::expected<servant_return>(servant->funcname(std::move(as)...)));}             \
                                     catch(std::exception& e){cb_func(boost::asynchronous::expected<servant_return>(boost::copy_exception(e)));}    \
                                     catch(...){cb_func(boost::asynchronous::expected<servant_return>(boost::current_exception()));}                \
                                    },std::move(args)...),taskname,prio);                                                                           \
    }                                                                                                                                               \
    template <typename F,typename... Args>                                                                                                          \
    auto funcname(F&& cb_func, Args... args)const                                                                                                   \
    -> typename std::enable_if<std::is_same<decltype(boost::shared_ptr<servant_type>()->funcname(std::move(args)...)),void>::value,void>::type    \
    {                                                                                                                                               \
        auto servant = this->m_servant;                                                                                                             \
        std::size_t prio = 100000 * this->m_offset_id;                                                                                              \
        boost::asynchronous::post_future(this->m_proxy,                                                                                             \
                boost::asynchronous::move_bind([servant,cb_func](Args... as)                                                                        \
                                    {try{servant->funcname(std::move(as)...); cb_func(boost::asynchronous::expected<void>());}                      \
                                     catch(std::exception& e){cb_func(boost::asynchronous::expected<void>(boost::copy_exception(e)));}              \
                                     catch(...){cb_func(boost::asynchronous::expected<void>(boost::current_exception()));}                          \
                                    },std::move(args)...),taskname,prio);                                                                           \
    }
#endif

#define BOOST_ASYNC_MEMBER_UNSAFE_CALLBACK_LOG_3(funcname,taskname,prio)                                                                            \
    template <typename F,typename... Args>                                                                                                          \
    auto funcname(F&& cb_func, Args... args)const                                                                                                   \
    -> typename std::enable_if<!std::is_same<decltype(boost::shared_ptr<servant_type>()->funcname(std::move(args)...)),void>::value,void>::type   \
    {                                                                                                                                               \
        struct workaround_gcc{typedef decltype(boost::shared_ptr<servant_type>()->funcname(std::move(args)...)) servant_return;};                   \
        auto servant = this->m_servant;                                                                                                             \
        std::size_t p = prio + 100000 * this->m_offset_id;                                                                                          \
        using servant_return = typename workaround_gcc::servant_return;                                                                             \
        boost::asynchronous::post_future(this->m_proxy,                                                                                             \
                boost::asynchronous::move_bind([servant,cb_func](Args... as)                                                                        \
                                    {try{ cb_func(boost::asynchronous::expected<servant_return>(servant->funcname(std::move(as)...)));}             \
                                     catch(std::exception& e){cb_func(boost::asynchronous::expected<servant_return>(boost::copy_exception(e)));}    \
                                     catch(...){cb_func(boost::asynchronous::expected<servant_return>(boost::current_exception()));}                \
                                    },std::move(args)...),taskname,p);                                                                              \
    }                                                                                                                                               \
    template <typename F,typename... Args>                                                                                                          \
    auto funcname(F&& cb_func, Args... args)const                                                                                                   \
    -> typename std::enable_if<std::is_same<decltype(boost::shared_ptr<servant_type>()->funcname(std::move(args)...)),void>::value,void>::type         \
    {                                                                                                                                               \
        auto servant = this->m_servant;                                                                                                             \
        std::size_t p = prio + 100000 * this->m_offset_id;                                                                                          \
        boost::asynchronous::post_future(this->m_proxy,                                                                                             \
                boost::asynchronous::move_bind([servant,cb_func](Args... as)                                                                        \
                                    {try{servant->funcname(std::move(as)...); cb_func(boost::asynchronous::expected<void>());}                      \
                                     catch(std::exception& e){cb_func(boost::asynchronous::expected<void>(boost::copy_exception(e)));}              \
                                     catch(...){cb_func(boost::asynchronous::expected<void>(boost::current_exception()));}                          \
                                    },std::move(args)...),taskname,p);                                                                           \
    }


#define BOOST_ASYNC_MEMBER_UNSAFE_CALLBACK_LOG(...)                                                                          \
    BOOST_PP_OVERLOAD(BOOST_ASYNC_MEMBER_UNSAFE_CALLBACK_LOG_,__VA_ARGS__)(__VA_ARGS__)


#ifndef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
#define BOOST_ASYNC_POST_CALLBACK_MEMBER_1(funcname)                                                                                                \
    template <typename F, typename S,typename... Args>                                                                                              \
    void funcname(F&& cb_func,S const& weak_cb_scheduler,std::size_t cb_prio, Args... args)const                                                    \
    {                                                                                                                                               \
        auto servant = this->m_servant;                                                                                                             \
        std::size_t prio = 100000 * this->m_offset_id;                                                                                              \
        boost::asynchronous::post_callback(m_proxy,                                                                                                 \
                boost::asynchronous::move_bind([servant](Args... as)                                                                                \
                                    {return servant->funcname(std::move(as)...);                                                                    \
                                    },std::move(args)...),weak_cb_scheduler,std::forward<F>(cb_func),"",prio,cb_prio);                              \
    }
#endif

#define BOOST_ASYNC_POST_CALLBACK_MEMBER_2(funcname,prio)                                                                                           \
    template <typename F, typename S,typename... Args>                                                                                              \
    void funcname(F&& cb_func,S const& weak_cb_scheduler,std::size_t cb_prio, Args... args)const                                                    \
    {                                                                                                                                               \
        auto servant = this->m_servant;                                                                                                             \
        std::size_t p = prio + 100000 * this->m_offset_id;                                                                                          \
        boost::asynchronous::post_callback(m_proxy,                                                                                                 \
                boost::asynchronous::move_bind([servant](Args... as)                                                                                \
                                    {return servant->funcname(std::move(as)...);                                                                    \
                                    },std::move(args)...),weak_cb_scheduler,std::forward<F>(cb_func),"",p,cb_prio);                                 \
    }

#define BOOST_ASYNC_POST_CALLBACK_MEMBER(...)                                                                                                       \
    BOOST_PP_OVERLOAD(BOOST_ASYNC_POST_CALLBACK_MEMBER_,__VA_ARGS__)(__VA_ARGS__)    

#ifdef BOOST_NO_CXX14_RETURN_TYPE_DEDUCTION
#define BOOST_ASYNC_UNSAFE_MEMBER(funcname)                                                                                                         \
    template <typename... Args>                                                                                                                     \
    auto funcname(Args... args)const                                                                                                                \
    -> std::function<decltype(boost::shared_ptr<servant_type>()->funcname(std::move(args)...))()>               \
    {                                                                                                                                               \
        auto servant = m_servant;                                                                                                                   \
        return boost::asynchronous::move_bind([servant](Args... as){return servant->funcname(std::move(as)...);},std::move(args)...);               \
    }    
#else
#define BOOST_ASYNC_UNSAFE_MEMBER(funcname)                                                                                                         \
    template <typename... Args>                                                                                                                     \
    auto funcname(Args... args)const                                                                                                                \
    {                                                                                                                                               \
        auto servant = m_servant;                                                                                                                   \
        return boost::asynchronous::move_bind([servant](Args... as){return servant->funcname(std::move(as)...);},std::move(args)...);               \
    }
#endif
#ifndef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
#define BOOST_ASYNC_SERVANT_POST_CTOR_0()                                                                       \
    static std::size_t get_ctor_prio() {return 0;}
#endif

#define BOOST_ASYNC_SERVANT_POST_CTOR_1(priority)                                                               \
    static std::size_t get_ctor_prio() {return priority;}

#define BOOST_ASYNC_SERVANT_POST_CTOR(...)                                                                      \
    BOOST_PP_OVERLOAD(BOOST_ASYNC_SERVANT_POST_CTOR_,__VA_ARGS__)(__VA_ARGS__)

#ifndef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
#define BOOST_ASYNC_SERVANT_POST_CTOR_LOG_1(taskname)                                                           \
    static const char* get_ctor_name() {return taskname;}
#endif

#define BOOST_ASYNC_SERVANT_POST_CTOR_LOG_2(taskname,priority)                                                  \
    static std::size_t get_ctor_prio() {return priority;}                                                       \
    static const char* get_ctor_name() {return taskname;}

#define BOOST_ASYNC_SERVANT_POST_CTOR_LOG(...)                                                                  \
    BOOST_PP_OVERLOAD(BOOST_ASYNC_SERVANT_POST_CTOR_LOG_,__VA_ARGS__)(__VA_ARGS__)

#ifndef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
#define BOOST_ASYNC_SERVANT_POST_DTOR_0()                                                                       \
    static std::size_t get_dtor_prio() {return 0;}
#endif

#define BOOST_ASYNC_SERVANT_POST_DTOR_1(priority)                                                               \
    static std::size_t get_dtor_prio() {return priority;}

#define BOOST_ASYNC_SERVANT_POST_DTOR(...)                                                                      \
    BOOST_PP_OVERLOAD(BOOST_ASYNC_SERVANT_POST_DTOR_,__VA_ARGS__)(__VA_ARGS__)

#ifndef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
#define BOOST_ASYNC_SERVANT_POST_DTOR_LOG_1(taskname)                                                           \
    static const char* get_dtor_name() {return taskname;}
#endif

#define BOOST_ASYNC_SERVANT_POST_DTOR_LOG_2(taskname,priority)                                                  \
    static std::size_t get_dtor_prio() {return priority;}                                                       \
    static const char* get_dtor_name() {return taskname;}

#define BOOST_ASYNC_SERVANT_POST_DTOR_LOG(...)                                                                  \
    BOOST_PP_OVERLOAD(BOOST_ASYNC_SERVANT_POST_DTOR_LOG_,__VA_ARGS__)(__VA_ARGS__)



struct servant_proxy_timeout : public virtual boost::exception, public virtual std::exception
{
};
struct empty_servant : public virtual boost::exception, public virtual std::exception
{
};

template <class ServantProxy,class Servant, class Callable = BOOST_ASYNCHRONOUS_DEFAULT_JOB,int max_create_wait_ms = 5000>
class servant_proxy
{
public:
    typedef Servant servant_type;
    typedef Callable callable_type;
    typedef ServantProxy derived_proxy_type;
    typedef boost::asynchronous::any_shared_scheduler_proxy<callable_type> scheduler_proxy_type;
    typedef boost::asynchronous::any_weak_scheduler<callable_type> weak_scheduler_proxy_type;
    //constexpr int max_create_wait = max_create_wait_ms;
    typedef std::integral_constant<int,max_create_wait_ms > max_create_wait;

    // private constructor for use with dynamic_servant_proxy_cast
protected:
    // AnotherProxy must be a derived class of a servant_proxy (used by dynamic_servant_proxy_cast)
    template <class AnotherProxy>
    servant_proxy(AnotherProxy p, typename std::enable_if<boost::asynchronous::has_servant_type<AnotherProxy>::value>::type* = 0)BOOST_NOEXCEPT
        : m_proxy(p.m_proxy)
        , m_servant(boost::dynamic_pointer_cast<typename AnotherProxy::servant_type>(p.m_servant))
        , m_offset_id(0)
    {
    }
public:

    /*!
     * \brief Constructor
     * \brief Constructs a servant_proxy and a servant. The any_shared_scheduler_proxy<Callable> will be saved and
     * \brief passed as an any_weak_scheduler<Callable> to the servant. Variadic parameters will be forwarded to the servant.
     * \param p any_shared_scheduler_proxy<Callable> where the servant will execute.
     * \param args variadic number of parameters, number and type defined by the servant, which will be forwarded to the servant.
     */
    template <typename... Args>
    servant_proxy(scheduler_proxy_type p, Args... args)
        : m_proxy(p)
        , m_servant()
        , m_offset_id(0)
    {
        std::vector<boost::thread::id> ids = m_proxy.thread_ids();
        if ((std::find(ids.begin(),ids.end(),boost::this_thread::get_id()) != ids.end()))
        {
            // our thread, not possible to wait for a future
            // TODO forward
            // if a servant has a simple_ctor, then he MUST get a weak scheduler as he might get it too late with tss
            //m_servant = boost::make_shared<servant_type>(m_proxy.get_weak_scheduler(),args...);
            m_servant = servant_create_helper::template create<servant_type>(m_proxy.get_weak_scheduler(),std::move(args)...);
        }
        else
        {
            // outside thread, create in scheduler thread if no other choice
            init_servant_proxy<servant_type>(std::move(args)...);
        }
    }

    /*!
     * \brief Constructor
     * \brief Constructs a servant_proxy from an already existing servant. The any_shared_scheduler_proxy<Callable> will be saved and
     * \brief used to serialize calls to the servant.
     * \param p any_shared_scheduler_proxy where the servant was created and calls executed.
     * \param s servant created in the thread context of the provided scheduler proxy.
     */
    servant_proxy(scheduler_proxy_type p, boost::future<boost::shared_ptr<servant_type> > s)
        : m_proxy(p)
        , m_servant()
        , m_offset_id(0)
    {
        bool ok = s.timed_wait(boost::posix_time::milliseconds(max_create_wait_ms));
        if(ok)
        {
            m_servant = std::move(s.get());
            // servant ought not be empty
            if (!m_servant)
            {
                throw empty_servant();
            }
        }
        else
        {
            throw servant_proxy_timeout();
        }
    }
    // templatize to avoid gcc complaining in case servant is pure virtual
    /*!
     * \brief Constructor
     * \brief Constructs a servant_proxy from an already existing servant. The any_shared_scheduler_proxy<Callable> will be saved and
     * \brief used to serialize calls to the servant.
     * \param p any_shared_scheduler_proxy where the servant was created and calls executed.
     * \param s servant created in the thread context of the provided scheduler proxy.
     */
    template <class CServant>
    servant_proxy(scheduler_proxy_type p, boost::future<CServant> s)
        : m_proxy(p)
        , m_servant()
        , m_offset_id(0)
    {
        bool ok = s.timed_wait(boost::posix_time::milliseconds(max_create_wait_ms));
        if(ok)
        {
            m_servant = boost::make_shared<servant_type>(std::move(s.get()));
            // servant ought not be empty
            if (!m_servant)
            {
                throw empty_servant();
            }
        }
        else
        {
            throw servant_proxy_timeout();
        }
    }

    // version for multiple_thread_scheduler
    //TODO other ctors
    template <typename... Args>
    servant_proxy(std::tuple<scheduler_proxy_type, std::size_t> p, Args... args)
        : m_proxy(std::get<0>(p))
        , m_servant()
        , m_offset_id(std::get<1>(p))
    {
        std::vector<boost::thread::id> ids = m_proxy.thread_ids();
        if ((std::find(ids.begin(),ids.end(),boost::this_thread::get_id()) != ids.end()))
        {
            // our thread, not possible to wait for a future
            // TODO forward
            // if a servant has a simple_ctor, then he MUST get a weak scheduler as he might get it too late with tss
            //m_servant = boost::make_shared<servant_type>(m_proxy.get_weak_scheduler(),args...);
            m_servant = servant_create_helper::template create<servant_type>(m_proxy.get_weak_scheduler(),std::move(args)...);
        }
        else
        {
            // outside thread, create in scheduler thread if no other choice
            init_servant_proxy<servant_type>(std::move(args)...);
        }
    }

    /*!
     * \brief Destructor
     * \brief Decrements servant count usage.
     * \brief Joins threads of scheduler if holding last instance of it.
     */
    ~servant_proxy()
    {
        if (!!m_servant)
        {
            servant_deleter n(std::move(m_servant));
            boost::future<void> fu = n.done_promise->get_future();
            typename boost::asynchronous::job_traits<callable_type>::wrapper_type  a(std::move(n));
            a.set_name(ServantProxy::get_dtor_name());
            m_proxy.post(std::move(a),ServantProxy::get_dtor_prio());
            m_servant.reset();
        }
        m_proxy.reset();
    }
    // we provide destructor so we need to provide the other 4
    /*!
     * \brief Move constructor.
     */
    servant_proxy(servant_proxy&&)                =default;

    /*!
     * \brief Move assignment.
     */
    servant_proxy& operator=(servant_proxy&&)     =default;

    /*!
     * \brief Copy constructor.
     */
    servant_proxy(servant_proxy const&)           =default;

    /*!
     * \brief Copy assignment.
     */
    servant_proxy& operator=(servant_proxy const&)=default;

#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    void post(callable_type job, std::size_t prio) const
#else
    void post(callable_type job, std::size_t prio=0) const
#endif
    {
        m_proxy.post(std::move(job),prio + 100000 * m_offset_id);
    }

    /*!
     * \brief Returns the underlying any_shared_scheduler_proxy
     */
    scheduler_proxy_type get_proxy()const
    {
        return m_proxy;
    }
    // for derived to overwrite if needed
    /*!
     * \brief Returns priority of the servant constructor. It is advised to set the priority using BOOST_ASYNC_SERVANT_POST_CTOR or
     * \brief BOOST_ASYNC_SERVANT_POST_CTOR_LOG
     */
    static std::size_t get_ctor_prio() {return 0;}


    /*!
     * \brief Returns priority of the servant destructor. It is advised to set the priority using BOOST_ASYNC_SERVANT_POST_DTOR or
     * \brief BOOST_ASYNC_SERVANT_POST_DTOR_LOG
     */
    static std::size_t get_dtor_prio() {return 0;}

    /*!
     * \brief Returns name of the servant constructor task. It can be overriden by derived class
     */
    static const char* get_ctor_name() {return "ctor";}


    /*!
     * \brief Returns name of the servant destructor task. It can be overriden by derived class
     */
    static const char* get_dtor_name() {return "dtor";}

    /*!
     * \brief return a shared_ptr to our servant (careful! Use at own risk)
     */
    boost::shared_ptr<servant_type> get_servant() const
    {
        return m_servant;
    }

    scheduler_proxy_type m_proxy;
    boost::shared_ptr<servant_type> m_servant;
    std::size_t m_offset_id;

private:
    // safe creation of servant in our thread ctor is trivial or told us so
    template <typename S,typename... Args>
    typename std::enable_if<
                boost::has_trivial_constructor<S>::value ||
                boost::asynchronous::has_simple_ctor<S>::value,
            void>::type
    init_servant_proxy(Args... args)
    {
        // TODO forward
        // if a servant has a simple_ctor, then he MUST get a weak scheduler as he might get it too late with tss
        m_servant = boost::make_shared<servant_type>(m_proxy.get_weak_scheduler(),std::move(args)...);
    }
    // ctor has to be posted
    template <typename S,typename... Args>
    typename std::enable_if<
                !(boost::has_trivial_constructor<S>::value ||
                  boost::asynchronous::has_simple_ctor<S>::value),
            void>::type
    init_servant_proxy(Args... args)
    {
        // outside thread, create in scheduler thread
        boost::shared_ptr<boost::promise<boost::shared_ptr<servant_type> > > p =
                boost::make_shared<boost::promise<boost::shared_ptr<servant_type> > >();
        boost::future<boost::shared_ptr<servant_type> > fu (p->get_future());
        typename boost::asynchronous::job_traits<callable_type>::wrapper_type  a(
                    boost::asynchronous::any_callable(
                    boost::asynchronous::move_bind(init_helper(p),m_proxy.get_weak_scheduler(),std::move(args)...)));
        a.set_name(ServantProxy::get_ctor_name());
#ifndef BOOST_NO_RVALUE_REFERENCES
        post(std::move(a),ServantProxy::get_ctor_prio());
#else
        post(a,ServantProxy::get_ctor_prio());
#endif
        bool ok = fu.timed_wait(boost::posix_time::milliseconds(max_create_wait_ms));
        if(ok)
        {
            m_servant = std::move(fu.get());
        }
        else
        {
            throw servant_proxy_timeout();
        }
    }
    struct init_helper : public boost::asynchronous::job_traits<callable_type>::diagnostic_type
    {
        init_helper(boost::shared_ptr<boost::promise<boost::shared_ptr<servant_type> > > p)
          :boost::asynchronous::job_traits<callable_type>::diagnostic_type(),m_promise(p){}
        init_helper(init_helper const& rhs)
          :boost::asynchronous::job_traits<callable_type>::diagnostic_type(),m_promise(rhs.m_promise){}
#ifndef BOOST_NO_RVALUE_REFERENCES
        init_helper(init_helper&& rhs) noexcept :m_promise(std::move(rhs.m_promise)){}
        init_helper& operator= (init_helper const&& rhs)noexcept {m_promise = std::move(rhs.m_promise);}
#endif
        template <typename... Args>
        void operator()(weak_scheduler_proxy_type proxy,Args... as)const
        {
            try
            {
                m_promise->set_value(servant_create_helper::template create<servant_type>(proxy,std::move(as)...));
            }
            catch(std::exception& e){m_promise->set_exception(boost::copy_exception(e));}
        }
        boost::shared_ptr<boost::promise<boost::shared_ptr<servant_type> > > m_promise;
    };
    struct servant_create_helper
    {
        template <typename S,typename... Args>
        static
        typename std::enable_if< boost::asynchronous::has_requires_weak_scheduler<S>::value ||
                                 boost::has_trivial_constructor<S>::value ||
                                 boost::asynchronous::has_simple_ctor<S>::value,
        boost::shared_ptr<servant_type> >::type
        create(weak_scheduler_proxy_type proxy,Args... args)
        {
            boost::shared_ptr<servant_type> res = boost::make_shared<servant_type>(proxy,std::move(args)...);
            return res;
        }
        template <typename S,typename... Args>
        static
        typename std::enable_if<!(boost::asynchronous::has_requires_weak_scheduler<S>::value ||
                                  boost::has_trivial_constructor<S>::value ||
                                  boost::asynchronous::has_simple_ctor<S>::value),
                                boost::shared_ptr<servant_type> >::type
        create(weak_scheduler_proxy_type ,Args... args)
        {
            boost::shared_ptr<servant_type> res = boost::make_shared<servant_type>(std::move(args)...);
            return res;
        }
    };

    struct servant_deleter : public boost::asynchronous::job_traits<callable_type>::diagnostic_type
    {
#ifndef BOOST_NO_RVALUE_REFERENCES
        servant_deleter(boost::shared_ptr<servant_type> t)
            : boost::asynchronous::job_traits<callable_type>::diagnostic_type()
            , data(std::move(t))
            , done_promise(boost::make_shared<boost::promise<void>>())
        {
            t.reset();
        }
        servant_deleter(servant_deleter&& rhs) noexcept
            : boost::asynchronous::job_traits<callable_type>::diagnostic_type()
            , data(std::move(rhs.data))
            , done_promise(std::move(rhs.done_promise))
        {

        }
#endif
        servant_deleter(boost::shared_ptr<servant_type> & t):boost::asynchronous::job_traits<callable_type>::diagnostic_type(), data(t)
        {
            t.reset();
        }
        servant_deleter(servant_deleter const& r): boost::asynchronous::job_traits<callable_type>::diagnostic_type(), data(r.data)
        {
            const_cast<servant_deleter&>(r).data.reset();
            done_promise = std::move(const_cast<servant_deleter&>(r).done_promise);
        }
        servant_deleter& operator= (servant_deleter const& r)noexcept
        {
            std::swap(data,r.data);
            std::swap(done_promise,const_cast<servant_deleter&>(r).done_promise);
            return *this;
        }
#ifndef BOOST_NO_RVALUE_REFERENCES
        servant_deleter& operator= (servant_deleter&& r) noexcept
        {
            std::swap(data,r.data);
            std::swap(done_promise,r.done_promise);
            return *this;
        }
#endif
        ~servant_deleter()
        {
            // this has to be done whatever happens
            if(done_promise)
                done_promise->set_value();
        }

        void operator()()
        {
            data.reset();            
        }
        boost::shared_ptr<servant_type> data;
        boost::shared_ptr<boost::promise<void>> done_promise;
    };
};

/*!
 * \brief Casts a servant_proxy to the servant type into a servant_proxy to a servant's base type.
 * \brief considering struct Servant : public Base and class BaseServantProxy : public boost::asynchronous::servant_proxy<BaseServantProxy,Base>
 * \brief and class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
 * \brief then ServantProxy proxy can be casted:
 * \brief BaseServantProxy base_proxy = boost::asynchronous::dynamic_servant_proxy_cast<BaseServantProxy>(proxy);
 * \param p any_shared_scheduler_proxy where the servant was created and calls executed.
 * \param s servant created in the thread context of the provided scheduler proxy.
 */
template<class T, class U>
T dynamic_servant_proxy_cast( U const & r ) BOOST_NOEXCEPT
{
    return T(r);
}

}} // boost::async

#endif // BOOST_ASYNC_SERVANT_PROXY_H
