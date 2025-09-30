#ifndef BOOST_ASYNCHRON_EXTENSIONS_ASIO_DETAIL_TSS_HPP
#define BOOST_ASYNCHRON_EXTENSIONS_ASIO_DETAIL_TSS_HPP

#include <boost/thread/tss.hpp>
#include <boost/asio.hpp>

namespace boost { namespace asynchronous
{

struct tss_io_service_wrapper
{
    tss_io_service_wrapper(boost::asio::io_context* ios):m_io_service(ios){}
    boost::asio::io_context* m_io_service;
};

template <class T=void>
boost::asio::io_context* get_io_service(boost::asio::io_context* ioservice= 0 )
{
    static boost::thread_specific_ptr<boost::asynchronous::tss_io_service_wrapper> s_io_service;
    if (ioservice != 0)
    {
        s_io_service.reset(new boost::asynchronous::tss_io_service_wrapper(ioservice));
    }
    return s_io_service.get()->m_io_service;
}

}}

#endif // BOOST_ASYNCHRON_EXTENSIONS_ASIO_DETAIL_TSS_HPP
