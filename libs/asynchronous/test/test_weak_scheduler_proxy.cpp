
// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/test/unit_test.hpp>

using namespace std;

BOOST_AUTO_TEST_CASE( test_weak_scheduler_proxy_not_empty )
{
#ifdef BOOST_ASYNCHRONOUS_NO_TYPE_ERASURE
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                        boost::asynchronous::threadpool_scheduler<
                                boost::asynchronous::lockfree_queue<>>>(1);

    boost::asynchronous::scheduler_weak_proxy<> wproxy(scheduler);
    auto locked = wproxy.lock();

    BOOST_CHECK_MESSAGE(locked.is_valid(),"locked scheduler should be valid.");
#endif
}

BOOST_AUTO_TEST_CASE( test_weak_scheduler_proxy_empty )
{
#ifdef BOOST_ASYNCHRONOUS_NO_TYPE_ERASURE
    boost::asynchronous::scheduler_weak_proxy<> wproxy;
    auto locked = wproxy.lock();

    BOOST_CHECK_MESSAGE(!locked.is_valid(),"locked scheduler should not be valid.");
#endif
}

