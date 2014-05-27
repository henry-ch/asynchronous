// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_SERVANT_PROXY_H
#define BOOST_ASYNC_SERVANT_PROXY_H

#include <functional>
#include <utility>
#include <exception>
#include <cstddef>

#ifndef BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE
#endif
#include  <boost/preprocessor/facilities/overload.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits/has_trivial_constructor.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <boost/preprocessor/facilities/overload.hpp>

#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/job_traits.hpp>

//BOOST_MPL_HAS_XXX_TRAIT_DEF(post_servant_ctor)
BOOST_MPL_HAS_XXX_TRAIT_DEF(simple_ctor)
BOOST_MPL_HAS_XXX_TRAIT_DEF(requires_weak_scheduler)

namespace boost { namespace asynchronous
{    
#define BOOST_ASYNC_POST_MEMBER_1(funcname)                                                                     \
    template <typename... Args>                                                                                 \
    void funcname(Args... args)const                                                                            \
    {                                                                                                           \
        boost::shared_ptr<servant_type> servant = this->m_servant;                                                    \
        this->post(typename boost::asynchronous::job_traits<callable_type>::wrapper_type                              \
        (std::bind([servant](Args... as){servant->funcname(as...);},args...)));                                 \
    }

#define BOOST_ASYNC_POST_MEMBER_2(funcname,prio)                                                                \
    template <typename... Args>                                                                                 \
    void funcname(Args... args)const                                                                            \
    {                                                                                                           \
        boost::shared_ptr<servant_type> servant = this->m_servant;                                                    \
        typename boost::asynchronous::job_traits<callable_type>::wrapper_type  a                                \
                        (std::bind([servant](Args... as){servant->funcname(as...);},args...));                  \
        this->post(typename boost::asynchronous::job_traits<callable_type>::wrapper_type                              \
        (std::bind([servant](Args... as){servant->funcname(as...);},args...)),prio);                            \
    }

#define BOOST_ASYNC_POST_MEMBER(...)                                                                            \
    BOOST_PP_OVERLOAD(BOOST_ASYNC_POST_MEMBER_,__VA_ARGS__)(__VA_ARGS__)

#ifndef BOOST_NO_RVALUE_REFERENCES
#define BOOST_ASYNC_POST_MEMBER_LOG_2(funcname,taskname)                                                        \
    template <typename... Args>                                                                                 \
    void funcname(Args... args)const                                                                            \
    {                                                                                                           \
        boost::shared_ptr<servant_type> servant = this->m_servant;                                                    \
        typename boost::asynchronous::job_traits<callable_type>::wrapper_type  a                                \
                        (std::bind([servant](Args... as){servant->funcname(as...);},args...));                  \
        a.set_name(taskname);                                                                                   \
        this->post(std::move(a));                                                                                     \
    }

#define BOOST_ASYNC_POST_MEMBER_LOG_3(funcname,taskname,prio)                                                   \
    template <typename... Args>                                                                                 \
    void funcname(Args... args)const                                                                            \
    {                                                                                                           \
        boost::shared_ptr<servant_type> servant = this->m_servant;                                                    \
        typename boost::asynchronous::job_traits<callable_type>::wrapper_type  a                                \
                        (std::bind([servant](Args... as){servant->funcname(as...);},args...));                  \
        a.set_name(taskname);                                                                                   \
        this->post(std::move(a));                                                                                     \
    }
#else
#define BOOST_ASYNC_POST_MEMBER_LOG_2(funcname,taskname)                                                        \
    template <typename... Args>                                                                                 \
    void funcname(Args... args)const                                                                            \
    {                                                                                                           \
        boost::shared_ptr<servant_type> servant = this->m_servant;                                                    \
        typename boost::asynchronous::job_traits<callable_type>::wrapper_type  a                                \
                        (std::bind([servant](Args... as){servant->funcname(as...);},args...));                  \
        a.set_name(taskname);                                                                                   \
        this->post(a);                                                                                                \
    }

#define BOOST_ASYNC_POST_MEMBER_LOG_3(funcname,taskname,prio)                                                   \
    template <typename... Args>                                                                                 \
    void funcname(Args... args)const                                                                            \
    {                                                                                                           \
        boost::shared_ptr<servant_type> servant = this->m_servant;                                                    \
        typename boost::asynchronous::job_traits<callable_type>::wrapper_type  a                                \
                        (std::bind([servant](Args... as){servant->funcname(as...);},args...));                  \
        a.set_name(taskname);                                                                                   \
        this->post(a,prio);                                                                                           \
    }
#endif

#define BOOST_ASYNC_POST_MEMBER_LOG(...)                                                                        \
    BOOST_PP_OVERLOAD(BOOST_ASYNC_POST_MEMBER_LOG_,__VA_ARGS__)(__VA_ARGS__)


// with this, will not compile with gcc 4.7 :(
// ,typename boost::disable_if< boost::is_same<void,decltype(m_servant->funcname(args...))> >::type* dummy = 0
#define BOOST_ASYNC_FUTURE_MEMBER_LOG_2(funcname,taskname)                                                      \
    template <typename... Args>                                                                                 \
    auto funcname(Args... args)const                                                                            \
        -> typename boost::disable_if< boost::is_same<void,decltype(boost::shared_ptr<servant_type>()->funcname(args...))>,             \
                                       boost::future<decltype(boost::shared_ptr<servant_type>()->funcname(args...))> >::type            \
    {                                                                                                           \
        boost::shared_ptr<servant_type> servant = this->m_servant;                                                    \
        boost::future<decltype(boost::shared_ptr<servant_type>()->funcname(args...))> fu=boost::asynchronous::post_future(this->m_proxy,      \
                std::bind([servant](Args... as)->decltype(boost::shared_ptr<servant_type>()->funcname(args...))                         \
                                    {return servant->funcname(as...);                                           \
                                    },args...),taskname);                                                       \
        return std::move(fu);                                                                                   \
    }                                                                                                           \
    template <typename... Args>                                                                                 \
    auto funcname(Args... args)const                                                                            \
        -> typename boost::enable_if< boost::is_same<void,decltype(boost::shared_ptr<servant_type>()->funcname(args...))>,              \
                                       boost::future<void> >::type                                              \
    {                                                                                                           \
        boost::shared_ptr<servant_type> servant = this->m_servant;                                                    \
        boost::future<void> fu = boost::asynchronous::post_future(this->m_proxy,                                      \
                std::bind([servant](Args... as)                                                                 \
                                {servant->funcname(as...);                                                      \
                                },args...),taskname);                                                           \
        return std::move(fu);                                                                                   \
    }

#define BOOST_ASYNC_FUTURE_MEMBER_LOG_3(funcname,taskname,prio)                                                 \
    template <typename... Args>                                                                                 \
    auto funcname(Args... args)const                                                                            \
        -> typename boost::disable_if< boost::is_same<void,decltype(boost::shared_ptr<servant_type>()->funcname(args...))>,             \
                                       boost::future<decltype(boost::shared_ptr<servant_type>()->funcname(args...))> >::type            \
    {                                                                                                           \
        boost::shared_ptr<servant_type> servant = this->m_servant;                                                    \
        boost::future<decltype(boost::shared_ptr<servant_type>()->funcname(args...))> fu=boost::asynchronous::post_future(this->m_proxy,      \
                std::bind([servant](Args... as)->decltype(boost::shared_ptr<servant_type>()->funcname(args...))                         \
                                    {return servant->funcname(as...);                                           \
                                    },args...),taskname,prio);                                                  \
        return std::move(fu);                                                                                   \
    }                                                                                                           \
    template <typename... Args>                                                                                 \
    auto funcname(Args... args)const                                                                            \
        -> typename boost::enable_if< boost::is_same<void,decltype(boost::shared_ptr<servant_type>()->funcname(args...))>,              \
                                       boost::future<void> >::type                                              \
    {                                                                                                           \
        boost::shared_ptr<servant_type> servant = this->m_servant;                                                    \
        boost::future<void> fu = boost::asynchronous::post_future(this->m_proxy,                                      \
                std::bind([servant](Args... as)                                                                 \
                                {servant->funcname(as...);                                                      \
                                },args...),taskname,prio);                                                      \
        return std::move(fu);                                                                                   \
    }

#define BOOST_ASYNC_FUTURE_MEMBER_LOG(...)                                                                      \
    BOOST_PP_OVERLOAD(BOOST_ASYNC_FUTURE_MEMBER_LOG_,__VA_ARGS__)(__VA_ARGS__)


#define BOOST_ASYNC_FUTURE_MEMBER_1(funcname)                                                                   \
    template <typename... Args>                                                                                 \
    auto funcname(Args... args)const                                                                            \
        -> typename boost::disable_if< boost::is_same<void,decltype(boost::shared_ptr<servant_type>()->funcname(args...))>,             \
                                       boost::future<decltype(boost::shared_ptr<servant_type>()->funcname(args...))> >::type            \
    {                                                                                                           \
        boost::shared_ptr<servant_type> servant = this->m_servant;                                                    \
        boost::future<decltype(boost::shared_ptr<servant_type>()->funcname(args...))> fu=boost::asynchronous::post_future(this->m_proxy,      \
                std::bind([servant](Args... as)->decltype(boost::shared_ptr<servant_type>()->funcname(args...))                         \
                                    {return servant->funcname(as...);                                           \
                                    },args...));                                                                \
        return std::move(fu);                                                                                   \
    }                                                                                                           \
    template <typename... Args>                                                                                 \
    auto funcname(Args... args)const                                                                            \
        -> typename boost::enable_if< boost::is_same<void,decltype(boost::shared_ptr<servant_type>()->funcname(args...))>,              \
                                       boost::future<void> >::type                                              \
    {                                                                                                           \
        boost::shared_ptr<servant_type> servant = this->m_servant;                                                    \
        boost::future<void> fu = boost::asynchronous::post_future(this->m_proxy,                                      \
                std::bind([servant](Args... as)                                                                 \
                                {servant->funcname(as...);                                                      \
                                },args...));                                                                    \
        return std::move(fu);                                                                                   \
    }

#define BOOST_ASYNC_FUTURE_MEMBER_2(funcname,prio)                                                              \
    template <typename... Args>                                                                                 \
    auto funcname(Args... args)const                                                                            \
        -> typename boost::disable_if< boost::is_same<void,decltype(boost::shared_ptr<servant_type>()->funcname(args...))>,             \
                                       boost::future<decltype(boost::shared_ptr<servant_type>()->funcname(args...))> >::type            \
    {                                                                                                           \
        boost::shared_ptr<servant_type> servant = this->m_servant;                                                    \
        boost::future<decltype(boost::shared_ptr<servant_type>()->funcname(args...))> fu=boost::asynchronous::post_future(this->m_proxy,      \
                std::bind([servant](Args... as)->decltype(boost::shared_ptr<servant_type>()->funcname(args...))                         \
                                    {return servant->funcname(as...);                                           \
                                    },args...),"",prio);                                                        \
        return std::move(fu);                                                                                   \
    }                                                                                                           \
    template <typename... Args>                                                                                 \
    auto funcname(Args... args)const                                                                            \
        -> typename boost::enable_if< boost::is_same<void,decltype(boost::shared_ptr<servant_type>()->funcname(args...))>,              \
                                       boost::future<void> >::type                                              \
    {                                                                                                           \
        boost::shared_ptr<servant_type> servant = this->m_servant;                                                    \
        boost::future<void> fu = boost::asynchronous::post_future(this->m_proxy,                                      \
                std::bind([servant](Args... as)                                                                 \
                                {servant->funcname(as...);                                                      \
                                },args...),"",prio);                                                            \
        return std::move(fu);                                                                                   \
    }

#define BOOST_ASYNC_FUTURE_MEMBER(...)                                                                          \
    BOOST_PP_OVERLOAD(BOOST_ASYNC_FUTURE_MEMBER_,__VA_ARGS__)(__VA_ARGS__)

    
#define BOOST_ASYNC_POST_CALLBACK_MEMBER_1(funcname)                                                            \
    template <typename F, typename S,typename... Args>                                                          \
    void funcname(F&& cb_func,S const& weak_cb_scheduler,std::size_t cb_prio, Args... args)const                \
    {                                                                                                           \
        boost::shared_ptr<servant_type> servant = m_servant;                                                    \
        boost::asynchronous::post_callback(m_proxy,                                                             \
                std::bind([servant](Args... as)->decltype(boost::shared_ptr<servant_type>()->funcname(args...))                         \
                                    {return servant->funcname(as...);                                           \
                                    },args...),weak_cb_scheduler,std::forward<F>(cb_func),"",0,cb_prio);        \
    }

#define BOOST_ASYNC_POST_CALLBACK_MEMBER_2(funcname,prio)                                                       \
    template <typename F, typename S,typename... Args>                                                          \
    void funcname(F&& cb_func,S const& weak_cb_scheduler,std::size_t cb_prio, Args... args)const                \
    {                                                                                                           \
        boost::shared_ptr<servant_type> servant = m_servant;                                                    \
        boost::asynchronous::post_callback(m_proxy,                                                             \
                std::bind([servant](Args... as)->decltype(boost::shared_ptr<servant_type>()->funcname(args...))                         \
                                    {return servant->funcname(as...);                                           \
                                    },args...),weak_cb_scheduler,std::forward<F>(cb_func),"",prio,cb_prio);     \
    }

#define BOOST_ASYNC_POST_CALLBACK_MEMBER(...)                                                                   \
    BOOST_PP_OVERLOAD(BOOST_ASYNC_POST_CALLBACK_MEMBER_,__VA_ARGS__)(__VA_ARGS__)    

#define BOOST_ASYNC_UNSAFE_MEMBER(funcname)                                                                     \
    template <typename... Args>                                                                                 \
    auto funcname(Args... args)const                                                                            \
    -> typename boost::enable_if< boost::is_same<void,decltype(boost::shared_ptr<servant_type>()->funcname(args...))>,                  \
                                  std::function<void()> >::type                                                 \
    {                                                                                                           \
        boost::shared_ptr<servant_type> servant = m_servant;                                                    \
        return std::bind([servant](Args... as){servant->funcname(as...);},args...);                             \
    }                                                                                                           \
    template <typename... Args>                                                                                 \
    auto funcname(Args... args)const                                                                            \
    -> typename boost::disable_if< boost::is_same<void,decltype(boost::shared_ptr<servant_type>()->funcname(args...))>,                 \
                                   std::function<decltype(boost::shared_ptr<servant_type>()->funcname(args...))()> >::type \
    {                                                                                                           \
        boost::shared_ptr<servant_type> servant = m_servant;                                                    \
        return std::bind([servant](Args... as){return servant->funcname(as...);},args...);                      \
    }    
    
#define BOOST_ASYNC_SERVANT_POST_CTOR_0()                                                                       \
    static std::size_t get_ctor_prio() {return 0;}

#define BOOST_ASYNC_SERVANT_POST_CTOR_1(priority)                                                               \
    static std::size_t get_ctor_prio() {return priority;}

#define BOOST_ASYNC_SERVANT_POST_CTOR(...)                                                                      \
    BOOST_PP_OVERLOAD(BOOST_ASYNC_SERVANT_POST_CTOR_,__VA_ARGS__)(__VA_ARGS__)

#define BOOST_ASYNC_SERVANT_POST_CTOR_LOG_1(taskname)                                                           \
    static const char* get_ctor_name() {return taskname;}

#define BOOST_ASYNC_SERVANT_POST_CTOR_LOG_2(taskname,priority)                                                  \
    static std::size_t get_ctor_prio() {return priority;}                                                       \
    static const char* get_ctor_name() {return taskname;}

#define BOOST_ASYNC_SERVANT_POST_CTOR_LOG(...)                                                                  \
    BOOST_PP_OVERLOAD(BOOST_ASYNC_SERVANT_POST_CTOR_LOG_,__VA_ARGS__)(__VA_ARGS__)

#define BOOST_ASYNC_SERVANT_POST_DTOR_0()                                                                       \
    static std::size_t get_dtor_prio() {return 0;}

#define BOOST_ASYNC_SERVANT_POST_DTOR_1(priority)                                                               \
    static std::size_t get_dtor_prio() {return priority;}

#define BOOST_ASYNC_SERVANT_POST_DTOR(...)                                                                      \
    BOOST_PP_OVERLOAD(BOOST_ASYNC_SERVANT_POST_DTOR_,__VA_ARGS__)(__VA_ARGS__)

#define BOOST_ASYNC_SERVANT_POST_DTOR_LOG_1(taskname)                                                           \
    static const char* get_dtor_name() {return taskname;}

#define BOOST_ASYNC_SERVANT_POST_DTOR_LOG_2(taskname,priority)                                                  \
    static std::size_t get_dtor_prio() {return priority;}                                                       \
    static const char* get_dtor_name() {return taskname;}

#define BOOST_ASYNC_SERVANT_POST_DTOR_LOG(...)                                                                  \
    BOOST_PP_OVERLOAD(BOOST_ASYNC_SERVANT_POST_DTOR_LOG_,__VA_ARGS__)(__VA_ARGS__)



struct servant_proxy_timeout : public std::exception
{
};

template <class ServantProxy,class Servant, class Callable = boost::asynchronous::any_callable,int max_create_wait_ms = 1000>
class servant_proxy
{
public:
    typedef Servant servant_type;
    typedef Callable callable_type;
    typedef boost::asynchronous::any_shared_scheduler_proxy<callable_type> scheduler_proxy_type;

