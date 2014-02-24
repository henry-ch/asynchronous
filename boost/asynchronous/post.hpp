// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRON_POST_HPP
#define BOOST_ASYNCHRON_POST_HPP

#include <cstddef>

#ifndef BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE
#endif
#include <boost/thread/future.hpp>
#include <boost/exception/all.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/asynchronous/any_shared_scheduler_proxy.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/exceptions.hpp>
#include <boost/asynchronous/job_traits.hpp>
#include <boost/asynchronous/scheduler/detail/any_continuation.hpp>
#include <boost/asynchronous/scheduler/tss_scheduler.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/client_request.hpp>

namespace boost { namespace asynchronous
{

//TODO move to own file in detail
template <class R, class Task,class Callback/*,class Enable=void*/>
struct move_task_helper
{
    typedef R result_type;
    typedef Task task_type;
    move_task_helper(boost::shared_ptr<Task>& t, boost::shared_ptr<Callback> & c)
        : m_callback(c), m_task(t)
    {
        t.reset();
        c.reset();
    }
    move_task_helper(move_task_helper const& r): m_callback(r.m_callback), m_task(r.m_task)
    {
        const_cast<move_task_helper&>(r).m_callback.reset();
        const_cast<move_task_helper&>(r).m_task.reset();
    }
    move_task_helper& operator= (move_task_helper& r)
    {
        m_callback = r.m_callback; r.m_callback.reset();
        m_task = r.m_task; r.m_task.reset();
    }
#ifndef BOOST_NO_RVALUE_REFERENCES
    move_task_helper(boost::shared_ptr<Task>&& t,boost::shared_ptr<Callback> && c)
        :m_callback(std::forward<boost::shared_ptr<Callback> >(c))
        ,m_task(std::forward<boost::shared_ptr<Task> >(t))
    {
        t.reset();
        c.reset();
    }
    move_task_helper(move_task_helper && r)
        : m_callback(std::move(r.m_callback))
        , m_task(std::move(r.m_task))
    {
        r.m_callback.reset();
        r.m_task.reset();
    }
    move_task_helper& operator= (move_task_helper&& r)
    {
        m_callback = std::move(r.m_callback); r.m_callback.reset();
        m_task = std::move(r.m_task); r.m_task.reset();
    }
#endif
    void operator()(boost::future<R> result_func)const
    {
        (*m_callback)(std::move(result_func));
    }
    boost::shared_ptr<Callback> m_callback;
    // task is only here to be moved to worker and then back for destruction in originator thread
    boost::shared_ptr<Task> m_task;
};

namespace detail
{
    template <class R,class F,class JOB, class OP,class Scheduler,class Enable=void>
    struct post_future_helper_base : public boost::asynchronous::job_traits<JOB>::diagnostic_type
    {
        post_future_helper_base():boost::asynchronous::job_traits<JOB>::diagnostic_type(){}
#ifndef BOOST_NO_RVALUE_REFERENCES
        post_future_helper_base(boost::shared_ptr<boost::promise<R> > p, F&& f)
            : boost::asynchronous::job_traits<JOB>::diagnostic_type(),
              m_promise(p),m_func(boost::make_shared<F>(std::forward<F>(f))) {}
        post_future_helper_base(post_future_helper_base&& rhs)
            : boost::asynchronous::job_traits<JOB>::diagnostic_type(),
              m_promise(std::move(rhs.m_promise)),m_func(std::move(rhs.m_func)) {}
        post_future_helper_base& operator= (post_future_helper_base&& rhs)
        {
            m_promise = std::move(rhs.m_promise);
            m_func = std::move(rhs.m_func);
            return *this;
        }
#endif
    //#else
        post_future_helper_base(post_future_helper_base const& rhs)
            :boost::asynchronous::job_traits<JOB>::diagnostic_type()
            ,m_promise(rhs.m_promise),m_func(rhs.m_func){}
    //#endif
        void operator()()
        {
            OP()(m_promise,*m_func);
        }
        boost::shared_ptr<boost::promise<R> > m_promise;
        boost::shared_ptr<F> m_func;
    };

