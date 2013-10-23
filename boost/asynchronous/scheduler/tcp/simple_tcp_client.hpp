// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  This is a modified version of the example provided in the documentation of Boost.Asio.
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_SCHEDULER_TCP_SIMPLE_TCP_CLIENT_HPP
#define BOOST_ASYNCHRONOUS_SCHEDULER_TCP_SIMPLE_TCP_CLIENT_HPP

#include <string>
#include <sstream>
#include <atomic>

#include <boost/asio.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <boost/asynchronous/servant_proxy.h>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/extensions/asio/tss_asio.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/client_request.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/server_response.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/transport_exception.hpp>

namespace boost { namespace asynchronous { namespace tcp {

struct simple_tcp_client : boost::asynchronous::trackable_servant<>
{
    simple_tcp_client(boost::asynchronous::any_weak_scheduler<> scheduler,
                      const std::string& server, const std::string& path,
                      long time_in_ms_between_requests,
                      std::function<void(std::string const&,boost::archive::text_iarchive&, boost::archive::text_oarchive&)> const& executor)
        : boost::asynchronous::trackable_servant<>(scheduler)
        , m_connection_state(connection_state::none)
        , m_server(server)
        , m_path(path)
        , m_resolver(*boost::asynchronous::get_io_service<>())
        , m_socket(*boost::asynchronous::get_io_service<>())
        , m_time_in_ms_between_requests(time_in_ms_between_requests)
        , m_executor(executor)
    {}
    boost::future<void> run()
    {
        check_for_work();
        // start continuous task
        return m_done.get_future();
    }

private:
    // called in case of an error
    void stop()
    {
         m_socket.close();
         m_connection_state = connection_state::none;
    }

    void check_for_work()
    {
        //TODO string&
        std::function<void(std::string)> cb =
        [this](std::string archive_data)
        {
            // got work, deserialize message
            std::istringstream archive_stream(archive_data);
            boost::archive::text_iarchive archive(archive_stream);
            boost::asynchronous::tcp::server_reponse resp(0,"","");
            archive >> resp;
            // deserialize job, execute code, serialize result
            std::istringstream task_stream(resp.m_task);
            boost::archive::text_iarchive task_archive(task_stream);
            std::ostringstream res_archive_stream;
            boost::archive::text_oarchive res_archive(res_archive_stream);
            boost::asynchronous::tcp::client_request request (BOOST_ASYNCHRONOUS_TCP_CLIENT_JOB_RESULT);
            request.m_task_id = resp.m_task_id;
            try
            {
                m_executor(resp.m_task_name,task_archive,res_archive);
                request.m_load.m_data = res_archive_stream.str();
            }
            catch (std::exception& e)
            {
                request.m_load.m_has_exception=true;
                request.m_load.m_exception=boost::asynchronous::tcp::transport_exception(e.what());
            }
            catch(...)
            {
                request.m_load.m_has_exception=true;
                request.m_load.m_exception=boost::asynchronous::tcp::transport_exception("...");
            }

            this->send_task_result(request);
        };
        boost::shared_ptr<boost::asio::deadline_timer> atimer
                (boost::make_shared<boost::asio::deadline_timer>(*boost::asynchronous::get_io_service<>(),
                                                                 boost::posix_time::milliseconds(m_time_in_ms_between_requests)));

        std::function<void(const boost::system::error_code&)> checked =
                [this,cb,atimer](const boost::system::error_code&){this->request_content(cb);this->check_for_work();};

        atimer->async_wait(make_safe_callback(checked));
    }
    void request_content(std::function<void(std::string)> cb)
    {
        connection_state test_state=connection_state::none;
        if (m_connection_state.load() == connection_state::connecting)
        {
            // we will have to wait
            return;
        }
        else if (m_connection_state.compare_exchange_strong(test_state,connection_state::connecting))
        {
            // resolve and connect
            boost::asio::ip::tcp::resolver::query query(m_server, m_path);
                      m_resolver.async_resolve(query,
                          boost::bind(&simple_tcp_client::handle_resolve, this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::iterator,cb));
        }
        else if (m_connection_state.load() == connection_state::connected)
        {
            // we are connected and request data
            get_task(cb);
        }
    }
    void handle_resolve(const boost::system::error_code& err,
                        boost::asio::ip::tcp::tcp::resolver::iterator endpoint_iterator,
                        std::function<void(std::string)> cb)
    {
        if (!err)
        {
            // Attempt a connection to each endpoint in the list until we
            // successfully establish a connection.
            boost::asio::async_connect(m_socket, endpoint_iterator,
              boost::bind(&simple_tcp_client::handle_connect, this,
                boost::asio::placeholders::error,cb));
        }
        // else bad luck, will try later
        else
        {
            stop();
        }
    }
    void handle_connect(const boost::system::error_code& err,std::function<void(std::string)> cb)
    {
        if (!err)
        {
            m_connection_state = connection_state::connected;
            // The connection was successful. Send the request for job.
            get_task(cb);
        }
        // else bad luck, will try later
        else
        {
            stop();
        }
    }
    void get_task(std::function<void(std::string)> cb)
    {
        std::ostringstream archive_stream;
        boost::archive::text_oarchive archive(archive_stream);
        boost::asynchronous::tcp::client_request request (BOOST_ASYNCHRONOUS_TCP_CLIENT_GET_JOB);
        archive << request;
        // shared to keep it alive until end of sending
        boost::shared_ptr<std::string> outbound_buffer = boost::make_shared<std::string>(archive_stream.str());
        // Format the header.
        std::ostringstream header_stream;
        header_stream << std::setw(m_header_length)<< std::hex << outbound_buffer->size();
        if (!header_stream || header_stream.str().size() != m_header_length)
        {
          // Something went wrong
          stop();
        }
        boost::shared_ptr<std::string> outbound_header = boost::make_shared<std::string>(header_stream.str());
        // Write the serialized data to the socket. We use "gather-write" to send
        // both the header and the data in a single write operation.
        std::vector<boost::asio::const_buffer> buffers;
        buffers.push_back(boost::asio::buffer(*outbound_header));
        buffers.push_back(boost::asio::buffer(*outbound_buffer));
        boost::asio::async_write(m_socket, buffers,
                               boost::bind(&simple_tcp_client::handle_write_request, this,
                                 boost::asio::placeholders::error,cb,outbound_buffer,outbound_header) );
    }
    void send_task_result(boost::asynchronous::tcp::client_request const& request)
    {
        std::ostringstream archive_stream;
        boost::archive::text_oarchive archive(archive_stream);
        archive << request;
        boost::shared_ptr<std::string> outbound_buffer = boost::make_shared<std::string>(archive_stream.str());
        // Format the header.
        std::ostringstream header_stream;
        header_stream << std::setw(m_header_length)<< std::hex << outbound_buffer->size();
        if (!header_stream || header_stream.str().size() != m_header_length)
        {
          // Something went wrong
          stop();
        }
        boost::shared_ptr<std::string> outbound_header = boost::make_shared<std::string>(header_stream.str());
        // Write the serialized data to the socket. We use "gather-write" to send
        // both the header and the data in a single write operation.
        std::vector<boost::asio::const_buffer> buffers;
        buffers.push_back(boost::asio::buffer(*outbound_header));
        buffers.push_back(boost::asio::buffer(*outbound_buffer));
        boost::asio::async_write(m_socket, buffers,[outbound_buffer,outbound_header](const boost::system::error_code&,std::size_t) {/*ignore*/});
    }
    void handle_write_request(const boost::system::error_code& err,std::function<void(std::string)> callback,
                              boost::shared_ptr<std::string> /*ignored*/,boost::shared_ptr<std::string> /*ignored*/)
    {
        if (err)
        {
            // ok, we'll try again later
            stop();
            return;
        }
        boost::shared_ptr<std::vector<char> > inbound_header = boost::make_shared<std::vector<char> >(m_header_length);
        boost::asio::async_read(m_socket,boost::asio::buffer(*inbound_header),
            [this, callback,inbound_header](boost::system::error_code ec, size_t /*bytes_transferred*/)mutable
            {
                if (!ec)
                {
                    std::istringstream is(std::string(&(*inbound_header)[0], this->m_header_length));
                    std::size_t inbound_data_size = 0;
                    if (!(is >> std::hex >> inbound_data_size))
                    {
                        // Header doesn't seem to be valid. Inform the caller.
                        // ok, we'll try again later
                        this->stop();
                        return;
                    }
                    // read message
                    boost::shared_ptr<std::vector<char> > inbound_buffer = boost::make_shared<std::vector<char> >();
                    inbound_buffer->resize(inbound_data_size);
                    boost::asio::async_read(this->m_socket,boost::asio::buffer(*inbound_buffer),
                                            [this, callback,inbound_buffer](boost::system::error_code ec, size_t /*bytes_transferred*/) mutable
                                            {
                                                if (ec)
                                                {
                                                    // ok, we'll try again later
                                                    this->stop();
                                                }
                                                else
                                                {
                                                    std::string archive_data(&(*inbound_buffer)[0], inbound_buffer->size());
                                                    callback(archive_data);
                                                }
                                            });
                }
                else
                {
                    this->stop();
                }
            });
    }

