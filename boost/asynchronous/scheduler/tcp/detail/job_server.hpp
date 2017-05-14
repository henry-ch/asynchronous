// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_SCHEDULER_TCP_JOB_SERVER_HPP
#define BOOST_ASYNCHRONOUS_SCHEDULER_TCP_JOB_SERVER_HPP

#include <set>
#include <vector>
#include <deque>
#include <functional>

#include <memory>

#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/server_response.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/extensions/asio/asio_scheduler.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/server_connection.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/asio_comm_server.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/client_request.hpp>
#include <boost/asynchronous/diagnostics/any_loggable.hpp>
#include <boost/asynchronous/diagnostics/any_loggable_serializable.hpp>

namespace boost { namespace asynchronous { namespace tcp {
template <class PoolJobType, class SerializableType = boost::asynchronous::any_serializable >
struct job_server : boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,PoolJobType>
{
    typedef int simple_ctor;
    typedef std::shared_ptr<typename boost::asynchronous::job_traits<SerializableType>::diagnostic_table_type> diag_type;
    typedef server_connection_proxy<SerializableType> server_connection_type;

    job_server(boost::asynchronous::any_weak_scheduler<boost::asynchronous::any_callable> scheduler,
               boost::asynchronous::any_shared_scheduler_proxy<PoolJobType> worker,
               std::string const & address,
               unsigned int port,
               bool stealing)
        : boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,PoolJobType>(scheduler,worker)
        , m_next_task_id(0)
        , m_stealing(stealing)
        , m_address(address)
        , m_port(port)
    {
        // For TCP communication we use an asio-based scheduler with 1 thread
        m_asioWorkers =
                boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::asio_scheduler<>>(1);

        setup_connection();
    }

    void add_task(SerializableType job,diag_type diag)
    {
        std::shared_ptr<SerializableType> moved_job(std::make_shared<SerializableType>(std::move(job)));
        this->post_callback(
                    [moved_job]()mutable
                    {
                        // serialize job
                        std::ostringstream archive_stream;
                        typename SerializableType::oarchive archive(archive_stream);
                        moved_job->serialize(archive,0);
                        return archive_stream.str();
                    },
                    [moved_job,diag,this](boost::asynchronous::expected<std::string> fu)mutable
                    {
                        waiting_job new_job(std::move(*moved_job),
                                            boost::asynchronous::tcp::server_reponse(++this->m_next_task_id,fu.get(),
                                                                                     moved_job->get_task_name()),
                                            diag);
                        this->m_unprocessed_jobs.emplace_back(std::move(new_job));
                        // if we have a waiting connection, immediately send
                        if (!this->m_waiting_connections.empty())
                        {
                            // send
                            this->send_first_job(this->m_waiting_connections.front());
                            this->m_waiting_connections.pop_front();
                        }
                    },
                    "boost::asynchronous::tcp::job_server::serialize"
        );
    }
    bool requests_stealing()const
    {
        return !m_waiting_connections.empty();
    }
    void no_jobs()
    {
        // inform waiting connections
        for (typename std::deque<std::shared_ptr<server_connection_type> >::iterator it
             = m_waiting_connections.begin();
             it != m_waiting_connections.end(); ++it)
        {
            std::function<void(boost::asynchronous::tcp::server_reponse)> cb=
                    [this](boost::asynchronous::tcp::server_reponse ){};
             (*it)->send(boost::asynchronous::tcp::server_reponse(0,"",""),cb);
        }
        m_waiting_connections.clear();
    }

private:

    void handle_connect(std::shared_ptr<boost::asio::ip::tcp::socket> s)
    {
        auto c = std::make_shared<server_connection_type >(m_asioWorkers,s);
        // short wait to see if we can steal some job
        m_keepalive_connections.insert(c);

        // use a weak_ptr so that connection will not keep itself alive
        std::weak_ptr<server_connection_type> wconnection(c);
        std::function<void(boost::asynchronous::tcp::client_request)> cb=
                [this,wconnection](boost::asynchronous::tcp::client_request request){this->handleRequest(request, wconnection);};
        c->start(this->make_safe_callback(cb,"",0));
    }

    void setup_connection()
    {
        // created posted function when a client connects
        std::function<void(std::shared_ptr<boost::asio::ip::tcp::socket>)> f =
                this->make_safe_callback(std::function<void(std::shared_ptr<boost::asio::ip::tcp::socket>)>(
                                         [this](std::shared_ptr<boost::asio::ip::tcp::socket> socket)
                                         {this->handle_connect(socket);}),"",0);
        m_asio_comm.push_back(boost::asynchronous::tcp::asio_comm_server_proxy(m_asioWorkers, m_address, m_port, f));
    }

