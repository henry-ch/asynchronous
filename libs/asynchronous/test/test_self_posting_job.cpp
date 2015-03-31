// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <vector>
#include <set>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/stealing_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/stealing_multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/composite_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/extensions/asio/asio_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/scheduler/tss_scheduler.hpp>
#include <boost/asynchronous/diagnostics/any_loggable.hpp>

#include "test_common.hpp"
#include <boost/test/unit_test.hpp>

using namespace boost::asynchronous::test;

namespace
{
// main thread id
boost::thread::id main_thread_id;
std::vector<boost::thread::id> sched_ids;

struct DummyJob
{
    DummyJob(boost::shared_ptr<boost::promise<void> > done,std::string const& name=""):m_done(done),m_name(name){}
    void operator()()const
    {
        BOOST_CHECK_MESSAGE(contains_id(sched_ids.begin(),sched_ids.end(),boost::this_thread::get_id()),"2nd work called in wrong thread.");
        BOOST_CHECK_MESSAGE(boost::this_thread::get_id()!=main_thread_id,"2nd work called in main thread.");
        m_done->set_value();
    }
    boost::shared_ptr<boost::promise<void> > m_done;
    // for logging test cases
    std::string m_name;
    void set_name(std::string const& name)
    {
        m_name = name;
    }
    std::string get_name() const
    {
        return m_name;
    }
    void set_posted_time()
    {
    }
    void set_started_time()
    {
    }
    void set_failed()
    {
    }
    void set_finished_time()
    {
    }
    bool get_failed()const{return false;}

    boost::asynchronous::diagnostic_item<boost::chrono::high_resolution_clock> get_diagnostic_item() const
    {
        return boost::asynchronous::diagnostic_item<boost::chrono::high_resolution_clock>();
    }
};
typedef boost::asynchronous::any_loggable<boost::chrono::high_resolution_clock> servant_job;
typedef std::map<std::string,std::list<boost::asynchronous::diagnostic_item<boost::chrono::high_resolution_clock> > > diag_type;
struct PostJob
{
    PostJob(boost::shared_ptr<boost::promise<void> > done,std::string const& name=""):m_done(done),m_name(name){}
    void operator()()const
    {
        BOOST_CHECK_MESSAGE(contains_id(sched_ids.begin(),sched_ids.end(),boost::this_thread::get_id()),"1st work called in wrong thread.");
        BOOST_CHECK_MESSAGE(boost::this_thread::get_id()!=main_thread_id,"1st work called in main thread.");
        boost::asynchronous::any_weak_scheduler<servant_job> weak_scheduler = boost::asynchronous::get_thread_scheduler<servant_job>();
        boost::asynchronous::any_shared_scheduler<servant_job> locked_scheduler = weak_scheduler.lock();
        if (locked_scheduler.is_valid())
        {
            locked_scheduler.post(DummyJob(m_done,"DummyJob"));
        }
    }
    boost::shared_ptr<boost::promise<void> > m_done;
    std::string m_name;
    void set_name(std::string const& name)
    {
        m_name = name;
    }
    std::string get_name() const
    {
        return m_name;
    }
    void set_posted_time()
    {
    }
    void set_started_time()
    {
    }
    void set_failed()
    {
    }
    void set_finished_time()
    {
    }
    bool get_failed()const{return false;}
    boost::asynchronous::diagnostic_item<boost::chrono::high_resolution_clock> get_diagnostic_item() const
    {
        return boost::asynchronous::diagnostic_item<boost::chrono::high_resolution_clock>();
    }
};

}