    template <typename... Args>
    servant_proxy(scheduler_proxy_type p, Args... args)
        : m_proxy(p)
        , m_servant()
    {
        std::vector<boost::thread::id> ids = m_proxy.thread_ids();
        if ((std::find(ids.begin(),ids.end(),boost::this_thread::get_id()) != ids.end()))
        {
            // our thread, not possible to wait for a future
            // TODO forward
            // if a servant has a simple_ctor, then he MUST get a weak scheduler as he might get it too late with tss
            //m_servant = boost::make_shared<servant_type>(m_proxy.get_weak_scheduler(),args...);
            m_servant = servant_create_helper::template create<servant_type>(m_proxy,args...);
        }
        else
        {
            // outside thread, create in scheduler thread if no other choice
            init<servant_type>(args...);
        }
    }
    servant_proxy(scheduler_proxy_type p, boost::future<boost::shared_ptr<servant_type> > s)
        : m_proxy(p)
        , m_servant()
    {
        bool ok = s.timed_wait(boost::posix_time::milliseconds(max_create_wait_ms));
        if(ok)
        {
            m_servant = s.get();
        }
        else
        {
            throw servant_proxy_timeout();
        }
    }
    servant_proxy(scheduler_proxy_type p, boost::future<servant_type> s)
        : m_proxy(p)
        , m_servant()
    {
        bool ok = s.timed_wait(boost::posix_time::milliseconds(max_create_wait_ms));
        if(ok)
        {
            m_servant = boost::make_shared<servant_type>(std::move(s.get()));
        }
        else
        {
            throw servant_proxy_timeout();
        }
    }
    ~servant_proxy()
    {
#ifndef BOOST_NO_RVALUE_REFERENCES
        servant_deleter n(std::move(m_servant));
        typename boost::asynchronous::job_traits<callable_type>::wrapper_type  a(std::move(n));
        a.set_name(ServantProxy::get_dtor_name());
        m_proxy.post(std::move(a),ServantProxy::get_dtor_prio());
#else
        servant_deleter n(m_servant);
        typename boost::asynchronous::job_traits<callable_type>::wrapper_type  a(n);
        a.set_name(ServantProxy::get_dtor_name());
        m_proxy.post(a,ServantProxy::get_dtor_prio());
#endif
        m_servant.reset();
        m_proxy.reset();
    }
#ifndef BOOST_NO_RVALUE_REFERENCES
    void post(callable_type&& job, std::size_t prio=0) const
    {
        m_proxy.post(std::forward<callable_type>(job),prio);
    }
#else
    void post(callable_type job, std::size_t prio=0) const
    {
        m_proxy.post(job,prio);
    }
#endif
    scheduler_proxy_type get_proxy()const
    {
        return m_proxy;
    }
// g++ 4.5 is uncooperative
#if defined __GNUC__ == 4 && __GNUC_MINOR__ < 6
protected:
#endif
    // for derived to overwrite if needed
    static std::size_t get_ctor_prio() {return 0;}
    static std::size_t get_dtor_prio() {return 0;}
    static const char* get_ctor_name() {return "";}
    static const char* get_dtor_name() {return "";}

