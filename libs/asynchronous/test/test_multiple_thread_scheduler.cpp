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
        test_data.push_back(42);
    }
    void do_it2()
    {
        test_data.push_back(42);
    }
    std::size_t size()const
    {
        return test_data.size();
    }
    // check for race
    std::vector<int> test_data;
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
    BOOST_ASYNC_FUTURE_MEMBER(do_it2,1)
    BOOST_ASYNC_FUTURE_MEMBER(size,1)
#else
    BOOST_ASYNC_FUTURE_MEMBER_1(do_it,1)
    BOOST_ASYNC_FUTURE_MEMBER_1(do_it2,1)
    BOOST_ASYNC_FUTURE_MEMBER_1(size,1)
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
        BOOST_CHECK_MESSAGE(proxy0.size().get() == 1,"wrong number of test data");
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
        BOOST_CHECK_MESSAGE(proxy0.size().get() == 1,"wrong number of test data");
    }
    // at this point, the dtor has been called
    BOOST_CHECK_MESSAGE(dtor_called,"servant dtor not called.");
}
/*
BOOST_AUTO_TEST_CASE( test_multiple_thread_scheduler_many_workers_and_no_race )
{
    main_thread_id = boost::this_thread::get_id();
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiple_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>(
                                                                                    boost::thread::hardware_concurrency(),5);
        tpids = scheduler.thread_ids();

        ServantProxy<int> proxy0(scheduler,0);
        ServantProxy<int> proxy1(scheduler,1);
        ServantProxy<int> proxy2(scheduler,2);
        ServantProxy<int> proxy3(scheduler,3);
        ServantProxy<int> proxy4(scheduler,4);
        for (int i = 0; i< 10000; ++i)
        {
            proxy0.do_it2();
            proxy1.do_it2();
            proxy2.do_it2();
            proxy4.do_it2();
            proxy3.do_it2();
        }
        BOOST_CHECK_MESSAGE(proxy0.size().get() == 10000,"wrong number of test data");
        BOOST_CHECK_MESSAGE(proxy1.size().get() == 10000,"wrong number of test data");
        BOOST_CHECK_MESSAGE(proxy2.size().get() == 10000,"wrong number of test data");
        BOOST_CHECK_MESSAGE(proxy3.size().get() == 10000,"wrong number of test data");
        BOOST_CHECK_MESSAGE(proxy4.size().get() == 10000,"wrong number of test data");
    }
    // at this point, the dtor has been called
    BOOST_CHECK_MESSAGE(dtor_called,"servant dtor not called.");
}
*/
