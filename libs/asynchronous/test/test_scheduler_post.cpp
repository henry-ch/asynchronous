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
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/queue/any_queue_container.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/stealing_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/stealing_multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/composite_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/extensions/tbb/tbb_concurrent_queue.hpp>
#include "test_common.hpp"

//#define BOOST_TEST_MODULE MyTest2
//#define BOOST_TEST_DYN_LINK
//#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace boost::asynchronous::test;

namespace
{
struct DummyJob
{
    DummyJob(boost::shared_ptr<boost::promise<boost::thread::id> > p):m_done(p){}
    void operator()()const
    {
        //std::cout << "DummyJob called in thread:" << boost::this_thread::get_id() << std::endl;
        boost::this_thread::sleep(boost::posix_time::milliseconds(50));
        m_done->set_value(boost::this_thread::get_id());
    }
    // to check we ran in a correct thread
    boost::shared_ptr<boost::promise<boost::thread::id> > m_done;
};

struct BlockingJob
{
    BlockingJob(boost::shared_future<void> fu):m_ready(fu){}
    void operator()()
    {
        m_ready.get();
    }
    boost::shared_future<void> m_ready;
};
}

BOOST_AUTO_TEST_CASE( test_lockfree_max_size )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                        boost::asynchronous::single_thread_scheduler<
                            boost::asynchronous::lockfree_queue<boost::asynchronous::any_callable,boost::asynchronous::lockfree_size_max_size>>>();

    boost::promise<void> p;
    scheduler.post(boost::asynchronous::any_callable(BlockingJob(p.get_future())));
    for (auto i=0; i< 5 ; ++i)
    {
        scheduler.post([](){});
    }
    boost::shared_ptr<boost::promise<void>> pend = boost::make_shared<boost::promise<void>>();
    auto fu = pend->get_future();
    scheduler.post([pend](){pend->set_value();});
    p.set_value();
    fu.get();

    auto max_size = scheduler.get_max_queue_size()[0];
    BOOST_CHECK_MESSAGE((max_size == 6) || (max_size == 7),"wrong get_max_queue_size");
}

BOOST_AUTO_TEST_CASE( post_single_thread_scheduler )
{        
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                    boost::asynchronous::lockfree_queue<>>>();
    
    std::vector<boost::thread::id> sids = scheduler.thread_ids();
    BOOST_CHECK_MESSAGE(number_of_threads(sids.begin(),sids.end())==1,"scheduler has wrong number of threads");
    std::vector<boost::shared_future<boost::thread::id> > fus;
    for (int i = 0 ; i< 10 ; ++i)
    {
        boost::shared_ptr<boost::promise<boost::thread::id> > p = boost::make_shared<boost::promise<boost::thread::id> >();
        fus.push_back(p->get_future());
        scheduler.post(boost::asynchronous::any_callable(DummyJob(p)));
    }
    boost::wait_for_all(fus.begin(), fus.end());
    std::set<boost::thread::id> ids;
    for (std::vector<boost::shared_future<boost::thread::id> >::iterator it = fus.begin(); it != fus.end() ; ++it)
    {
        boost::thread::id tid = (*it).get();
        std::vector<boost::thread::id> itids = scheduler.thread_ids();
        BOOST_CHECK_MESSAGE(contains_id(itids.begin(),itids.end(),tid),"task executed in the wrong thread");
        ids.insert(tid);
    }
    BOOST_CHECK_MESSAGE(ids.size() ==1,"too many threads used in scheduler");
}

