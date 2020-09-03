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
#include <deque>
#include <numeric>
#include <boost/asio.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
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

// this policy checks for work after a given time
template <class JobType = boost::asynchronous::any_callable >
struct client_time_check_policy: boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,JobType>
{
    client_time_check_policy(boost::asynchronous::any_weak_scheduler<boost::asynchronous::any_callable> scheduler,
                             boost::asynchronous::any_shared_scheduler_proxy<JobType> pool,
                             long time_in_ms_between_requests)
        :boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,JobType>(scheduler,pool)
        , m_time_in_ms_between_requests(time_in_ms_between_requests){}

    // every policy for tcp clients must implement this
    template <class T, class WS>
    void prepare_check_for_work(T cb,WS /*weak_scheduler*/)
    {
        // start timer to check for work again later
        std::shared_ptr<boost::asio::deadline_timer> atimer
                (std::make_shared<boost::asio::deadline_timer>(*boost::asynchronous::get_io_service<>(),
                                                                 boost::posix_time::milliseconds(m_time_in_ms_between_requests)));

        std::function<void(const boost::system::error_code&)> checked =
                [atimer,cb](const boost::system::error_code&){cb();};

        atimer->async_wait(this->make_safe_callback(checked,"",0));
    }

    long m_time_in_ms_between_requests;
};

// this policy checks for work if the queue size falls under a given length
// Note: this works only with queues supporting giving their size
template <class JobType = boost::asynchronous::any_callable >
struct queue_size_check_policy: boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,JobType>
{
    queue_size_check_policy(boost::asynchronous::any_weak_scheduler<boost::asynchronous::any_callable> scheduler,
                             boost::asynchronous::any_shared_scheduler_proxy<JobType> pool,
                             long time_in_ms_between_requests,
                             unsigned int min_queue_size)
        :boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,JobType>(scheduler,pool)
        , m_time_in_ms_between_requests(time_in_ms_between_requests)
        , m_min_queue_size(min_queue_size){}

