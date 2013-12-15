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
#include <boost/asynchronous/scheduler/tss_scheduler.hpp>
#include <boost/asynchronous/any_serializable.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/continuation_task.hpp>

namespace boost { namespace asynchronous { namespace tcp {

namespace detail {
static void send_task_result(boost::asynchronous::tcp::client_request const& request, boost::shared_ptr<boost::asio::ip::tcp::socket> asocket, unsigned int header_length)
{
    std::ostringstream archive_stream;
    boost::archive::text_oarchive archive(archive_stream);
    archive << request;
    boost::shared_ptr<std::string> outbound_buffer = boost::make_shared<std::string>(archive_stream.str());
    // Format the header.
    std::ostringstream header_stream;
    header_stream << std::setw(header_length)<< std::hex << outbound_buffer->size();
    if (!header_stream || header_stream.str().size() != header_length)
    {
      // Something went wrong, ignore
    }
    boost::shared_ptr<std::string> outbound_header = boost::make_shared<std::string>(header_stream.str());
    // Write the serialized data to the socket. We use "gather-write" to send
    // both the header and the data in a single write operation.
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(boost::asio::buffer(*outbound_header));
    buffers.push_back(boost::asio::buffer(*outbound_buffer));
    boost::asio::async_write(*asocket, buffers,[outbound_buffer,outbound_header](const boost::system::error_code&,std::size_t) {/*ignore*/});
}

}

// this policy checks for work after a given time
struct client_time_check_policy: boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,boost::asynchronous::any_serializable>
{
    client_time_check_policy(boost::asynchronous::any_weak_scheduler<boost::asynchronous::any_callable> scheduler,
                             boost::asynchronous::any_shared_scheduler_proxy<boost::asynchronous::any_serializable> pool,
                             long time_in_ms_between_requests)
        :boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,boost::asynchronous::any_serializable>(scheduler,pool)
        , m_time_in_ms_between_requests(time_in_ms_between_requests){}

    // every policy for tcp clients must implement this
    template <class T, class WS>
    void prepare_check_for_work(T cb,WS /*weak_scheduler*/)
    {
        // start timer to check for work again later
        boost::shared_ptr<boost::asio::deadline_timer> atimer
                (boost::make_shared<boost::asio::deadline_timer>(*boost::asynchronous::get_io_service<>(),
                                                                 boost::posix_time::milliseconds(m_time_in_ms_between_requests)));

        std::function<void(const boost::system::error_code&)> checked =
                [atimer,cb](const boost::system::error_code&){cb();};

        atimer->async_wait(make_safe_callback(checked));
    }

    long m_time_in_ms_between_requests;
};

// this policy checks for work if the queue size falls under a given length
// Note: this works only with queues supporting giving their size
struct queue_size_check_policy: boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,boost::asynchronous::any_serializable>
{
    queue_size_check_policy(boost::asynchronous::any_weak_scheduler<boost::asynchronous::any_callable> scheduler,
                             boost::asynchronous::any_shared_scheduler_proxy<boost::asynchronous::any_serializable> pool,
                             long time_in_ms_between_requests,
                             unsigned int min_queue_size)
        :boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,boost::asynchronous::any_serializable>(scheduler,pool)
        , m_time_in_ms_between_requests(time_in_ms_between_requests)
        , m_min_queue_size(min_queue_size){}

    // every policy for tcp clients must implement this
    template <class T, class WS>
    void prepare_check_for_work(T cb,WS weak_scheduler)
    {
        // start timer to check for work again later
        boost::shared_ptr<boost::asio::deadline_timer> atimer
                (boost::make_shared<boost::asio::deadline_timer>(*boost::asynchronous::get_io_service<>(),
                                                                 boost::posix_time::milliseconds(m_time_in_ms_between_requests)));

        std::function<void(const boost::system::error_code&)> checked =
                [atimer,cb,this,weak_scheduler](const boost::system::error_code& err)
        {
            std::size_t s = 0;
            {
                auto sched = weak_scheduler.lock();
                // if no valid scheduler, stop working
                if (!sched.is_valid())
                    return;
                s = sched.get_queue_size();
            }
            // if not enough jobs, try getting more
            if (!err && (s < this->m_min_queue_size))
            {
                cb();
            }
            else
            {
                // retry later
                this->prepare_check_for_work(cb,weak_scheduler);
            }
        };

        atimer->async_wait(make_safe_callback(checked));
    }