BOOST_AUTO_TEST_CASE( self_posting_job_threadpool_scheduler)
{
    main_thread_id = boost::this_thread::get_id();

    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(4);

    sched_ids = scheduler.thread_ids();
    boost::shared_ptr<boost::promise<void> > done (new boost::promise<void>);
    boost::future<void> fu = done->get_future();

    scheduler.post([done]()
        {
            BOOST_CHECK_MESSAGE(contains_id(sched_ids.begin(),sched_ids.end(),boost::this_thread::get_id()),"1st work called in wrong thread.");
            BOOST_CHECK_MESSAGE(boost::this_thread::get_id()!=main_thread_id,"1st work called in main thread.");
            boost::asynchronous::any_weak_scheduler<> weak_scheduler = boost::asynchronous::get_thread_scheduler<>();
            boost::asynchronous::any_shared_scheduler<> locked_scheduler = weak_scheduler.lock();
            if (locked_scheduler.is_valid())
            {
                locked_scheduler.post(DummyJob(done));
            }
        });
    fu.get();
}

BOOST_AUTO_TEST_CASE( self_posting_job_stealing_multiqueue_threadpool_scheduler)
{
    main_thread_id = boost::this_thread::get_id();

    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                        boost::asynchronous::stealing_multiqueue_threadpool_scheduler<
                                                      boost::asynchronous::lockfree_queue<>,
                                                      boost::asynchronous::default_find_position<>,
                                                      boost::asynchronous::no_cpu_load_saving,true >>(4);

    sched_ids = scheduler.thread_ids();
    boost::shared_ptr<boost::promise<void> > done (new boost::promise<void>);
    boost::future<void> fu = done->get_future();

    scheduler.post([done]()
        {
            BOOST_CHECK_MESSAGE(contains_id(sched_ids.begin(),sched_ids.end(),boost::this_thread::get_id()),"1st work called in wrong thread.");
            BOOST_CHECK_MESSAGE(boost::this_thread::get_id()!=main_thread_id,"1st work called in main thread.");
            boost::asynchronous::any_weak_scheduler<> weak_scheduler = boost::asynchronous::get_thread_scheduler<>();
            boost::asynchronous::any_shared_scheduler<> locked_scheduler = weak_scheduler.lock();
            if (locked_scheduler.is_valid())
            {
                locked_scheduler.post(DummyJob(done));
            }
        });
    fu.get();
}

BOOST_AUTO_TEST_CASE( self_posting_job_stealing_threadpool_scheduler)
{
    main_thread_id = boost::this_thread::get_id();

    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::stealing_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>,
                                                                        boost::asynchronous::no_cpu_load_saving,true>>(4);

    sched_ids = scheduler.thread_ids();
    boost::shared_ptr<boost::promise<void> > done (new boost::promise<void>);
    boost::future<void> fu = done->get_future();

    scheduler.post([done]()
        {
            BOOST_CHECK_MESSAGE(contains_id(sched_ids.begin(),sched_ids.end(),boost::this_thread::get_id()),"1st work called in wrong thread.");
            BOOST_CHECK_MESSAGE(boost::this_thread::get_id()!=main_thread_id,"1st work called in main thread.");
            boost::asynchronous::any_weak_scheduler<> weak_scheduler = boost::asynchronous::get_thread_scheduler<>();
            boost::asynchronous::any_shared_scheduler<> locked_scheduler = weak_scheduler.lock();
            if (locked_scheduler.is_valid())
            {
                locked_scheduler.post(DummyJob(done));
            }
        });
    fu.get();
}

BOOST_AUTO_TEST_CASE( self_posting_job_multiqueue_threadpool_scheduler)
{
    main_thread_id = boost::this_thread::get_id();

    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(4);

    sched_ids = scheduler.thread_ids();
    boost::shared_ptr<boost::promise<void> > done (new boost::promise<void>);
    boost::future<void> fu = done->get_future();

    scheduler.post([done]()
        {
            BOOST_CHECK_MESSAGE(contains_id(sched_ids.begin(),sched_ids.end(),boost::this_thread::get_id()),"1st work called in wrong thread.");
            BOOST_CHECK_MESSAGE(boost::this_thread::get_id()!=main_thread_id,"1st work called in main thread.");
            boost::asynchronous::any_weak_scheduler<> weak_scheduler = boost::asynchronous::get_thread_scheduler<>();
            boost::asynchronous::any_shared_scheduler<> locked_scheduler = weak_scheduler.lock();
            if (locked_scheduler.is_valid())
            {
                locked_scheduler.post(DummyJob(done));
            }
        });
    fu.get();
}