    // every policy for tcp clients must implement this
    template <class T, class WS>
    void prepare_check_for_work(T cb,WS weak_scheduler)
    {
        // start timer to check for work again later
        std::shared_ptr<boost::asio::deadline_timer> atimer
                (std::make_shared<boost::asio::deadline_timer>(*boost::asynchronous::get_io_service<>(),
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
                auto queues = sched.get_queue_size();
                s = std::accumulate(queues.begin(),queues.end(),0,[](std::size_t rhs,std::size_t lhs){return rhs + lhs;});
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

        atimer->async_wait(this->make_safe_callback(checked,"",0));
    }

    long m_time_in_ms_between_requests;
    unsigned int m_min_queue_size;
};

template <class CheckPolicy, class SerializableType = boost::asynchronous::any_serializable, class PoolJob = boost::asynchronous::any_callable>
struct simple_tcp_client : boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,PoolJob>
{
    template <typename... Args>
    simple_tcp_client(boost::asynchronous::any_weak_scheduler<boost::asynchronous::any_callable> scheduler,
                      boost::asynchronous::any_shared_scheduler_proxy<PoolJob> pool,
                      const std::string& server, const std::string& path,
                      std::function<void(std::string const&,boost::asynchronous::tcp::server_reponse,
                                         std::function<void(boost::asynchronous::tcp::client_request const&)>)> const& executor,
                      Args... args)
        : boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,PoolJob>(scheduler,pool)
        , m_is_writing(false)
        , m_check_policy(scheduler,pool,args...)
        , m_connection_state(connection_state::none)
        , m_server(server)
        , m_path(path)
        , m_resolver(*boost::asynchronous::get_io_service<>())
        , m_socket(std::make_shared<boost::asio::ip::tcp::socket>(*boost::asynchronous::get_io_service<>()))
        , m_executor(executor)
    {
        // no delay
        boost::asio::ip::tcp::no_delay option(true);
        boost::system::error_code ec;
        m_socket->set_option(option,ec);
    }
    std::future<void> run()
    {
        check_for_work();
        // start continuous task
        return std::move(m_done.get_future());
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
        stealable_job(std::shared_ptr<std::vector<char>> archive_data,
                      std::function<void(std::string const&,boost::asynchronous::tcp::server_reponse,
                                         std::function<void(boost::asynchronous::tcp::client_request const&)>)> executor,
                      boost::asynchronous::any_weak_scheduler<> this_scheduler,
                      std::function<void(std::shared_ptr<boost::asynchronous::tcp::client_request>)> done_fct)
            : m_done(done_fct)
            , m_archive_data(std::move(archive_data))
            , m_response(0,"","")
            , m_executor(executor)
            , m_scheduler(this_scheduler)
        {
        }

        void operator()()
        {
            // got work, deserialize message
            auto start = std::chrono::high_resolution_clock::now();
            std::string archive_data(&(*m_archive_data)[0], m_archive_data->size());
            std::istringstream archive_stream(std::move(archive_data));
            typename SerializableType::iarchive archive(archive_stream);
            archive >> m_response;
            auto tmsg = (std::chrono::nanoseconds(std::chrono::high_resolution_clock::now() - start).count() / 1000000);
            if (tmsg > 50)
            {
                std::cout << "msg deserialize took in ms: "<< tmsg << std::endl;
            }
            // if no task, give up
            if (!m_response.m_task.empty())
            {
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
                    typename SerializableType::iarchive task_archive(task_stream);
                    std::string as_string;
                    task_archive >> as_string;
                    // replace
                    m_response.m_task = std::move(as_string);
                    --cpt_stolen;
                }
                auto done_fct = m_done;
                m_executor(m_response.m_task_name,m_response,
                    [done_fct](boost::asynchronous::tcp::client_request const& req)
                    {
                        std::shared_ptr<boost::asynchronous::tcp::client_request> request (std::make_shared<boost::asynchronous::tcp::client_request>(std::move(req)));
                        done_fct(request);
                    }
                );
            }
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
            std::shared_ptr<boost::asynchronous::tcp::client_request> request (std::make_shared<boost::asynchronous::tcp::client_request>(BOOST_ASYNCHRONOUS_TCP_CLIENT_JOB_RESULT));
            request->m_task_id = m_response.m_task_id;
            request->m_load = std::move(payload);

            auto done_fct = m_done;
            done_fct(request);
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()

        std::function<void(std::shared_ptr<boost::asynchronous::tcp::client_request>)> m_done;
        std::shared_ptr<std::vector<char>> m_archive_data;
        boost::asynchronous::tcp::server_reponse m_response;
        std::function<void(std::string const&,boost::asynchronous::tcp::server_reponse,
                           std::function<void(boost::asynchronous::tcp::client_request const&)>)> m_executor;
        boost::asynchronous::any_weak_scheduler<> m_scheduler;
    };

    void check_for_work()
    {
        //TODO string&
        std::function<void(std::shared_ptr<std::vector<char> >)> cb =
        [this](std::shared_ptr<std::vector<char> > archive_data)
        {
            // if no data, give up
            if (!archive_data->empty())
            {
                std::function<void(std::shared_ptr<boost::asynchronous::tcp::client_request>)> sending_fct =
                        this->make_safe_callback(std::function<void(std::shared_ptr<boost::asynchronous::tcp::client_request>)>(
                                                     [this](std::shared_ptr<boost::asynchronous::tcp::client_request> req)
                                                     {this->send_task_result(req);}),"",0);
                this->get_worker().post(
                            stealable_job(std::move(archive_data),m_executor,this->get_scheduler(),sending_fct)
                );
            }
            // delegate next work checking to policy
            m_check_policy.template prepare_check_for_work([this](){this->check_for_work();},this->get_worker().get_weak_scheduler());
        };
        request_content(cb);
    }
    void request_content(std::function<void(std::shared_ptr<std::vector<char> >)> cb)
    {
        if ((m_connection_state == connection_state::connecting)||(m_connection_state == connection_state::getting_work))
        {
            // we will have to wait
            cb(std::make_shared<std::vector<char> >());
            return;
        }
        else if (m_connection_state == connection_state::connected)
        {
            // we are connected and request data
            get_task(cb);
        }
        else
        {
            // resolve and connect
            boost::asio::ip::tcp::resolver::query query(m_server, m_path);
            m_resolver.async_resolve(
                      query,
                      this->make_safe_callback(std::function<void(const boost::system::error_code&,boost::asio::ip::tcp::tcp::resolver::iterator)>(
                                             [cb,this](const boost::system::error_code& err,boost::asio::ip::tcp::tcp::resolver::iterator endpoint_iterator)mutable
                                             {this->handle_resolve(err,endpoint_iterator,std::move(cb)); }),"",0));
        }

    }
    void handle_resolve(const boost::system::error_code& err,
                        boost::asio::ip::tcp::tcp::resolver::iterator endpoint_iterator,
                        std::function<void(std::shared_ptr<std::vector<char> >)> cb)
    {
        if (!err)
        {
            // Attempt a connection to each endpoint in the list until we
            // successfully establish a connection.
            boost::asio::async_connect(
                        *m_socket, endpoint_iterator,
                        this->make_safe_callback(std::function<void(const boost::system::error_code&,boost::asio::ip::tcp::resolver::iterator)>(
                                               [this,cb](const boost::system::error_code& err,boost::asio::ip::tcp::resolver::iterator){this->handle_connect(err,std::move(cb));}),"",0));
        }
        // else bad luck, will try later
        else
        {
            //ignore
            m_connection_state = connection_state::none;
            cb(std::make_shared<std::vector<char> >());
        }
    }
    void handle_connect(const boost::system::error_code& err,std::function<void(std::shared_ptr<std::vector<char> >)> cb)
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
            cb(std::make_shared<std::vector<char> >());
        }
    }
    void get_task(std::function<void(std::shared_ptr<std::vector<char> >)> cb)
    {
        if (m_is_writing)
        {
            // already writing give up
            cb(std::make_shared<std::vector<char> >());
            return;
        }
        m_connection_state = connection_state::getting_work;
        m_is_writing = true;
        std::ostringstream archive_stream;
        typename SerializableType::oarchive archive(archive_stream);
        boost::asynchronous::tcp::client_request request (BOOST_ASYNCHRONOUS_TCP_CLIENT_GET_JOB);
        archive << request;
        // shared to keep it alive until end of sending
        std::shared_ptr<std::string> outbound_buffer = std::make_shared<std::string>(archive_stream.str());
        // Format the header.
        std::ostringstream header_stream;
        header_stream << std::setw(m_header_length)<< std::hex << outbound_buffer->size();
        if (!header_stream || header_stream.str().size() != m_header_length)
        {
          // Something went wrong
          stop();
          cb(std::make_shared<std::vector<char> >());
        }
        std::shared_ptr<std::string> outbound_header = std::make_shared<std::string>(header_stream.str());
        // Write the serialized data to the socket. We use "gather-write" to send
        // both the header and the data in a single write operation.
        std::vector<boost::asio::const_buffer> buffers;
        buffers.push_back(boost::asio::buffer(*outbound_header));
        buffers.push_back(boost::asio::buffer(*outbound_buffer));
        boost::asio::async_write(*m_socket, buffers,
                                 this->make_safe_callback(std::function<void(const boost::system::error_code&,std::size_t)>(
                                                        [this,cb,outbound_buffer,outbound_header](const boost::system::error_code& err,std::size_t)mutable
                                                        {this->m_is_writing = false;this->handle_write_request(err,cb);}),"",0));
    }
    void handle_write_request(const boost::system::error_code& err,std::function<void(std::shared_ptr<std::vector<char> >)> callback)
    {
        if (err)
        {
            // ok, we'll try again later
            stop();
            callback(std::make_shared<std::vector<char> >());
            return;
        }
        std::shared_ptr<std::vector<char> > inbound_header = std::make_shared<std::vector<char> >();
        inbound_header->resize(m_header_length);
        boost::asio::async_read(*m_socket,boost::asio::buffer(*inbound_header),
            this->make_safe_callback(std::function<void(const boost::system::error_code&,size_t)>(
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
                        callback(std::make_shared<std::vector<char> >());
                        return;
                    }
                    // read message
                    std::shared_ptr<std::vector<char> > inbound_buffer = std::make_shared<std::vector<char> >();
                    inbound_buffer->resize(inbound_data_size);
                    boost::asio::async_read(*(this->m_socket),boost::asio::buffer(*inbound_buffer),
                                            this->make_safe_callback(std::function<void(const boost::system::error_code&,size_t)>(
                                            [this, callback,inbound_buffer](boost::system::error_code ec, size_t /*bytes_transferred*/) mutable
                                            {
                                                if (ec)
                                                {
                                                    // ok, we'll try again later
                                                    this->stop();
                                                    callback(std::make_shared<std::vector<char> >());
                                                }
                                                else
                                                {
                                                    m_connection_state = connection_state::connected;
                                                    callback(inbound_buffer);
                                                }
                                            }),"",0));
                }
                else
                {
                    this->stop();
                    callback(std::make_shared<std::vector<char> >());
                }
            }),"",0));
    }
    void send_task_result(std::shared_ptr<boost::asynchronous::tcp::client_request> request)
    {
        // if writing, we will have to wait...
        if (m_is_writing)
        {
            m_waiting_requests.push_back(request);
            return;
        }
        m_is_writing = true;
        std::ostringstream archive_stream;
        typename SerializableType::oarchive archive(archive_stream);
        archive << *request;
        std::shared_ptr<std::string> outbound_buffer = std::make_shared<std::string>(archive_stream.str());
        // Format the header.
        std::ostringstream header_stream;
        header_stream << std::setw(m_header_length)<< std::hex << outbound_buffer->size();
        if (!header_stream || header_stream.str().size() != m_header_length)
        {
          // Something went wrong, ignore
        }
        std::shared_ptr<std::string> outbound_header = std::make_shared<std::string>(header_stream.str());
        // Write the serialized data to the socket. We use "gather-write" to send
        // both the header and the data in a single write operation.
        std::vector<boost::asio::const_buffer> buffers;
        buffers.push_back(boost::asio::buffer(*outbound_header));
        buffers.push_back(boost::asio::buffer(*outbound_buffer));
        boost::asio::async_write(*m_socket, buffers,
                                 this->make_safe_callback(std::function<void(const boost::system::error_code&,std::size_t)>(
                                 [this,outbound_buffer,outbound_header](const boost::system::error_code&,std::size_t )
                                 {this->m_is_writing=false; this->try_send_one_waiting_request();}),"",0));
    }
    void try_send_one_waiting_request()
    {
        if (!m_is_writing && !m_waiting_requests.empty())
        {
            send_task_result(m_waiting_requests.front());
            m_waiting_requests.pop_front();
        }
    }

    // TODO state machine...
    enum class connection_state
    {
        none,
        connecting,
        connected,
        getting_work
    };
    // we need to write once at a time and queue requests
    bool m_is_writing;
    std::deque<std::shared_ptr<boost::asynchronous::tcp::client_request>> m_waiting_requests;
    CheckPolicy m_check_policy;
    connection_state m_connection_state;
    std::string m_server;
    std::string m_path;
    boost::asio::ip::tcp::resolver m_resolver;
    std::shared_ptr<boost::asio::ip::tcp::socket> m_socket;
    std::function<void(std::string const&,boost::asynchronous::tcp::server_reponse,
                       std::function<void(boost::asynchronous::tcp::client_request const&)>)> m_executor;
    std::promise<void> m_done;
    // The size of a fixed length header.
    // TODO not fixed
    enum { m_header_length = 10 };
};