    long m_time_in_ms_between_requests;
    unsigned int m_min_queue_size;
};

template <class CheckPolicy>
struct simple_tcp_client : boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,boost::asynchronous::any_serializable>
{
    template <typename... Args>
    simple_tcp_client(boost::asynchronous::any_weak_scheduler<boost::asynchronous::any_callable> scheduler,
                      boost::asynchronous::any_shared_scheduler_proxy<boost::asynchronous::any_serializable> pool,
                      const std::string& server, const std::string& path,
                      std::function<void(std::string const&,boost::asynchronous::tcp::server_reponse,
                                         std::function<void(boost::asynchronous::tcp::client_request const&)>)> const& executor,
                      Args... args)
        : boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,boost::asynchronous::any_serializable>(scheduler,pool)
        , m_check_policy(scheduler,pool,args...)
        , m_connection_state(connection_state::none)
        , m_server(server)
        , m_path(path)
        , m_resolver(*boost::asynchronous::get_io_service<>())
        , m_socket(boost::make_shared<boost::asio::ip::tcp::socket>(*boost::asynchronous::get_io_service<>()))
        , m_executor(executor)
    {
        // no delay
        boost::asio::ip::tcp::no_delay option(true);
        boost::system::error_code ec;
        m_socket->set_option(option,ec);
    }
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
         m_socket->close();
         m_connection_state = connection_state::none;
    }

    struct stealable_job
    {
        stealable_job(boost::shared_ptr<boost::asio::ip::tcp::socket> asocket, boost::asynchronous::tcp::server_reponse&& resp,
                      std::function<void(std::string const&,boost::asynchronous::tcp::server_reponse,
                                         std::function<void(boost::asynchronous::tcp::client_request const&)>)> executor,
                      boost::asynchronous::any_weak_scheduler<> this_scheduler,unsigned int header_length)
            : m_socket(asocket)
            , m_response(std::forward<boost::asynchronous::tcp::server_reponse>(resp))
            , m_executor(executor)
            , m_scheduler(this_scheduler)
            , m_header_length(header_length)
        {
        }

        void operator()()
        {
            auto this_scheduler = m_scheduler;
            boost::shared_ptr<boost::asio::ip::tcp::socket> asocket = this->m_socket;
            unsigned int header_length = m_header_length;
            // if job was stolen, we need to deserialize to a string first
            unsigned cpt_stolen=0;
            std::size_t found = m_response.m_task_name.find_first_of("/");
            while(found != std::string::npos)
            {
                m_response.m_task_name = m_response.m_task_name.substr(found+1);
                ++cpt_stolen;
                found = m_response.m_task_name.find_first_of("/");
            }

            while(cpt_stolen > 0)
            {
                std::istringstream task_stream(m_response.m_task);
                boost::archive::text_iarchive task_archive(task_stream);
                std::string as_string;
                task_archive >> as_string;
                // replace
                m_response.m_task = as_string;
                --cpt_stolen;
            }
            m_executor(m_response.m_task_name,m_response,
                [asocket,this_scheduler,header_length](boost::asynchronous::tcp::client_request const& req)
                {
                    auto locked_scheduler = this_scheduler.lock();
                    if (locked_scheduler.is_valid())
                    {
                        locked_scheduler.post(
                                    [req,asocket,header_length](){
                                    boost::asynchronous::tcp::detail::send_task_result(req,asocket,header_length);});
                    }
                }
            );
        }
        std::string get_task_name()const
        {
            // std::string is reserved name (limited archive capability, no reset() available)
            // "/" not allowed in user names
            return std::string("std::string/" + m_response.m_task_name);
        }
        // the task is about to be stolen
        template <class Archive>
        void save(Archive & ar, const unsigned int /*version*/)const
        {
            ar & m_response.m_task;

        }
        // for non-continuation tasks, this being called means that the task was tolen
        // then excuted elsewhere. We're now getting the result of the task execution
        template <class Archive>
        void load(Archive & ar, const unsigned int /*version*/)
        {
            // copy result
            boost::asynchronous::tcp::client_request::message_payload payload;
            ar >> payload;
            // send to original server
            boost::asynchronous::tcp::client_request request (BOOST_ASYNCHRONOUS_TCP_CLIENT_JOB_RESULT);
            request.m_task_id = m_response.m_task_id;
            request.m_load = payload;

            auto locked_scheduler = m_scheduler.lock();
            boost::shared_ptr<boost::asio::ip::tcp::socket> asocket = this->m_socket;
            unsigned int header_length = m_header_length;

            if (locked_scheduler.is_valid())
            {
                locked_scheduler.post(
                            [request,asocket,header_length](){
                            boost::asynchronous::tcp::detail::send_task_result(request,asocket,header_length);});
            }
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()

        boost::shared_ptr<boost::asio::ip::tcp::socket> m_socket;
        boost::asynchronous::tcp::server_reponse m_response;
        std::function<void(std::string const&,boost::asynchronous::tcp::server_reponse,
                           std::function<void(boost::asynchronous::tcp::client_request const&)>)> m_executor;
        boost::asynchronous::any_weak_scheduler<> m_scheduler;
        unsigned int m_header_length;
    };

