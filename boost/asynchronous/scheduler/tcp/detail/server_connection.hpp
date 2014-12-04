// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_SCHEDULER_TCP_SERVER_CONNECTION_HPP
#define BOOST_ASYNCHRONOUS_SCHEDULER_TCP_SERVER_CONNECTION_HPP

#include <array>
#include <functional>
#include <string>
#include <sstream>

#include <boost/asio.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/client_request.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/server_response.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/diagnostics/any_loggable_serializable.hpp>

namespace boost { namespace asynchronous { namespace tcp {
template<class SerializableType>
class server_connection: public boost::asynchronous::trackable_servant<boost::asynchronous::any_callable>
{
public:
    explicit server_connection(boost::asynchronous::any_weak_scheduler<boost::asynchronous::any_callable> scheduler,
                               boost::shared_ptr<boost::asio::ip::tcp::socket> socket)
        : boost::asynchronous::trackable_servant<boost::asynchronous::any_callable>(scheduler)
        , m_socket(socket)
    {
    }

    server_connection(server_connection const &) = delete;
    server_connection & operator=(server_connection const &) = delete;

    void start(std::function<void(boost::asynchronous::tcp::client_request)> callback)
    {
        boost::shared_ptr<std::vector<char> > inbound_header = boost::make_shared<std::vector<char> >(m_header_length);
        auto asocket= m_socket;
        boost::asio::async_read(*m_socket,boost::asio::buffer(*inbound_header),
                                this->make_safe_callback(std::function<void(boost::system::error_code ec, size_t )>(
            [this, callback,inbound_header,asocket](boost::system::error_code ec, size_t /*bytes_transferred*/)mutable
            {
                if (!ec)
                {
                    // reading header
                    std::istringstream is(std::string(&(*inbound_header)[0], this->m_header_length));
                    std::size_t inbound_data_size = 0;
                    if (!(is >> std::hex >> inbound_data_size))
                    {
                        // Header doesn't seem to be valid. Close connection.
                        // wrong header
                        callback(boost::asynchronous::tcp::client_request(BOOST_ASYNCHRONOUS_TCP_CLIENT_COM_ERROR));
                    }
                    // read message
                    boost::shared_ptr<std::vector<char> > inbound_buffer = boost::make_shared<std::vector<char> >();
                    inbound_buffer->resize(inbound_data_size);
                    auto asocket= m_socket;
                    boost::asio::async_read(*m_socket,boost::asio::buffer(*inbound_buffer),
                                            this->make_safe_callback(std::function<void(boost::system::error_code ec, size_t )>(
                                            [this, callback,inbound_buffer,inbound_header,asocket]
                                            (boost::system::error_code ec1, size_t /*bytes_transferred*/)mutable
                                            {
                                                if (ec1)
                                                {
                                                    callback(boost::asynchronous::tcp::client_request(BOOST_ASYNCHRONOUS_TCP_CLIENT_COM_ERROR));
                                                }
                                                else
                                                {
                                                    try
                                                    {
                                                        std::string archive_data(&(*inbound_buffer)[0], inbound_buffer->size());
                                                        std::istringstream archive_stream(archive_data);
                                                        typename SerializableType::iarchive archive(archive_stream);
                                                        boost::asynchronous::tcp::client_request msg;
                                                        archive >> msg;
                                                        callback(msg);
                                                        this->start(callback);
                                                    }
                                                    catch (std::exception& e)
                                                    {
                                                        // Unable to decode data.
                                                        callback(boost::asynchronous::tcp::client_request(BOOST_ASYNCHRONOUS_TCP_CLIENT_COM_ERROR));
                                                    }
                                                }
                                            }),"",0));
                }
                else
                {
                    callback(boost::asynchronous::tcp::client_request(BOOST_ASYNCHRONOUS_TCP_CLIENT_COM_ERROR));
                }
            }),"",0));
    }

    // callback will be called only if failed, otherwise wait for result
    void send(boost::asynchronous::tcp::server_reponse const & reply,
              std::function<void(boost::asynchronous::tcp::server_reponse)> cb_if_failed)
    {
        // Serialize the data first so we know how large it is.
        std::ostringstream archive_stream;
        typename SerializableType::oarchive archive(archive_stream);
        archive << reply;
        boost::shared_ptr<std::string> outbound_buffer = boost::make_shared<std::string>(archive_stream.str());

        // Format the header.
        std::ostringstream header_stream;
        header_stream << std::setw(m_header_length)
          << std::hex << outbound_buffer->size();
        if (!header_stream || header_stream.str().size() != m_header_length)
        {
            // Something went wrong
            cb_if_failed(reply);
        }
        boost::shared_ptr<std::string> outbound_header = boost::make_shared<std::string>(header_stream.str());
        // Write the serialized data to the socket. We use "gather-write" to send
        // both the header and the data in a single write operation.
        std::vector<boost::asio::const_buffer> buffers;
        buffers.push_back(boost::asio::buffer(*outbound_header));
        buffers.push_back(boost::asio::buffer(*outbound_buffer));
        boost::asynchronous::tcp::server_reponse error_response(reply.m_task_id,"","");
        auto asocket= m_socket;
        boost::asio::async_write(*m_socket, buffers,
                                 this->make_safe_callback(std::function<void(boost::system::error_code ec, size_t )>(
                                 [this,outbound_header,outbound_buffer,cb_if_failed,error_response,asocket](boost::system::error_code ec, size_t /*bytes_sent*/)
                                 {
                                    if (ec)
                                    {
                                        cb_if_failed(error_response);
                                    }
                                 }),"",0));
    }

private:
    boost::shared_ptr<boost::asio::ip::tcp::socket> m_socket;
    std::string m_address;
    // The size of a fixed length header.
    // TODO not fixed
    enum { m_header_length = 10 };
};

template <class Job>
struct server_connection_proxy : public boost::asynchronous::servant_proxy<server_connection_proxy<Job>,
                                                                           boost::asynchronous::tcp::server_connection<Job>>
{
public:
    template<class Scheduler>
    server_connection_proxy( Scheduler scheduler,
                            boost::shared_ptr<boost::asio::ip::tcp::socket> socket)
        : boost::asynchronous::servant_proxy<server_connection_proxy,
                                             boost::asynchronous::tcp::server_connection<Job>>
          (scheduler,std::move(socket))
    {}
    typedef typename boost::asynchronous::servant_proxy<server_connection_proxy<Job>,
                                             boost::asynchronous::tcp::server_connection<Job>>::servant_type servant_type;
    typedef typename boost::asynchronous::servant_proxy<server_connection_proxy<Job>,
                                             boost::asynchronous::tcp::server_connection<Job>>::callable_type callable_type;
    BOOST_ASYNC_POST_MEMBER(start)
    BOOST_ASYNC_POST_MEMBER(send)
};

}}}

#endif // BOOST_ASYNCHRONOUS_SCHEDULER_TCP_SERVER_CONNECTION_HPP
