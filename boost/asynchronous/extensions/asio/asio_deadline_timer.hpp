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

    asio_deadline_timer(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler)
        , m_timer(new boost::asio::steady_timer(*boost::asynchronous::get_io_service<>()))
    {
    }
    template <class Duration>
    asio_deadline_timer(boost::asynchronous::any_weak_scheduler<> scheduler, Duration timer_duration)
        : boost::asynchronous::trackable_servant<>(scheduler)
        , m_timer(new boost::asio::steady_timer(*boost::asynchronous::get_io_service<>(),timer_duration))
    {        
    }
    template <class Duration>
    asio_deadline_timer(boost::asynchronous::any_weak_scheduler<> scheduler, Duration timer_duration,
                        std::function<void(const::boost::system::error_code&)> fct)
        : boost::asynchronous::trackable_servant<>(scheduler)
        , m_timer(new boost::asio::steady_timer(*boost::asynchronous::get_io_service<>(),timer_duration))
    {
        m_timer->async_wait(std::move(fct));
    }
    void async_wait(std::function<void(const::boost::system::error_code&)> fct)
    {
        m_timer->async_wait(std::move(fct));
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
    template <typename Duration>
    void reset(Duration duration)
    {
        m_timer.reset(new boost::asio::steady_timer(*boost::asynchronous::get_io_service<>(),duration));
    }
    template <class Duration>
    void reset(Duration timer_duration, std::function<void(const::boost::system::error_code&)> fct)
    {
        reset(timer_duration);
        m_timer->async_wait(std::move(fct));
    }
private:
   std::shared_ptr<boost::asio::steady_timer> m_timer; 
};

class asio_deadline_timer_proxy : public boost::asynchronous::servant_proxy< boost::asynchronous::asio_deadline_timer_proxy, boost::asynchronous::asio_deadline_timer >
{
public:
    template <class Scheduler, class Duration>
    asio_deadline_timer_proxy(Scheduler s, Duration timer_duration):
        boost::asynchronous::servant_proxy< boost::asynchronous::asio_deadline_timer_proxy, boost::asynchronous::asio_deadline_timer >(s, timer_duration)
    {}
    template <class Scheduler, class Duration>
    asio_deadline_timer_proxy(Scheduler s, Duration timer_duration, std::function<void(const::boost::system::error_code&)> fct):
        boost::asynchronous::servant_proxy< boost::asynchronous::asio_deadline_timer_proxy, boost::asynchronous::asio_deadline_timer >
        (s, timer_duration, std::move(fct))
    {}
    template <class Scheduler>
    asio_deadline_timer_proxy(Scheduler s):
        boost::asynchronous::servant_proxy< boost::asynchronous::asio_deadline_timer_proxy, boost::asynchronous::asio_deadline_timer >(s)
    {}
    BOOST_ASYNC_POST_MEMBER_LOG(async_wait,"async_wait",1)
    BOOST_ASYNC_POST_MEMBER_LOG(cancel,"cancel",1)
    BOOST_ASYNC_POST_MEMBER_LOG(cancel_one,"cancel_one",1)
    BOOST_ASYNC_POST_MEMBER_LOG(reset,"reset",1)
};

}}

#endif // BOOST_ASYNCHRON_EXTENSIONS_ASIO_DEADLINE_TIMER_HPP
