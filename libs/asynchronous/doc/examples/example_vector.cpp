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
#include <atomic>

#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/asynchronous/container/vector.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>

namespace
{
    struct long_type
    {
        long_type(int size = 0)
            :data(size,42)
        {
        }
        long_type& operator= (long_type const& rhs)
        {
            data = rhs.data;
            return *this;
        }

        std::vector<int> data;
    };

bool operator== (long_type const& lhs, long_type const& rhs)
{
    return rhs.data == lhs.data;
}

}
void example_vector()
{
    // we need a threadpool, we will use all available cores
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(boost::thread::hardware_concurrency());

    // create a vector, 1024 is a good default cutoff
    // create a vector of 100000 elements of size 100 * sizeof(int)
    boost::asynchronous::vector<long_type> v(scheduler, 1024 /* cutoff */, 100000 /* number of elements */, long_type(100));

    // push_back will force reallocation, then move all elements into a bigger buffer
    v.push_back(long_type(100));

    assert(v.size() == 100001);
    assert(v.back().data[0] == 42);

    // resize will allocate a new, bigger chunk of memory and move elements there
    v.resize(200000);

    // copy in parallel
    auto v1 = v;

    // compare also
    assert(v1 == v);

    // destructors will be called in parallel
    v.clear();
    v1.clear();
}
