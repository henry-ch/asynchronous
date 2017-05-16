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
typedef boost::asynchronous::any_loggable servant_job;
typedef std::map<std::string,std::list<boost::asynchronous::diagnostic_item> > diag_type;

// main thread id
boost::thread::id main_thread_id;

struct Servant : boost::asynchronous::trackable_servant<servant_job,servant_job>
{
    typedef int simple_ctor;
    typedef int requires_weak_scheduler;
    Servant(boost::asynchronous::any_weak_scheduler<servant_job> scheduler,int data) 
        : boost::asynchronous::trackable_servant<servant_job,servant_job>(scheduler)
        , m_data(data)
    {
        BOOST_CHECK_MESSAGE(m_data==42,"servant got incorrect data in ctor. Expected 42.");
        BOOST_CHECK_MESSAGE(main_thread_id==boost::this_thread::get_id(),"servant ctor posted instead of being called directly.");        
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
    }
    int foo()const
    {
        boost::asynchronous::any_shared_scheduler<servant_job> s = get_scheduler().lock();
        std::vector<boost::thread::id> ids = s.thread_ids();
        BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"foo running in wrong thread.");
        return 5;
    }
    void foobar(int i, char c)const
    {
        BOOST_CHECK_MESSAGE(i==1,"servant got incorrect data in foobar. Expected 1.");
        BOOST_CHECK_MESSAGE(c=='a',"servant got incorrect data in foobar. Expected 'a'.");
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant foobar not posted.");
        boost::asynchronous::any_shared_scheduler<servant_job> s = get_scheduler().lock();
        std::vector<boost::thread::id> ids = s.thread_ids();
        BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"foo running in wrong thread.");        
    }
    int m_data;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant,servant_job>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s, int data):
        boost::asynchronous::servant_proxy<ServantProxy,Servant,servant_job>(s, data)
    {}
    BOOST_ASYNC_UNSAFE_MEMBER(foo)
    BOOST_ASYNC_UNSAFE_MEMBER(foobar)
};

struct Servant2 : boost::asynchronous::trackable_servant<servant_job,servant_job>
{
    typedef int simple_ctor;
    
    Servant2(boost::asynchronous::any_weak_scheduler<servant_job> scheduler,ServantProxy worker)
        :boost::asynchronous::trackable_servant<servant_job,servant_job>(scheduler)
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
                      [this_thread_id](boost::asynchronous::expected<int> res)
                      {
                          BOOST_REQUIRE_MESSAGE(this_thread_id==boost::this_thread::get_id(),"servant callback in wrong thread.");
                          BOOST_REQUIRE_MESSAGE(res.get()==5,"foo delivered wrong response.");
                      },
                      // task name for logging
                      "foo"
        );
        call_callback(m_worker.get_proxy(),
                      m_worker.foobar(1,'a'),
                      // callback functor.
                      [this_thread_id,p](boost::asynchronous::expected<void> )
                      {
                            BOOST_CHECK_MESSAGE(this_thread_id==boost::this_thread::get_id(),"servant callback in wrong thread.");
                            p->set_value();
                      },
                      // task name for logging
                      "foobar"
        );
        return fu;
    }
private:
    ServantProxy m_worker;
};

class ServantProxy2 : public boost::asynchronous::servant_proxy<ServantProxy2,Servant2,servant_job>
{
public:
    template <class Scheduler>
    ServantProxy2(Scheduler s, ServantProxy worker):
        boost::asynchronous::servant_proxy<ServantProxy2,Servant2,servant_job>(s, worker)
    {}

    BOOST_ASYNC_FUTURE_MEMBER(doIt)
};

}

BOOST_AUTO_TEST_CASE( test_two_servants_log )
{ 
    {
        main_thread_id = boost::this_thread::get_id();
        // with c++11
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                    boost::asynchronous::single_thread_scheduler<
                          boost::asynchronous::lockfree_queue<servant_job>>>();
        
        auto scheduler2 = boost::asynchronous::make_shared_scheduler_proxy<
                    boost::asynchronous::single_thread_scheduler<
                          boost::asynchronous::lockfree_queue<servant_job>>>();

        {
            ServantProxy proxy(scheduler,42);
            ServantProxy2 proxy2(scheduler2,proxy);
            boost::future<boost::future<void> > fu = proxy2.doIt();
            boost::future<void> fu2 = fu.get();
            fu2.get();
        }
        {
            diag_type diag = scheduler.get_diagnostics().totals();
            bool has_foo=false;
            bool has_foobar=false;
            for (auto mit = diag.begin(); mit != diag.end() ; ++mit)
            {
                if ((*mit).first == "foo")
                    has_foo = true;
                if ((*mit).first == "foobar")
                    has_foobar = true;
                for (auto jit = (*mit).second.begin(); jit != (*mit).second.end();++jit)
                {
                    BOOST_CHECK_MESSAGE(std::chrono::nanoseconds((*jit).get_finished_time() - (*jit).get_started_time()).count() >= 0,"task finished before it started.");
                    BOOST_CHECK_MESSAGE(!(*jit).is_interrupted(),"no task should have been interrupted.");
                    BOOST_CHECK_MESSAGE(!(*jit).is_failed(),"no task should have failed.");
                }
            }
            BOOST_CHECK_MESSAGE(has_foo,"foo not found in diagnostics.");
            BOOST_CHECK_MESSAGE(has_foobar,"foobar not found in diagnostics.");
        }
        {
            diag_type diag = scheduler2.get_diagnostics().totals();
            bool has_foo=false;
            bool has_foobar=false;
            for (auto mit = diag.begin(); mit != diag.end() ; ++mit)
            {
                if ((*mit).first == "foo")
                    has_foo = true;
                if ((*mit).first == "foobar")
                    has_foobar = true;
                for (auto jit = (*mit).second.begin(); jit != (*mit).second.end();++jit)
                {
                    BOOST_CHECK_MESSAGE(std::chrono::nanoseconds((*jit).get_finished_time() - (*jit).get_started_time()).count() >= 0,"task finished before it started.");
                    BOOST_CHECK_MESSAGE(!(*jit).is_interrupted(),"no task should have been interrupted.");
                    BOOST_CHECK_MESSAGE(!(*jit).is_failed(),"no task should have failed.");
                }
            }
            BOOST_CHECK_MESSAGE(has_foo,"foo not found in diagnostics.");
            BOOST_CHECK_MESSAGE(has_foobar,"foobar not found in diagnostics.");
        }
    }
}