    scheduler_proxy_type m_proxy;
    boost::shared_ptr<servant_type> m_servant;

private:
    // safe creation of servant in our thread ctor is trivial or told us so
    template <typename S,typename... Args>
    typename boost::enable_if<
                typename ::boost::mpl::or_<
                    typename boost::has_trivial_constructor<S>::type,
                    typename has_simple_ctor<S>::type
                >,
            void>::type
    init(Args... args)
    {
        // TODO forward
        // if a servant has a simple_ctor, then he MUST get a weak scheduler as he might get it too late with tss
        m_servant = boost::make_shared<servant_type>(m_proxy.get_weak_scheduler(),args...);
        //m_servant = servant_create_helper::template create<servant_type>(m_proxy,args...);
    }
    // ctor has to be posted
    template <typename S,typename... Args>
    typename boost::disable_if<
                typename ::boost::mpl::or_<
                    typename boost::has_trivial_constructor<S>::type,
                    typename has_simple_ctor<S>::type
                >,
            void>::type
    init(Args... args)
    {
        // outside thread, create in scheduler thread
        boost::shared_ptr<boost::promise<boost::shared_ptr<servant_type> > > p =
                boost::make_shared<boost::promise<boost::shared_ptr<servant_type> > >();
        boost::future<boost::shared_ptr<servant_type> > fu (p->get_future());
        typename boost::asynchronous::job_traits<callable_type>::wrapper_type  a(std::bind(init_helper(p),m_proxy,args...));
        a.set_name(ServantProxy::get_ctor_name());
#ifndef BOOST_NO_RVALUE_REFERENCES
        post(std::move(a),ServantProxy::get_ctor_prio());
#else
        post(a,ServantProxy::get_ctor_prio());
#endif
        bool ok = fu.timed_wait(boost::posix_time::milliseconds(max_create_wait_ms));
        if(ok)
        {
            m_servant = fu.get();
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
        init_helper(init_helper&& rhs):m_promise(std::move(rhs.m_promise)){}
        init_helper& operator= (init_helper const&& rhs){m_promise = std::move(rhs.m_promise);}
#endif
        template <typename... Args>
        void operator()(scheduler_proxy_type proxy,Args... as)const
        {
            try
            {
                m_promise->set_value(servant_create_helper::template create<servant_type>(proxy,as...));
            }
            catch(std::exception& e){m_promise->set_exception(boost::copy_exception(e));}
        }
        boost::shared_ptr<boost::promise<boost::shared_ptr<servant_type> > > m_promise;
    };
    struct servant_create_helper
    {
        template <typename S,typename... Args>
        static
        typename boost::enable_if<  typename ::boost::mpl::or_<
                                            typename has_requires_weak_scheduler<S>::type,
                                            typename ::boost::mpl::or_<
                                                typename boost::has_trivial_constructor<S>::type,
                                                typename has_simple_ctor<S>::type
                                            >::type
                                          >::type,
        boost::shared_ptr<servant_type> >::type
        create(scheduler_proxy_type proxy,Args... args)
        {
            boost::shared_ptr<servant_type> res = boost::make_shared<servant_type>(proxy.get_weak_scheduler(),args...);
            return res;
        }
        template <typename S,typename... Args>
        static
        typename boost::disable_if<  typename ::boost::mpl::or_<
                                            typename has_requires_weak_scheduler<S>::type,
                                            typename ::boost::mpl::or_<
                                                typename boost::has_trivial_constructor<S>::type,
                                                typename has_simple_ctor<S>::type
                                            >::type
                                          >::type,
        boost::shared_ptr<servant_type> >::type
        create(scheduler_proxy_type ,Args... args)
        {
            boost::shared_ptr<servant_type> res = boost::make_shared<servant_type>(args...);
            return res;
        }
    };

    struct servant_deleter : public boost::asynchronous::job_traits<callable_type>::diagnostic_type
    {
        //servant_deleter(boost::shared_ptr<servant_type> && t):data(t){t.reset();}
        //servant_deleter(servant_deleter&& r): data(r.data){r.data.reset();}
        //servant_deleter& operator()(servant_deleter&& r){data = r.data; r.data.reset();}
#ifndef BOOST_NO_RVALUE_REFERENCES
        servant_deleter(boost::shared_ptr<servant_type> && t):boost::asynchronous::job_traits<callable_type>::diagnostic_type(), data(t)
        {
            t.reset();
        }
#endif
        servant_deleter(boost::shared_ptr<servant_type> & t):boost::asynchronous::job_traits<callable_type>::diagnostic_type(), data(t)
        {
            t.reset();
        }
        //TODO move copy-ctor / operator()
        servant_deleter(servant_deleter const& r): boost::asynchronous::job_traits<callable_type>::diagnostic_type(), data(r.data)
        {
            const_cast<servant_deleter&>(r).data.reset();
        }
        servant_deleter& operator= (servant_deleter const& r)
        {
            data = r.data; const_cast<servant_deleter&>(r).data.reset();
        }
#ifndef BOOST_NO_RVALUE_REFERENCES
        servant_deleter& operator= (servant_deleter&& r)
        {
            data = r.data; r.data.reset();
        }
#endif
        void operator()()const
        {
        }
        boost::shared_ptr<servant_type> data;
    };
};
}} // boost::async

#endif // BOOST_ASYNC_SERVANT_PROXY_H