BOOST_AUTO_TEST_CASE( self_posting_job_single_thread_scheduler)
{
    main_thread_id = boost::this_thread::get_id();

    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<boost::asynchronous::lockfree_queue<>>>();

    sched_ids = scheduler.thread_ids();
    boost::shared_ptr<boost::promise<void> > done (new boost::promise<void>);
    boost::future<void> fu = done->get_future();

   scheduler.post([done]()
        {
            BOOST_CHECK_MESSAGE(contains_id(sched_ids.begin(),sched_ids.end(),boost::this_thread::get_id()),"1st work called in wrong thread.");
            BOOST_CHECK_MESSAGE(boost::this_thread::get_id()!=main_thread_id,"1st work called in main thread.");
            boost::asynchronous::any_weak_scheduler<> weak_scheduler = boost::asynchronous::get_thread_scheduler<>();
            boost::asynchronous::any_shared_scheduler<> locked_scheduler = weak_scheduler.lock();
            if (locked_scheduler.is_valid())
            {
                locked_scheduler.post(DummyJob(done));
            }
        });
    fu.get();
}

BOOST_AUTO_TEST_CASE( self_posting_job_composite_threadpool_scheduler)
{
    main_thread_id = boost::this_thread::get_id();

    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(4);
    auto scheduler2 = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(4);

    auto worker = boost::asynchronous::make_shared_scheduler_proxy<
                    boost::asynchronous::composite_threadpool_scheduler<>>(scheduler,scheduler2);


    sched_ids = scheduler.thread_ids();
    boost::shared_ptr<boost::promise<void> > done (new boost::promise<void>);
    boost::future<void> fu = done->get_future();

   scheduler.post([done]()
        {
            BOOST_CHECK_MESSAGE(contains_id(sched_ids.begin(),sched_ids.end(),boost::this_thread::get_id()),"1st work called in wrong thread.");
            BOOST_CHECK_MESSAGE(boost::this_thread::get_id()!=main_thread_id,"1st work called in main thread.");
            boost::asynchronous::any_weak_scheduler<> weak_scheduler = boost::asynchronous::get_thread_scheduler<>();
            boost::asynchronous::any_shared_scheduler<> locked_scheduler = weak_scheduler.lock();
            if (locked_scheduler.is_valid())
            {
                locked_scheduler.post(DummyJob(done));
            }
        });
    fu.get();
}

BOOST_AUTO_TEST_CASE( self_posting_job_asio_scheduler)
{
    main_thread_id = boost::this_thread::get_id();

    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::asio_scheduler<>>(1);

    sched_ids = scheduler.thread_ids();
    boost::shared_ptr<boost::promise<void> > done (new boost::promise<void>);
    boost::future<void> fu = done->get_future();

   scheduler.post([done]()
        {
            BOOST_CHECK_MESSAGE(contains_id(sched_ids.begin(),sched_ids.end(),boost::this_thread::get_id()),"1st work called in wrong thread.");
            BOOST_CHECK_MESSAGE(boost::this_thread::get_id()!=main_thread_id,"1st work called in main thread.");
            boost::asynchronous::any_weak_scheduler<> weak_scheduler = boost::asynchronous::get_thread_scheduler<>();
            boost::asynchronous::any_shared_scheduler<> locked_scheduler = weak_scheduler.lock();
            if (locked_scheduler.is_valid())
            {
                locked_scheduler.post(DummyJob(done));
            }
        });
    fu.get();
}

BOOST_AUTO_TEST_CASE( self_posting_job_threadpool_scheduler_log)
{
    main_thread_id = boost::this_thread::get_id();

    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<servant_job>>>(4);

    sched_ids = scheduler.thread_ids();
    boost::shared_ptr<boost::promise<void> > done (new boost::promise<void>);
    boost::future<void> fu = done->get_future();

    scheduler.post(PostJob(done,"PostJob"));
    fu.get();
}
