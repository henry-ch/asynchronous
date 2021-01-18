// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_QT_SERVANT_HPP
#define BOOST_ASYNCHRONOUS_QT_SERVANT_HPP

#include <QObject>
#include <QCoreApplication>

#include <cstddef>
#include <map>
#include <memory>
#include <functional>

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/detail/function_traits.hpp>
#include <boost/asynchronous/detail/move_bind.hpp>

#include <boost/thread.hpp>

namespace boost { namespace asynchronous
{
namespace detail {
struct qt_track{};
template <class T>
class qt_async_custom_event : public QEvent
{
public:
    qt_async_custom_event( T f, int id)
        : QEvent(static_cast<QEvent::Type>(id))
        , m_future(std::move(f))
    {}

    virtual ~qt_async_custom_event()
    {}
    T m_future;
};

class connect_functor_helper : public QObject
{
    Q_OBJECT
public:
    virtual ~connect_functor_helper(){}
    connect_functor_helper(unsigned long id, const std::function<void(QEvent*)> &f)
#ifdef BOOST_ASYNCHRONOUS_QT_WORKAROUND
;
#else
        : QObject(0)
        , m_id(id)
        , m_function(f)
    {}
#endif
    connect_functor_helper(connect_functor_helper const& rhs)
#ifdef BOOST_ASYNCHRONOUS_QT_WORKAROUND
  ;
#else
  :QObject(0), m_id(rhs.m_id),m_function(rhs.m_function)
  {}
#endif
    unsigned long get_id()const
    {
        return m_id;
    }
    virtual void customEvent(QEvent* event)
#ifdef BOOST_ASYNCHRONOUS_QT_WORKAROUND
;
#else
    {
        m_function(event);
        QObject::customEvent(event);
    }
#endif

private:
    unsigned long m_id;
    std::function<void(QEvent*)> m_function;
};

class qt_post_helper : public QObject
{
    Q_OBJECT
public:
    virtual ~qt_post_helper()
#ifdef BOOST_ASYNCHRONOUS_QT_WORKAROUND
    ;
#else
    {}
#endif

    typedef boost::asynchronous::any_callable job_type;
    qt_post_helper(connect_functor_helper* c, int event_id)
#ifdef BOOST_ASYNCHRONOUS_QT_WORKAROUND
    ;
#else
    : QObject(0)
    , m_connect(c)
    , m_event_id(event_id)
    {}
#endif
    qt_post_helper(qt_post_helper const& rhs)
#ifdef BOOST_ASYNCHRONOUS_QT_WORKAROUND
;
#else
    : QObject(0)
    , m_connect(rhs.m_connect)
    , m_event_id(rhs.m_event_id)
    {}
#endif

    template <class Future>
    void operator()(Future f)
    {
        QCoreApplication::postEvent(m_connect,new qt_async_custom_event<Future>(std::move(f),m_event_id));
        QCoreApplication::sendPostedEvents();
    }

private:
   connect_functor_helper*              m_connect;
   int                                  m_event_id;
};

// for use by make_safe_callback
class qt_async_safe_callback_custom_event : public QEvent
{
public:
    qt_async_safe_callback_custom_event(std::function<void()>&& cb , int id)
        : QEvent(static_cast<QEvent::Type>(id))
        , m_cb(std::forward<std::function<void()>>(cb))
    {}

    virtual ~qt_async_safe_callback_custom_event()
    {}

    std::function<void()>                m_cb;
};
class qt_safe_callback_helper : public QObject
{
    Q_OBJECT
public:
    virtual ~qt_safe_callback_helper()
#ifdef BOOST_ASYNCHRONOUS_QT_WORKAROUND
    ;
#else
    {}
#endif