    // TODO state machine...
    enum class connection_state
    {
        none,
        connecting,
        connected
    };
    std::atomic<connection_state> m_connection_state;
    std::string m_server;
    std::string m_path;
    boost::asio::ip::tcp::resolver m_resolver;
    boost::asio::ip::tcp::socket m_socket;
    long m_time_in_ms_between_requests;
    std::function<void(std::string const&,boost::archive::text_iarchive&, boost::archive::text_oarchive&)> m_executor;
    boost::promise<void> m_done;
    // The size of a fixed length header.
    // TODO not fixed
    enum { m_header_length = 10 };
};

// the proxy of AsioCommunicationServant for use in an external thread
class simple_tcp_client_proxy: public boost::asynchronous::servant_proxy<simple_tcp_client_proxy,boost::asynchronous::tcp::simple_tcp_client >
{
public:
    // ctor arguments are forwarded to AsioCommunicationServant
    template <class Scheduler>
    simple_tcp_client_proxy(Scheduler s,const std::string& server, const std::string& path,long time_in_ms_between_requests,
                            std::function<void(std::string const&,boost::archive::text_iarchive&, boost::archive::text_oarchive&)> const& executor):
        boost::asynchronous::servant_proxy<simple_tcp_client_proxy,boost::asynchronous::tcp::simple_tcp_client >(s,server,path,time_in_ms_between_requests,executor)
    {}
    // we offer a single member for posting
    BOOST_ASYNC_FUTURE_MEMBER(run)
};

// macro to make registration of tasks shorter
#define BOOST_ASYNCHRONOUS_DESERIALIZE_AND_CALL(task,in,out)    \
    in >> task;auto task_res = task();out << task_res;

}}}
#endif // BOOST_ASYNCHRONOUS_SCHEDULER_TCP_SIMPLE_TCP_CLIENT_HPP
