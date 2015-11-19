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

#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/multiple_thread_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>

#include "test_common.hpp"
#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

namespace
{
// main thread id
boost::thread::id main_thread_id;
std::vector<boost::thread::id> tpids;

bool dtor_called=false;
//make template just to try it out
template <class T>
struct Servant : boost::asynchronous::trackable_servant<>
{
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<>>>(1))
    {
    }
    ~Servant()
    {
        dtor_called=true;
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
    }
    void do_it()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work in main thread.");
        BOOST_CHECK_MESSAGE(contains_id(tpids.begin(),tpids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
    }
};

//make template just to try it out
template <class T>
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy<T>,Servant<T>>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s, std::size_t index):
        boost::asynchronous::servant_proxy<ServantProxy,Servant<T>>(std::make_tuple(s,index))
    {}
    // this is only for c++11 compilers necessary
#ifdef BOOST_NO_CXX14_RETURN_TYPE_DEDUCTION
    using servant_type = typename boost::asynchronous::servant_proxy<ServantProxy<T>,Servant<T>>::servant_type;
#endif

#ifndef _MSC_VER
    BOOST_ASYNC_FUTURE_MEMBER(do_it,1)
#else
    BOOST_ASYNC_FUTURE_MEMBER_1(do_it,1)
#endif
};

}

BOOST_AUTO_TEST_CASE( test_multiple_thread_scheduler_one_worker )
{
    main_thread_id = boost::this_thread::get_id();
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiple_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>(1,5);
        tpids = scheduler.thread_ids();

        ServantProxy<int> proxy0(scheduler,0);
        proxy0.do_it();
        ServantProxy<int> proxy1(scheduler,1);
        proxy1.do_it();
        ServantProxy<int> proxy2(scheduler,2);
        proxy2.do_it();
        ServantProxy<int> proxy3(scheduler,3);
        proxy3.do_it();
        ServantProxy<int> proxy4(scheduler,4);
        proxy4.do_it();
    }
    // at this point, the dtor has been called
    BOOST_CHECK_MESSAGE(dtor_called,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_multiple_thread_scheduler_two_workers )
{
    main_thread_id = boost::this_thread::get_id();
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiple_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>(2,5);
        tpids = scheduler.thread_ids();

        ServantProxy<int> proxy0(scheduler,0);
        proxy0.do_it();
        ServantProxy<int> proxy1(scheduler,1);
        proxy1.do_it();
        ServantProxy<int> proxy2(scheduler,2);
        proxy2.do_it();
        ServantProxy<int> proxy3(scheduler,3);
        proxy3.do_it();
        ServantProxy<int> proxy4(scheduler,4);
        proxy4.do_it();
    }
    // at this point, the dtor has been called
    BOOST_CHECK_MESSAGE(dtor_called,"servant dtor not called.");
}

