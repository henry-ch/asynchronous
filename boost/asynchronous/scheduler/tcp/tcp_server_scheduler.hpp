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
#include <memory>
#include <functional>
#include <future>

#include <boost/thread/thread.hpp>
#include <boost/thread/tss.hpp>
#include <boost/config.hpp>

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
#include <boost/asynchronous/scheduler/detail/execute_in_all_threads.hpp>

namespace boost { namespace asynchronous
{

template<class Q, class PoolJobType = BOOST_ASYNCHRONOUS_DEFAULT_JOB,
         bool IsStealing=false,
         class CPULoad =
#ifdef BOOST_ASYNCHRONOUS_NO_SAVING_CPU_LOAD
        boost::asynchronous::no_cpu_load_saving
#else
        boost::asynchronous::default_save_cpu_load<>
#endif
                  >
class tcp_server_scheduler: public boost::asynchronous::detail::single_queue_scheduler_policy<Q>
{
public:
    typedef PoolJobType pool_job_type;
    typedef Q queue_type;
    typedef typename Q::job_type job_type;
    typedef typename boost::asynchronous::job_traits<typename Q::job_type>::diagnostic_table_type diag_type;
    typedef tcp_server_scheduler<Q,pool_job_type,IsStealing,CPULoad> this_type;

    template<typename... Args>
    tcp_server_scheduler(boost::asynchronous::any_shared_scheduler_proxy<pool_job_type> worker_pool,
                         std::string const & address,
                         unsigned int port,
                         std::string const& name,
                         Args... args)
        : boost::asynchronous::detail::single_queue_scheduler_policy<Q>(std::make_shared<queue_type>(std::move(args)...))
        , m_private_queue (std::make_shared<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> >())
        , m_worker_pool(worker_pool)
        , m_address(address)
        , m_port(port)
        , m_name(name)
    {
        set_name(name);
    }
    template<typename... Args>
    tcp_server_scheduler(boost::asynchronous::any_shared_scheduler_proxy<pool_job_type> worker_pool,
                         std::string const & address,
                         unsigned int port,
                         Args... args)
        : boost::asynchronous::detail::single_queue_scheduler_policy<Q>(std::make_shared<queue_type>(std::move(args)...))
        , m_private_queue (std::make_shared<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> >())
        , m_worker_pool(worker_pool)
        , m_address(address)
        , m_port(port)
    {
    }

    std::vector<std::size_t> get_queue_size() const
    {
        return this->m_queue->get_queue_size();
    }
    std::vector<std::size_t> get_max_queue_size() const
    {
        return this->m_queue->get_max_queue_size();
    }
    void reset_max_queue_size()
    {
        this->m_queue->reset_max_queue_size();
    }

    void init(std::vector<boost::asynchronous::any_queue_ptr<job_type> > const& others)
    {
        m_diagnostics = std::make_shared<diag_type>(1);
        std::promise<boost::thread*> new_thread_promise;
        std::shared_future<boost::thread*> fu = new_thread_promise.get_future();
        boost::thread* new_thread =
                new boost::thread(std::bind(&tcp_server_scheduler::run,this->m_queue,m_diagnostics,m_private_queue,
                                              others,fu,m_weak_self,m_worker_pool,m_address,m_port));
        new_thread_promise.set_value(new_thread);
        m_thread.reset(new_thread);
    }

    void constructor_done(std::weak_ptr<this_type> weak_self)
    {
        m_weak_self = weak_self;
        if (!IsStealing)
            init(std::vector<boost::asynchronous::any_queue_ptr<job_type> >());
    }
    void set_steal_from_queues(std::vector<boost::asynchronous::any_queue_ptr<job_type> > const& others)
    {
        init(others);
    }
    ~tcp_server_scheduler()
    {
        auto fct = m_diagnostics_fct;
        auto diag = m_diagnostics;
        auto l = [fct,diag]() mutable
        {
            if (fct)
                fct(boost::asynchronous::scheduler_diagnostics(diag->get_map(),diag->get_current()));
        };
        boost::asynchronous::detail::default_termination_task<typename Q::diagnostic_type,boost::thread>
                ttask(std::move(l));
            // this task has to be executed lat => lowest prio
#ifndef BOOST_NO_RVALUE_REFERENCES
        boost::asynchronous::any_callable job(std::move(ttask));
        m_private_queue->push(std::move(job),std::numeric_limits<std::size_t>::max());
#else
        m_private_queue->push(boost::asynchronous::any_callable(ttask),std::numeric_limits<std::size_t>::max());
#endif
    }
    void set_name(std::string const& name)
    {
        boost::asynchronous::detail::set_name_task<typename Q::diagnostic_type> ntask(name);
#ifndef BOOST_NO_RVALUE_REFERENCES
        boost::asynchronous::any_callable job(std::move(ntask));
        m_private_queue->push(std::move(job),std::numeric_limits<std::size_t>::max());
#else
        m_private_queue->push(boost::asynchronous::any_callable(ntask),std::numeric_limits<std::size_t>::max());
#endif
    }
    std::string get_name()const
    {
        return m_name;
    }
    void processor_bind(std::vector<std::tuple<unsigned int/*first core*/,unsigned int/*number of threads*/>> p)
    {
        boost::asynchronous::detail::processor_bind_task task(std::get<0>(p[0]));
        boost::asynchronous::any_callable job(std::move(task));
        m_private_queue->push(std::move(job),std::numeric_limits<std::size_t>::max());
    }
    BOOST_ATTRIBUTE_NODISCARD std::vector<std::future<void>> execute_in_all_threads(boost::asynchronous::any_callable c)
    {
        std::vector<std::future<void>> res;
        res.reserve(1);
        std::promise<void> p;
        auto fu = p.get_future();
        res.emplace_back(std::move(fu));
        boost::asynchronous::detail::execute_in_all_threads_task task(std::move(c),std::move(p));
        m_private_queue->push(std::move(task),std::numeric_limits<std::size_t>::max());
        return res;
    }

