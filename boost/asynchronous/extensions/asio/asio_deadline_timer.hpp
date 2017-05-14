#ifndef BOOST_ASYNCHRON_EXTENSIONS_ASIO_DEADLINE_TIMER_HPP
#define BOOST_ASYNCHRON_EXTENSIONS_ASIO_DEADLINE_TIMER_HPP

#include <cstddef>
#include <boost/asio.hpp>
#include <functional>
#include <memory>

#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
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
    asio_deadline_timer(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler)
        , m_timer(new boost::asio::deadline_timer(*boost::asynchronous::get_io_service<>()))
    {
    }
    void unsafe_async_wait(std::function<void(const::boost::system::error_code&)> fct)
    {
        m_timer->async_wait(fct);
    }
    template <class Duration>
    void unsafe_async_wait(std::function<void(const::boost::system::error_code&)> fct, Duration timer_duration)
    {
        reset(timer_duration);
        m_timer->async_wait(fct);
    }
    template <typename... T>
    void cancel(T&&...  args)
    {
        m_timer->cancel(std::forward<T>(args)...);
    }
    template <typename... T>
    void cancel_one(T&&...  args)
    {
        m_timer->cancel_one(std::forward<T>(args)...);
    }
    template <typename... T>
    void expires_from_now(T&&...  args)
    {
        m_timer->expires_from_now(std::forward<T>(args)...);
    }
    template <typename Duration>
    void reset(Duration duration)
    {
        m_timer.reset(new boost::asio::deadline_timer(*boost::asynchronous::get_io_service<>(),duration));
    }
private:
   std::shared_ptr<boost::asio::deadline_timer> m_timer; 
};

class asio_deadline_timer_proxy : public boost::asynchronous::servant_proxy< boost::asynchronous::asio_deadline_timer_proxy, boost::asynchronous::asio_deadline_timer >
{
public:
    template <class Scheduler, class Duration>
    asio_deadline_timer_proxy(Scheduler s, Duration timer_duration):
        boost::asynchronous::servant_proxy< boost::asynchronous::asio_deadline_timer_proxy, boost::asynchronous::asio_deadline_timer >(s, timer_duration)
    {}
    template <class Scheduler>
    asio_deadline_timer_proxy(Scheduler s):
        boost::asynchronous::servant_proxy< boost::asynchronous::asio_deadline_timer_proxy, boost::asynchronous::asio_deadline_timer >(s)
    {}
    BOOST_ASYNC_UNSAFE_MEMBER(unsafe_async_wait)
    BOOST_ASYNC_POST_MEMBER_LOG(cancel,"cancel",1)
    BOOST_ASYNC_POST_MEMBER_LOG(cancel_one,"cancel_one",1)
    BOOST_ASYNC_POST_MEMBER_LOG(expires_from_now,"expires_from_now",1)
    BOOST_ASYNC_POST_MEMBER_LOG(reset,"reset",1)
};

}}

#endif // BOOST_ASYNCHRON_EXTENSIONS_ASIO_DEADLINE_TIMER_HPP