BOOST_AUTO_TEST_CASE( post_threadpool_scheduler )
{        
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(3);
    
    std::vector<boost::thread::id> sids = scheduler.thread_ids();
    BOOST_CHECK_MESSAGE(number_of_threads(sids.begin(),sids.end())==3,"scheduler has wrong number of threads");
    std::vector<boost::shared_future<boost::thread::id> > fus;
    for (int i = 0 ; i< 10 ; ++i)
    {
        boost::shared_ptr<boost::promise<boost::thread::id> > p = boost::make_shared<boost::promise<boost::thread::id> >();
        fus.push_back(p->get_future());
        scheduler.post(boost::asynchronous::any_callable(DummyJob(p)));
    }
    boost::wait_for_all(fus.begin(), fus.end());
    std::set<boost::thread::id> ids;
    
    for (std::vector<boost::shared_future<boost::thread::id> >::iterator it = fus.begin(); it != fus.end() ; ++it)
    {
        boost::thread::id tid = (*it).get();
        std::vector<boost::thread::id> itids = scheduler.thread_ids();
        BOOST_CHECK_MESSAGE(contains_id(itids.begin(),itids.end(),tid),"task executed in the wrong thread");
        ids.insert(tid);
    }
    BOOST_CHECK_MESSAGE(ids.size() <=3,"too many threads used in scheduler");
}
BOOST_AUTO_TEST_CASE( post_stealing_threadpool_scheduler )
{        
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                boost::asynchronous::stealing_threadpool_scheduler<
                    boost::asynchronous::lockfree_queue<>,boost::asynchronous::no_cpu_load_saving,true>>(4,10);
    
    std::vector<boost::thread::id> sids = scheduler.thread_ids();
    BOOST_CHECK_MESSAGE(number_of_threads(sids.begin(),sids.end())==4,"scheduler has wrong number of threads");
    std::vector<boost::shared_future<boost::thread::id> > fus;
    for (int i = 0 ; i< 10 ; ++i)
    {
        boost::shared_ptr<boost::promise<boost::thread::id> > p = boost::make_shared<boost::promise<boost::thread::id> >();
        fus.push_back(p->get_future());
        scheduler.post(boost::asynchronous::any_callable(DummyJob(p)));
    }
    boost::wait_for_all(fus.begin(), fus.end());
    std::set<boost::thread::id> ids;
    
    for (std::vector<boost::shared_future<boost::thread::id> >::iterator it = fus.begin(); it != fus.end() ; ++it)
    {
        boost::thread::id tid = (*it).get();
        std::vector<boost::thread::id> itids = scheduler.thread_ids();
        BOOST_CHECK_MESSAGE(contains_id(itids.begin(),itids.end(),tid),"task executed in the wrong thread");
        ids.insert(tid);
    }
    BOOST_CHECK_MESSAGE(ids.size() <=4,"too many threads used in scheduler");
}

BOOST_AUTO_TEST_CASE( post_stealing_multiqueue_threadpool_scheduler )
{        
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                boost::asynchronous::stealing_multiqueue_threadpool_scheduler<
                        boost::asynchronous::lockfree_queue<>,boost::asynchronous::default_find_position<>,
                                boost::asynchronous::no_cpu_load_saving,true>>
                            (4,10);
    
    std::vector<boost::thread::id> sids = scheduler.thread_ids();
    BOOST_CHECK_MESSAGE(number_of_threads(sids.begin(),sids.end())==4,"scheduler has wrong number of threads");
    std::vector<boost::shared_future<boost::thread::id> > fus;
    for (int i = 0 ; i< 10 ; ++i)
    {
        boost::shared_ptr<boost::promise<boost::thread::id> > p = boost::make_shared<boost::promise<boost::thread::id> >();
        fus.push_back(p->get_future());
        scheduler.post(boost::asynchronous::any_callable(DummyJob(p)));
    }
    boost::wait_for_all(fus.begin(), fus.end());
    std::set<boost::thread::id> ids;
    
    for (std::vector<boost::shared_future<boost::thread::id> >::iterator it = fus.begin(); it != fus.end() ; ++it)
    {
        boost::thread::id tid = (*it).get();
        std::vector<boost::thread::id> itids = scheduler.thread_ids();
        BOOST_CHECK_MESSAGE(contains_id(itids.begin(),itids.end(),tid),"task executed in the wrong thread");
        ids.insert(tid);
    }
    BOOST_CHECK_MESSAGE(ids.size() <=4,"too many threads used in scheduler");
}