    void post(typename queue_type::job_type job, std::size_t prio)
    {
        boost::asynchronous::job_traits<typename queue_type::job_type>::set_posted_time(job);
        this->m_queue->push(std::move(job),prio);
    }
    void post(typename queue_type::job_type job)
    {
        post(std::move(job),0);
    }

    boost::asynchronous::any_interruptible interruptible_post(typename queue_type::job_type /*job*/,
                                                          std::size_t /*prio*/)
    {
//        std::shared_ptr<boost::asynchronous::detail::interrupt_state>
//                state = std::make_shared<boost::asynchronous::detail::interrupt_state>();
//        std::shared_ptr<std::promise<boost::thread*> > wpromise = std::make_shared<std::promise<boost::thread*> >();
//        boost::asynchronous::job_traits<typename queue_type::job_type>::set_posted_time(job);
//        boost::asynchronous::interruptible_job<typename queue_type::job_type,this_type>
//                ijob(std::forward<typename queue_type::job_type>(job),wpromise,state);

//        this->m_queue->push(std::forward<typename queue_type::job_type>(ijob),prio);

//        std::future<boost::thread*> fu = wpromise->get_future();
//        boost::asynchronous::interrupt_helper interruptible(std::move(fu),state);

//        return boost::asynchronous::any_interruptible(interruptible);
        //TODO
        return boost::asynchronous::any_interruptible();
    }
    boost::asynchronous::any_interruptible interruptible_post(typename queue_type::job_type job)
    {
        return interruptible_post(std::move(job),0);
    }
    boost::asynchronous::any_interruptible interruptible_post(boost::asynchronous::any_callable /*job*/,const std::string& /*name*/)
    {
        //TODO
        return boost::asynchronous::any_interruptible();
    }
    boost::asynchronous::any_interruptible interruptible_post(boost::asynchronous::any_callable /*job*/,const std::string& /*name*/,
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

    boost::asynchronous::scheduler_diagnostics
    get_diagnostics(std::size_t =0)const
    {
        return boost::asynchronous::scheduler_diagnostics(m_diagnostics->get_map(),m_diagnostics->get_current());
    }
    void clear_diagnostics()
    {
        m_diagnostics->clear();
    }
    void register_diagnostics_functor(std::function<void(boost::asynchronous::scheduler_diagnostics)> fct,
                                      boost::asynchronous::register_diagnostics_type =
                                                    boost::asynchronous::register_diagnostics_type())
    {
        m_diagnostics_fct = fct;
    }
    std::vector<boost::asynchronous::any_queue_ptr<job_type> > get_queues()
    {
        // this scheduler doesn't give any queues for stealing
        return std::vector<boost::asynchronous::any_queue_ptr<job_type> >();
    }

    static void run(std::shared_ptr<queue_type> queue,std::shared_ptr<diag_type> diagnostics,
                    std::shared_ptr<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> > private_queue,
                    std::vector<boost::asynchronous::any_queue_ptr<job_type> > other_queues,
                    std::shared_future<boost::thread*> self,
                    std::weak_ptr<this_type> this_,
                    boost::asynchronous::any_shared_scheduler_proxy<pool_job_type> worker_pool,
                    std::string const& address,
                    unsigned int port)
    {
        boost::thread* t = self.get();
        boost::asynchronous::detail::single_queue_scheduler_policy<Q>::m_self_thread.reset(new thread_ptr_wrapper(t));
        // thread scheduler => tss
        boost::asynchronous::any_weak_scheduler<job_type> self_as_weak = boost::asynchronous::detail::lockable_weak_scheduler<this_type>(this_);
        boost::asynchronous::get_thread_scheduler<job_type>(self_as_weak,true);

        // create server object and thread
        auto simplescheduler = boost::asynchronous::make_shared_scheduler_proxy<
                                boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::lockfree_queue<>>>();
        typename boost::asynchronous::tcp::get_correct_job_server_proxy<pool_job_type,job_type>::type
                server(simplescheduler,worker_pool,address,port,IsStealing);

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
                        // forward to server
                        server.add_task(job,diagnostics);

                    }
                    if (!popped && server.requests_stealing().get())
                    {                        
                        // ok we have nothing to do, maybe we can steal some work?
                        for (std::size_t i=0; i< other_queues.size(); ++i)
                        {
                            popped = (*other_queues[i]).try_steal(job);
                            if (popped)
                            {
                                // forward to server
                                server.add_task(job,diagnostics);
                                break;
                            }
                        }
                    }
                    if (!popped)
                    {
                        // no job stolen
                        server.no_jobs().get();
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
    std::shared_ptr<boost::thread> m_thread;
    std::shared_ptr<diag_type> m_diagnostics;
    std::shared_ptr<boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable> > m_private_queue;
    boost::asynchronous::any_shared_scheduler_proxy<pool_job_type> m_worker_pool;
    std::string m_address;
    unsigned int m_port;
    std::weak_ptr<this_type> m_weak_self;
    std::function<void(boost::asynchronous::scheduler_diagnostics)> m_diagnostics_fct;
    const std::string m_name;
};

}} // boost::async::scheduler

#endif /* BOOST_ASYNCHRONOUS_TCP_SERVER_SCHEDULER_HPP */
