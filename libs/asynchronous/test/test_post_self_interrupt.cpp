
// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <vector>
#include <set>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/threadsafe_list.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>

#include <boost/test/unit_test.hpp>

namespace
{
// main thread id
boost::thread::id main_thread_id;
bool dtor_called=false;
bool task_called=false;

struct Servant : boost::asynchronous::trackable_servant<>
{
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               boost::asynchronous::create_shared_scheduler_proxy(
                                                   new boost::asynchronous::threadpool_scheduler<
                                                           boost::asynchronous::threadsafe_list<> >(1)))
    {
    }
    ~Servant()
    {
        dtor_called=true;
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
    }
    std::tuple<boost::future<void>,boost::asynchronous::any_interruptible>
    start_posting(boost::shared_ptr<boost::promise<void> > done, boost::shared_ptr<boost::promise<void> > ready)
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_posting not posted.");
        // post task to self
        return interruptible_post_self(
           [done,ready]()mutable
            {
                task_called = true;
                BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant task not posted.");
                ready->set_value();
                boost::this_thread::sleep(boost::posix_time::milliseconds(600000));
                // should not come here (interrupted)
                done->set_value();
            }// work
        );
    }
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(start_posting)
};

}

BOOST_AUTO_TEST_CASE( test_post_self_interrupt )
{
    main_thread_id = boost::this_thread::get_id();
    {
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::threadsafe_list<> >);

        boost::shared_ptr<boost::promise<void> > done(new boost::promise<void>);
        boost::shared_ptr<boost::promise<void> > ready(new boost::promise<void>);
        boost::shared_future<void> end=ready->get_future();
        {
            ServantProxy proxy(scheduler);
            boost::future<std::tuple<boost::future<void>,boost::asynchronous::any_interruptible> > res = proxy.start_posting(done,ready);
            // wait for task to start
            boost::asynchronous::any_interruptible i = std::get<1>(res.get());
            // interrupt
            i.interrupt();
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
            BOOST_CHECK_MESSAGE(!done->get_future().has_value(),"task not interrupted.");
        }
    }
    // at this point, the dtor has been called
    BOOST_CHECK_MESSAGE(dtor_called,"servant dtor not called.");
    BOOST_CHECK_MESSAGE(task_called,"posted task not called.");
}


