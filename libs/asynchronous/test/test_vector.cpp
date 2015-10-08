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
    some_type(int d=0)
        :data(d)
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

BOOST_AUTO_TEST_CASE( test_vector_ctor_size_value )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */, 10000 /* number of elements */, some_type(42));
    BOOST_CHECK_MESSAGE(v.size()==10000,"vector size should be 10000.");

    // check iterators
    auto cpt = std::count_if(v.begin(),v.end(),[](some_type const & i){return i.data == 42;});
    BOOST_CHECK_MESSAGE(cpt==10000,"vector should have 10000 int with value 42.");
    BOOST_CHECK_MESSAGE(v[500].data == 42,"vector[500] should have value 42.");
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

BOOST_AUTO_TEST_CASE( test_vector_clear )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */, 10000 /* number of elements */);
    BOOST_CHECK_MESSAGE(v.size()==10000,"vector size should be 10000.");
    v.clear();
    BOOST_CHECK_MESSAGE(v.size() == 0,"vector.size() should have value 0.");
    BOOST_CHECK_MESSAGE(v.empty(),"vector.empty() should be true.");
}

BOOST_AUTO_TEST_CASE( test_vector_push_back_no_realloc )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */);
    BOOST_CHECK_MESSAGE(v.size()==0,"vector size should be 0.");
    BOOST_CHECK_MESSAGE(v.capacity()== v.default_capacity,"vector capacity should be 10.");

    v.push_back(some_type(42));
    BOOST_CHECK_MESSAGE(v[0].data == 42,"vector[0] should have value 42.");
    BOOST_CHECK_MESSAGE(v.size()==1,"vector size should be 1.");

    v.push_back(some_type(41));
    BOOST_CHECK_MESSAGE(v[0].data == 42,"vector[0] should have value 42.");
    BOOST_CHECK_MESSAGE(v[1].data == 41,"vector[1] should have value 41.");
    BOOST_CHECK_MESSAGE(v.size()==2,"vector size should be 2.");
    BOOST_CHECK_MESSAGE(v.capacity()== v.default_capacity,"vector capacity should be 10.");

    // test pop_back
    v.pop_back();
    BOOST_CHECK_MESSAGE(v.size()==1,"vector size should be 1.");
    v.pop_back();
    BOOST_CHECK_MESSAGE(v.size()==0,"vector size should be 0.");
    BOOST_CHECK_MESSAGE(v.empty(),"vector.empty() should be true.");
    BOOST_CHECK_MESSAGE(v.capacity()== v.default_capacity ,"vector capacity should be 10.");
}

BOOST_AUTO_TEST_CASE( test_vector_push_back_realloc )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */);
    BOOST_CHECK_MESSAGE(v.size()==0,"vector size should be 0.");
    BOOST_CHECK_MESSAGE(v.capacity()== v.default_capacity,"vector capacity should be 10.");

    for (auto i = 0; i < 10; ++i)
    {
        v.push_back(some_type(i));
        BOOST_CHECK_MESSAGE(v[i].data == i,"vector[i] should have value i.");
        BOOST_CHECK_MESSAGE(v.size()==(std::size_t)i+1,"vector size should be i+1.");
        BOOST_CHECK_MESSAGE(v.capacity()== (std::size_t)(v.default_capacity),"vector capacity should be 10.");
    }
    // realloc happens now
    v.push_back(some_type(10));
    for (auto i = 0; i < 10; ++i)
    {
        BOOST_CHECK_MESSAGE(v[i].data == i,"vector[i] should have value i.");
    }
    BOOST_CHECK_MESSAGE(v[10].data == 10,"vector[10] should have value 10.");
    BOOST_CHECK_MESSAGE(v.size()==11,"vector size should be 11.");
    BOOST_CHECK_MESSAGE(v.capacity()== 30,"vector capacity should be 30.");
}

BOOST_AUTO_TEST_CASE( test_vector_emplace_back_no_realloc )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */);
    BOOST_CHECK_MESSAGE(v.size()==0,"vector size should be 0.");
    BOOST_CHECK_MESSAGE(v.capacity()== v.default_capacity,"vector capacity should be 10.");

    v.emplace_back(42);
    BOOST_CHECK_MESSAGE(v[0].data == 42,"vector[0] should have value 42.");
    BOOST_CHECK_MESSAGE(v.size()==1,"vector size should be 1.");

    v.emplace_back(41);
    BOOST_CHECK_MESSAGE(v[0].data == 42,"vector[0] should have value 42.");
    BOOST_CHECK_MESSAGE(v[1].data == 41,"vector[1] should have value 41.");
    BOOST_CHECK_MESSAGE(v.size()==2,"vector size should be 2.");
    BOOST_CHECK_MESSAGE(v.capacity()== v.default_capacity,"vector capacity should be 10.");
}

