#include <iostream>
#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/extensions/asio/asio_scheduler.hpp>
#include <boost/asynchronous/queue/threadsafe_list.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/servant_proxy.h>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/extensions/asio/asio_tcp_resolver.hpp>
#include <boost/asynchronous/extensions/asio/asio_tcp_servant.hpp>
#include "test_common.hpp"

#include <boost/test/unit_test.hpp>

using namespace std;
using namespace boost::asynchronous::test;

namespace
{
// main thread id
boost::thread::id main_thread_id;
unsigned resolved_count=0;


struct Servant : boost::asynchronous::trackable_servant<>, boost::asynchronous::asio_tcp_servant<Servant>
{
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler) 
        : boost::asynchronous::trackable_servant<>(scheduler,
                                                   // as worker we use an asio-based scheduler with 1 thread
                                                   boost::asynchronous::create_shared_scheduler_proxy(
                                                       new boost::asynchronous::asio_scheduler<>(1)))
        , m_tcp_resolver(get_worker())
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant ctor not posted.");
        m_threadid = boost::this_thread::get_id();
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
    } 
    void resolve(boost::shared_ptr<boost::promise<void> > p)
    {
        boost::asynchronous::any_shared_scheduler<> s = get_scheduler().lock();
        std::vector<boost::thread::id> ids = s.thread_ids();
        BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"resolve running in wrong thread.");
        boost::thread::id threadid = m_threadid;
        
        boost::asio::ip::tcp::resolver::query q("boost.org","http");
        async_resolve(m_tcp_resolver,q,
                      [p,threadid](const ::boost::system::error_code& err,boost::asio::ip::tcp::resolver::iterator )
                      {
                         BOOST_CHECK_MESSAGE(threadid==boost::this_thread::get_id(),"resolve callback in wrong thread.");
                         BOOST_CHECK_MESSAGE(!err,"resolve failed.");
                         ++resolved_count;
                         p->set_value();
                      }
         );
    }
private:
    boost::asynchronous::asio_tcp_resolver_proxy m_tcp_resolver;
    boost::thread::id m_threadid;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(resolve)
};

}

BOOST_AUTO_TEST_CASE( test_asio_resolve )
{        
    auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::single_thread_scheduler<
                                                                    boost::asynchronous::threadsafe_list<> >);
    
    main_thread_id = boost::this_thread::get_id();   
    ServantProxy proxy(scheduler);
    boost::shared_ptr<boost::promise<void> > p(new boost::promise<void>);
    boost::shared_future<void> fu = p->get_future();
    boost::shared_future<void> fuv = proxy.resolve(p);
    fu.get();
    BOOST_CHECK_MESSAGE(resolved_count==1,"resolve callback not called.");
}



