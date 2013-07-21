#ifndef BOOST_ASYNCHRON_EXTENSIONS_ASIO_DEADLINE_TIMER_HPP
#define BOOST_ASYNCHRON_EXTENSIONS_ASIO_DEADLINE_TIMER_HPP

#include <cstddef>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/servant_proxy.h>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/extensions/asio/tss_asio.hpp>

namespace boost { namespace asynchronous
{
class asio_deadline_timer : boost::asynchronous::trackable_servant<>
{
public:
    typedef int requires_weak_scheduler;

    template <class Duration>
    asio_deadline_timer(boost::asynchronous::any_weak_scheduler<> scheduler, Duration timer_duration)
        : boost::asynchronous::trackable_servant<>(scheduler)
        , m_timer(new boost::asio::deadline_timer(*boost::asynchronous::get_io_service<>(),timer_duration))
    {        
    }
    void unsafe_async_wait(boost::function<void(const::boost::system::error_code&)> fct)
    {
        m_timer->async_wait(fct);
    }

private:
   boost::shared_ptr<boost::asio::deadline_timer> m_timer; 
};

class asio_deadline_timer_proxy : public boost::asynchronous::servant_proxy< boost::asynchronous::asio_deadline_timer_proxy, boost::asynchronous::asio_deadline_timer >
{
public:
    template <class Scheduler, class Duration>
    asio_deadline_timer_proxy(Scheduler s, Duration timer_duration):
        boost::asynchronous::servant_proxy< boost::asynchronous::asio_deadline_timer_proxy, boost::asynchronous::asio_deadline_timer >(s, timer_duration)
    {}
    BOOST_ASYNC_UNSAFE_MEMBER(unsafe_async_wait)
};

}}

#endif // BOOST_ASYNCHRON_EXTENSIONS_ASIO_DEADLINE_TIMER_HPP