    void send_first_job(std::shared_ptr<server_connection_type > connection)
    {
        // prepare callback if failed
        std::function<void(boost::asynchronous::tcp::server_reponse)> cb=
                [this](boost::asynchronous::tcp::server_reponse msg){this->handle_error_send(msg);};

        // set started time to when job gets stolen
        boost::asynchronous::job_traits<SerializableType>::set_started_time(m_unprocessed_jobs.front().m_job);
        connection->send(m_unprocessed_jobs.front().m_serialized,cb);
        m_waiting_jobs.insert(m_unprocessed_jobs.front());
        m_unprocessed_jobs.pop_front();
    }
    void handleRequest(boost::asynchronous::tcp::client_request request,
                       std::weak_ptr<server_connection_type> wconnection)
    {
        std::shared_ptr<server_connection_type> connection = wconnection.lock();
        if (!connection)
            // there must have been a tcp error, ignore
            return;

        // is it a request for job?
        if (request.m_cmd_id == BOOST_ASYNCHRONOUS_TCP_CLIENT_GET_JOB)
        {
            // send the first available job if any
            if (!m_unprocessed_jobs.empty())
            {
                send_first_job(connection);
            }
            else if(!m_stealing)
            {
                // no job and no stealing => immediate answer
                std::function<void(boost::asynchronous::tcp::server_reponse)> cb=
                        [this](boost::asynchronous::tcp::server_reponse ){};
                connection->send(boost::asynchronous::tcp::server_reponse(0,"",""),cb);
            }
            else
            {
                // short wait to see if we can steal some job
                m_waiting_connections.push_back(connection);
            }
        }
        else if (request.m_cmd_id == BOOST_ASYNCHRONOUS_TCP_CLIENT_JOB_RESULT)
        {
            // dummy job just to find with it
            waiting_job searched_job(SerializableType(),boost::asynchronous::tcp::server_reponse(request.m_task_id,"",""),diag_type());
            typename std::set<waiting_job,sort_tasks>::iterator it = m_waiting_jobs.find(searched_job);
            if (it != m_waiting_jobs.end())
            {
                // return result. Serialize data and exception
                std::ostringstream load_archive_stream;
                typename SerializableType::oarchive load_archive(load_archive_stream);
                load_archive << request.m_load;
                std::string msg_load=load_archive_stream.str();

                std::istringstream archive_stream(msg_load);
                typename SerializableType::iarchive archive(archive_stream);
                // set finish time to when result is sent, close diagnostics
                boost::asynchronous::job_traits<SerializableType>::set_finished_time(const_cast<SerializableType&>((*it).m_job));
                boost::asynchronous::job_traits<SerializableType>::add_diagnostic(const_cast<SerializableType&>((*it).m_job),(*it).m_diag.get());

                const_cast<SerializableType&>((*it).m_job).serialize(archive,0);
            }
            // else ignore TODO log?
        }
        else if (request.m_cmd_id == BOOST_ASYNCHRONOUS_TCP_CLIENT_COM_ERROR)
        {
            // let connection die
            m_keepalive_connections.erase(connection);
        }
        else
        {
            // unknown command, ignore
        }
    }
    void handle_error_send(boost::asynchronous::tcp::server_reponse msg)
    {
        // if this was not a real job, ignore
        if (msg.m_task_id != 0)
        {
            // a real task got an error during communication and will no be processed this time, get it back
            waiting_job searched_job(SerializableType(),boost::asynchronous::tcp::server_reponse(msg.m_task_id,"",""),diag_type());
            auto it = m_waiting_jobs.find(searched_job);
            if (it != m_waiting_jobs.end())
            {
                m_unprocessed_jobs.emplace_front(std::move(*it));
                m_waiting_jobs.erase(it);
            }
        }
    }

    // waiting jobs in serialized and callable forms
    struct waiting_job
    {
        waiting_job(SerializableType&& job, boost::asynchronous::tcp::server_reponse serialized, diag_type const& diag)
            : m_job(std::forward<SerializableType>(job)), m_serialized(serialized), m_diag(diag)
        {}
        SerializableType m_job;
        boost::asynchronous::tcp::server_reponse m_serialized;
        diag_type m_diag;
    };
    struct sort_tasks
    {
        bool operator()(waiting_job const& rhs, waiting_job const& lhs)const
        {
            return rhs.m_serialized.m_task_id < lhs.m_serialized.m_task_id;
        }
    };
    // always increasing task counter for ids
    long m_next_task_id;
    bool m_stealing;
    std::string m_address;
    unsigned int m_port;
    // jobs waiting for a client offer to process
    std::deque<waiting_job> m_unprocessed_jobs;
    // job being executed and waiting for a result
    std::set<waiting_job,sort_tasks> m_waiting_jobs;
    boost::asynchronous::any_shared_scheduler_proxy<> m_asioWorkers;
    std::vector<boost::asynchronous::tcp::asio_comm_server_proxy> m_asio_comm;
    // connections for clients which connected and made a request but are waiting for us to answer
    std::deque<std::shared_ptr<server_connection_type > > m_waiting_connections;
    // connections for clients who just connected and made no request yet
    // TODO timer which closes them after a timeout
    std::set<std::shared_ptr<server_connection_type > > m_keepalive_connections;
};
#ifndef BOOST_ASYNCHRONOUS_USE_TEMPLATE_PROXY_CLASSES

class job_server_proxy : public boost::asynchronous::servant_proxy<job_server_proxy,job_server<boost::asynchronous::any_callable> >
{
public:

