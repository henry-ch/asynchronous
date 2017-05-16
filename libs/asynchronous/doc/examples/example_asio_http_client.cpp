#include <iostream>
#include <sstream>
#include <vector>
#include <future>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/composite_threadpool_scheduler.hpp>
#include <boost/asynchronous/extensions/asio/asio_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/extensions/asio/tss_asio.hpp>

#include "asio/asio_http_async_client.hpp"

using namespace std;

namespace
{
// Objects of this type are made to live inside an asio_scheduler,
// they get their associated io_service object from TLS
struct AsioCommunicationServant : boost::asynchronous::trackable_servant<>
{
    AsioCommunicationServant(boost::asynchronous::any_weak_scheduler<> scheduler,
                             const std::string& server, const std::string& path)
        : boost::asynchronous::trackable_servant<>(scheduler)
        , m_client(*boost::asynchronous::get_io_service<>(),server,path)
    {}
    void test(std::function<void(std::string)> cb)
    {
        // just forward call to asio asynchronous http client
        // the only change being the (safe) callback which will be called when http get is done
        m_client.request_content(cb);
    }
private:
    client m_client;
};
// the proxy of AsioCommunicationServant for use in an external thread
class AsioCommunicationServantProxy: public boost::asynchronous::servant_proxy<AsioCommunicationServantProxy,AsioCommunicationServant >
{
public:
    // ctor arguments are forwarded to AsioCommunicationServant
    template <class Scheduler>
    AsioCommunicationServantProxy(Scheduler s,const std::string& server, const std::string& path):
        boost::asynchronous::servant_proxy<AsioCommunicationServantProxy,AsioCommunicationServant >(s,server,path)
    {}
    // we offer a single member for posting
    BOOST_ASYNC_POST_MEMBER(test)
};
// our manager object, thread unsafe, so leaved in the single-threaded scheduler created in example_asio_http_client()
// this object is trackable. If the callback of an operation, asio or work posted to threadpool is done after
// this object has been destroyed, no member function will be called
struct Servant : boost::asynchronous::trackable_servant<>
{
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler,const std::string& server, const std::string& path)
        : boost::asynchronous::trackable_servant<>(scheduler)
        , m_check_string_count(0)
    {
        // as worker we use a simple threadpool scheduler with 4 threads (0 would also do as the asio pool steals)
        auto worker_tp = boost::asynchronous::make_shared_scheduler_proxy<
                    boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>> (4);

        // for tcp communication we use an asio-based scheduler with 3 threads
        auto asio_workers = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::asio_scheduler<>>(3);

        // we create a composite pool whose only goal is to allow asio worker threads to steal tasks from the threadpool
        m_pools = boost::asynchronous::make_shared_scheduler_proxy<
                    boost::asynchronous::composite_threadpool_scheduler<>> (worker_tp,asio_workers);

        set_worker(worker_tp);
        // we create one asynchronous communication manager in each thread
        m_asio_comm.push_back(AsioCommunicationServantProxy(asio_workers,server,path));
        m_asio_comm.push_back(AsioCommunicationServantProxy(asio_workers,server,path));
        m_asio_comm.push_back(AsioCommunicationServantProxy(asio_workers,server,path));
    }
    // the following work is done in a few lines of code:
    // 1. call the same http get on all asio servants
    // 2. at each callback, check if we got all callbacks
    // 3. if yes, post some work to threadpool, compare the returned strings (should be all the same)
    // 4. if all strings equal as they should be, cout the page
    std::shared_future<void> get_data()
    {
        // provide this callback (executing in our thread) to all asio servants as task result. A string will contain the page
        std::function<void(std::string)> f =
                [this](std::string s)
                {
                   this->m_requested_data.push_back(s);
                   // poor man's state machine saying we got the result of our asio requests :)
                   if (this->m_requested_data.size() == 3)
                   {
                       // ok, this has really been called for all servants, compare.
                       // but it could be long, so we will post it to threadpool
                       std::cout << "got all tcp data, parallel check it's correct" << std::endl;
                       std::string s1 = this->m_requested_data[0];
                       std::string s2 = this->m_requested_data[1];
                       std::string s3 = this->m_requested_data[2];
                       // this callback (executing in our thread) will be called after each comparison
                       auto cb1 = [this,s1](boost::asynchronous::expected<bool> res)
                       {
                          if (res.get())
                              ++this->m_check_string_count;
                          else
                              std::cout << "uh oh, the pages do not match, data not confirmed" << std::endl;
                          if (this->m_check_string_count ==2)
                          {
                              // we started 2 comparisons, so it was the last one, data confirmed
                              std::cout << "data has been confirmed, here it is:" << std::endl;
                              std::cout << s1;
                              // we are done, main will terminate us
                              this->m_done_promise.set_value();
                          }
                       };
                       auto cb2=cb1;
                       // post 2 string comparison tasks, provide callback where the last step will run
                       this->post_callback([s1,s2](){return s1 == s2;},std::move(cb1));
                       this->post_callback([s2,s3](){return s2 == s3;},std::move(cb2));
                   }
                };
        // ok, our simple "state machine" is ready, get the work going and start tcp tasks
        // make_safe_callback produces callbacks safe on 2 fronts
        //(no race as posted, tracking in case this object has been destroyed)
        m_asio_comm[0].test(make_safe_callback(f));
        m_asio_comm[1].test(make_safe_callback(f));
        m_asio_comm[2].test(make_safe_callback(f));
        std::shared_future<void> fu = m_done_promise.get_future();
        return fu;
    }
private:
    // a composite pool composed of asio workers and threadpool workers from which asio-serving threads can steal
    boost::asynchronous::any_shared_scheduler_proxy<> m_pools;
    std::vector<AsioCommunicationServantProxy> m_asio_comm;
    std::vector<std::string> m_requested_data;// pages returned from tcp communication
    unsigned m_check_string_count;//has to be 2 when done (2 string compares)
    std::promise<void> m_done_promise;
};
// manager (Servant) proxy, for use in main thread
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s,const std::string& server, const std::string& path):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s,server,path)
    {}
    // get_data is posted, no future, no callback
    BOOST_ASYNC_FUTURE_MEMBER(get_data)
};
}

void example_asio_http_client()
{
    std::cout << "example_asio_http_client" << std::endl;
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                                boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::lockfree_queue<>>>();
        {
            ServantProxy proxy(scheduler,"boost.org","/development/index.html");
            // call member, as if it was from Servant
            auto called_fu = proxy.get_data();
            auto fu_done = called_fu.get();
            // when the next line returns, we got the response from the server, we are done
            fu_done.get();
        }
    }
}