    // version for serializable tasks but not continuations
    // TODO a bit better than checking R...
    template <class R,class F,class JOB, class OP,class Scheduler>
    struct post_future_helper_base<R,F,JOB,OP,Scheduler,
            typename ::boost::enable_if<boost::mpl::and_<
                boost::asynchronous::detail::is_serializable<F>,
                boost::mpl::not_<boost::is_same<R,void> > > >::type >
            : public boost::asynchronous::job_traits<JOB>::diagnostic_type
    {
        post_future_helper_base():boost::asynchronous::job_traits<JOB>::diagnostic_type(){}
#ifndef BOOST_NO_RVALUE_REFERENCES
        post_future_helper_base(boost::shared_ptr<boost::promise<R> > p, F&& f)
            : boost::asynchronous::job_traits<JOB>::diagnostic_type(),
              m_promise(p),m_func(boost::make_shared<F>(std::forward<F>(f))) {}
        post_future_helper_base(post_future_helper_base&& rhs)
            : boost::asynchronous::job_traits<JOB>::diagnostic_type(),
              m_promise(std::move(rhs.m_promise)),m_func(std::move(rhs.m_func)) {}
        post_future_helper_base& operator= (post_future_helper_base&& rhs)
        {
            m_promise = std::move(rhs.m_promise);
            m_func = std::move(rhs.m_func);
            return *this;
        }
#endif
    //#else
        post_future_helper_base(post_future_helper_base const& rhs)
            :boost::asynchronous::job_traits<JOB>::diagnostic_type(),m_promise(rhs.m_promise)
            ,m_func(rhs.m_func){}
    //#endif
        template <class Archive>
        void save(Archive & ar, const unsigned int /*version*/)const
        {
            ar & *m_func;
        }
        template <class Archive>
        void load(Archive & ar, const unsigned int /*version*/)
        {
            boost::asynchronous::tcp::client_request::message_payload payload;
            ar >> payload;
            if (!payload.m_has_exception)
            {
                std::istringstream archive_stream(payload.m_data);
                typedef typename Scheduler::job_type jobtype;
                typename jobtype::iarchive archive(archive_stream);
                R res;
                archive >> res;
                m_promise->set_value(res);
            }
            else
            {
                m_promise->set_exception(boost::copy_exception(payload.m_exception));
            }
        }
        std::string get_task_name()const
        {
            return m_func->get_task_name();
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()
        void operator()()
        {
            OP()(m_promise,*m_func);
        }
        boost::shared_ptr<boost::promise<R> > m_promise;
        boost::shared_ptr<F> m_func;
    };
    // version for tasks which are serializable AND continuations
    // TODO a bit better than checking R...
    template <class R,class F,class JOB, class OP,class Scheduler>
    struct post_future_helper_base<R,F,JOB,OP,Scheduler,
            typename ::boost::enable_if<boost::mpl::and_<
                boost::asynchronous::detail::is_serializable<F>,
                boost::is_same<R,void> > >::type >
            : public boost::asynchronous::job_traits<JOB>::diagnostic_type
    {
        post_future_helper_base():boost::asynchronous::job_traits<JOB>::diagnostic_type(){}
#ifndef BOOST_NO_RVALUE_REFERENCES
        post_future_helper_base(boost::shared_ptr<boost::promise<R> > p, F&& f)
            : boost::asynchronous::job_traits<JOB>::diagnostic_type(),
              m_promise(p),m_func(boost::make_shared<F>(std::forward<F>(f))) {}
        post_future_helper_base(post_future_helper_base&& rhs)
            : boost::asynchronous::job_traits<JOB>::diagnostic_type(),
              m_promise(std::move(rhs.m_promise)),m_func(std::move(rhs.m_func)) {}
        post_future_helper_base& operator= (post_future_helper_base&& rhs)
        {
            m_promise = std::move(rhs.m_promise);
            m_func = std::move(rhs.m_func);
            return *this;
        }
#endif
    //#else
        post_future_helper_base(post_future_helper_base const& rhs)
            :boost::asynchronous::job_traits<JOB>::diagnostic_type(),m_promise(rhs.m_promise),m_func(rhs.m_func){}
    //#endif
        template <class Archive>
        void save(Archive & ar, const unsigned int /*version*/)const
        {
            ar & *m_func;
        }
        template <class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            m_func->template as_result<typename Scheduler::job_type::iarchive,Archive>(ar,version);
        }
        std::string get_task_name()const
        {
            return m_func->get_task_name();
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()
        void operator()()
        {
            OP()(m_promise,*m_func);
        }
        boost::shared_ptr<boost::promise<R> > m_promise;
        boost::shared_ptr<F> m_func;
    };
}

template <class F, class S>
#ifndef BOOST_NO_RVALUE_REFERENCES
auto post_future(S const& scheduler, F func,
#else
auto post_future(S const& scheduler, F const& func,
#endif
                 const std::string& task_name="", std::size_t prio=0)
    -> typename boost::disable_if< boost::is_same<void,decltype(func())>,
                                   boost::future<decltype(func())> >::type
{
    boost::shared_ptr<boost::promise<decltype(func())> > p = boost::make_shared<boost::promise<decltype(func())> >();
    boost::future<decltype(func())> fu(p->get_future());

    struct post_helper
    {
        void operator()(boost::shared_ptr<boost::promise<decltype(func())> > sp, F& f)const
        {
            try{sp->set_value(f());}
            catch(...){sp->set_exception(boost::current_exception());}
        }
    };

#ifndef BOOST_NO_RVALUE_REFERENCES
    detail::post_future_helper_base<decltype(func()),F,typename S::job_type,post_helper,S> fct (p,std::move(func));
    typename boost::asynchronous::job_traits<typename S::job_type>::wrapper_type w(std::move(fct));
    w.set_name(task_name);
    scheduler.post(std::move(w),prio);
#else
    detail::post_future_helper_base<decltype(func()),F,typename S::job_type,post_helper,S> fct (p,func);
    if (task_name.empty())
    {
        scheduler.post(fct);
    }
    else
    {
        scheduler.post_log(fct,task_name);
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
                 const std::string& task_name="", std::size_t prio=0)
    -> typename boost::enable_if< boost::is_same<void,decltype(func())>,
                                  boost::future<void> >::type
{
    boost::shared_ptr<boost::promise<void> > p= boost::make_shared<boost::promise<void> >();
    boost::future<void> fu(p->get_future());

    struct post_helper
    {
        void operator()(boost::shared_ptr<boost::promise<decltype(func())> > sp, F& f)const
        {
            try{f();sp->set_value();}
            catch(...){sp->set_exception(boost::current_exception());}
        }
    };

#ifndef BOOST_NO_RVALUE_REFERENCES
    detail::post_future_helper_base<decltype(func()),F,typename S::job_type,post_helper,S> fct (p,std::move(func));
    typename boost::asynchronous::job_traits<typename S::job_type>::wrapper_type w(std::move(fct));
    w.set_name(task_name);
    scheduler.post(std::move(w),prio);
#else
    detail::post_future_helper_base<decltype(func()),F,typename S::job_type,post_helper,S> fct (p,func);
    if (task_name.empty())
    {
        scheduler.post(fct);
    }
    else
    {
        scheduler.post_log(fct,task_name);
    }
#endif

    return std::move(fu);
}

template <class F, class S>
#ifndef BOOST_NO_RVALUE_REFERENCES
auto interruptible_post_future(S const& scheduler, F func,
#else
auto interruptible_post_future(S const& scheduler, F const& func,
#endif
                 const std::string& task_name="", std::size_t prio=0)
    -> typename boost::disable_if< boost::is_same<void,decltype(func())>,
                                   std::pair<boost::future<decltype(func())>,boost::asynchronous::any_interruptible > >::type
{
    boost::shared_ptr<boost::promise<decltype(func())> > p = boost::make_shared<boost::promise<decltype(func())> >();
    boost::future<decltype(func())> fu(p->get_future());

    struct post_helper
    {
        void operator()(boost::shared_ptr<boost::promise<decltype(func())> > sp, F& f)const
        {
            try{sp->set_value(f());}
            catch(boost::thread_interrupted&)
            {
                boost::asynchronous::task_aborted_exception ta;
                sp->set_exception(boost::copy_exception(ta));
            }
            catch(...){sp->set_exception(boost::current_exception());}
        }
    };

    detail::post_future_helper_base<decltype(func()),F,typename S::job_type,post_helper,S> fct (p,std::move(func));
    typename boost::asynchronous::job_traits<typename S::job_type>::wrapper_type w(std::move(fct));
    w.set_name(task_name);
    return std::make_pair(std::move(fu),scheduler.interruptible_post(std::move(w),prio));
}
template <class F, class S>
#ifndef BOOST_NO_RVALUE_REFERENCES
auto interruptible_post_future(S const& scheduler, F func,
#else
auto interruptible_post_future(S const& scheduler, F const& func,
#endif
                 const std::string& task_name="", std::size_t prio=0)
    -> typename boost::enable_if< boost::is_same<void,decltype(func())>,
                                  std::pair<boost::future<void>,boost::asynchronous::any_interruptible > >::type
{
    boost::shared_ptr<boost::promise<void> > p= boost::make_shared<boost::promise<void> >();
    boost::future<void> fu(p->get_future());

    struct post_helper
    {
        void operator()(boost::shared_ptr<boost::promise<decltype(func())> > sp, F& f)const
        {
            try{f();sp->set_value();}
            catch(boost::thread_interrupted&)
            {
                boost::asynchronous::task_aborted_exception ta;
                sp->set_exception(boost::copy_exception(ta));
            }
            catch(...){sp->set_exception(boost::current_exception());}
        }
    };

    detail::post_future_helper_base<decltype(func()),F,typename S::job_type,post_helper,S> fct (p,std::move(func));
    typename boost::asynchronous::job_traits<typename S::job_type>::wrapper_type w(std::move(fct));
    w.set_name(task_name);
    return std::make_pair(std::move(fu),scheduler.interruptible_post(std::move(w),prio));
}

namespace detail
{
    template <class Work,class Sched,class PostSched,class OP,class CB,class Enable=void>
    struct post_callback_helper_base : public boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type
    {
#ifndef BOOST_NO_RVALUE_REFERENCES
        post_callback_helper_base(Work&& w, Sched const& s, const std::string& task_name="",
                                  std::size_t cb_prio=0)
            : m_work(std::forward<Work>(w)),m_scheduler(s),m_task_name(task_name),m_cb_prio(cb_prio) {}
        post_callback_helper_base(post_callback_helper_base&& rhs)
            : m_work(std::move(rhs.m_work)),m_scheduler(rhs.m_scheduler)
            , m_task_name(std::move(rhs.m_task_name)),m_cb_prio(rhs.m_cb_prio) {}
        post_callback_helper_base& operator= (post_callback_helper_base&& rhs)
        {
            m_work = std::move(rhs.m_work);
            m_scheduler = rhs.m_scheduler;
            m_task_name = std::move(rhs.m_task_name);
            m_cb_prio = rhs.m_cb_prio;
            return *this;
        }
        // TODO type_erasure problem?
        //post_callback_helper_base& operator =( const post_callback_helper_base& ) = delete;
        //post_callback_helper_base ( const post_callback_helper_base& ) = delete;
#endif
    //#else
        post_callback_helper_base(post_callback_helper_base const& rhs)
            : boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type(),
              m_work(rhs.m_work),m_scheduler(rhs.m_scheduler),m_task_name(rhs.m_task_name),m_cb_prio(rhs.m_cb_prio){}
        post_callback_helper_base(Work const& w, Sched const& s, const std::string& task_name="",std::size_t cb_prio=0)
            : boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type(),
              m_work(w),m_scheduler(s),m_task_name(task_name),m_cb_prio(cb_prio){}
    //#endif

        struct callback_fct : public boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type
        {
            callback_fct(Work const& w,boost::future<typename Work::result_type> fu)
                :boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type()
                ,m_work(w)
                ,m_fu(boost::make_shared<boost::future<typename Work::result_type> >(std::move(fu))){}
            // TODO type_erasure problem?
            callback_fct(callback_fct&& rhs):m_work(std::move(rhs.m_work)),m_fu(std::move(rhs.m_fu)){}
            callback_fct& operator= (callback_fct&& rhs)
            {
                m_work = std::move(rhs.m_work);
                m_fu = std::move(rhs.m_fu);
                return *this;
            }
            callback_fct(callback_fct const& rhs)
                :boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type()
                ,m_work(rhs.m_work)
                ,m_fu(rhs.m_fu){}
            callback_fct& operator= (callback_fct const& rhs)
            {
                m_work = rhs.m_work;
                m_fu = rhs.m_fu;
                rhs.m_fu.reset();
                return *this;
            }
            // TODO type_erasure problem?
            //callback_fct& operator =( const callback_fct& ) = delete;
            //callback_fct ( const callback_fct& ) = delete;
            
            void operator()()const
            {
                m_work(std::move(*m_fu));
            }
            Work m_work;
            boost::shared_ptr<boost::future<typename Work::result_type> > m_fu;
        };

        void operator()()
        {
            // by convention, we decide to lock the caller scheduler only from from task callback
            // but not start if calling scheduler is gone
            {
                auto shared_scheduler = m_scheduler.lock();
                if (!shared_scheduler.is_valid())
                    // no need to do any work as there is no way to callback
                    return;
            }
            boost::promise<typename Work::result_type> work_result;
            boost::future<typename Work::result_type> work_fu = work_result.get_future();
            // TODO check not empty
            try
            {
                // call task
                OP()(m_work,work_result);
            }
            catch(boost::asynchronous::task_aborted_exception& e){work_result.set_exception(boost::copy_exception(e));}
            catch(...){work_result.set_exception(boost::current_exception());}
            try
            {
                auto shared_scheduler = m_scheduler.lock();
                if (!shared_scheduler.is_valid())
                    // no need to do any work as there is no way to callback
                    return;
                // call callback
                callback_fct cb(std::move(m_work),std::move(work_fu));
                CB()(shared_scheduler,std::move(cb),m_task_name,m_cb_prio);
            }
            catch(std::exception& ){/* TODO */}
        }
        Work m_work;
        Sched m_scheduler;
        std::string m_task_name;
        std::size_t m_cb_prio;
    };
    struct cb_helper
    {
        //TODO static?
        template <class S, class CB>
        void operator()(S const& s, CB&& cb,std::string const& name,std::size_t cb_prio)const
        {
#ifndef BOOST_NO_RVALUE_REFERENCES
            typename boost::asynchronous::job_traits<typename S::job_type>::wrapper_type w(std::forward<CB>(cb));
            w.set_name(name);
            s.post(std::move(w),cb_prio);
#else
            if (name.empty())
            {
                s.post(cb);
            }
            else
            {
                s.post_log(cb,name);
            }
#endif
        }
    };
    // version for serializable tasks
    template <class Work,class Sched,class PostSched,class OP,class CB>
    struct post_callback_helper_base<Work,Sched,PostSched,OP,CB,typename ::boost::enable_if<boost::asynchronous::detail::is_serializable<typename Work::task_type> >::type>
            : public boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type
    {
#ifndef BOOST_NO_RVALUE_REFERENCES
        post_callback_helper_base(Work&& w, Sched const& s, const std::string& task_name="",
                                  std::size_t cb_prio=0)
            : m_work(std::forward<Work>(w)),m_scheduler(s),m_task_name(task_name),m_cb_prio(cb_prio)
        {
        }
        post_callback_helper_base(post_callback_helper_base&& rhs)
            : m_work(std::move(rhs.m_work)),m_scheduler(rhs.m_scheduler)
            , m_task_name(std::move(rhs.m_task_name)),m_cb_prio(rhs.m_cb_prio) {}
        post_callback_helper_base& operator= (post_callback_helper_base&& rhs)
        {
            m_work = std::move(rhs.m_work);
            m_scheduler = rhs.m_scheduler;
            m_task_name = std::move(rhs.m_task_name);
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
            callback_fct(Work const& w,boost::future<typename Work::result_type> fu)
                :boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type()
                ,m_work(w)
                ,m_fu(boost::make_shared<boost::future<typename Work::result_type> >(std::move(fu))){}
            callback_fct(callback_fct&& rhs):m_work(std::move(rhs.m_work)),m_fu(std::move(rhs.m_fu)){}
            callback_fct& operator= (callback_fct&& rhs)
            {
                m_work = std::move(rhs.m_work);
                m_fu = std::move(rhs.m_fu);
                return *this;
            }
            callback_fct(callback_fct const& rhs)
                :boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type()
                ,m_work(rhs.m_work)
                ,m_fu(rhs.m_fu){}
            callback_fct& operator= (callback_fct const& rhs)
            {
                m_work = rhs.m_work;
                m_fu = rhs.m_fu;
                rhs.m_fu.reset();
                return *this;
            }
            void operator()()const
            {
                m_work(std::move(*m_fu));
            }
            Work m_work;
            boost::shared_ptr<boost::future<typename Work::result_type> > m_fu;
        };
        std::string get_task_name()const
        {
            return (*(m_work.m_task)).get_task_name();
        }
        template <class Archive>
        void save(Archive & ar, const unsigned int /*version*/)const
        {
            ar & *(m_work.m_task);
        }
        template <class Archive>
        void load(Archive & ar, const unsigned int /*version*/)
        {
            boost::asynchronous::tcp::client_request::message_payload payload;
            ar >> payload;
            boost::promise<typename Work::result_type> work_p;
            boost::future<typename Work::result_type> work_fu = work_p.get_future();
            if (!payload.m_has_exception)
            {
                std::istringstream archive_stream(payload.m_data);
                typedef typename PostSched::job_type jobtype;
                typename jobtype::iarchive archive(archive_stream);

                typename Work::result_type res;
                archive >> res;
                work_p.set_value(res);
            }
            else
            {
                work_p.set_exception(boost::copy_exception(payload.m_exception));
            }
            callback_ready(std::move(work_fu));
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()
        void operator()()
        {
            // by convention, we decide to lock the caller scheduler only from from task callback
            // but not start if calling scheduler is gone
            {
                auto shared_scheduler = m_scheduler.lock();
                if (!shared_scheduler.is_valid())
                    // no need to do any work as there is no way to callback
                    return;
            }
            boost::promise<typename Work::result_type> work_result;
            boost::future<typename Work::result_type> work_fu = work_result.get_future();
            // TODO check not empty
            try
            {
                // call task
                OP()(m_work,work_result);
            }
            catch(boost::asynchronous::task_aborted_exception& e){work_result.set_exception(boost::copy_exception(e));}
            catch(...){work_result.set_exception(boost::current_exception());}
            try
            {
                auto shared_scheduler = m_scheduler.lock();
                if (!shared_scheduler.is_valid())
                    // no need to do any work as there is no way to callback
                    return;
                // call callback
                callback_fct cb(std::move(m_work),std::move(work_fu));
                CB()(shared_scheduler,std::move(cb),m_task_name,m_cb_prio);
            }
            catch(std::exception& ){/* TODO */}
        }

        //void operator()(boost::future<typename Work::result_type> work_fu)
        void callback_ready(boost::future<typename Work::result_type> work_fu)
        {
            try
            {
                auto shared_scheduler = m_scheduler.lock();
                if (!shared_scheduler.is_valid())
                    // no need to do any work as there is no way to callback
                    return;
                // call callback
                callback_fct cb(std::move(m_work),std::move(work_fu));
                CB()(shared_scheduler,std::move(cb),m_task_name,m_cb_prio);
            }
            catch(std::exception& ){/* TODO */}
        }
        Work m_work;
        Sched m_scheduler;
        std::string m_task_name;
        std::size_t m_cb_prio;
    };
    template <class Ret,class Sched,class Func,class Work,class F1,class F2,class CallbackFct,class CB,class EnablePostHelper=void>
    struct post_helper_continuation
    {
        void operator()(std::string const& task_name,std::size_t cb_prio, Sched scheduler,
                        move_task_helper<typename Func::return_type,F1,F2>& work,
                        boost::shared_ptr<boost::promise<typename Func::return_type> > work_result)const
        {
            auto cont = (*work.m_task.get())();
            //cont.on_done(detail::promise_mover<typename Func::return_type>(res));
            cont.on_done([work,scheduler,work_result,task_name,cb_prio](std::tuple<boost::future<typename Func::return_type> >&& continuation_res)
                {
                    try
                    {
                        auto shared_scheduler = scheduler.lock();
                        if (!shared_scheduler.is_valid())
                            // no need to do any work as there is no way to callback
                            return;
                        // call callback
                        boost::future<typename Work::result_type> work_fu = work_result->get_future();
                        if(!std::get<0>(continuation_res).has_value())
                        {
                            boost::asynchronous::task_aborted_exception ta;
                            work_result->set_exception(boost::copy_exception(ta));
                        }
                        else
                        {
                            try
                            {
                                work_result->set_value(std::get<0>(continuation_res).get());
                            }
                            catch(std::exception& e)
                            {
                                work_result->set_exception(boost::copy_exception(e));
                            }
                        }

                        CallbackFct cb(std::move(work),std::move(work_fu));
                        CB()(shared_scheduler,std::move(cb),task_name,cb_prio);
                    }
                    catch(std::exception& ){/* TODO */}
                }
            );
            boost::asynchronous::any_continuation ac(cont);
            boost::asynchronous::get_continuations().push_front(ac);
        }
    };
    template <class Ret,class Sched,class Func,class Work,class F1,class F2,class CallbackFct,class CB>
    struct post_helper_continuation<Ret,Sched,Func,Work,F1,F2,CallbackFct,CB,typename ::boost::enable_if<boost::is_same<Ret,void> >::type>
    {
        void operator()(std::string const& task_name,std::size_t cb_prio, Sched scheduler,
                        move_task_helper<typename Func::return_type,F1,F2>& work,
                        boost::shared_ptr<boost::promise<typename Func::return_type> > work_result)const
        {
            auto cont = (*work.m_task.get())();
            //cont.on_done(detail::promise_mover<typename Func::return_type>(res));
            cont.on_done([work,scheduler,work_result,task_name,cb_prio](std::tuple<boost::future<typename Func::return_type> >&& continuation_res)
                {
                    try
                    {
                        auto shared_scheduler = scheduler.lock();
                        if (!shared_scheduler.is_valid())
                            // no need to do any work as there is no way to callback
                            return;
                        // call callback
                        boost::future<typename Work::result_type> work_fu = work_result->get_future();
                        if(!std::get<0>(continuation_res).has_value())
                        {
                            boost::asynchronous::task_aborted_exception ta;
                            work_result->set_exception(boost::copy_exception(ta));
                        }
                        else
                        {
                            try
                            {
                                work_result->set_value();
                            }
                            catch(std::exception& e)
                            {
                                work_result->set_exception(boost::copy_exception(e));
                            }
                        }

                        CallbackFct cb(std::move(work),std::move(work_fu));
                        CB()(shared_scheduler,std::move(cb),task_name,cb_prio);
                    }
                    catch(std::exception& ){/* TODO */}
                }
            );
            boost::asynchronous::any_continuation ac(cont);
            boost::asynchronous::get_continuations().push_front(ac);
        }
    };

    //TODO no copy/paste
    template <class Func,class F1, class F2,class Work,class Sched,class PostSched,class CB,class Enable=void>
    struct post_callback_helper_continuation : public boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type
    {
#ifndef BOOST_NO_RVALUE_REFERENCES
        post_callback_helper_continuation(Work&& w, Sched const& s, const std::string& task_name="",
                                  std::size_t cb_prio=0)
            : m_work(std::forward<Work>(w)),m_scheduler(s),m_task_name(task_name),m_cb_prio(cb_prio) {}
        post_callback_helper_continuation(post_callback_helper_continuation&& rhs)
            : m_work(std::move(rhs.m_work)),m_scheduler(rhs.m_scheduler)
            , m_task_name(std::move(rhs.m_task_name)),m_cb_prio(rhs.m_cb_prio) {}
        post_callback_helper_continuation& operator= (post_callback_helper_continuation&& rhs)
        {
            m_work = std::move(rhs.m_work);
            m_scheduler = rhs.m_scheduler;
            m_task_name = std::move(rhs.m_task_name);
            m_cb_prio = rhs.m_cb_prio;
            return *this;
        }
#endif
    //#else
        post_callback_helper_continuation(post_callback_helper_continuation const& rhs)
            : boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type(),
              m_work(rhs.m_work),m_scheduler(rhs.m_scheduler),m_task_name(rhs.m_task_name),m_cb_prio(rhs.m_cb_prio){}
        post_callback_helper_continuation(Work const& w, Sched const& s, const std::string& task_name="",std::size_t cb_prio=0)
            : boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type(),
              m_work(w),m_scheduler(s),m_task_name(task_name),m_cb_prio(cb_prio){}
    //#endif

        struct callback_fct : public boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type
        {
            callback_fct(Work const& w,boost::future<typename Work::result_type> fu)
                :boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type()
                ,m_work(w)
                ,m_fu(boost::make_shared<boost::future<typename Work::result_type> >(std::move(fu))){}
            callback_fct(callback_fct const& rhs)
                :boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type()
                ,m_work(rhs.m_work)
                ,m_fu(rhs.m_fu){}
            callback_fct& operator= (callback_fct const& rhs)
            {
                m_work = rhs.m_work;
                m_fu = rhs.m_fu;
                rhs.m_fu.reset();
                return *this;
            }
            void operator()()const
            {
                m_work(std::move(*m_fu));
            }
            Work m_work;
            boost::shared_ptr<boost::future<typename Work::result_type> > m_fu;
        };

        void operator()()
        {
            // by convention, we decide to lock the caller scheduler only from from task callback
            // but not start if calling scheduler is gone
            {
                auto shared_scheduler = m_scheduler.lock();
                if (!shared_scheduler.is_valid())
                    // no need to do any work as there is no way to callback
                    return;
            }
            boost::shared_ptr<boost::promise<typename Work::result_type> > work_result(new boost::promise<typename Work::result_type>);
            // TODO check not empty
            try
            {
                // call task
                post_helper_continuation<typename Work::result_type,Sched,Func,Work,F1,F2,callback_fct,CB>()(m_task_name,m_cb_prio,m_scheduler,m_work,work_result);
            }
            catch(boost::asynchronous::task_aborted_exception& e){work_result->set_exception(boost::copy_exception(e));}
            catch(...){work_result->set_exception(boost::current_exception());}
        }
        Work m_work;
        Sched m_scheduler;
        std::string m_task_name;
        std::size_t m_cb_prio;
    };
    //TODO no copy/paste
    template <class Func,class F1, class F2,class Work,class Sched,class PostSched,class CB>
    struct post_callback_helper_continuation<Func,F1,F2,Work,Sched,PostSched,CB,typename ::boost::enable_if<boost::asynchronous::detail::is_serializable<typename Work::task_type> >::type>
            : public boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type
    {
#ifndef BOOST_NO_RVALUE_REFERENCES
        post_callback_helper_continuation(Work&& w, Sched const& s, const std::string& task_name="",
                                  std::size_t cb_prio=0)
            : m_work(std::forward<Work>(w)),m_scheduler(s),m_task_name(task_name),m_cb_prio(cb_prio) {}
        post_callback_helper_continuation(post_callback_helper_continuation&& rhs)
            : m_work(std::move(rhs.m_work)),m_scheduler(rhs.m_scheduler)
            , m_task_name(std::move(rhs.m_task_name)),m_cb_prio(rhs.m_cb_prio) {}
        post_callback_helper_continuation& operator= (post_callback_helper_continuation&& rhs)
        {
            m_work = std::move(rhs.m_work);
            m_scheduler = rhs.m_scheduler;
            m_task_name = std::move(rhs.m_task_name);
            m_cb_prio = rhs.m_cb_prio;
            return *this;
        }
#endif
    //#else
        post_callback_helper_continuation(post_callback_helper_continuation const& rhs)
            : boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type(),
              m_work(rhs.m_work),m_scheduler(rhs.m_scheduler),m_task_name(rhs.m_task_name),m_cb_prio(rhs.m_cb_prio){}
        post_callback_helper_continuation(Work const& w, Sched const& s, const std::string& task_name="",std::size_t cb_prio=0)
            : boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type(),
              m_work(w),m_scheduler(s),m_task_name(task_name),m_cb_prio(cb_prio){}
    //#endif

        struct callback_fct : public boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type
        {
            callback_fct(Work const& w,boost::future<typename Work::result_type> fu)
                :boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type()
                ,m_work(w)
                ,m_fu(boost::make_shared<boost::future<typename Work::result_type> >(std::move(fu))){}
            callback_fct(callback_fct const& rhs)
                :boost::asynchronous::job_traits<typename Sched::job_type>::diagnostic_type()
                ,m_work(rhs.m_work)
                ,m_fu(rhs.m_fu){}
            callback_fct& operator= (callback_fct const& rhs)
            {
                m_work = rhs.m_work;
                m_fu = rhs.m_fu;
                rhs.m_fu.reset();
                return *this;
            }
            void operator()()const
            {
                m_work(std::move(*m_fu));
            }
            Work m_work;
            boost::shared_ptr<boost::future<typename Work::result_type> > m_fu;
        };
        std::string get_task_name()const
        {
            return (*(m_work.m_task)).get_task_name();
        }
        template <class Archive>
        void save(Archive & ar, const unsigned int /*version*/)const
        {
            ar & *(m_work.m_task);
        }
        template <class Archive>
        void load(Archive & ar, const unsigned int /*version*/)
        {
            boost::asynchronous::tcp::client_request::message_payload payload;
            ar >> payload;
            boost::promise<typename Work::result_type> work_p;
            boost::future<typename Work::result_type> work_fu = work_p.get_future();
            if (!payload.m_has_exception)
            {
                std::istringstream archive_stream(payload.m_data);
                typedef typename PostSched::job_type jobtype;
                typename jobtype::iarchive archive(archive_stream);

                typename Work::result_type res;
                archive >> res;
                work_p.set_value(res);
            }
            else
            {
                work_p.set_exception(boost::copy_exception(payload.m_exception));
            }
            callback_ready(std::move(work_fu));
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()
        void callback_ready(boost::future<typename Work::result_type> work_fu)
        {
            try
            {
                auto shared_scheduler = m_scheduler.lock();
                if (!shared_scheduler.is_valid())
                    // no need to do any work as there is no way to callback
                    return;
                // call callback
                callback_fct cb(std::move(m_work),std::move(work_fu));
                CB()(shared_scheduler,std::move(cb),m_task_name,m_cb_prio);
            }
            catch(std::exception& ){/* TODO */}
        }
        void operator()()
        {
            // by convention, we decide to lock the caller scheduler only from from task callback
            // but not start if calling scheduler is gone
            {
                auto shared_scheduler = m_scheduler.lock();
                if (!shared_scheduler.is_valid())
                    // no need to do any work as there is no way to callback
                    return;
            }
            boost::shared_ptr<boost::promise<typename Work::result_type> > work_result(new boost::promise<typename Work::result_type>);
            // TODO check not empty
            try
            {
                // call task
                post_helper_continuation<typename Work::result_type,Sched,Func,Work,F1,F2,callback_fct,CB>()(m_task_name,m_cb_prio,m_scheduler,m_work,work_result);
            }
            catch(boost::asynchronous::task_aborted_exception& e){work_result->set_exception(boost::copy_exception(e));}
            catch(...){work_result->set_exception(boost::current_exception());}
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
                   const std::string& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0)
    -> typename boost::enable_if< boost::is_same<void,decltype(func())>,void >::type
{
    // abstract away the return value of work functor
    struct post_helper
    {
        void operator()(move_task_helper<void,F1,F2>& work,boost::promise<void>& res)const
        {
            (*work.m_task.get())();
            res.set_value();
        }
    };

    // we make a copy of the callback in a shared_ptr to be sure it gets destroyed in the
    // originator thread. The shared_ptr is then "moved" to be sure the last copy is not in the worker thread
#ifndef BOOST_NO_RVALUE_REFERENCES
    boost::shared_ptr<F2> cb_ptr = boost::make_shared<F2>(std::move(cb_func));
    boost::shared_ptr<F1> func_ptr = boost::make_shared<F1>(std::move(func));
    move_task_helper<void,F1,F2> moved_work(std::move(func_ptr),std::move(cb_ptr));
    // create a task which calls passed task, then post the callback
    detail::post_callback_helper_base<move_task_helper<void,F1,F2>,S2,S1,post_helper,detail::cb_helper > task_then_cb (
                std::move(moved_work), weak_cb_scheduler,task_name,cb_prio);
    typename boost::asynchronous::job_traits<typename S1::job_type>::wrapper_type w(std::move(task_then_cb));
    w.set_name(task_name);
    scheduler.post(std::move(w),post_prio);
#else
    boost::shared_ptr<F2> cb_ptr = boost::make_shared<F2>(cb_func);
    boost::shared_ptr<F1> func_ptr = boost::make_shared<F1>(func);
    move_task_helper<void,F1,F2> moved_work(func_ptr,cb_ptr);
    // create a task which calls passed task, then post the callback
    detail::post_callback_helper_base<move_task_helper<void,F1,F2>,S2,S1,post_helper,detail::cb_helper > task_then_cb (
                moved_work, weak_cb_scheduler,task_name,cb_prio);
    if (task_name.empty())
    {
        scheduler.post(task_then_cb);
    }
    else
    {
        scheduler.post_log(task_then_cb,task_name);
    }
#endif
}

template <class F1, class S1,class F2, class S2>
#ifndef BOOST_NO_RVALUE_REFERENCES
auto post_callback(S1 const& scheduler,F1 func,S2 const& weak_cb_scheduler,F2 cb_func,
#else
auto post_callback(S1 const& scheduler,F1 const& func,S2 const& weak_cb_scheduler,F2 const& cb_func,
#endif
                   const std::string& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0)
    -> typename boost::enable_if< boost::mpl::and_<
                                     boost::mpl::not_<boost::is_same<void,decltype(func())> >,
                                     boost::mpl::not_< has_is_continuation_task<decltype(func())> >
                                  >
                                  ,void >::type
{
    // abstract away the return value of work functor
    struct post_helper
    {
        void operator()(move_task_helper<decltype(func()),F1,F2>& work,boost::promise<decltype(func())>& res)const
        {
#ifndef BOOST_NO_RVALUE_REFERENCES   
            res.set_value(std::move((*work.m_task.get())()));
#else
            res.set_value((*work.m_task.get())());
#endif
        }
    };
    // we make a copy of the callback in a shared_ptr to be sure it gets destroyed in the
    // originator thread. The shared_ptr is then "moved" to be sure the last copy is not in the worker thread

#ifndef BOOST_NO_RVALUE_REFERENCES
    boost::shared_ptr<F2> cb_ptr = boost::make_shared<F2>(std::move(cb_func));
    boost::shared_ptr<F1> func_ptr = boost::make_shared<F1>(std::move(func));
    move_task_helper<decltype(func()),F1,F2> moved_work(std::move(func_ptr),std::move(cb_ptr));
    // create a task which calls passed task, then post the callback
    detail::post_callback_helper_base<move_task_helper<decltype(func()),F1,F2>,S2,S1,post_helper,detail::cb_helper > task_then_cb (
                std::move(moved_work), weak_cb_scheduler,task_name,cb_prio);
    typename boost::asynchronous::job_traits<typename S1::job_type>::wrapper_type w(std::move(task_then_cb));
    w.set_name(task_name);
    scheduler.post(std::move(w),post_prio);
#else
    boost::shared_ptr<F2> cb_ptr = boost::make_shared<F2>(cb_func);
    boost::shared_ptr<F1> func_ptr = boost::make_shared<F1>(func);
    move_task_helper<decltype(func()),F1,F2> moved_work(func_ptr,cb_ptr);
    // create a task which calls passed task, then post the callback
    detail::post_callback_helper_base<move_task_helper<decltype(func()),F1,F2>,S2,S1,post_helper, detail::cb_helper > task_then_cb (
                moved_work, weak_cb_scheduler,task_name,cb_prio);

    if (task_name.empty())
    {
        scheduler.post(task_then_cb);
    }
    else
    {
        scheduler.post_log(task_then_cb,task_name);
    }
#endif
}

// version for continuations
template <class F1, class S1,class F2, class S2>
#ifndef BOOST_NO_RVALUE_REFERENCES
auto post_callback(S1 const& scheduler,F1 func,S2 const& weak_cb_scheduler,F2 cb_func,
#else
auto post_callback(S1 const& scheduler,F1 const& func,S2 const& weak_cb_scheduler,F2 const& cb_func,
#endif
                   const std::string& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0)
    -> typename boost::enable_if< has_is_continuation_task<decltype(func())>,void >::type
{
//    // abstract away the return value of work functor
//    struct post_helper
//    {
//        void operator()(move_task_helper<typename decltype(func())::return_type,F1,F2>& work,boost::shared_ptr<boost::promise<typename decltype(func())::return_type> > res)const
//        {
//            auto cont = (*work.m_task.get())();
//            cont.on_done(detail::promise_mover<typename decltype(func())::return_type>(res));
//            boost::asynchronous::any_continuation ac(cont);
//            boost::asynchronous::get_continuations().push_front(ac);
//        }
//    };
    // we make a copy of the callback in a shared_ptr to be sure it gets destroyed in the
    // originator thread. The shared_ptr is then "moved" to be sure the last copy is not in the worker thread

#ifndef BOOST_NO_RVALUE_REFERENCES
    boost::shared_ptr<F2> cb_ptr = boost::make_shared<F2>(std::move(cb_func));
    boost::shared_ptr<F1> func_ptr = boost::make_shared<F1>(std::move(func));
    move_task_helper<typename decltype(func())::return_type,F1,F2> moved_work(std::move(func_ptr),std::move(cb_ptr));
    // create a task which calls passed task, then post the callback
    detail::post_callback_helper_continuation<decltype(func()),F1,F2,move_task_helper<typename decltype(func())::return_type,F1,F2>,S2,S1,detail::cb_helper > task_then_cb (
                std::move(moved_work), weak_cb_scheduler,task_name,cb_prio);
    typename boost::asynchronous::job_traits<typename S1::job_type>::wrapper_type w(std::move(task_then_cb));
    w.set_name(task_name);
    scheduler.post(std::move(w),post_prio);
#else
    boost::shared_ptr<F2> cb_ptr = boost::make_shared<F2>(cb_func);
    boost::shared_ptr<F1> func_ptr = boost::make_shared<F1>(func);
    move_task_helper<typename decltype(func())::return_type,F1,F2> moved_work(func_ptr,cb_ptr);
    // create a task which calls passed task, then post the callback
    detail::post_callback_helper_continuation<decltype(func()),F1,F2,move_task_helper<typename decltype(func())::return_type,F1,F2>,S2,S1,detail::cb_helper > task_then_cb (
                moved_work, weak_cb_scheduler,task_name,cb_prio);

    if (task_name.empty())
    {
        scheduler.post(task_then_cb);
    }
    else
    {
        scheduler.post_log(task_then_cb,task_name);
    }
#endif
}

//TODO macro...
template <class F1, class S1,class F2, class S2>
#ifndef BOOST_NO_RVALUE_REFERENCES
auto interruptible_post_callback(S1 const& scheduler,F1 func,S2 const& weak_cb_scheduler,F2 cb_func,
#else
auto interruptible_post_callback(S1 const& scheduler,F1 const& func,S2 const& weak_cb_scheduler,F2 const& cb_func,
#endif
                                 const std::string& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0)
    -> typename boost::enable_if< boost::is_same<void,decltype(func())>,boost::asynchronous::any_interruptible >::type
{
    // abstract away the return value of work functor
    struct post_helper
    {
        void operator()(move_task_helper<void,F1,F2>& work,boost::promise<void>& res)const
        {
            (*work.m_task.get())();
            res.set_value();
        }
    };
    // we make a copy of the callback in a shared_ptr to be sure it gets destroyed in the
    // originator thread. The shared_ptr is then "moved" to be sure the last copy is not in the worker thread
#ifndef BOOST_NO_RVALUE_REFERENCES
    boost::shared_ptr<F2> cb_ptr = boost::make_shared<F2>(std::move(cb_func));
    boost::shared_ptr<F1> func_ptr = boost::make_shared<F1>(std::move(func));
    move_task_helper<void,F1,F2> moved_work(std::move(func_ptr),std::move(cb_ptr));
    // create a task which calls passed task, then post the callback
    detail::post_callback_helper_base<move_task_helper<void,F1,F2>,S2,S1,post_helper,detail::cb_helper > task_then_cb (
                std::move(moved_work), weak_cb_scheduler,task_name,cb_prio);
    typename boost::asynchronous::job_traits<typename S1::job_type>::wrapper_type w(std::move(task_then_cb));
    w.set_name(task_name);
    return scheduler.interruptible_post(std::move(w),post_prio);
#else
    boost::shared_ptr<F2> cb_ptr = boost::make_shared<F2>(cb_func);
    boost::shared_ptr<F1> func_ptr = boost::make_shared<F1>(func);
    move_task_helper<void,F1,F2> moved_work(func_ptr,cb_ptr);
    // create a task which calls passed task, then post the callback
    detail::post_callback_helper_base<move_task_helper<void,F1,F2>,S2,S1,post_helper,detail::cb_helper > task_then_cb (
                moved_work, weak_cb_scheduler,task_name,cb_prio);
    if (task_name.empty())
    {
        return scheduler.interruptible_post(task_then_cb);
    }
    else
    {
        return scheduler.interruptible_post_log(task_then_cb,task_name);
    }
#endif
}

template <class F1, class S1,class F2, class S2>
#ifndef BOOST_NO_RVALUE_REFERENCES
auto interruptible_post_callback(S1 const& scheduler,F1 func,S2 const& weak_cb_scheduler,F2 cb_func,
#else
auto interruptible_post_callback(S1 const& scheduler,F1 const& func,S2 const& weak_cb_scheduler,F2 const& cb_func,
#endif
                                 const std::string& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0)
        -> typename boost::enable_if< boost::mpl::and_<
                                         boost::mpl::not_<boost::is_same<void,decltype(func())> >,
                                         boost::mpl::not_< has_is_continuation_task<decltype(func())> >
                                      >
                                      ,boost::asynchronous::any_interruptible >::type
{
    // abstract away the return value of work functor
    struct post_helper
    {
        void operator()(move_task_helper<decltype(func()),F1,F2>& work,boost::promise<decltype(func())>& res)const
        {
#ifndef BOOST_NO_RVALUE_REFERENCES   
            res.set_value(std::move((*work.m_task.get())()));
#else
            res.set_value((*work.m_task.get())());
#endif
        }
    };
    // we make a copy of the callback in a shared_ptr to be sure it gets destroyed in the
    // originator thread. The shared_ptr is then "moved" to be sure the last copy is not in the worker thread

#ifndef BOOST_NO_RVALUE_REFERENCES
    boost::shared_ptr<F2> cb_ptr = boost::make_shared<F2>(std::move(cb_func));
    boost::shared_ptr<F1> func_ptr = boost::make_shared<F1>(std::move(func));
    move_task_helper<decltype(func()),F1,F2> moved_work(std::move(func_ptr),std::move(cb_ptr));
    // create a task which calls passed task, then post the callback
    detail::post_callback_helper_base<move_task_helper<decltype(func()),F1,F2>,S2,S1,post_helper,detail::cb_helper > task_then_cb (
                std::move(moved_work), weak_cb_scheduler,task_name,cb_prio);
    typename boost::asynchronous::job_traits<typename S1::job_type>::wrapper_type w(std::move(task_then_cb));
    w.set_name(task_name);
    return scheduler.interruptible_post(std::move(w),post_prio);
#else
    boost::shared_ptr<F2> cb_ptr = boost::make_shared<F2>(cb_func);
    boost::shared_ptr<F1> func_ptr = boost::make_shared<F1>(func);
    move_task_helper<decltype(func()),F1,F2> moved_work(func_ptr,cb_ptr);
    // create a task which calls passed task, then post the callback
    detail::post_callback_helper_base<move_task_helper<decltype(func()),F1,F2>,S2,S1,post_helper,detail::cb_helper > task_then_cb (
                moved_work, weak_cb_scheduler,task_name,cb_prio);

    if (task_name.empty())
    {
        return scheduler.interruptible_post(task_then_cb);
    }
    else
    {
        return scheduler.interruptible_post_log(task_then_cb,task_name);
    }
#endif
}

// version for continuations
template <class F1, class S1,class F2, class S2>
#ifndef BOOST_NO_RVALUE_REFERENCES
auto interruptible_post_callback(S1 const& scheduler,F1 func,S2 const& weak_cb_scheduler,F2 cb_func,
#else
auto interruptible_post_callback(S1 const& scheduler,F1 const& func,S2 const& weak_cb_scheduler,F2 const& cb_func,
#endif
                   const std::string& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0)
    -> typename boost::enable_if< has_is_continuation_task<decltype(func())>,boost::asynchronous::any_interruptible >::type
{
//    // abstract away the return value of work functor
//    struct post_helper
//    {
//        void operator()(move_task_helper<typename decltype(func())::return_type,F1,F2>& work,boost::shared_ptr<boost::promise<typename decltype(func())::return_type> > res)const
//        {
//            auto cont = (*work.m_task.get())();
//            cont.on_done(detail::promise_mover<typename decltype(func())::return_type>(res));
//            boost::asynchronous::any_continuation ac(cont);
//            boost::asynchronous::get_continuations().push_front(ac);
//        }
//    };
    // we make a copy of the callback in a shared_ptr to be sure it gets destroyed in the
    // originator thread. The shared_ptr is then "moved" to be sure the last copy is not in the worker thread

#ifndef BOOST_NO_RVALUE_REFERENCES
    boost::shared_ptr<F2> cb_ptr = boost::make_shared<F2>(std::move(cb_func));
    boost::shared_ptr<F1> func_ptr = boost::make_shared<F1>(std::move(func));
    move_task_helper<typename decltype(func())::return_type,F1,F2> moved_work(std::move(func_ptr),std::move(cb_ptr));
    // create a task which calls passed task, then post the callback
    detail::post_callback_helper_continuation<decltype(func()),F1,F2,move_task_helper<typename decltype(func())::return_type,F1,F2>,S2,S1,detail::cb_helper > task_then_cb (
                std::move(moved_work), weak_cb_scheduler,task_name,cb_prio);
    typename boost::asynchronous::job_traits<typename S1::job_type>::wrapper_type w(std::move(task_then_cb));
    w.set_name(task_name);
    return scheduler.interruptible_post(std::move(w),post_prio);
#else
    boost::shared_ptr<F2> cb_ptr = boost::make_shared<F2>(cb_func);
    boost::shared_ptr<F1> func_ptr = boost::make_shared<F1>(func);
    move_task_helper<typename decltype(func())::return_type,F1,F2> moved_work(func_ptr,cb_ptr);
    // create a task which calls passed task, then post the callback
    detail::post_callback_helper_continuation<decltype(func()),F1,F2,move_task_helper<typename decltype(func())::return_type,F1,F2>,S2,S1,detail::cb_helper > task_then_cb (
                moved_work, weak_cb_scheduler,task_name,cb_prio);

    if (task_name.empty())
    {
        return scheduler.interruptible_post(task_then_cb);
    }
    else
    {
        return scheduler.interruptible_post_log(task_then_cb,task_name);
    }
#endif
}

}}

#endif // BOOST_ASYNCHRON_POST_HPP
