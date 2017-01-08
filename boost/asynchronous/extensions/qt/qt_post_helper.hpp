#ifndef QT_POST_HELPER_HPP
#define QT_POST_HELPER_HPP

#include <QObject>
#include <QCoreApplication>

#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/extensions/qt/connect_functor_helper.hpp>

namespace boost { namespace asynchronous
{
namespace detail {

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