    typedef boost::asynchronous::any_callable job_type;
    qt_safe_callback_helper(connect_functor_helper* c, std::function<void()>&& cb, int event_id)
#ifdef BOOST_ASYNCHRONOUS_QT_WORKAROUND
    ;
#else
    : QObject(0)
    , m_connect(c)
    , m_cb(std::forward<std::function<void()>>(cb))
    , m_event_id(event_id)
    {}
#endif
    qt_safe_callback_helper(qt_safe_callback_helper const& rhs)
#ifdef BOOST_ASYNCHRONOUS_QT_WORKAROUND
;
#else
    : QObject(0)
    , m_connect(rhs.m_connect)
    , m_cb(rhs.m_cb)
    , m_event_id(rhs.m_event_id)
    {}
#endif
    qt_safe_callback_helper(qt_safe_callback_helper&& rhs)
#ifdef BOOST_ASYNCHRONOUS_QT_WORKAROUND
;
#else
    : QObject(0)
    , m_connect(rhs.m_connect)
    , m_cb(std::move(rhs.m_cb))
    , m_event_id(rhs.m_event_id)
    {}
#endif
    qt_safe_callback_helper& operator=(qt_safe_callback_helper&&)=default;
    qt_safe_callback_helper& operator=(qt_safe_callback_helper const&)=default;
    void operator()()
    {
        QCoreApplication::postEvent(m_connect,new qt_async_safe_callback_custom_event(std::move(m_cb),m_event_id));
        QCoreApplication::sendPostedEvents();
    }

private:
   connect_functor_helper*              m_connect;
   std::function<void()>                m_cb;
   int                                  m_event_id;
};
// a stupid scheduler which just calls the functor passed. It doesn't even post as the functor already contains a post call.
struct dummy_qt_scheduler
{
    typedef boost::asynchronous::any_callable job_type;
    void post(boost::asynchronous::any_callable&& c,std::size_t) const
    {
        c();
    }
    bool is_valid()const
    {
        return true;
    }
};
struct dymmy_weak_qt_scheduler
{
    typedef boost::asynchronous::any_callable job_type;
    dummy_qt_scheduler lock() const
    {
        return dummy_qt_scheduler();
    }
};

}
// servant for using a Qt event loop for callback dispatching.
// hides threadpool, adds automatic trackability for callbacks and tasks
// inherit from it to get functionality
template <class WJOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB>
class qt_servant
{
public:
    qt_servant(boost::asynchronous::any_shared_scheduler_proxy<WJOB> w=boost::asynchronous::any_shared_scheduler_proxy<WJOB>())
        : m_tracking(std::make_shared<boost::asynchronous::detail::qt_track>())
        , m_worker(w)
        , m_next_helper_id(0)
    {
        // set_post_callback_event_type not called
        assert(m_qt_async_custom_event_id != -1);
        // set_safe_callback_event_type
        assert(m_qt_async_safe_callback_custom_event_id != -1);
    }
    // copy-ctor and operator= are needed for correct tracking
    qt_servant(qt_servant const& rhs)
        : m_tracking(std::make_shared<boost::asynchronous::detail::qt_track>())
        , m_worker(rhs.m_worker)
        , m_next_helper_id(0)
    {
        // set_post_callback_event_type not called
        assert(m_qt_async_custom_event_id != -1);
        // set_safe_callback_event_type
        assert(m_qt_async_safe_callback_custom_event_id != -1);
    }
    ~qt_servant()
    {
    }

    qt_servant& operator= (qt_servant const& rhs)
    {
        m_tracking = std::make_shared<boost::asynchronous::detail::qt_track>();
        m_worker = rhs.m_worker;
        m_next_helper_id = rhs.m_next_helper_id;
    }