    void check_for_work()
    {
        //TODO string&
        std::function<void(std::string)> cb =
        [this](std::string archive_data)
        {
            // if no data, give up
            if (!archive_data.empty())
            {
                // got work, deserialize message
                std::istringstream archive_stream(archive_data);
                boost::archive::text_iarchive archive(archive_stream);
                boost::asynchronous::tcp::server_reponse resp(0,"","");
                archive >> resp;
                // if no task, give up
                if (!resp.m_task.empty())
                {
                    this->get_worker().post(
                                stealable_job(this->m_socket,std::move(resp),m_executor,get_scheduler(),m_header_length)
                    );
                }
            }
            // delegate next work checking to policy
            m_check_policy.template prepare_check_for_work([this](){this->check_for_work();},this->get_worker().get_weak_scheduler());
        };
        request_content(cb);
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
            boost::asio::async_connect(*m_socket, endpoint_iterator,
              boost::bind(&simple_tcp_client::handle_connect, this,
                boost::asio::placeholders::error,cb));
        }
        // else bad luck, will try later
        else
        {
            //ignore
            m_connection_state = connection_state::none;
            cb(std::string(""));
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
            //ignore
            m_connection_state = connection_state::none;
            cb(std::string(""));
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
          cb(std::string(""));
        }
        boost::shared_ptr<std::string> outbound_header = boost::make_shared<std::string>(header_stream.str());
        // Write the serialized data to the socket. We use "gather-write" to send
        // both the header and the data in a single write operation.
        std::vector<boost::asio::const_buffer> buffers;
        buffers.push_back(boost::asio::buffer(*outbound_header));
        buffers.push_back(boost::asio::buffer(*outbound_buffer));
        boost::asio::async_write(*m_socket, buffers,
                               boost::bind(&simple_tcp_client::handle_write_request, this,
                                 boost::asio::placeholders::error,cb,outbound_buffer,outbound_header) );
    }
    void handle_write_request(const boost::system::error_code& err,std::function<void(std::string)> callback,
                              boost::shared_ptr<std::string> /*ignored*/,boost::shared_ptr<std::string> /*ignored*/)
    {
        if (err)
        {
            // ok, we'll try again later
            stop();
            callback(std::string(""));
            return;
        }
        boost::shared_ptr<std::vector<char> > inbound_header = boost::make_shared<std::vector<char> >();
        inbound_header->resize(m_header_length);
        boost::asio::async_read(*m_socket,boost::asio::buffer(*inbound_header),
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
                        callback(std::string(""));
                        return;
                    }
                    // read message
                    boost::shared_ptr<std::vector<char> > inbound_buffer = boost::make_shared<std::vector<char> >();
                    inbound_buffer->resize(inbound_data_size);
                    boost::asio::async_read(*(this->m_socket),boost::asio::buffer(*inbound_buffer),
                                            [this, callback,inbound_buffer](boost::system::error_code ec, size_t /*bytes_transferred*/) mutable
                                            {
                                                if (ec)
                                                {
                                                    // ok, we'll try again later
                                                    this->stop();
                                                    callback(std::string(""));
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
                    callback(std::string(""));
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
    CheckPolicy m_check_policy;
    std::atomic<connection_state> m_connection_state;
    std::string m_server;
    std::string m_path;
    boost::asio::ip::tcp::resolver m_resolver;
    boost::shared_ptr<boost::asio::ip::tcp::socket> m_socket;
    std::function<void(std::string const&,boost::asynchronous::tcp::server_reponse,
                       std::function<void(boost::asynchronous::tcp::client_request const&)>)> m_executor;
    boost::promise<void> m_done;
    // The size of a fixed length header.
    // TODO not fixed
    enum { m_header_length = 10 };
};

#ifdef BOOST_ASYNCHRONOUS_NO_TEMPLATE_PROXY_CLASSES

// the proxy of AsioCommunicationServant for use in an external thread
class simple_tcp_client_proxy: public boost::asynchronous::servant_proxy<simple_tcp_client_proxy,
                               boost::asynchronous::tcp::simple_tcp_client<boost::asynchronous::tcp::client_time_check_policy > >
{
public:
    // ctor arguments are forwarded to AsioCommunicationServant
    template <class Scheduler,typename... Args>
    simple_tcp_client_proxy(Scheduler s,
                            boost::asynchronous::any_shared_scheduler_proxy<boost::asynchronous::any_serializable> pool,
                            const std::string& server, const std::string& path,
                            std::function<void(std::string const&,boost::asynchronous::tcp::server_reponse,
                                               std::function<void(boost::asynchronous::tcp::client_request const&)>)> const& executor,
                            Args... args):
        boost::asynchronous::servant_proxy<simple_tcp_client_proxy,boost::asynchronous::tcp::simple_tcp_client<boost::asynchronous::tcp::client_time_check_policy > >
            (s,pool,server,path,executor,args...)
    {}
    // we offer a single member for posting
    BOOST_ASYNC_FUTURE_MEMBER(run)
};
class simple_tcp_client_proxy_queue_size: public boost::asynchronous::servant_proxy<simple_tcp_client_proxy_queue_size,
                                          boost::asynchronous::tcp::simple_tcp_client<boost::asynchronous::tcp::queue_size_check_policy > >
{
public:
    // ctor arguments are forwarded to AsioCommunicationServant
    template <class Scheduler,typename... Args>
    simple_tcp_client_proxy_queue_size(Scheduler s,
                                       boost::asynchronous::any_shared_scheduler_proxy<boost::asynchronous::any_serializable> pool,
                                       const std::string& server, const std::string& path,
                                       std::function<void(std::string const&,boost::asynchronous::tcp::server_reponse,
                                               std::function<void(boost::asynchronous::tcp::client_request const&)>)> const& executor,
                                       Args... args):
        boost::asynchronous::servant_proxy<simple_tcp_client_proxy_queue_size,boost::asynchronous::tcp::simple_tcp_client<boost::asynchronous::tcp::queue_size_check_policy > >
            (s,pool,server,path,executor,args...)
    {}
    // we offer a single member for posting
    BOOST_ASYNC_FUTURE_MEMBER(run)
};

// choose correct proxy
template <class Job>
struct get_correct_simple_tcp_client_proxy
{
    typedef boost::asynchronous::tcp::simple_tcp_client_proxy type;
};
template <>
struct get_correct_simple_tcp_client_proxy<boost::asynchronous::tcp::queue_size_check_policy>
{
    typedef boost::asynchronous::tcp::simple_tcp_client_proxy_queue_size type;
};

#else
// the proxy of AsioCommunicationServant for use in an external thread
template <class T>
class simple_tcp_client_proxy: public boost::asynchronous::servant_proxy<simple_tcp_client_proxy<T>,
                               boost::asynchronous::tcp::simple_tcp_client<T> >
{
public:
    // ctor arguments are forwarded to AsioCommunicationServant
    template <class Scheduler,typename... Args>
    simple_tcp_client_proxy(Scheduler s,
                            boost::asynchronous::any_shared_scheduler_proxy<boost::asynchronous::any_serializable> pool,
                            const std::string& server, const std::string& path,
                            std::function<void(std::string const&,boost::asynchronous::tcp::server_reponse,
                                               std::function<void(boost::asynchronous::tcp::client_request const&)>)> const& executor,
                            Args... args):
        boost::asynchronous::servant_proxy<simple_tcp_client_proxy<T>,boost::asynchronous::tcp::simple_tcp_client<T> >
            (s,pool,server,path,executor,args...)
    {}
    // we offer a single member for posting
    BOOST_ASYNC_FUTURE_MEMBER(run)
};
// choose correct proxy (just for compatibility with limited version from above
template <class Job>
struct get_correct_simple_tcp_client_proxy
{
    typedef boost::asynchronous::tcp::simple_tcp_client_proxy<Job> type;
};
#endif

// register a task for deserialization
template <class Task>
void deserialize_and_call_task(Task& t,boost::asynchronous::tcp::server_reponse const& resp,
                               std::function<void(boost::asynchronous::tcp::client_request const&)>const& when_done)
{
    std::istringstream task_stream(resp.m_task);
    boost::archive::text_iarchive task_archive(task_stream);
    std::ostringstream res_archive_stream;
    boost::archive::text_oarchive res_archive(res_archive_stream);
    boost::asynchronous::tcp::client_request request (BOOST_ASYNCHRONOUS_TCP_CLIENT_JOB_RESULT);
    request.m_task_id = resp.m_task_id;
    try
    {
        task_archive >> t;
        auto task_res = t();
        res_archive << task_res;
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
    when_done(request);
}
                                                                  \
// register a top-levelcontinuation task for deserialization
template <class Task>
void deserialize_and_call_top_level_continuation_task(
        Task& t,boost::asynchronous::tcp::server_reponse const& resp,
        std::function<void(boost::asynchronous::tcp::client_request const&)>const& when_done)
{
    // deserialize job, execute code, serialize result
    std::istringstream task_stream(resp.m_task);
    boost::archive::text_iarchive task_archive(task_stream);
    boost::asynchronous::tcp::client_request request (BOOST_ASYNCHRONOUS_TCP_CLIENT_JOB_RESULT);
    request.m_task_id = resp.m_task_id;
    task_archive >> t;
    // call task
    auto cont = t();
    cont.on_done([request,when_done](std::tuple<boost::future<typename decltype(t())::return_type> >&& continuation_res)mutable
        {
            std::ostringstream res_archive_stream;
            boost::archive::text_oarchive res_archive(res_archive_stream);
            try
            {
                // serialize result
                auto res = (std::get<0>(continuation_res)).get();
                res_archive << res;
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
            when_done(request);
        }
    );
    boost::asynchronous::any_continuation ac(cont);
    boost::asynchronous::get_continuations().push_front(ac);
}
// register a top-levelcontinuation task for deserialization
template <class Task>
void deserialize_and_call_continuation_task(
        Task& t,boost::asynchronous::tcp::server_reponse const& resp,
        std::function<void(boost::asynchronous::tcp::client_request const&)>const& when_done)
{
    // deserialize job, execute code, serialize result
    std::istringstream task_stream(resp.m_task);
    boost::archive::text_iarchive task_archive(task_stream);
    boost::asynchronous::tcp::client_request request (BOOST_ASYNCHRONOUS_TCP_CLIENT_JOB_RESULT);
    request.m_task_id = resp.m_task_id;
    task_archive >> t;
    // call task
    t();
    // create continuation waiting for task completion
    boost::asynchronous::create_continuation
        ([request,when_done](std::tuple<boost::future<typename Task::res_type> > continuation_res)mutable
         {
            std::ostringstream res_archive_stream;
            boost::archive::text_oarchive res_archive(res_archive_stream);
            try
            {
                // serialize result
                auto res = (std::get<0>(continuation_res)).get();
                res_archive << res;
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
            when_done(request);
         }
    , t.get_future()
    );
}
}}}
#endif // BOOST_ASYNCHRONOUS_SCHEDULER_TCP_SIMPLE_TCP_CLIENT_HPP
