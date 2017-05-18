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

struct Servant : boost::asynchronous::trackable_servant<>
{
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler,int data)
        : boost::asynchronous::trackable_servant<>(scheduler)
        , m_data(data)
    {
        BOOST_CHECK_MESSAGE(m_data==42,"servant got incorrect data in ctor. Expected 42.");
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant ctor in wrong thread.");
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
    }
    int foo()const
    {
        boost::asynchronous::any_shared_scheduler<> s = get_scheduler().lock();
        std::vector<boost::thread::id> ids = s.thread_ids();
        BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"foo running in wrong thread.");
        return 5;
    }
    int m_data;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s, boost::future<std::shared_ptr<Servant> > servant):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s, std::move(servant))
    {}
    BOOST_ASYNC_FUTURE_MEMBER(foo)
};

struct Servant2 : boost::asynchronous::trackable_servant<>
{
    typedef int simple_ctor;
    Servant2(boost::asynchronous::any_weak_scheduler<> scheduler)
        :boost::asynchronous::trackable_servant<>(scheduler)
    {
        BOOST_CHECK_MESSAGE(main_thread_id==boost::this_thread::get_id(),"servant2 ctor posted instead of being called directly.");
    }
    ~Servant2()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant2 dtor not posted.");
    }
    std::shared_ptr<Servant> get_servant2()
    {
        return std::make_shared<Servant>(get_scheduler(),42);
    }
private:
};

class ServantProxy2 : public boost::asynchronous::servant_proxy<ServantProxy2,Servant2>
{
public:
    template <class Scheduler>
    ServantProxy2(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy2,Servant2>(s)
    {}

    BOOST_ASYNC_FUTURE_MEMBER(get_servant2)
};

}

BOOST_AUTO_TEST_CASE( test_returned_servant )
{
    {
        main_thread_id = boost::this_thread::get_id();
        // with c++11
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                    boost::asynchronous::single_thread_scheduler<
                          boost::asynchronous::lockfree_queue<>>>();


        {
            ServantProxy2 proxy2(scheduler);
            ServantProxy proxy( scheduler,std::move(proxy2.get_servant2()));
            auto fu = proxy.foo();
            int res = fu.get();
            BOOST_CHECK_MESSAGE(res==5,"servant returned incorrect data. Expected 5.");
        }
    }
}


