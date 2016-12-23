#ifndef QT_POST_HELPER_HPP
#define QT_POST_HELPER_HPP

#include <QObject>
#include <QCoreApplication>

#include <boost/asynchronous/detail/metafunctions.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/scheduler/tss_scheduler.hpp>
#include <boost/asynchronous/detail/function_traits.hpp>
#include <boost/asynchronous/detail/move_bind.hpp>

namespace boost { namespace asynchronous
{
namespace detail {

class connect_functor_helper : public QObject
{
    Q_OBJECT
public:
    virtual ~connect_functor_helper(){}
    connect_functor_helper(unsigned long id, const boost::function<void(QEvent*)> &f)
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
    boost::function<void(QEvent*)> m_function;
};

// for use by make_safe_callback
class qt_async_safe_callback_custom_event : public QEvent
{
public:
    qt_async_safe_callback_custom_event(std::function<void()>&& cb )
        : QEvent(static_cast<QEvent::Type>(QEvent::registerEventType()))
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
    qt_safe_callback_helper(connect_functor_helper* c, std::function<void()>&& cb)
#ifdef BOOST_ASYNCHRONOUS_QT_WORKAROUND
    ;
#else
    : QObject(0)
    , m_connect(c)
    , m_cb(std::forward<std::function<void()>>(cb))
    {}
#endif
    qt_safe_callback_helper(qt_safe_callback_helper const& rhs)
#ifdef BOOST_ASYNCHRONOUS_QT_WORKAROUND
;
#else
    : QObject(0)
    , m_connect(rhs.m_connect)
    , m_cb(rhs.m_cb)
    {}
#endif
    qt_safe_callback_helper(qt_safe_callback_helper&& rhs)
#ifdef BOOST_ASYNCHRONOUS_QT_WORKAROUND
;
#else
    : QObject(0)
    , m_connect(rhs.m_connect)
    , m_cb(std::move(rhs.m_cb))
    {}
#endif
    qt_safe_callback_helper& operator=(qt_safe_callback_helper&&)=default;
    qt_safe_callback_helper& operator=(qt_safe_callback_helper const&)=default;
    void operator()()
    {
        QCoreApplication::postEvent(m_connect,new qt_async_safe_callback_custom_event(std::move(m_cb)));
    }

private:
   connect_functor_helper*              m_connect;
   std::function<void()>                m_cb;
};

template <class T>
class qt_async_custom_event : public QEvent
{
public:
    qt_async_custom_event( T f)
        : QEvent(static_cast<QEvent::Type>(QEvent::registerEventType()))
        , m_future(std::move(f))
    {}

    virtual ~qt_async_custom_event()
    {}
    T m_future;
};

class qt_post_helper : public QObject
{
    Q_OBJECT
public:
    typedef boost::asynchronous::any_callable job_type;

#ifdef BOOST_ASYNCHRONOUS_QT_WORKAROUND
    virtual ~qt_post_helper();
    qt_post_helper(connect_functor_helper* c);
    qt_post_helper(qt_post_helper const& rhs);
#else
    virtual ~qt_post_helper(){}
    qt_post_helper(connect_functor_helper* c)
        : QObject(0)
        , m_connect(c)
    {}
    qt_post_helper(qt_post_helper const& rhs)
        : QObject(0)
        , m_connect(rhs.m_connect)
    {}
#endif

    template <class Future>
    void operator()(Future f)
    {
        QCoreApplication::postEvent(m_connect,new qt_async_custom_event<Future>(std::move(f)));
    }

private:
   connect_functor_helper*              m_connect;
};
}}}

#endif // QT_POST_HELPER_HPP

