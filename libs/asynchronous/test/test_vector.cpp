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
#include <vector>

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

bool operator== (some_type const& lhs, some_type const& rhs)
{
    return rhs.data == lhs.data;
}
bool operator< (some_type const& lhs, some_type const& rhs)
{
    return lhs.data < rhs.data;
}
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

BOOST_AUTO_TEST_CASE( test_vector_ctor_iterators )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    std::vector<some_type> source(10000);
    int i=0;
    for(auto& e:source)
    {
        e = some_type(i++);
    }

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */, source.begin(),source.end());
    BOOST_CHECK_MESSAGE(v.size()==10000,"vector size should be 10000.");

    // check iterators
    BOOST_CHECK_MESSAGE(v[500].data == 500,"vector[500] should have value 500.");
    BOOST_CHECK_MESSAGE(v[100].data == 100,"vector[100] should have value 100.");
    BOOST_CHECK_MESSAGE(v[800].data == 800,"vector[800] should have value 800.");
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

BOOST_AUTO_TEST_CASE( test_vector_assignment_operator_to_smaller )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */, 10000 /* number of elements */);
    boost::asynchronous::vector<some_type> v2(scheduler, 100 /* cutoff */, 5000 /* number of elements */);

    v2[0].data = 1;
    v2[2000].data = 2;
    v2[4999].data = 3;
    v = v2;
    BOOST_CHECK_MESSAGE(v2[0].data == 1,"vector[0] should have value 1.");
    BOOST_CHECK_MESSAGE(v2[2000].data == 2,"vector[2000] should have value 2.");
    BOOST_CHECK_MESSAGE(v2[4999].data == 3,"vector[4999] should have value 3.");
    BOOST_CHECK_MESSAGE(v[0].data == 1,"vector[0] should have value 1.");
    BOOST_CHECK_MESSAGE(v[2000].data == 2,"vector[2000] should have value 2.");
    BOOST_CHECK_MESSAGE(v[4999].data == 3,"vector[4999] should have value 3.");
}

BOOST_AUTO_TEST_CASE( test_vector_assignment_operator_to_larger )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */, 5000 /* number of elements */);
    boost::asynchronous::vector<some_type> v2(scheduler, 100 /* cutoff */, 10000 /* number of elements */);

    v2[0].data = 1;
    v2[2000].data = 2;
    v2[9999].data = 3;
    v = v2;
    BOOST_CHECK_MESSAGE(v2[0].data == 1,"vector[0] should have value 1.");
    BOOST_CHECK_MESSAGE(v2[2000].data == 2,"vector[2000] should have value 2.");
    BOOST_CHECK_MESSAGE(v2[9999].data == 3,"vector[9999] should have value 3.");
    BOOST_CHECK_MESSAGE(v[0].data == 1,"vector[0] should have value 1.");
    BOOST_CHECK_MESSAGE(v[2000].data == 2,"vector[2000] should have value 2.");
    BOOST_CHECK_MESSAGE(v[9999].data == 3,"vector[9999] should have value 3.");
}

BOOST_AUTO_TEST_CASE( test_vector_assignment_to_smaller )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */, 10000 /* number of elements */);
    std::vector<some_type> v2( 5000 /* number of elements */);

    v2[0].data = 1;
    v2[2000].data = 2;
    v2[4999].data = 3;
    v.assign(v2.begin(),v2.end());
    BOOST_CHECK_MESSAGE(v2[0].data == 1,"vector[0] should have value 1.");
    BOOST_CHECK_MESSAGE(v2[2000].data == 2,"vector[2000] should have value 2.");
    BOOST_CHECK_MESSAGE(v2[4999].data == 3,"vector[4999] should have value 3.");
    BOOST_CHECK_MESSAGE(v[0].data == 1,"vector[0] should have value 1.");
    BOOST_CHECK_MESSAGE(v[2000].data == 2,"vector[2000] should have value 2.");
    BOOST_CHECK_MESSAGE(v[4999].data == 3,"vector[4999] should have value 3.");
}

