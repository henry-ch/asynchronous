#include <iostream>
#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/extensions/asio/asio_scheduler.hpp>
#include <boost/asynchronous/queue/threadsafe_list.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/servant_proxy.h>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/extensions/asio/asio_deadline_timer.hpp>

using namespace std;

namespace
{
struct Servant : boost::asynchronous::trackable_servant<>
{
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler) 
        : boost::asynchronous::trackable_servant<>(scheduler,
                                                   // as timer servant we use an asio-based scheduler with 1 thread
                                                   boost::asynchronous::create_shared_scheduler_proxy(
                                                       new boost::asynchronous::asio_scheduler<>(1)))
        , m_timer(get_worker(),boost::posix_time::milliseconds(1000))
    {
    }
    void start_timer()
    {
        // same thread id as the callback, safe.
        std::cout << "start_timer called in thread: " << boost::this_thread::get_id() << std::endl;
        // err will be either operation_canceled if cancel_timer(9 is called, or success otherwise
        async_wait(m_timer,
                   [](const ::boost::system::error_code& err){ 
                      std::cout << "timer expired? "<< std::boolalpha << (bool)err << ", called in thread: " << boost::this_thread::get_id() << std::endl; }
                   );
    }
    void cancel_timer()
    {
        // cancel timer by replacing it by a new one
        m_timer =  boost::asynchronous::asio_deadline_timer_proxy(get_worker(),boost::posix_time::milliseconds(1000));
    }
private:
    boost::asynchronous::asio_deadline_timer_proxy m_timer;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    BOOST_ASYNC_POST_MEMBER(start_timer)
    BOOST_ASYNC_FUTURE_MEMBER(cancel_timer)
};

}

void example_asio_timer_expired()
{    
    auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::single_thread_scheduler<
                                                                    boost::asynchronous::threadsafe_list<> >);
     
    ServantProxy proxy(scheduler);
    proxy.start_timer();
    boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
}

void example_asio_timer_canceled()
{    
    auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::single_thread_scheduler<
                                                                    boost::asynchronous::threadsafe_list<> >);
     
    ServantProxy proxy(scheduler);
    proxy.start_timer();
    // calling this will cause the callback to be called with an error code
    proxy.cancel_timer();
    boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
}
