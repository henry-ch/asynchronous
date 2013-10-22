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
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/server_response.hpp>
#include <boost/asynchronous/servant_proxy.h>
#include <boost/asynchronous/extensions/asio/asio_scheduler.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/server_connection.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/asio_comm_server.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/client_request.hpp>

namespace boost { namespace asynchronous { namespace tcp {
struct job_server : boost::asynchronous::trackable_servant<>
{
    typedef int simple_ctor;
    job_server(boost::asynchronous::any_weak_scheduler<> scheduler,
               boost::asynchronous::any_shared_scheduler_proxy<> worker,
               std::string const & address,
               unsigned int port)
        : boost::asynchronous::trackable_servant<>(scheduler,worker)
        , m_next_task_id(0)
    {
        // For TCP communication we use an asio-based scheduler with 1 thread
        auto asioWorkers =
                boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::asio_scheduler<>(1));

        // created posted function when a client connects
        std::function<void(boost::shared_ptr<boost::asynchronous::tcp::server_connection>)> f =
                [this](boost::shared_ptr<boost::asynchronous::tcp::server_connection> c)mutable
                {
                    std::function<void(boost::asynchronous::tcp::client_request)> cb=
                            [this,c](boost::asynchronous::tcp::client_request request){this->handleRequest(request, c);};
                    c->start(this->make_safe_callback(cb));
                };

        m_asio_comm.push_back(boost::asynchronous::tcp::asio_comm_server_proxy(asioWorkers, address, port, f));
    }

    void add_task(boost::asynchronous::any_serializable job)
    {
        boost::shared_ptr<boost::asynchronous::any_serializable> moved_job(boost::make_shared<boost::asynchronous::any_serializable>(std::move(job)));
        post_callback(
                    [moved_job]()mutable
                    {
                        // serialize job
                        std::ostringstream archive_stream;
                        boost::archive::text_oarchive archive(archive_stream);
                        moved_job->serialize(archive,0);
                        return archive_stream.str();
                    },
                    [moved_job,this](boost::future<std::string> fu)mutable
                    {
                        waiting_job new_job(std::move(*moved_job),boost::asynchronous::tcp::server_reponse(this->m_next_task_id++,fu.get(),moved_job->get_task_name()));
                        this->m_unprocessed_jobs.emplace_back(std::move(new_job));
                    }
        );
    }

private:
    void handleRequest(boost::asynchronous::tcp::client_request request, boost::shared_ptr<boost::asynchronous::tcp::server_connection> connection)
    {        
        // is it a request for job?
        if (request.m_cmd_id == BOOST_ASYNCHRONOUS_TCP_CLIENT_GET_JOB)
        {
            // send the first available job if any
            if (!m_unprocessed_jobs.empty())
            {
                connection->send(m_unprocessed_jobs.front().m_serialized);
                m_waiting_jobs.insert(m_unprocessed_jobs.front());
                m_unprocessed_jobs.pop_front();
            }
            else
            {
                // no job
                connection->send(boost::asynchronous::tcp::server_reponse(0,"",""));
            }
        }
        else if (request.m_cmd_id == BOOST_ASYNCHRONOUS_TCP_CLIENT_JOB_RESULT)
        {
            // dummy job just to find with it
            waiting_job searched_job(boost::asynchronous::any_serializable(),boost::asynchronous::tcp::server_reponse(request.m_task_id,"",""));
            std::set<waiting_job,sort_tasks>::iterator it = m_waiting_jobs.find(searched_job);
            if (it != m_waiting_jobs.end())
            {
                // return result
                std::istringstream archive_stream(request.m_data);
                boost::archive::text_iarchive archive(archive_stream);
                const_cast<boost::asynchronous::any_serializable&>((*it).m_job).serialize(archive,0);
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
        waiting_job(boost::asynchronous::any_serializable&& job, boost::asynchronous::tcp::server_reponse serialized)
            : m_job(std::forward<boost::asynchronous::any_serializable>(job)), m_serialized(serialized)
        {}
        boost::asynchronous::any_serializable m_job;
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
    // jobs waiting for a client offer to process
    std::deque<waiting_job> m_unprocessed_jobs;
    // job being executed and waiting for a result
    std::set<waiting_job,sort_tasks> m_waiting_jobs;
    std::vector<boost::asynchronous::tcp::asio_comm_server_proxy> m_asio_comm;
};

class job_server_proxy : public boost::asynchronous::servant_proxy<job_server_proxy,job_server>
{
public:
    template <class Scheduler,class Worker>
    job_server_proxy(Scheduler s, Worker w,std::string const & address,unsigned int port):
        boost::asynchronous::servant_proxy<job_server_proxy,job_server>(s,w,address,port)
    {}
    // get_data is posted, no future, no callback
    BOOST_ASYNC_FUTURE_MEMBER(add_task)
};

}}}
#endif // BOOST_ASYNCHRONOUS_SCHEDULER_TCP_JOB_SERVER_HPP
