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

#include <boost/shared_ptr.hpp>

#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/server_response.hpp>
#include <boost/asynchronous/servant_proxy.h>
#include <boost/asynchronous/extensions/asio/asio_scheduler.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/server_connection.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/asio_comm_server.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/client_request.hpp>
#include <boost/asynchronous/diagnostics/any_loggable.hpp>

namespace boost { namespace asynchronous { namespace tcp {
template <class PoolJobType, class SerializableType = boost::asynchronous::any_serializable >
struct job_server : boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,PoolJobType>
{
    typedef int simple_ctor;
    job_server(boost::asynchronous::any_weak_scheduler<boost::asynchronous::any_callable> scheduler,
               boost::asynchronous::any_shared_scheduler_proxy<PoolJobType> worker,
               std::string const & address,
               unsigned int port,
               bool stealing)
        : boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,PoolJobType>(scheduler,worker)
        , m_next_task_id(0)
        , m_stealing(stealing)
    {
        // For TCP communication we use an asio-based scheduler with 1 thread
        auto asioWorkers =
                boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::asio_scheduler<>(1));

        // created posted function when a client connects
        std::function<void(boost::asio::ip::tcp::socket)> f =
                [this](boost::asio::ip::tcp::socket s)mutable
                {
                    const auto c = boost::make_shared<boost::asynchronous::tcp::server_connection<SerializableType> >(std::move(s));
                    std::function<void(boost::asynchronous::tcp::client_request)> cb=
                            [this,c](boost::asynchronous::tcp::client_request request){this->handleRequest(request, c);};
                    c->start(this->make_safe_callback(cb));
                };

        m_asio_comm.push_back(boost::asynchronous::tcp::asio_comm_server_proxy(asioWorkers, address, port, f));
    }

    void add_task(SerializableType job)
    {
        boost::shared_ptr<SerializableType> moved_job(boost::make_shared<SerializableType>(std::move(job)));
        this->post_callback(
                    [moved_job]()mutable
                    {
                        // serialize job
                        std::ostringstream archive_stream;
                        typename SerializableType::oarchive archive(archive_stream);
                        moved_job->serialize(archive,0);
                        return archive_stream.str();
                    },
                    [moved_job,this](boost::future<std::string> fu)mutable
                    {
                        waiting_job new_job(std::move(*moved_job),
                                            boost::asynchronous::tcp::server_reponse(this->m_next_task_id++,fu.get(),
                                                                                     moved_job->get_task_name()));
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
        for (typename std::deque<boost::shared_ptr<boost::asynchronous::tcp::server_connection<SerializableType> > >::iterator it
             = m_waiting_connections.begin();
             it != m_waiting_connections.end(); ++it)
        {
             (*it)->send(boost::asynchronous::tcp::server_reponse(0,"",""));
        }
        m_waiting_connections.clear();
    }

private:
    void send_first_job(boost::shared_ptr<boost::asynchronous::tcp::server_connection<SerializableType> > connection)
    {
        connection->send(m_unprocessed_jobs.front().m_serialized);
        m_waiting_jobs.insert(m_unprocessed_jobs.front());
        m_unprocessed_jobs.pop_front();
    }
    void handleRequest(boost::asynchronous::tcp::client_request request,
                       boost::shared_ptr<boost::asynchronous::tcp::server_connection<SerializableType> > connection)
    {
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
                connection->send(boost::asynchronous::tcp::server_reponse(0,"",""));
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
            waiting_job searched_job(SerializableType(),boost::asynchronous::tcp::server_reponse(request.m_task_id,"",""));
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
                const_cast<SerializableType&>((*it).m_job).serialize(archive,0);
            }
            // else ignore TODO log?
        }
        else
        {
            // unknown command, ignore
        }
    }

    // waiting jobs in serialized and callable forms
    struct waiting_job
    {
        waiting_job(SerializableType&& job, boost::asynchronous::tcp::server_reponse serialized)
            : m_job(std::forward<SerializableType>(job)), m_serialized(serialized)
        {}
        SerializableType m_job;
        boost::asynchronous::tcp::server_reponse m_serialized;
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
    // jobs waiting for a client offer to process
    std::deque<waiting_job> m_unprocessed_jobs;
    // job being executed and waiting for a result
    std::set<waiting_job,sort_tasks> m_waiting_jobs;
    std::vector<boost::asynchronous::tcp::asio_comm_server_proxy> m_asio_comm;
    std::deque<boost::shared_ptr<boost::asynchronous::tcp::server_connection<SerializableType> > > m_waiting_connections;
};
#ifdef BOOST_ASYNCHRONOUS_NO_TEMPLATE_PROXY_CLASSES

class job_server_proxy : public boost::asynchronous::servant_proxy<job_server_proxy,job_server<boost::asynchronous::any_callable> >
{
public:

    typedef job_server<boost::asynchronous::any_callable> servant_type;
    template <class Scheduler,class Worker>
    job_server_proxy(Scheduler s, Worker w,std::string const & address,unsigned int port, bool stealing):
        boost::asynchronous::servant_proxy<job_server_proxy,job_server<boost::asynchronous::any_callable> >(s,w,address,port,stealing)
    {}

    // get_data is posted, no future, no callback
    BOOST_ASYNC_FUTURE_MEMBER(add_task)
    BOOST_ASYNC_FUTURE_MEMBER(requests_stealing)
    BOOST_ASYNC_FUTURE_MEMBER(no_jobs)

};
class job_server_proxy_log :
        public boost::asynchronous::servant_proxy<job_server_proxy,
        job_server<boost::asynchronous::any_loggable<boost::chrono::high_resolution_clock> > >
{
public:
    template <class Scheduler,class Worker>
    job_server_proxy_log(Scheduler s, Worker w,std::string const & address,unsigned int port, bool stealing):
        boost::asynchronous::servant_proxy<job_server_proxy,
                                           job_server<boost::asynchronous::any_loggable<boost::chrono::high_resolution_clock> > >
        (s,w,address,port,stealing)
    {}
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
struct get_correct_job_server_proxy<boost::asynchronous::any_loggable<boost::chrono::high_resolution_clock>,
                                    boost::asynchronous::any_serializable >
{
    typedef boost::asynchronous::tcp::job_server_proxy_log type;
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
