// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE
#endif
#include <algorithm>

#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/asynchronous/container/vector.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/diagnostics/any_loggable.hpp>
#include "test_common.hpp"

#include <boost/test/unit_test.hpp>


using namespace boost::asynchronous::test;

namespace
{
struct some_type
{
    some_type()
        :data(0)
    {
    }
    int data;
};
}

BOOST_AUTO_TEST_CASE( test_vector_ctor_size )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */, 10000 /* number of elements */);
    BOOST_CHECK_MESSAGE(v.size()==10000,"vector size should be 10000.");

    // check iterators
    auto cpt = std::count_if(v.begin(),v.end(),[](some_type const & i){return i.data == 0;});
    BOOST_CHECK_MESSAGE(cpt==10000,"vector should have 10000 int with value 0.");
    BOOST_CHECK_MESSAGE(v[500].data == 0,"vector[500] should have value 0.");
}

BOOST_AUTO_TEST_CASE( test_vector_ctor_size_job )
{
    typedef boost::asynchronous::any_loggable servant_job;
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<servant_job>>>(8);

    boost::asynchronous::vector<int,servant_job> v(scheduler, 100 /* cutoff */, 10000 /* number of elements */);
    BOOST_CHECK_MESSAGE(v.size()==10000,"vector size should be 10000.");
    BOOST_CHECK_MESSAGE(!v.empty(),"vector should not be empty.");
}

BOOST_AUTO_TEST_CASE( test_vector_access )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */, 10000 /* number of elements */);
    BOOST_CHECK_MESSAGE(v.size()==10000,"vector size should be 10000.");
    v[500].data = 10;
    BOOST_CHECK_MESSAGE(v[500].data == 10,"vector[500] should have value 10.");
}

BOOST_AUTO_TEST_CASE( test_vector_at_ok )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */, 10000 /* number of elements */);
    BOOST_CHECK_MESSAGE(v.size()==10000,"vector size should be 10000.");
    v.at(500).data = 10;
    BOOST_CHECK_MESSAGE(v.at(500).data == 10,"vector[500] should have value 10.");
}

BOOST_AUTO_TEST_CASE( test_vector_at_nok )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */, 10000 /* number of elements */);
    BOOST_CHECK_MESSAGE(v.size()==10000,"vector size should be 10000.");
    bool caught = false;
    try
    {
        v.at(10000).data = 10;
    }
    catch(std::out_of_range&)
    {
        caught = true;
    }
    BOOST_CHECK_MESSAGE(caught,"vector::at should have thrown");
}

BOOST_AUTO_TEST_CASE( test_vector_front_back )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */, 10000 /* number of elements */);
    BOOST_CHECK_MESSAGE(v.size()==10000,"vector size should be 10000.");
    v[0].data = 10;
    v[9999].data = 11;
    BOOST_CHECK_MESSAGE(v.front().data == 10,"vector.front() should have value 10.");
    BOOST_CHECK_MESSAGE(v.back().data == 11,"vector.back() should have value 11.");
}