BOOST_AUTO_TEST_CASE( test_vector_swap )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */);

    v.push_back(some_type(42));
    v.push_back(some_type(41));
    // test swap
    boost::asynchronous::vector<some_type> v2(scheduler, 100 /* cutoff */);
    v.swap(v2);

    BOOST_CHECK_MESSAGE(v2[0].data == 42,"vector[0] should have value 42.");
    BOOST_CHECK_MESSAGE(v2[1].data == 41,"vector[1] should have value 41.");
    BOOST_CHECK_MESSAGE(v2.size()==2,"vector size should be 2.");
    BOOST_CHECK_MESSAGE(v2.capacity()== v2.default_capacity,"vector capacity should be 10.");

    BOOST_CHECK_MESSAGE(v.size()==0,"vector size should be 0.");
    BOOST_CHECK_MESSAGE(v.capacity()== v.default_capacity,"vector capacity should be 10.");
}

BOOST_AUTO_TEST_CASE( test_vector_reserve )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */);
    v.push_back(some_type(41));
    v.push_back(some_type(42));
    // reserve without change
    v.reserve(1);
    BOOST_CHECK_MESSAGE(v.size()==2,"vector size should be 2.");
    BOOST_CHECK_MESSAGE(v.capacity()== v.default_capacity,"vector capacity should be 10.");

    // reserve with change
    v.reserve(20);
    BOOST_CHECK_MESSAGE(v.size()==2,"vector size should be 2.");
    BOOST_CHECK_MESSAGE(v.capacity()== 20,"vector capacity should be 20.");
    BOOST_CHECK_MESSAGE(v[0].data == 41,"vector[0] should have value 41.");
    BOOST_CHECK_MESSAGE(v[1].data == 42,"vector[1] should have value 42.");
}

BOOST_AUTO_TEST_CASE( test_vector_shrink_to_fit )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */);
    v.push_back(some_type(41));
    v.push_back(some_type(42));
    v.shrink_to_fit();
    BOOST_CHECK_MESSAGE(v.size()==2,"vector size should be 2.");
    BOOST_CHECK_MESSAGE(v.capacity()== 2,"vector capacity should be 2.");
    BOOST_CHECK_MESSAGE(v[0].data == 41,"vector[0] should have value 41.");
    BOOST_CHECK_MESSAGE(v[1].data == 42,"vector[1] should have value 42.");
}

BOOST_AUTO_TEST_CASE( test_vector_iterators )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */);
    v.push_back(some_type(1));
    v.push_back(some_type(2));
    v.push_back(some_type(3));
    BOOST_CHECK_MESSAGE((*v.begin()).data==1,"vector begin should be 1.");
    BOOST_CHECK_MESSAGE((*(v.begin()+1)).data==2,"vector begin+1 should be 2.");
    BOOST_CHECK_MESSAGE((*v.rbegin()).data==3,"vector rbegin should be 3.");
    BOOST_CHECK_MESSAGE((*(v.rbegin()+1)).data==2,"vector begin+1 should be 2.");
    BOOST_CHECK_MESSAGE((*v.cbegin()).data==1,"vector begin should be 1.");
    BOOST_CHECK_MESSAGE((*(v.cbegin()+1)).data==2,"vector begin+1 should be 2.");
    BOOST_CHECK_MESSAGE((*v.crbegin()).data==3,"vector rbegin should be 3.");
    BOOST_CHECK_MESSAGE((*(v.crbegin()+1)).data==2,"vector begin+1 should be 2.");
}

BOOST_AUTO_TEST_CASE( test_vector_resize)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */, 10000 /* number of elements */);
    v.resize(10000);
    BOOST_CHECK_MESSAGE(v.size() == 10000,"vector size should be 10000.");

    v.resize(20000,some_type(42));
    BOOST_CHECK_MESSAGE(v.size() == 20000,"vector size should be 20000.");
    BOOST_CHECK_MESSAGE(v[15000].data == 42,"vector[15000] should have value 42.");
    BOOST_CHECK_MESSAGE(v.capacity() == 20000,"vector capacity should be 20000.");

    v.resize(5000);
    BOOST_CHECK_MESSAGE(v.size() == 5000,"vector size should be 5000.");
    BOOST_CHECK_MESSAGE(v.capacity() == 20000,"vector capacity should be 20000.");
    BOOST_CHECK_MESSAGE(v[4999].data == 0,"vector[4999] should have value 0.");
}

