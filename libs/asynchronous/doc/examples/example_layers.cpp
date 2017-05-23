// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2016
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <iostream>
#include <functional>
#include <string>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>

using namespace std;

namespace
{
// a trackable servant is protecting the servant object by providing safe callbacks
struct BottomLayerServant : boost::asynchronous::trackable_servant<>
{
    BottomLayerServant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler)

    {
    }
    // caller (middle level) provides a callback, call it when done
    void do_something(std::function<void(int)> callback)
    {
        // do something useful, communication, parallel algorithm etc.

        // we are done, inform caller, from our thread
        std::cout << "low-level layer is done" << std::endl;
        callback(42);
    }

private:
};

// a proxy protects a servant from outside calls running in different threads
class BottomLayerProxy : public boost::asynchronous::servant_proxy<BottomLayerProxy,BottomLayerServant>
{
public:
    template <class Scheduler>
    BottomLayerProxy(Scheduler s):
        boost::asynchronous::servant_proxy<BottomLayerProxy,BottomLayerServant>(s)
    {}
    // caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER(do_something)
};

// a trackable servant is protecting the servant object by providing safe callbacks
struct MiddleLayerServant : boost::asynchronous::trackable_servant<>
{
    MiddleLayerServant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler)
        // create a low-level layer in its own thread world
        , m_bottom(boost::asynchronous::make_shared_scheduler_proxy<
          boost::asynchronous::single_thread_scheduler<
               boost::asynchronous::lockfree_queue<>>>())
    {
    }
    // caller (top level) provides a callback, call it when done
    void do_something(std::function<void(std::string)> callback)
    {
        // check internal state, do something

        // then delegate part of work to bottom layer
        m_bottom.do_something(
                    make_safe_callback(
                        [this, callback](int res)
                        {
                            // this callback, though coming from the bottom layer, is executing within our thread
                            // it is therefore safe to use "this"
                            std::cout << "middle-level layer is done" << std::endl;
                            if (res == 42)
                            {
                                callback("ok");
                            }
                            else
                            {
                                callback("nok");
                            }
                        }
                        ));
    }
private:
    // middle layer keeps low-level alive
    BottomLayerProxy m_bottom;
};

// a proxy protects a servant from outside calls running in different threads
class MiddleLayerProxy : public boost::asynchronous::servant_proxy<MiddleLayerProxy,MiddleLayerServant>
{
public:
    template <class Scheduler>
    MiddleLayerProxy(Scheduler s):
        boost::asynchronous::servant_proxy<MiddleLayerProxy,MiddleLayerServant>(s)
    {}
    // caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER(do_something)
};
// a trackable servant is protecting the servant object by providing safe callbacks
struct TopLayerServant : boost::asynchronous::trackable_servant<>
{
    TopLayerServant(boost::asynchronous::any_weak_scheduler<> scheduler, int threads)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               // threadpool and a simple lockfree_queue queue
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::multiqueue_threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<>>>(threads))
        // to signal that we shutdown
        , m_promise()
        // create a middle-level layer in its own thread world
        , m_middle(boost::asynchronous::make_shared_scheduler_proxy<
          boost::asynchronous::single_thread_scheduler<
               boost::asynchronous::lockfree_queue<>>>())
    {
    }
    // call to this is posted and executes in our (safe) single-thread scheduler
    std::future<bool> start()
    {
        // to inform main  of shutdown
        std::future<bool> fu = m_promise.get_future();
        // delegate work to middle layer
        m_middle.do_something(
                    make_safe_callback(
                        [this](std::string res)
                        {
                            // this callback, though coming from the bottom layer, is executing within our thread
                            // it is therefore safe to use "this"
                            std::cout << "top-level layer is done" << std::endl;
                            if (res == "ok")
                            {
                                // inform main
                                m_promise.set_value(true);
                            }
                            else
                            {
                                // inform main
                                m_promise.set_value(true);
                            }
                        }
                        ));
        return fu;
    }
private:
// to signal that we shutdown
std::promise<bool> m_promise;
// top layer holds middle-level alive, which keeps low-level alive
MiddleLayerProxy m_middle;
};

// a proxy protects a servant from outside calls running in different threads
class TopLayerProxy : public boost::asynchronous::servant_proxy<TopLayerProxy,TopLayerServant>
{
public:
    template <class Scheduler>
    TopLayerProxy(Scheduler s, int threads):
        boost::asynchronous::servant_proxy<TopLayerProxy,TopLayerServant>(s,threads)
    {}
    // caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER(start)
};

}

void example_layers()
{
    std::cout << "example_layers" << std::endl;
    {
        // a single-threaded world, where TopLayerServant will live.
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                                boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::lockfree_queue<>>>();
        {
            TopLayerProxy proxy(scheduler,boost::thread::hardware_concurrency());
            std::future<std::future<bool> > fu = proxy.start();
            // main just waits for end of application and shows result
            bool app_res = fu.get().get();
            std::cout << "app finished with success? " << std::boolalpha << app_res << std::endl;
        }
    }
    std::cout << "end example_layers \n" << std::endl;
}