    // helper for continuations
    template <class T>
    struct get_continuation_return
    {
        typedef typename T::return_type type;
    };
    template <class F1, class F2>
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    void post_callback(F1&& func,F2&& cb_func, std::string const& task_name, std::size_t post_prio, std::size_t cb_prio)
#else
    void post_callback(F1&& func,F2&& cb_func, std::string const& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0)
#endif
    {
        typedef typename ::boost::mpl::eval_if<
            typename boost::asynchronous::detail::has_is_continuation_task<decltype(func())>::type,
            get_continuation_return<decltype(func())>,
            ::boost::mpl::identity<decltype(func())>
        >::type f1_result_type;

        unsigned long connect_id = m_next_helper_id;
        std::shared_ptr<F2> cbptr(std::make_shared<F2>(std::forward<F2>(cb_func)));

        std::shared_ptr<boost::asynchronous::detail::connect_functor_helper> c =
                std::make_shared<boost::asynchronous::detail::connect_functor_helper>
                (m_next_helper_id,
                 [this,connect_id,cbptr](QEvent* e)
                 {
                    detail::qt_async_custom_event<boost::asynchronous::expected<f1_result_type> >* ce =
                            static_cast<detail::qt_async_custom_event<boost::asynchronous::expected<f1_result_type> >* >(e);
                    (*cbptr)(std::move(ce->m_future));
                    this->m_waiting_callbacks.erase(this->m_waiting_callbacks.find(connect_id));
                 }
               );

        m_waiting_callbacks[m_next_helper_id] = c;
        ++m_next_helper_id;
        // we want to log if possible
        boost::asynchronous::post_callback(m_worker,
                                        boost::asynchronous::check_alive_before_exec(std::move(func),m_tracking),
                                        boost::asynchronous::detail::dymmy_weak_qt_scheduler(),
                                        boost::asynchronous::check_alive(
                                               boost::asynchronous::detail::qt_post_helper(c.get(),m_qt_async_custom_event_id),
                                               m_tracking),
                                        task_name,
                                        post_prio,
                                        cb_prio);
    }
    template <class F1, class F2>
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    boost::asynchronous::any_interruptible interruptible_post_callback(F1&& func,F2&& cb_func, std::string const& task_name,
                                                                    std::size_t post_prio, std::size_t cb_prio)
#else
    boost::asynchronous::any_interruptible interruptible_post_callback(F1&& func,F2&& cb_func, std::string const& task_name="",
                                                                    std::size_t post_prio=0, std::size_t cb_prio=0)
#endif
    {
        typedef decltype(func()) f1_result_type;
        unsigned long connect_id = m_next_helper_id;

        std::shared_ptr<F2> cbptr(std::make_shared<F2>(std::forward<F2>(cb_func)));
        std::shared_ptr<boost::asynchronous::detail::connect_functor_helper> c =
                std::make_shared<boost::asynchronous::detail::connect_functor_helper>
                (m_next_helper_id,
                 [this,connect_id,cbptr](QEvent* e)
                 {
                    detail::qt_async_custom_event<boost::asynchronous::expected<f1_result_type> >* ce =
                            static_cast<detail::qt_async_custom_event<boost::asynchronous::expected<f1_result_type> >* >
                            (e,m_qt_async_custom_event_id);
                    (*cbptr)(std::move(ce->m_future));
                    this->m_waiting_callbacks.erase(this->m_waiting_callbacks.find(connect_id));
                 }
                );
        m_waiting_callbacks[m_next_helper_id] = c;
        ++m_next_helper_id;
        return boost::asynchronous::interruptible_post_callback(
                                        m_worker,
                                        boost::asynchronous::check_alive_before_exec(std::move(func),m_tracking),
                                        boost::asynchronous::detail::dymmy_weak_qt_scheduler(),
                                        boost::asynchronous::check_alive(
                                               boost::asynchronous::detail::qt_post_helper(c.get(),m_qt_async_custom_event_id),
                                               m_tracking),
                                        task_name,
                                        post_prio,
                                        cb_prio);
    }


    template <class F1>
    auto post_self(F1&& func,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                   std::string const& task_name, std::size_t post_prio)
#else
                   std::string const& task_name="", std::size_t post_prio=0)
#endif
     -> std::future<typename boost::asynchronous::detail::get_return_type_if_possible_continuation<decltype(func())>::type>
    {
        typedef typename ::boost::mpl::eval_if<
            typename boost::asynchronous::detail::has_is_continuation_task<decltype(func())>::type,
            get_continuation_return<decltype(func())>,
            ::boost::mpl::identity<decltype(func())>
        >::type f1_result_type;

        unsigned long connect_id = m_next_helper_id;

        std::shared_ptr<boost::asynchronous::detail::connect_functor_helper> c =
                std::make_shared<boost::asynchronous::detail::connect_functor_helper>
                (m_next_helper_id,
                 [this,connect_id](QEvent* e)
                 {
                    detail::qt_async_safe_callback_custom_event* ce =
                            static_cast<detail::qt_async_safe_callback_custom_event* >(e);
                    ce->m_cb();
                    this->m_waiting_callbacks.erase(this->m_waiting_callbacks.find(connect_id));
                 }
               );

        m_waiting_callbacks[m_next_helper_id] = c;
        ++m_next_helper_id;
        // we want to log if possible
        return boost::asynchronous::post_future(
                                        boost::asynchronous::detail::dummy_qt_scheduler(),
                                        boost::asynchronous::check_alive(
                                            boost::asynchronous::detail::qt_safe_callback_helper
                                                (c.get(),std::forward<F1>(func),m_qt_async_safe_callback_custom_event_id),
                                            m_tracking),
                                        task_name,
                                        post_prio);
    }

