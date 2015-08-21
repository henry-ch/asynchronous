#include <iostream>
#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/extensions/asio/asio_scheduler.hpp>
#include <boost/asynchronous/queue/threadsafe_list.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/extensions/asio/asio_deadline_timer.hpp>


#include <boost/test/unit_test.hpp>

using namespace std;
  
namespace
{
// main thread id
boost::thread::id main_thread_id;
unsigned timer_expired_count=0;
unsigned timer_cancelled_count=0;

struct Servant : boost::asynchronous::trackable_servant<>
{
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler) 
        : boost::asynchronous::trackable_servant<>(scheduler)
        , m_timer(get_scheduler(),boost::posix_time::milliseconds(500))
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant ctor not posted.");
        m_threadid = boost::this_thread::get_id();
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
    } 
    void timer_expired(boost::shared_ptr<boost::promise<void> > p)
    {
        boost::asynchronous::any_shared_scheduler<> s = get_scheduler().lock();
        BOOST_CHECK_MESSAGE(s.contains_id(boost::this_thread::get_id()),"timer_expired running in wrong thread.");
        boost::thread::id threadid = m_threadid;
        
        typename boost::chrono::high_resolution_clock::time_point start = boost::chrono::high_resolution_clock::now();

//        async_wait(m_timer,
//                   [p,start,threadid](const ::boost::system::error_code& err){
//                       BOOST_CHECK_MESSAGE(threadid==boost::this_thread::get_id(),"timer callback in wrong thread.");
//                       BOOST_CHECK_MESSAGE(!err,"timer not expired.");
//                       typename boost::chrono::high_resolution_clock::time_point stop = boost::chrono::high_resolution_clock::now();
//                       auto d =  boost::chrono::nanoseconds(stop - start).count();
//                       BOOST_CHECK_MESSAGE(d/1000000 >= 400,"timer was too long.");
//                       BOOST_CHECK_MESSAGE(d/1000000 >= 490,"timer was too short.");
//                       ++timer_expired_count;
//                       p->set_value();
//                   });
        std::function<void(const ::boost::system::error_code&)> f = 
                [p,threadid](const ::boost::system::error_code& err )
                {
                   p->set_value();
                };
        
        call_callback(get_worker(),
                      m_timer.unsafe_async_wait(make_safe_callback(f )),
                      [](boost::shared_future<void> ){}
                    );
    }
    void timer_cancelled(boost::shared_ptr<boost::promise<void> > p)
    {
        m_timer =  boost::asynchronous::asio_deadline_timer(get_scheduler(),boost::posix_time::milliseconds(50000));
        boost::asynchronous::any_shared_scheduler<> s = get_scheduler().lock();
        BOOST_CHECK_MESSAGE(s.contains_id(boost::this_thread::get_id()),"timer_cancelled running in wrong thread.");
        boost::thread::id threadid = m_threadid;
        
        typename boost::chrono::high_resolution_clock::time_point start = boost::chrono::high_resolution_clock::now();
        
//        async_wait(m_timer,
//                   [p,start,threadid](const ::boost::system::error_code& err){
//                       BOOST_CHECK_MESSAGE(threadid==boost::this_thread::get_id(),"timer callback in wrong thread.");
//                       BOOST_CHECK_MESSAGE(err,"timer not expired.");
//                       typename boost::chrono::high_resolution_clock::time_point stop = boost::chrono::high_resolution_clock::now();
//                       auto d =  boost::chrono::nanoseconds(stop - start).count();
//                       BOOST_CHECK_MESSAGE(d/1000000 < 49999,"timer was too long.");
//                       ++timer_cancelled_count;
//                       p->set_value();
//        });
        // cancel timer
        m_timer =  boost::asynchronous::asio_deadline_timer<>(get_scheduler(),boost::posix_time::milliseconds(500));
    }
private:
    boost::asynchronous::asio_deadline_timer m_timer;
    boost::thread::id m_threadid;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(timer_expired)
    BOOST_ASYNC_FUTURE_MEMBER(timer_cancelled)
};

}

BOOST_AUTO_TEST_CASE( test_asio_timer_expired_single_thread )
{        
    auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::asio_scheduler<>(1));
    
    main_thread_id = boost::this_thread::get_id();   
    ServantProxy proxy(scheduler);
    boost::shared_ptr<boost::promise<void> > p(new boost::promise<void>);
    boost::shared_future<void> fu = p->get_future();
    boost::shared_future<void> fuv = proxy.timer_expired(p);
    fu.get();
    BOOST_CHECK_MESSAGE(timer_expired_count==1,"timer callback not called.");
}

BOOST_AUTO_TEST_CASE( test_asio_timer_cancelled_single_thread )
{        
    auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::asio_scheduler<>(1));
    
    main_thread_id = boost::this_thread::get_id();   
    ServantProxy proxy(scheduler);
    boost::shared_ptr<boost::promise<void> > p(new boost::promise<void>);
    boost::shared_future<void> fu = p->get_future();
    boost::shared_future<void> fuv = proxy.timer_cancelled(p);
    fu.get();
    BOOST_CHECK_MESSAGE(timer_cancelled_count==1,"timer callback not called.");
}

