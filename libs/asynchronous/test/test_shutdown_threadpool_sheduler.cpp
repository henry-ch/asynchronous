
// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/scheduler/stealing_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/stealing_multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/extensions/asio/asio_scheduler.hpp>
#include <boost/asynchronous/scheduler/io_threadpool_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/post.hpp>

#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_CASE( test_shutdown_threadpool_scheduler )
{
    bool called = false;
    size_t num_executed = 0;
    size_t num_failures = 0;

    for (int i = 0 ; i < 100 ; ++i)
    {
        called = false;
        num_executed++;
        {
          auto scheduler = boost::asynchronous::create_shared_scheduler_proxy( new boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<> >(1));
          boost::asynchronous::post_future( scheduler, [&called] () {called = true;});
        }
        if ( called == false)
        {
          num_failures++;
        }
    }
    BOOST_CHECK_MESSAGE(num_failures==0,"task not executed at shutdown.");
}

BOOST_AUTO_TEST_CASE( test_shutdown_threadpool_scheduler_2 )
{
    bool called = false;
    size_t num_executed = 0;
    size_t num_failures = 0;

    for (int i = 0 ; i < 100 ; ++i)
    {
        called = false;
        num_executed++;
        {
          auto scheduler = boost::asynchronous::create_shared_scheduler_proxy( new boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<> >(4));
          boost::asynchronous::post_future( scheduler, [&called] () {called = true;});
        }
        if ( called == false)
        {
          num_failures++;
        }
    }
    BOOST_CHECK_MESSAGE(num_failures==0,"task not executed at shutdown.");
}

BOOST_AUTO_TEST_CASE( test_shutdown_multiqueue_threadpool_scheduler )
{
    bool called = false;
    size_t num_executed = 0;
    size_t num_failures = 0;

    for (int i = 0 ; i < 100 ; ++i)
    {
        called = false;
        num_executed++;
        {
          auto scheduler = boost::asynchronous::create_shared_scheduler_proxy( new boost::asynchronous::multiqueue_threadpool_scheduler<boost::asynchronous::lockfree_queue<> >(1));
          boost::asynchronous::post_future( scheduler, [&called] () {called = true;});
        }
        if ( called == false)
        {
          num_failures++;
        }
    }
    BOOST_CHECK_MESSAGE(num_failures==0,"task not executed at shutdown.");
}

BOOST_AUTO_TEST_CASE( test_shutdown_single_thread_scheduler )
{
    bool called = false;
    size_t num_executed = 0;
    size_t num_failures = 0;

    for (int i = 0 ; i < 100 ; ++i)
    {
        called = false;
        num_executed++;
        {
          auto scheduler = boost::asynchronous::create_shared_scheduler_proxy( new boost::asynchronous::single_thread_scheduler<boost::asynchronous::lockfree_queue<> >());
          boost::asynchronous::post_future( scheduler, [&called] () {called = true;});
        }
        if ( called == false)
        {
          num_failures++;
        }
    }
    BOOST_CHECK_MESSAGE(num_failures==0,"task not executed at shutdown.");
}

BOOST_AUTO_TEST_CASE( test_shutdown_stealing_threadpool_scheduler )
{
    bool called = false;
    size_t num_executed = 0;
    size_t num_failures = 0;

    for (int i = 0 ; i < 100 ; ++i)
    {
        called = false;
        num_executed++;
        {
          auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                      new boost::asynchronous::stealing_threadpool_scheduler<boost::asynchronous::lockfree_queue<>,boost::asynchronous::no_cpu_load_saving,true >(1));
          boost::asynchronous::post_future( scheduler, [&called] () {called = true;});
        }
        if ( called == false)
        {
          num_failures++;
        }
    }
    BOOST_CHECK_MESSAGE(num_failures==0,"task not executed at shutdown.");
}

BOOST_AUTO_TEST_CASE( test_shutdown_stealing_multiqueue_threadpool_scheduler )
{
    bool called = false;
    size_t num_executed = 0;
    size_t num_failures = 0;

    for (int i = 0 ; i < 100 ; ++i)
    {
        called = false;
        num_executed++;
        {
          auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                      new boost::asynchronous::stealing_multiqueue_threadpool_scheduler<boost::asynchronous::lockfree_queue<>,
                                                                                        boost::asynchronous::default_find_position<>,
                                                                                        boost::asynchronous::no_cpu_load_saving,true >(1));
          boost::asynchronous::post_future( scheduler, [&called] () {called = true;});
        }
        if ( called == false)
        {
          num_failures++;
        }
    }
    BOOST_CHECK_MESSAGE(num_failures==0,"task not executed at shutdown.");
}

BOOST_AUTO_TEST_CASE( test_shutdown_asio_scheduler )
{
    bool called = false;
    size_t num_executed = 0;
    size_t num_failures = 0;

    for (int i = 0 ; i < 100 ; ++i)
    {
        called = false;
        num_executed++;
        {
          auto scheduler = boost::asynchronous::create_shared_scheduler_proxy( new boost::asynchronous::asio_scheduler<>(1));
          boost::asynchronous::post_future( scheduler, [&called] () {called = true;});
        }
        if ( called == false)
        {
          num_failures++;
        }
    }
    BOOST_CHECK_MESSAGE(num_failures==0,"task not executed at shutdown.");
}

BOOST_AUTO_TEST_CASE( test_shutdown_io_scheduler )
{
    bool called = false;
    size_t num_executed = 0;
    size_t num_failures = 0;

    for (int i = 0 ; i < 100 ; ++i)
    {
        called = false;
        num_executed++;
        {
          auto scheduler = boost::asynchronous::create_shared_scheduler_proxy( new boost::asynchronous::io_threadpool_scheduler<boost::asynchronous::lockfree_queue<>>(1,2));
          boost::asynchronous::post_future( scheduler, [&called] () {called = true;});
        }
        if ( called == false)
        {
          num_failures++;
        }
    }
    BOOST_CHECK_MESSAGE(num_failures==0,"task not executed at shutdown.");
}
