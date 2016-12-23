#ifndef QT_SAFE_CALLBACK_HELPER_HPP
#define QT_SAFE_CALLBACK_HELPER_HPP

#include <QObject>
#include <QCoreApplication>
#include <functional>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/extensions/qt/connect_functor_helper.hpp>

namespace boost { namespace asynchronous
{
namespace detail {

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

}}}

#endif // QT_SAFE_CALLBACK_HELPER_HPP

