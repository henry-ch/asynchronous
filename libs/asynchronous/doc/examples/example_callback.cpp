// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2016
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org
#include <iostream>
#include <thread>
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
    void count()
    {
        // to show our race-free code, we use a counter, set by external threads.
        // an even number in the counter will cause second task to not be executed
        ++needs_second_task;
    }

    // call to this is posted and executes in our (safe) single-thread scheduler
    std::future<void> start()
    {
        cout << "post first task to threadpool" << endl;
        // to inform main  of shutdown
        std::future<void> fu = m_promise.get_future();
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
                    if (needs_second_task % 2)
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
std::promise<void> m_promise;
int needs_second_task=0;
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
    BOOST_ASYNC_FUTURE_MEMBER(count)
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
        ManagerProxy proxy(scheduler,boost::thread::hardware_concurrency());
        std::future<std::future<void> > fu = proxy.start();
        // show that we have in our ManagerProxy a completely safe thread world which can be accessed by several threads
        // add more threads for fun if needed. An odd number of them will cause second task to be called.
        std::thread t1([proxy](){proxy.count();});
        std::thread t2([proxy](){proxy.count();});
        // our proxy is now shared by 3 external threads, has no lifecycle or race issue
        t1.join();
        t2.join();

        // wait for shutdown
        fu.get().get();
    }
    std::cout << "end example_callback \n" << std::endl;
}
