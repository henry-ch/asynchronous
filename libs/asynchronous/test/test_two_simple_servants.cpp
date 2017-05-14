#include <iostream>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>

#include <boost/test/unit_test.hpp>

#include "test_common.hpp"

using namespace std;
using namespace boost::asynchronous::test;

namespace
{
// main thread id
boost::thread::id main_thread_id;

struct non_default_constructible
{
    non_default_constructible()=delete;
    non_default_constructible(int i)
    {
        m_data=i;
    }
    int m_data=0;
};

struct Servant : boost::asynchronous::trackable_servant<>
{
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler,int data) 
        : boost::asynchronous::trackable_servant<>(scheduler)
        , m_data(data)
    {
        BOOST_CHECK_MESSAGE(m_data==42,"servant got incorrect data in ctor. Expected 42.");
        BOOST_CHECK_MESSAGE(main_thread_id==boost::this_thread::get_id(),"servant ctor posted instead of being called directly.");        
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
    }
    non_default_constructible foo()const
    {
        boost::asynchronous::any_shared_scheduler<> s = get_scheduler().lock();
        std::vector<boost::thread::id> ids = s.thread_ids();
        BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"foo running in wrong thread.");
        return non_default_constructible(5);
    }
    void foobar(int i, char c)const
    {
        BOOST_CHECK_MESSAGE(i==1,"servant got incorrect data in foobar. Expected 1.");
        BOOST_CHECK_MESSAGE(c=='a',"servant got incorrect data in foobar. Expected 'a'.");
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant foobar not posted.");
        boost::asynchronous::any_shared_scheduler<> s = get_scheduler().lock();
        std::vector<boost::thread::id> ids = s.thread_ids();
        BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"foo running in wrong thread.");        
    }
    int m_data;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s, int data):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s, data)
    {}
    BOOST_ASYNC_UNSAFE_MEMBER(foo)
    BOOST_ASYNC_UNSAFE_MEMBER(foobar)
};

struct Servant2 : boost::asynchronous::trackable_servant<>
{
    typedef int simple_ctor;
    
    Servant2(boost::asynchronous::any_weak_scheduler<> scheduler,ServantProxy worker)
        :boost::asynchronous::trackable_servant<>(scheduler)
        ,m_worker(worker)
    {
        BOOST_CHECK_MESSAGE(main_thread_id==boost::this_thread::get_id(),"servant2 ctor posted instead of being called directly.");  
    }
    ~Servant2()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant2 dtor not posted.");
    }
    boost::future<void> doIt()
    {
        std::shared_ptr<boost::promise<void> > p (new boost::promise<void>);
        boost::future<void> fu = p->get_future();
        boost::thread::id this_thread_id = boost::this_thread::get_id();
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant2 doIt not posted.");
        call_callback(m_worker.get_proxy(),
                      m_worker.foo(),
                      // callback functor.
                      [this_thread_id](boost::asynchronous::expected<non_default_constructible> res)
                      {
                          BOOST_REQUIRE_MESSAGE(this_thread_id==boost::this_thread::get_id(),"servant callback in wrong thread.");
                          BOOST_REQUIRE_MESSAGE(res.get().m_data==5,"foo delivered wrong response.");
                      }
        );
        call_callback(m_worker.get_proxy(),
                      m_worker.foobar(1,'a'),
                      // callback functor.
                      [this_thread_id,p](boost::asynchronous::expected<void> )
                      {
                            BOOST_CHECK_MESSAGE(this_thread_id==boost::this_thread::get_id(),"servant callback in wrong thread.");
                            p->set_value();
                      }
        );
        return fu;
    }
private:
    ServantProxy m_worker;
};

class ServantProxy2 : public boost::asynchronous::servant_proxy<ServantProxy2,Servant2>
{
public:
    template <class Scheduler>
    ServantProxy2(Scheduler s, ServantProxy worker):
        boost::asynchronous::servant_proxy<ServantProxy2,Servant2>(s, worker)
    {}

    BOOST_ASYNC_FUTURE_MEMBER(doIt)
};

}

BOOST_AUTO_TEST_CASE( test_two_servants )
{ 
    {
        main_thread_id = boost::this_thread::get_id();
        // with c++11
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                    boost::asynchronous::single_thread_scheduler<
                          boost::asynchronous::lockfree_queue<>>>();
        
        auto scheduler2 = boost::asynchronous::make_shared_scheduler_proxy<
                    boost::asynchronous::single_thread_scheduler<
                          boost::asynchronous::lockfree_queue<>>>();

        {
            ServantProxy proxy(scheduler,42);
            ServantProxy2 proxy2(scheduler2,proxy);
            boost::future<boost::future<void> > fu = proxy2.doIt();
            boost::future<void> fu2 = fu.get();
            fu2.get();
        }
    }
}

