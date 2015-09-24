
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
#include <boost/asynchronous/queue/threadsafe_list.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/stealing_multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/composite_threadpool_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
 
using namespace std;
using namespace boost::asynchronous::test;

namespace
{
// main thread id
boost::thread::id main_thread_id;

struct Servant : boost::asynchronous::trackable_servant<>
{
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler)
        // for testing purpose
        , m_promise(new boost::promise<int>)
    {
        // create a composite threadpool made of:
        // a multiqueue_threadpool_scheduler, 0 thread
        // This scheduler does not steal from other schedulers, but will lend its queues for stealing
        auto tp = boost::asynchronous::make_shared_scheduler_proxy<
                    boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>> (0);
        // a stealing_multiqueue_threadpool_scheduler, 3 threads, each with a threadsafe_list
        // this scheduler will steal from other schedulers if it can. In this case it will manage only with tp, not tp3
        auto tp2 = boost::asynchronous::make_shared_scheduler_proxy<
                    boost::asynchronous::stealing_multiqueue_threadpool_scheduler<boost::asynchronous::threadsafe_list<>>> (3);
        // composite pool made of the previous 2
        auto tp_worker =
                boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::composite_threadpool_scheduler<>>(tp,tp2);

        m_tp2_ids = tp2.thread_ids();
        // use it as worker
        set_worker(tp_worker);
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
    }
    // called when task done, in our thread
    void on_callback(int res)
    {
        // inform test caller
        m_promise->set_value(res);
    }
    // call to this is posted and executes in our (safe) single-thread scheduler
    boost::future<int> start_async_work()
    {
        boost::thread::id ao_id = boost::this_thread::get_id();
        // for testing purpose
        boost::future<int> fu = m_promise->get_future();
        // start long tasks in threadpool (first lambda) and callback in our thread
        post_callback(
               [this](){
                        BOOST_CHECK_MESSAGE(contains_id(this->m_tp2_ids.begin(),this->m_tp2_ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                        return 42; //always...
                    }// work
                ,
               [this,ao_id](boost::asynchronous::expected<int> res){
                        BOOST_CHECK_MESSAGE(ao_id == boost::this_thread::get_id(),"servant callback in wrong thread.");
                        this->on_callback(res.get());
               },// callback functor.
               "",1,0
        );
        return fu;
    }
private:
// for testing
boost::shared_ptr<boost::promise<int> > m_promise;
std::vector<boost::thread::id> m_tp2_ids;
};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    // caller will get a future
#ifndef _MSC_VER
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work)
#else
    BOOST_ASYNC_FUTURE_MEMBER_1(start_async_work)
#endif
};

}

BOOST_AUTO_TEST_CASE( test_composite_stealing )
{        
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                    boost::asynchronous::lockfree_queue<>>>();
    
    main_thread_id = boost::this_thread::get_id();   
    ServantProxy proxy(scheduler);
    boost::future<boost::future<int> > fuv = proxy.start_async_work();
    try
    {
        boost::future<int> resfuv = fuv.get();
        BOOST_CHECK_MESSAGE(resfuv.get()==42,"servant work return wrong result.");
    }
    catch(...)
    {
        BOOST_FAIL( "unexpected exception" );
    }
}


