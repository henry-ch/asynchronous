#ifndef BOOST_ASYNCHRON_EXTENSIONS_ASIO_TCP_RESOLVER_HPP
#define BOOST_ASYNCHRON_EXTENSIONS_ASIO_TCP_RESOLVER_HPP

#include <cstddef>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/extensions/asio/tss_asio.hpp>

namespace boost { namespace asynchronous
{
class asio_tcp_resolver : boost::asynchronous::trackable_servant<>
{
public:
    typedef int requires_weak_scheduler;
    asio_tcp_resolver(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler)
        , m_resolver(*boost::asynchronous::get_io_service<>())
    {        
    }

    void unsafe_async_resolve(boost::asio::ip::tcp::resolver::query q, 
                              boost::function<void(const::boost::system::error_code&,boost::asio::ip::tcp::resolver::iterator )> fct)
    {
        m_resolver.async_resolve(q,fct);
    }

private:
   boost::asio::ip::tcp::resolver m_resolver; 
};

class asio_tcp_resolver_proxy : public boost::asynchronous::servant_proxy< boost::asynchronous::asio_tcp_resolver_proxy, boost::asynchronous::asio_tcp_resolver >
{
public:
    template <class Scheduler>
    asio_tcp_resolver_proxy(Scheduler s):
        boost::asynchronous::servant_proxy< boost::asynchronous::asio_tcp_resolver_proxy, boost::asynchronous::asio_tcp_resolver >(s)
    {}
    BOOST_ASYNC_UNSAFE_MEMBER(unsafe_async_resolve)
};

}}

#endif // BOOST_ASYNCHRON_EXTENSIONS_ASIO_TCP_RESOLVER_HPP