BOOST_AUTO_TEST_CASE( test_vector_assignment_to_larger )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */, 5000 /* number of elements */);
    std::vector<some_type> v2( 10000 /* number of elements */);

    v2[0].data = 1;
    v2[2000].data = 2;
    v2[9999].data = 3;
    v.assign(v2.begin(),v2.end());
    BOOST_CHECK_MESSAGE(v2[0].data == 1,"vector[0] should have value 1.");
    BOOST_CHECK_MESSAGE(v2[2000].data == 2,"vector[2000] should have value 2.");
    BOOST_CHECK_MESSAGE(v2[9999].data == 3,"vector[9999] should have value 3.");
    BOOST_CHECK_MESSAGE(v[0].data == 1,"vector[0] should have value 1.");
    BOOST_CHECK_MESSAGE(v[2000].data == 2,"vector[2000] should have value 2.");
    BOOST_CHECK_MESSAGE(v[9999].data == 3,"vector[9999] should have value 3.");
}

BOOST_AUTO_TEST_CASE( test_vector_assignment2_to_smaller )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */, 10000 /* number of elements */);
    v.assign(std::size_t(5000),42);
    BOOST_CHECK_MESSAGE(v[0].data == 42,"vector[0] should have value 1.");
    BOOST_CHECK_MESSAGE(v[2000].data == 42,"vector[2000] should have value 42.");
    BOOST_CHECK_MESSAGE(v[4999].data == 42,"vector[4999] should have value 42.");
}

BOOST_AUTO_TEST_CASE( test_vector_assignment2_to_larger )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */, 5000 /* number of elements */);
    v.assign(std::size_t(10000),42);
    BOOST_CHECK_MESSAGE(v[0].data == 42,"vector[0] should have value 42.");
    BOOST_CHECK_MESSAGE(v[2000].data == 42,"vector[2000] should have value 42.");
    BOOST_CHECK_MESSAGE(v[9999].data == 42,"vector[4999] should have value 42.");
}

BOOST_AUTO_TEST_CASE( test_vector_equal )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */, 5000 /* number of elements */);
    boost::asynchronous::vector<some_type> v2(scheduler, 100 /* cutoff */, 5000 /* number of elements */);
    boost::asynchronous::vector<some_type> v3(scheduler, 100 /* cutoff */, (std::size_t)5000 /* number of elements */, (int)42);
    boost::asynchronous::vector<some_type> v4(scheduler, 100 /* cutoff */, 4000 /* number of elements */);

    BOOST_CHECK_MESSAGE(v == v2,"vectors should be equal");
    BOOST_CHECK_MESSAGE(!(v == v3),"vectors should not be equal");
    BOOST_CHECK_MESSAGE(!(v == v4),"vectors should not be equal");
    BOOST_CHECK_MESSAGE(v != v3,"vectors should not be equal");
    BOOST_CHECK_MESSAGE(v != v4,"vectors should not be equal");
}

BOOST_AUTO_TEST_CASE( test_vector_compare )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v1(scheduler, 100 /* cutoff */, (std::size_t)5000 /* number of elements */, (int)42);
    boost::asynchronous::vector<some_type> v2(scheduler, 100 /* cutoff */, (std::size_t)5000 /* number of elements */, (int)42);
    boost::asynchronous::vector<some_type> v3(scheduler, 100 /* cutoff */, (std::size_t)5000 /* number of elements */, (int)42);
    ++v1[2000].data;

    BOOST_CHECK_MESSAGE(v2 < v1,"v2 should be less than v1");
    BOOST_CHECK_MESSAGE(!(v2 > v1),"v2 should be not be greater than v1");
    BOOST_CHECK_MESSAGE(v2 <= v1,"v2 should be less or equal than v1");
    BOOST_CHECK_MESSAGE(!(v2 >= v1),"v2 should be not be greater or equal than v1");

    BOOST_CHECK_MESSAGE(v3 <= v2,"v3 should be less or equal than v2");
    BOOST_CHECK_MESSAGE(v2 <= v3,"v3 should be less or equal than v2");
}

BOOST_AUTO_TEST_CASE( test_vector_erase)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(8);

    boost::asynchronous::vector<some_type> v(scheduler, 100 /* cutoff */, (std::size_t)10000 /* number of elements */, (int)42);
    v.erase(v.cbegin()+3000, v.cend());
    BOOST_CHECK_MESSAGE(v.size() == 3000,"vector size should be 3000.");
    BOOST_CHECK_MESSAGE(v[5000].data == 42,"vector[5000] should have value 42.");

    v.erase(v.cbegin()+1000,v.cbegin()+2000 );
    BOOST_CHECK_MESSAGE(v.size() == 2000,"vector size should be 2000.");
    BOOST_CHECK_MESSAGE(v[1000].data == 42,"vector[1000] should have value 42.");
}
