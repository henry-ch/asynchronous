// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2016
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org
#include <iostream>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>

using namespace std;

namespace
{
struct Manager : boost::asynchronous::trackable_servant<>
{
    Manager(boost::asynchronous::any_weak_scheduler<> scheduler, int threads)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               // threadpool and a simple lockfree_queue queue
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::multiqueue_threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<>>>(threads))
        // to signal that we shutdown
        , m_promise()
    {
    }
    void second_task()
    {
        // this will probably never be called as our example cancels immediately
        post_callback(
                []()
                {
                     cout << "execute second task" << endl;
                     // simulate long task
                     boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
                },
                // callback
                [this](boost::asynchronous::expected<void> /*res*/)
                {
                    // ignore expected as no exception will be thrown
                    cout << "shutdown servant after completion" <<endl;
                    m_promise.set_value();
                }
        );
    }
    void cancel()
    {
        cout << "second part cancelled" << endl;
        needs_second_task = false;
    }

    // call to this is posted and executes in our (safe) single-thread scheduler
    boost::future<void> start()
    {
        cout << "post first task to threadpool" << endl;
        // to inform main  of shutdown
        boost::future<void> fu = m_promise.get_future();
        // start long tasks in threadpool (first lambda) and callback in our thread
        post_callback(
                []()
                {
                     cout << "execute first task" << endl;
                     // simulate long task
                     boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
                },
                // callback
                [this](boost::asynchronous::expected<void> /*res*/)
                {
                    // ignore expected as no exception will be thrown
                    // check now, only after first call if we want to continue, based on whatever happened since we started a task
                    if (needs_second_task)
                    {
                        second_task();
                    }
                    else
                    {
                        cout << "shutdown servant after first task" <<endl;
                        m_promise.set_value();
                    }
                }
        );
        return fu;
    }
private:
// to signal that we shutdown
boost::promise<void> m_promise;
bool needs_second_task=true;
};
class ManagerProxy : public boost::asynchronous::servant_proxy<ManagerProxy,Manager>
{
public:
    template <class Scheduler>
    ManagerProxy(Scheduler s, int threads):
        boost::asynchronous::servant_proxy<ManagerProxy,Manager>(s,threads)
    {}
    // caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER(start)
    BOOST_ASYNC_FUTURE_MEMBER(cancel)
};

}

void example_callback()
{
    std::cout << "example_callback" << std::endl;
    {
        // a single-threaded world, where Manager will live.
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                                boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::lockfree_queue<>>>();
        {
            ManagerProxy proxy(scheduler,boost::thread::hardware_concurrency());
            boost::future<boost::future<void> > fu = proxy.start();
            // we changed our mind, inform Manager by posting him a call to cancel()
            proxy.cancel();
            // wait for shutdown
            fu.get().get();
        }
    }
    std::cout << "end example_callback \n" << std::endl;
}
