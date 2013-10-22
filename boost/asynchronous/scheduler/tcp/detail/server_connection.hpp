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

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/asio.hpp>

#include <boost/asynchronous/scheduler/tcp/detail/client_request.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/server_response.hpp>

namespace boost { namespace asynchronous { namespace tcp {

class server_connection
{
public:
    explicit server_connection(boost::asio::ip::tcp::socket socket)
    : m_socket(move(socket))
    {
    }

    server_connection(server_connection const &) = delete;
    server_connection & operator=(server_connection const &) = delete;

    void start(std::function<void(boost::asynchronous::tcp::client_request)> callback)
    {
        boost::asio::async_read(m_socket,boost::asio::buffer(m_inbound_header_),
            [this, callback](boost::system::error_code ec, size_t /*bytes_transferred*/)mutable
            {
                if (!ec)
                {
                    // reading header
                    std::istringstream is(std::string(this->m_inbound_header_, this->m_header_length));
                    std::size_t inbound_data_size = 0;
                    if (!(is >> std::hex >> inbound_data_size))
                    {
                        // Header doesn't seem to be valid. Close connection.
                        // wrong header
                        this->stop();
                    }
                    // read message
                    this->m_inbound_buffer.resize(inbound_data_size);
                    boost::asio::async_read(this->m_socket,boost::asio::buffer(this->m_inbound_buffer),
                                            [this, callback](boost::system::error_code ec, size_t /*bytes_transferred*/)mutable
                                            {
                                                if (ec)
                                                {
                                                    this->stop();
                                                }
                                                else
                                                {
                                                    try
                                                    {
                                                        std::string archive_data(&this->m_inbound_buffer[0], this->m_inbound_buffer.size());
                                                        std::istringstream archive_stream(archive_data);
                                                        boost::archive::text_iarchive archive(archive_stream);
                                                        boost::asynchronous::tcp::client_request msg;
                                                        archive >> msg;
                                                        callback(msg);
                                                        this->start(callback);
                                                    }
                                                    catch (std::exception& e)
                                                    {
                                                        // Unable to decode data.
                                                        this->stop();
                                                    }
                                                }
                                            });
                }
                else
                {
                    this->stop();
                }
            });
    }

    void stop()
    {
        m_socket.close();
    }

    void send(boost::asynchronous::tcp::server_reponse const & reply)
    {
        // Serialize the data first so we know how large it is.
        std::ostringstream archive_stream;
        boost::archive::text_oarchive archive(archive_stream);
        archive << reply;
        m_outbound_buffer = archive_stream.str();

        // Format the header.
        std::ostringstream header_stream;
        header_stream << std::setw(m_header_length)
          << std::hex << m_outbound_buffer.size();
        if (!header_stream || header_stream.str().size() != m_header_length)
        {
            // Something went wrong
            stop();
        }
        m_outbound_header = header_stream.str();
        // Write the serialized data to the socket. We use "gather-write" to send
        // both the header and the data in a single write operation.
        std::vector<boost::asio::const_buffer> buffers;
        buffers.push_back(boost::asio::buffer(m_outbound_header));
        buffers.push_back(boost::asio::buffer(m_outbound_buffer));
        boost::asio::async_write(m_socket, buffers,
                                 [this](boost::system::error_code ec, size_t /*bytes_sent*/)
                                 {
                                    if (ec)
                                    {
                                        this->stop();
                                    }
                                 });
    }

private:
    boost::asio::ip::tcp::socket m_socket;

    std::string m_outbound_buffer;
    // Holds the inbound data.
    std::vector<char> m_inbound_buffer;

    std::string m_address;
    // The size of a fixed length header.
    // TODO not fixed
    enum { m_header_length = 10 };
    // Holds an inbound header.
    char m_inbound_header_[m_header_length];
    // Holds an outbound header.
    std::string m_outbound_header;
};

}}}

#endif // BOOST_ASYNCHRONOUS_SCHEDULER_TCP_SERVER_CONNECTION_HPP