    // make a callback, which posts if not the correct thread, and call directly otherwise
    // in any case, check if this object is still alive
    template<class T>
    auto make_safe_callback(T func,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                                                     const std::string& task_name, std::size_t prio)
#else
                                                     const std::string& task_name="", std::size_t prio=0)
#endif
    -> decltype(boost::asynchronous::make_function(std::move(func)))
    {
        return this->make_safe_callback_helper(boost::asynchronous::make_function(std::move(func)),task_name,prio);
    }

    // these 2 statics must be called to avoid Qt event id conflicts
    // ids are reserved by QEvent::registerEventType()
    static void set_post_callback_event_type(int id)
    {
        m_qt_async_custom_event_id = id;
    }
    static void set_safe_callback_event_type(int id)
    {
        m_qt_async_safe_callback_custom_event_id = id;
    }

    /*!
     * \brief Returns a functor checking if servant is still alive
     */
    std::function<bool()> make_check_alive_functor()const
    {
        std::weak_ptr<boost::asynchronous::detail::qt_track> tracking (m_tracking);
        return [tracking](){return !tracking.expired();};
    }
protected:
    boost::asynchronous::any_shared_scheduler_proxy<WJOB> const& get_worker()const
    {
        return m_worker;
    }
    void set_worker(boost::asynchronous::any_shared_scheduler_proxy<WJOB> w)
    {
        m_worker=w;
    }
private:
    template<typename... Args>
    std::function<void(Args... )> make_safe_callback_helper(std::function<void(Args... )> func,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                                                     const std::string& task_name, std::size_t prio)
#else
                                                     const std::string& task_name="", std::size_t prio=0)
#endif
    {
        // now we are sure about our thread id
        m_own_thread_id = boost::this_thread::get_id();
        auto thread_id=m_own_thread_id;
        std::shared_ptr<std::function<void(Args... )>> func_ptr =
                std::make_shared<std::function<void(Args... )>>(std::move(func));

        unsigned long connect_id = m_next_helper_id;

        std::shared_ptr<boost::asynchronous::detail::connect_functor_helper> c =
                std::make_shared<boost::asynchronous::detail::connect_functor_helper>
                (m_next_helper_id,
                 [this,connect_id](QEvent* e)mutable
                 {
                    detail::qt_async_safe_callback_custom_event* ce =
                            static_cast<detail::qt_async_safe_callback_custom_event* >(e);
                    ce->m_cb();
                 }
                );
        m_waiting_callbacks[m_next_helper_id] = c;
        ++m_next_helper_id;
        std::weak_ptr<boost::asynchronous::detail::qt_track> tracking (m_tracking);

        std::function<void(Args...)> res = [this,tracking,func_ptr,task_name,prio,thread_id,c,connect_id](Args... as)mutable
        {
            if (thread_id == boost::this_thread::get_id())
            {
                // our thread, call if servant alive
               boost::asynchronous::move_bind( boost::asynchronous::check_alive([func_ptr](Args... args){(*func_ptr)(std::move(args)...);},tracking),
                                               std::move(as)...)();
            }
            else
            {
                // not in our thread, post
                std::function<void()> safe_cb = boost::asynchronous::move_bind( boost::asynchronous::check_alive([func_ptr](Args... args){(*func_ptr)(std::move(args)...);},tracking),
                        std::move(as)...);
                boost::asynchronous::post_future(boost::asynchronous::detail::dummy_qt_scheduler(),
                                                 boost::asynchronous::check_alive(
                                                     boost::asynchronous::detail::qt_safe_callback_helper
                                                        (c.get(),std::move(safe_cb),m_qt_async_safe_callback_custom_event_id),
                                                 tracking),
                                                 task_name,prio);
            }
        };
        return res;
    }
    // tracking object for callbacks / tasks
    std::shared_ptr<boost::asynchronous::detail::qt_track> m_tracking;

private:
    // our worker pool
    boost::asynchronous::any_shared_scheduler_proxy<WJOB> m_worker;
    unsigned long m_next_helper_id;
    std::map<unsigned long, std::shared_ptr<boost::asynchronous::detail::connect_functor_helper> > m_waiting_callbacks;
    // we support just one id in Qt
    boost::thread::id m_own_thread_id;
    // create only one id per servant, to reduce number usage
    static int m_qt_async_custom_event_id;
    static int m_qt_async_safe_callback_custom_event_id;
};

template <class WJOB>
int qt_servant<WJOB>::m_qt_async_custom_event_id=61256;
template <class WJOB>
int qt_servant<WJOB>::m_qt_async_safe_callback_custom_event_id=61257;
}}

#endif // BOOST_ASYNCHRONOUS_QT_SERVANT_HPP