BOOST_AUTO_TEST_CASE( post_multiqueue_threadpool_scheduler )
{        
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                boost::asynchronous::multiqueue_threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(4);
    
    std::vector<boost::thread::id> sids = scheduler.thread_ids();
    BOOST_CHECK_MESSAGE(number_of_threads(sids.begin(),sids.end())==4,"scheduler has wrong number of threads");
    std::vector<boost::shared_future<boost::thread::id> > fus;
    for (int i = 0 ; i< 10 ; ++i)
    {
        boost::shared_ptr<boost::promise<boost::thread::id> > p = boost::make_shared<boost::promise<boost::thread::id> >();
        fus.push_back(p->get_future());
        scheduler.post(boost::asynchronous::any_callable(DummyJob(p)));
    }
    boost::wait_for_all(fus.begin(), fus.end());
    std::set<boost::thread::id> ids;
    
    for (std::vector<boost::shared_future<boost::thread::id> >::iterator it = fus.begin(); it != fus.end() ; ++it)
    {
        boost::thread::id tid = (*it).get();
        std::vector<boost::thread::id> itids = scheduler.thread_ids();
        BOOST_CHECK_MESSAGE(contains_id(itids.begin(),itids.end(),tid),"task executed in the wrong thread");
        ids.insert(tid);
    }
    BOOST_CHECK_MESSAGE(ids.size() <=4,"too many threads used in scheduler");
}

BOOST_AUTO_TEST_CASE( post_composite_threadpool_scheduler )
{        
    auto tp = boost::asynchronous::make_shared_scheduler_proxy<
                boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>> (3);
    auto tp2 = boost::asynchronous::make_shared_scheduler_proxy<
                boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>> (4);

    BOOST_CHECK_MESSAGE(tp != tp2,"tp and tp2 should not be equal");
    BOOST_CHECK_MESSAGE(!(tp == tp2),"tp and tp2 should not be equal");
    BOOST_CHECK_MESSAGE(tp == tp,"tp and tp should not be equal");
    
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                boost::asynchronous::composite_threadpool_scheduler<>>(tp,tp2);

    std::vector<boost::thread::id> sids = scheduler.thread_ids();
    BOOST_CHECK_MESSAGE(number_of_threads(sids.begin(),sids.end())==7,"scheduler has wrong number of threads");
    std::vector<boost::shared_future<boost::thread::id> > fus;
    for (int i = 0 ; i< 10 ; ++i)
    {
        boost::shared_ptr<boost::promise<boost::thread::id> > p = boost::make_shared<boost::promise<boost::thread::id> >();
        fus.push_back(p->get_future());
        scheduler.post(boost::asynchronous::any_callable(DummyJob(p)));
    }
    boost::wait_for_all(fus.begin(), fus.end());
    std::set<boost::thread::id> ids;
    
    for (std::vector<boost::shared_future<boost::thread::id> >::iterator it = fus.begin(); it != fus.end() ; ++it)
    {
        boost::thread::id tid = (*it).get();
        std::vector<boost::thread::id> itids = scheduler.thread_ids();
        BOOST_CHECK_MESSAGE(contains_id(itids.begin(),itids.end(),tid),"task executed in the wrong thread");
        ids.insert(tid);
    }
    BOOST_CHECK_MESSAGE(ids.size() <=7,"too many threads used in scheduler");
}

BOOST_AUTO_TEST_CASE( post_multiqueue_threadpool_scheduler_tbb_concurrent_queue )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                boost::asynchronous::multiqueue_threadpool_scheduler<boost::asynchronous::tbb_concurrent_queue<>>>(4);

    std::vector<boost::thread::id> sids = scheduler.thread_ids();
    BOOST_CHECK_MESSAGE(number_of_threads(sids.begin(),sids.end())==4,"scheduler has wrong number of threads");
    std::vector<boost::shared_future<boost::thread::id> > fus;
    for (int i = 0 ; i< 10 ; ++i)
    {
        boost::shared_ptr<boost::promise<boost::thread::id> > p = boost::make_shared<boost::promise<boost::thread::id> >();
        fus.push_back(p->get_future());
        scheduler.post(boost::asynchronous::any_callable(DummyJob(p)));
    }
    boost::wait_for_all(fus.begin(), fus.end());
    std::set<boost::thread::id> ids;

    for (std::vector<boost::shared_future<boost::thread::id> >::iterator it = fus.begin(); it != fus.end() ; ++it)
    {
        boost::thread::id tid = (*it).get();
        std::vector<boost::thread::id> itids = scheduler.thread_ids();
        BOOST_CHECK_MESSAGE(contains_id(itids.begin(),itids.end(),tid),"task executed in the wrong thread");
        ids.insert(tid);
    }
    BOOST_CHECK_MESSAGE(ids.size() <=4,"too many threads used in scheduler");
}