// the proxy of AsioCommunicationServant for use in an external thread
template <class T = boost::asynchronous::tcp::client_time_check_policy<boost::asynchronous::any_callable> ,
          class SerializableType = boost::asynchronous::any_serializable,
          class PoolJob = boost::asynchronous::any_callable>
class simple_tcp_client_proxy_ext: public boost::asynchronous::servant_proxy<simple_tcp_client_proxy_ext<T,SerializableType,PoolJob>,
                                                                         boost::asynchronous::tcp::simple_tcp_client<T,SerializableType,PoolJob> >
{
public:
    // ctor arguments are forwarded to AsioCommunicationServant
    template <class Scheduler,typename... Args>
    simple_tcp_client_proxy_ext(Scheduler s,
                            boost::asynchronous::any_shared_scheduler_proxy<PoolJob> pool,
                            const std::string& server, const std::string& path,
                            std::function<void(std::string const&,boost::asynchronous::tcp::server_reponse,
                                               std::function<void(boost::asynchronous::tcp::client_request const&)>)> const& executor,
                            Args... args):
        boost::asynchronous::servant_proxy<simple_tcp_client_proxy_ext<T,SerializableType,PoolJob>,boost::asynchronous::tcp::simple_tcp_client<T,SerializableType,PoolJob> >
            (s,pool,server,path,executor,args...)
    {}
    typedef typename boost::asynchronous::servant_proxy<
                            simple_tcp_client_proxy_ext<T,SerializableType,PoolJob>,
                            boost::asynchronous::tcp::simple_tcp_client<T,SerializableType,PoolJob> >::servant_type servant_type;
    typedef typename boost::asynchronous::servant_proxy<
                            simple_tcp_client_proxy_ext<T,SerializableType,PoolJob>,
                            boost::asynchronous::tcp::simple_tcp_client<T,SerializableType,PoolJob> >::callable_type callable_type;

    // we offer a single member for posting
    BOOST_ASYNC_FUTURE_MEMBER(run)
};

