// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_TCP_SERVER_SCHEDULER_HPP
#define BOOST_ASYNCHRONOUS_TCP_SERVER_SCHEDULER_HPP

#include <utility>
#include <cstddef>
#include <fstream>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/tss.hpp>
#include <boost/bind.hpp>
//#include <boost/archive/text_oarchive.hpp>
//#include <boost/archive/text_iarchive.hpp>

#include <boost/asynchronous/scheduler/detail/scheduler_helpers.hpp>
#include <boost/asynchronous/detail/any_joinable.hpp>
#include <boost/asynchronous/detail/any_interruptible.hpp>
#include <boost/asynchronous/diagnostics/default_loggable_job.hpp>
#include <boost/asynchronous/job_traits.hpp>
#include <boost/asynchronous/scheduler/detail/job_diagnostic_closer.hpp>
#include <boost/asynchronous/queue/any_queue.hpp>
#include <boost/asynchronous/scheduler/detail/single_queue_scheduler_policy.hpp>
#include <boost/asynchronous/detail/any_joinable.hpp>
#include <boost/asynchronous/scheduler/tss_scheduler.hpp>
#include <boost/asynchronous/scheduler/detail/lockable_weak_scheduler.hpp>
#include <boost/asynchronous/scheduler/cpu_load_policies.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/job_server.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
////TODO temporary
//#include <libs/asynchronous/doc/examples/dummy_tcp_task.hpp>
namespace boost { namespace asynchronous
{

template<class Q, class CPULoad =
#ifndef BOOST_ASYNCHRONOUS_SAVE_CPU_LOAD
        boost::asynchronous::no_cpu_load_saving
#else
        boost::asynchronous::default_save_cpu_load<>
#endif
                  >
class tcp_server_scheduler: public boost::asynchronous::detail::single_queue_scheduler_policy<Q>
{
public:
    typedef Q queue_type;
    typedef typename Q::job_type job_type;
    typedef typename boost::asynchronous::job_traits<typename Q::job_type>::diagnostic_table_type diag_type;
    typedef tcp_server_scheduler<Q,CPULoad> this_type;

#ifndef BOOST_NO_RVALUE_REFERENCES
    tcp_server_scheduler(boost::shared_ptr<queue_type>&& queue, unsigned int worker_pool_size,
                         std::string const & address,
                         unsigned int port)
        : boost::asynchronous::detail::single_queue_scheduler_policy<Q>(std::forward<boost::shared_ptr<queue_type> >(queue))
        , m_private_queue (boost::make_shared<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> >())
        , m_worker_pool_size(worker_pool_size)
        , m_address(address)
        , m_port(port)
    {
    }
#endif
#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES
    template<typename... Args>
    tcp_server_scheduler(unsigned int worker_pool_size,
                         std::string const & address,
                         unsigned int port,
                         Args... args)
        : boost::asynchronous::detail::single_queue_scheduler_policy<Q>(boost::make_shared<queue_type>(args...))
        , m_private_queue (boost::make_shared<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> >())
        , m_worker_pool_size(worker_pool_size)
        , m_address(address)
        , m_port(port)
    {
    }
#endif

    tcp_server_scheduler(unsigned int worker_pool_size,
                         std::string const & address,
                         unsigned int port)
        : boost::asynchronous::detail::single_queue_scheduler_policy<Q>()
        , m_private_queue (boost::make_shared<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> >())
        , m_worker_pool_size(worker_pool_size)
        , m_address(address)
        , m_port(port)
    {
    }
    void constructor_done(boost::weak_ptr<this_type> weak_self)
    {
        m_diagnostics = boost::make_shared<diag_type>();
        boost::promise<boost::thread*> new_thread_promise;
        boost::shared_future<boost::thread*> fu = new_thread_promise.get_future();
        boost::thread* new_thread =
                new boost::thread(boost::bind(&tcp_server_scheduler::run,this->m_queue,m_diagnostics,m_private_queue,fu,weak_self,m_worker_pool_size,m_address,m_port));
        new_thread_promise.set_value(new_thread);
        m_thread.reset(new_thread);
    }

    ~tcp_server_scheduler()
    {
        boost::asynchronous::detail::default_termination_task<typename Q::diagnostic_type,boost::thread> ttask(m_thread);
            // this task has to be executed lat => lowest prio
#ifndef BOOST_NO_RVALUE_REFERENCES
        boost::asynchronous::any_callable job(ttask);
        m_private_queue->push(std::move(job),std::numeric_limits<std::size_t>::max());
#else
        m_private_queue->push(boost::asynchronous::any_callable(ttask),std::numeric_limits<std::size_t>::max());
#endif
    }
    void post(typename queue_type::job_type && job, std::size_t prio)
    {
        boost::asynchronous::job_traits<typename queue_type::job_type>::set_posted_time(job);
        this->m_queue->push(std::forward<typename queue_type::job_type>(job),prio);
    }
    void post(typename queue_type::job_type && job)
    {
        post(std::forward<typename queue_type::job_type>(job),0);
    }
    void post(boost::asynchronous::any_callable&& /*job*/,const std::string& /*name*/)
    {
        //TODO
    }
    void post(boost::asynchronous::any_callable&& /*job*/,const std::string& /*name*/,std::size_t /*priority*/)
    {
        //TODO
    }

