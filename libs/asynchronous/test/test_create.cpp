// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <iostream>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/threadsafe_list.hpp>
#include <boost/asynchronous/queue/any_queue_container.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/stealing_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/stealing_multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/composite_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.h>
#include <boost/asynchronous/post.hpp>

//#define BOOST_TEST_MODULE MyTest
//#define BOOST_TEST_DYN_LINK
//#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>



BOOST_AUTO_TEST_CASE( create_single_thread_scheduler )
{        
    {
        boost::asynchronous::create_shared_scheduler_proxy(
                    new boost::asynchronous::single_thread_scheduler<
                    boost::asynchronous::threadsafe_list<> >);
    }
}
BOOST_AUTO_TEST_CASE( create_threadpool_scheduler )
{        
    {
        boost::asynchronous::create_shared_scheduler_proxy(
                    new boost::asynchronous::threadpool_scheduler<
                    boost::asynchronous::threadsafe_list<> >(4));
    }
}
BOOST_AUTO_TEST_CASE( create_stealing_threadpool_scheduler )
{        
    {
        boost::asynchronous::create_shared_scheduler_proxy(
                    new boost::asynchronous::stealing_threadpool_scheduler<
                    boost::asynchronous::any_queue_container<>,
                    boost::asynchronous::no_cpu_load_saving,true >(4,boost::asynchronous::any_queue_container_config<boost::asynchronous::threadsafe_list<> >(4)));
    }
}
BOOST_AUTO_TEST_CASE( create_stealing_multiqueue_threadpool_scheduler )
{        
    {
        boost::asynchronous::create_shared_scheduler_proxy(
                    new boost::asynchronous::stealing_multiqueue_threadpool_scheduler<
                    boost::asynchronous::any_queue_container<>,boost::asynchronous::default_find_position<>,
                    boost::asynchronous::no_cpu_load_saving,true >
                        (3,boost::asynchronous::any_queue_container_config<boost::asynchronous::threadsafe_list<> >(3)));
    }
}
BOOST_AUTO_TEST_CASE( create_composite_threadpool_scheduler )
{        
    {
        auto tp = boost::asynchronous::create_shared_scheduler_proxy( 
                    new boost::asynchronous::threadpool_scheduler<boost::asynchronous::threadsafe_list<> > (3));
        auto tp2 = boost::asynchronous::create_shared_scheduler_proxy( 
                    new boost::asynchronous::threadpool_scheduler<boost::asynchronous::threadsafe_list<> > (3));

        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                    new boost::asynchronous::composite_threadpool_scheduler<> (tp,tp2));

    }
}
BOOST_AUTO_TEST_CASE( create_multiqueue_threadpool_scheduler )
{        
    {
        boost::asynchronous::create_shared_scheduler_proxy(
                    new boost::asynchronous::multiqueue_threadpool_scheduler<boost::asynchronous::any_queue_container<> >
                    (3,boost::asynchronous::any_queue_container_config<boost::asynchronous::threadsafe_list<> >(3)));
    }
}


