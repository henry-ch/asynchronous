#ifndef BOOST_ASYNCHRON_ASIO_TCP_SERVANT_HPP
#define BOOST_ASYNCHRON_ASIO_TCP_SERVANT_HPP

#include <boost/asio.hpp>
#ifndef BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE
#endif
#include <boost/thread/future.hpp>

namespace boost { namespace asynchronous
{
template <class Derived>
class asio_tcp_servant
{
public:
    // helper to make it easier using a tcp asio implementation
    template <class Resolver, class F>
    void async_resolve(Resolver& resolver, boost::asio::ip::tcp::resolver::query q, F&& func)
    {
        std::function<void(const ::boost::system::error_code&,boost::asio::ip::tcp::resolver::iterator)> f = std::forward<F>(func);
        (static_cast<Derived*>(this))->call_callback(resolver.get_proxy(),
                            resolver.unsafe_async_resolve(q,(static_cast<Derived*>(this))->make_safe_callback(f)),
                            // ignore async_resolve callback functor, real callback is above
                            [](boost::asynchronous::expected<void> ){}
                           );
    } 
};
}}

#endif // BOOST_ASYNCHRON_ASIO_TCP_SERVANT_HPP
