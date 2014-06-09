
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
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/queue/threadsafe_list.hpp>
#include <boost/asynchronous/queue/any_queue_container.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/test/unit_test.hpp>
 
namespace
{
// main thread id
boost::thread::id main_thread_id;
unsigned int cpt=0;

struct Servant : boost::asynchronous::trackable_servant<>
{
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler) 
        : boost::asynchronous::trackable_servant<>(scheduler,
                                                   boost::asynchronous::create_shared_scheduler_proxy(
                                                       new boost::asynchronous::threadpool_scheduler<
                                                                boost::asynchronous::threadsafe_list<> >(3)))
    {
    }
    void start_async_work(boost::shared_ptr<boost::promise<void> > res)
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work not posted.");
        BOOST_CHECK_MESSAGE(cpt==0,"start_async_work should have been called first.");
        ++cpt;
        post_callback(
               [](){},// work
               [res](boost::future<void> ){
                  BOOST_CHECK_MESSAGE(cpt==2,"start_async_work's callback' should have been called third.");++cpt;res->set_value();}// callback functor.
               ,"",
               3,3
        );
    }
    void start_async_work2()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work2 not posted.");
        BOOST_CHECK_MESSAGE(cpt==1,"start_async_work2 should have been called second.");
        ++cpt;
    }
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    BOOST_ASYNC_SERVANT_POST_CTOR(3)
    BOOST_ASYNC_SERVANT_POST_DTOR(4)
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work,1)
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work2,2)
};

}

BOOST_AUTO_TEST_CASE( test_queue_container_prio_single_scheduler )
{        
    auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                            new boost::asynchronous::single_thread_scheduler<
                                    boost::asynchronous::any_queue_container<> >
                    (boost::asynchronous::any_queue_container_config<boost::asynchronous::threadsafe_list<> >(1),
                     boost::asynchronous::any_queue_container_config<boost::asynchronous::lockfree_queue<> >(3)
                     ));
    
    main_thread_id = boost::this_thread::get_id();   
    ServantProxy proxy(scheduler);
    
    // create blocker to get us time
    boost::promise<void> p;
    boost::shared_future<void> fu=p.get_future();
    auto blocking = [fu]() mutable {fu.get();};
    scheduler.post(blocking);
    
    boost::shared_ptr<boost::promise<void> > res_p(new boost::promise<void>);
    boost::shared_future<void> fudone = res_p->get_future();
    boost::shared_future<void> fuv = proxy.start_async_work(res_p);
    boost::shared_future<void> fuv2 = proxy.start_async_work2();
    
    // let's start!
    p.set_value();

    fuv.get();
    fuv2.get();
    fudone.get();
    BOOST_CHECK_MESSAGE(cpt==3,"start_async_work's task's callback not called.");
}