    boost::asynchronous::any_interruptible interruptible_post(typename queue_type::job_type && /*job*/,
                                                          std::size_t /*prio*/)
    {
//        boost::shared_ptr<boost::asynchronous::detail::interrupt_state>
//                state = boost::make_shared<boost::asynchronous::detail::interrupt_state>();
//        boost::shared_ptr<boost::promise<boost::thread*> > wpromise = boost::make_shared<boost::promise<boost::thread*> >();
//        boost::asynchronous::job_traits<typename queue_type::job_type>::set_posted_time(job);
//        boost::asynchronous::interruptible_job<typename queue_type::job_type,this_type>
//                ijob(std::forward<typename queue_type::job_type>(job),wpromise,state);

//        this->m_queue->push(std::forward<typename queue_type::job_type>(ijob),prio);

//        boost::future<boost::thread*> fu = wpromise->get_future();
//        boost::asynchronous::interrupt_helper interruptible(std::move(fu),state);

//        return boost::asynchronous::any_interruptible(interruptible);
        //TODO
        return boost::asynchronous::any_interruptible();
    }
    boost::asynchronous::any_interruptible interruptible_post(typename queue_type::job_type && job)
    {
        return interruptible_post(std::forward<typename queue_type::job_type>(job),0);
    }
    boost::asynchronous::any_interruptible interruptible_post(boost::asynchronous::any_callable&& /*job*/,const std::string& /*name*/)
    {
        //TODO
        return boost::asynchronous::any_interruptible();
    }
    boost::asynchronous::any_interruptible interruptible_post(boost::asynchronous::any_callable&& /*job*/,const std::string& /*name*/,
                                                           std::size_t /*priority*/)
    {
        //TODO
        return boost::asynchronous::any_interruptible();
    }


    //TODO move?
    boost::asynchronous::any_joinable get_worker()const
    {
        return boost::asynchronous::any_joinable (boost::asynchronous::detail::worker_wrap<boost::thread>(m_thread));
    }

    boost::thread::id id()const
    {
        return m_thread->get_id();
    }

    std::vector<boost::thread::id> thread_ids()const
    {
        std::vector<boost::thread::id> ids;
        ids.push_back(m_thread->get_id());
        return ids;
    }

    std::map<std::string,
             std::list<typename boost::asynchronous::job_traits<typename queue_type::job_type>::diagnostic_item_type > >
    get_diagnostics(std::size_t =0)const
    {
        return m_diagnostics->get_map();
    }
    std::vector<boost::asynchronous::any_queue_ptr<job_type> > get_queues()const
    {
        // this scheduler doesn't give any queues for stealing
        return std::vector<boost::asynchronous::any_queue_ptr<job_type> >();
    }
    void set_steal_from_queues(std::vector<boost::asynchronous::any_queue_ptr<job_type> > const&)
    {
        // this scheduler does not steal
    }

    static void run(boost::shared_ptr<queue_type> queue,boost::shared_ptr<diag_type> diagnostics,
                    boost::shared_ptr<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> > private_queue,
                    boost::shared_future<boost::thread*> self,
                    boost::weak_ptr<this_type> this_,
                    unsigned int worker_pool_size,
                    std::string const& address,
                    unsigned int port)
    {
        boost::thread* t = self.get();
        boost::asynchronous::detail::single_queue_scheduler_policy<Q>::m_self_thread.reset(new thread_ptr_wrapper(t));
        // thread scheduler => tss
        boost::asynchronous::any_weak_scheduler<job_type> self_as_weak = boost::asynchronous::detail::lockable_weak_scheduler<this_type>(this_);
        boost::asynchronous::get_thread_scheduler<job_type>(self_as_weak,true);

        // create server object and thread
        auto simplescheduler = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::lockfree_queue<> >);
        boost::asynchronous::tcp::job_server_proxy server(simplescheduler,
                                                          boost::asynchronous::create_shared_scheduler_proxy(
                                                              new boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<> >(worker_pool_size)),
                                                          address,port);

        CPULoad cpu_load;
        while(true)
        {
            try
            {
                {
                    // get a job
                    //TODO rval ref?
                    typename Q::job_type job;
                    // try from queue
                    bool popped = queue->try_pop(job);
                    if (popped)
                    {
                        cpu_load.popped_job();
                        // automatic closing of log
                        boost::asynchronous::detail::job_diagnostic_closer<typename Q::job_type,diag_type> closer
                                (&job,diagnostics.get());
                        // log time
                        boost::asynchronous::job_traits<typename Q::job_type>::set_started_time(job);
                        // forward to server
                        server.add_task(job);

                    }
                    // check for shutdown
                    boost::asynchronous::any_callable djob;
                    popped = private_queue->try_pop(djob);
                    if (popped)
                    {
                        djob();
                    }


                } // job destroyed (for destruction useful)
                // check if we got an interruption job
                boost::this_thread::interruption_point();
            }
            catch(boost::asynchronous::detail::shutdown_exception&)
            {
                // we are done
                return;
            }
            catch(boost::thread_interrupted&)
            {
                // task interrupted, no problem, just continue
            }
            catch(std::exception&)
            {
                // TODO, user-defined error
            }
        }
    }
private:
    boost::shared_ptr<boost::thread> m_thread;
    boost::shared_ptr<diag_type> m_diagnostics;
    boost::shared_ptr<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> > m_private_queue;
    unsigned int m_worker_pool_size;
    std::string m_address;
    unsigned int m_port;
};

}} // boost::async::scheduler

#endif /* BOOST_ASYNCHRONOUS_TCP_SERVER_SCHEDULER_HPP */
