#ifndef QT_CONNECT_FUNCTOR_HELPER_HPP
#define QT_CONNECT_FUNCTOR_HELPER_HPP

#include <QObject>
#include <QCoreApplication>
#include <functional>
#include <boost/asynchronous/callable_any.hpp>

namespace boost { namespace asynchronous
{
namespace detail {

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
}}}

#endif // QT_CONNECT_FUNCTOR_HELPER_HPP

