#include "qt_servant.hpp"
#include <functional>

namespace boost { namespace asynchronous
{
namespace detail {
#ifdef BOOST_ASYNCHRONOUS_QT_WORKAROUND
connect_functor_helper::connect_functor_helper(unsigned long id, const std::function<void(QEvent*)> &f)
    : QObject(0)
    , m_id(id)
    , m_function(f)
{}
connect_functor_helper::connect_functor_helper(connect_functor_helper const& rhs)
    :QObject(0), m_id(rhs.m_id),m_function(rhs.m_function)
    {}

qt_post_helper::qt_post_helper(connect_functor_helper* c)
    : QObject(0)
    , m_connect(c)
{}
qt_post_helper::qt_post_helper(qt_post_helper const& rhs)
    : QObject(0)
    , m_connect(rhs.m_connect)
{}
qt_post_helper::~qt_post_helper()
{}
void connect_functor_helper::customEvent(QEvent* event)
{
    m_function(event);
    QObject::customEvent(event);
}
#endif

}}}