// register a task for deserialization
template <class Task, class SerializableType = boost::asynchronous::any_serializable>
void deserialize_and_call_task(Task& t,boost::asynchronous::tcp::server_reponse const& resp,
                               std::function<void(boost::asynchronous::tcp::client_request const&)>const& when_done)
{
    std::istringstream task_stream(resp.m_task);
    typename SerializableType::iarchive task_archive(task_stream);
    std::ostringstream res_archive_stream;
    typename SerializableType::oarchive res_archive(res_archive_stream);
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
// register a top-level continuation task for deserialization
template <class Task, class SerializableType = boost::asynchronous::any_serializable>
void deserialize_and_call_top_level_continuation_task(
        Task& t,boost::asynchronous::tcp::server_reponse const& resp,
        std::function<void(boost::asynchronous::tcp::client_request const&)>const& when_done)
{
    // deserialize job, execute code, serialize result
    std::istringstream task_stream(resp.m_task);
    typename SerializableType::iarchive task_archive(task_stream);
    boost::asynchronous::tcp::client_request request (BOOST_ASYNCHRONOUS_TCP_CLIENT_JOB_RESULT);
    request.m_task_id = resp.m_task_id;
    task_archive >> t;
    // call task
    auto cont = t();
    cont.on_done([request,when_done](std::tuple<std::future<typename decltype(t())::return_type> >&& continuation_res)mutable
        {
            std::ostringstream res_archive_stream;
            typename SerializableType::oarchive res_archive(res_archive_stream);
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
// register a continuation task for deserialization
template <class Task, class SerializableType = boost::asynchronous::any_serializable>
void deserialize_and_call_continuation_task(
        Task& t,boost::asynchronous::tcp::server_reponse const& resp,
        std::function<void(boost::asynchronous::tcp::client_request const&)>const& when_done)
{
    // deserialize job, execute code, serialize result
    std::istringstream task_stream(resp.m_task);
    typename SerializableType::iarchive task_archive(task_stream);
    boost::asynchronous::tcp::client_request request (BOOST_ASYNCHRONOUS_TCP_CLIENT_JOB_RESULT);
    request.m_task_id = resp.m_task_id;
    task_archive >> t;
    // call task
    t();
    // create continuation waiting for task completion
    boost::asynchronous::create_continuation
        ([request,when_done](std::tuple<std::future<typename Task::return_type> > continuation_res)mutable
         {
            std::ostringstream res_archive_stream;
            typename SerializableType::oarchive res_archive(res_archive_stream);
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

// register callback continuations
template <class Task, class Job=boost::asynchronous::any_callable ,class SerializableType = boost::asynchronous::any_serializable>
void deserialize_and_call_callback_continuation_task(
        Task& t,boost::asynchronous::tcp::server_reponse const& resp,
        std::function<void(boost::asynchronous::tcp::client_request const&)>const& when_done)
{
    // deserialize job, execute code, serialize result
    std::istringstream task_stream(resp.m_task);
    typename SerializableType::iarchive task_archive(task_stream);
    boost::asynchronous::tcp::client_request request (BOOST_ASYNCHRONOUS_TCP_CLIENT_JOB_RESULT);
    request.m_task_id = resp.m_task_id;
    task_archive >> t;
    // create continuation waiting for task completion
    boost::asynchronous::create_callback_continuation_job<Job>
        ([request,when_done](std::tuple<boost::asynchronous::expected<typename Task::return_type> > continuation_res)mutable
         {
            std::ostringstream res_archive_stream;
            typename SerializableType::oarchive res_archive(res_archive_stream);
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
         , std::move(t)
    );
}
//register top-level callback continuations
template <class Task, class SerializableType = boost::asynchronous::any_serializable>
void deserialize_and_call_top_level_callback_continuation_task(
        Task& t,boost::asynchronous::tcp::server_reponse const& resp,
        std::function<void(boost::asynchronous::tcp::client_request const&)>const& when_done)
{
    // deserialize job, execute code, serialize result
    auto start = std::chrono::high_resolution_clock::now();
    std::istringstream task_stream(std::move(resp.m_task));
    typename SerializableType::iarchive task_archive(task_stream);
    boost::asynchronous::tcp::client_request request (BOOST_ASYNCHRONOUS_TCP_CLIENT_JOB_RESULT);
    request.m_task_id = resp.m_task_id;
    task_archive >> t;
    auto tmsg = (std::chrono::nanoseconds(std::chrono::high_resolution_clock::now() - start).count() / 1000000);
    if (tmsg > 10)
    {
        std::cout << "task deserialize took in ms: "<< tmsg << std::endl;
    }
    // call task
    auto start_op = std::chrono::high_resolution_clock::now();
    auto cont = t();
    cont.on_done([request,when_done,start_op](std::tuple<boost::asynchronous::expected<typename decltype(t())::return_type> >&& continuation_res)mutable
        {
            auto top = (std::chrono::nanoseconds(std::chrono::high_resolution_clock::now() - start_op).count() / 1000000);
            std::cout << "task op() took in ms: "<< top << std::endl;

            std::ostringstream res_archive_stream;
            typename SerializableType::oarchive res_archive(res_archive_stream);
            try
            {
                // serialize result
                auto start = std::chrono::high_resolution_clock::now();
                auto&& res = std::get<0>(continuation_res).get();
                res_archive << res;
                request.m_load.m_data = res_archive_stream.str();
                auto tres = (std::chrono::nanoseconds(std::chrono::high_resolution_clock::now() - start).count() / 1000000);
                std::cout << "serialize result took in ms: "<< tres << std::endl;
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
}
}}}
#endif // BOOST_ASYNCHRONOUS_SCHEDULER_TCP_SIMPLE_TCP_CLIENT_HPP