    typedef job_server<boost::asynchronous::any_callable> servant_type;
    template <class Scheduler,class Worker>
    job_server_proxy(Scheduler s, Worker w,std::string const & address,unsigned int port, bool stealing):
        boost::asynchronous::servant_proxy<job_server_proxy,job_server<boost::asynchronous::any_callable> >(s,w,address,port,stealing)
    {
    }

    // get_data is posted, no future, no callback
    BOOST_ASYNC_FUTURE_MEMBER(add_task)
    BOOST_ASYNC_FUTURE_MEMBER(requests_stealing)
    BOOST_ASYNC_FUTURE_MEMBER(no_jobs)

};
class job_server_proxy_log :
        public boost::asynchronous::servant_proxy<job_server_proxy,
        job_server<boost::asynchronous::any_loggable> >
{
public:
    template <class Scheduler,class Worker>
    job_server_proxy_log(Scheduler s, Worker w,std::string const & address,unsigned int port, bool stealing):
        boost::asynchronous::servant_proxy<job_server_proxy,
                                           job_server<boost::asynchronous::any_loggable> >
        (s,w,address,port,stealing)
    {
    }
    // get_data is posted, no future, no callback
    BOOST_ASYNC_FUTURE_MEMBER(add_task)
    BOOST_ASYNC_FUTURE_MEMBER(requests_stealing)
    BOOST_ASYNC_FUTURE_MEMBER(no_jobs)
};
class job_server_proxy_log_serialize :
        public boost::asynchronous::servant_proxy<job_server_proxy_log_serialize,
                                                  job_server<boost::asynchronous::any_loggable,
                                                             boost::asynchronous::any_loggable_serializable > >
{
public:
    template <class Scheduler,class Worker>
    job_server_proxy_log_serialize(Scheduler s, Worker w,std::string const & address,unsigned int port, bool stealing):
        boost::asynchronous::servant_proxy<job_server_proxy_log_serialize,
                                           job_server<boost::asynchronous::any_loggable,
                                                      boost::asynchronous::any_loggable_serializable > >
        (s,w,address,port,stealing)
    {
    }
    // get_data is posted, no future, no callback
    BOOST_ASYNC_FUTURE_MEMBER(add_task)
    BOOST_ASYNC_FUTURE_MEMBER(requests_stealing)
    BOOST_ASYNC_FUTURE_MEMBER(no_jobs)
};
// choose correct proxy
template <class Job, class SerializableType = boost::asynchronous::any_serializable>
struct get_correct_job_server_proxy
{
    typedef boost::asynchronous::tcp::job_server_proxy type;
};
template <>
struct get_correct_job_server_proxy<boost::asynchronous::any_loggable,
                                    boost::asynchronous::any_serializable >
{
    typedef boost::asynchronous::tcp::job_server_proxy_log type;
};

template <>
struct get_correct_job_server_proxy<boost::asynchronous::any_loggable,
                                    boost::asynchronous::any_loggable_serializable >
{
    typedef boost::asynchronous::tcp::job_server_proxy_log_serialize type;
};
#else

template <class T>
class job_server_proxy : public boost::asynchronous::servant_proxy<job_server_proxy<T>,job_server<boost::asynchronous::any_callable> >
{
public:
    typedef job_server<boost::asynchronous::any_callable> servant_type;
    template <class Scheduler,class Worker>
    job_server_proxy(Scheduler s, Worker w,std::string const & address,unsigned int port, bool stealing):
        boost::asynchronous::servant_proxy<job_server_proxy<T>,job_server<boost::asynchronous::any_callable> >(s,w,address,port,stealing)
    {}

    // get_data is posted, no future, no callback
    BOOST_ASYNC_FUTURE_MEMBER(add_task)
    BOOST_ASYNC_FUTURE_MEMBER(requests_stealing)
    BOOST_ASYNC_FUTURE_MEMBER(no_jobs)

};

template <class Job>
struct get_correct_job_server_proxy
{
    typedef boost::asynchronous::tcp::job_server_proxy<Job> type;
};

#endif


}}}
#endif // BOOST_ASYNCHRONOUS_SCHEDULER_TCP_JOB_SERVER_HPP
