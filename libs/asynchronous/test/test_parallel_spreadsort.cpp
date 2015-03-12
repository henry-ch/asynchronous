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
#include <functional>
#include <random>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_sort.hpp>
#include <boost/asynchronous/algorithm/parallel_for.hpp>

#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

namespace
{
// main thread id
boost::thread::id main_thread_id;

struct my_exception : virtual boost::exception, virtual std::exception
{
    virtual const char* what() const throw()
    {
        return "my_exception";
    }
};
void generate(std::vector<int>& data)
{
    data = std::vector<int>(10000,1);
    // avoid mingw bug by not using random_device
    //std::random_device rd;
    //std::mt19937 mt(rd());
    std::mt19937 mt(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<> dis(0, 1000);
    std::generate(data.begin(), data.end(), std::bind(dis, std::ref(mt)));
}
struct increasing_sort_subtask
{
    increasing_sort_subtask(){}
    template <class Archive>
    void serialize(Archive & /*ar*/, const unsigned int /*version*/)
    {
    }
    template <class T>
    bool operator()(T const& i, T const& j)const
    {
        return i < j;
    }
    typedef int serializable_type;
    std::string get_task_name()const
    {
        return "";
    }
};
}
BOOST_AUTO_TEST_CASE( test_parallel_spreadsort_int_post_future )
{
    auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::threadpool_scheduler<
                                                                                boost::asynchronous::lockfree_queue<> >(6));
    std::vector<int> data;
    generate(data);
    // make a copy and execute in pool
    boost::future<std::vector<int>> fu = boost::asynchronous::post_future(
                scheduler,
                [data]() mutable {return boost::asynchronous::parallel_spreadsort(std::move(data),std::less<int>(),1500);});
    try
    {
        std::vector<int> res = std::move(fu.get());
        std::sort(data.begin(),data.end(),std::less<int>());
        BOOST_CHECK_MESSAGE(std::is_sorted(res.begin(),res.end(),std::less<int>()),"parallel_sort did not sort.");
        BOOST_CHECK_MESSAGE(res == data,"parallel_sort gave a wrong value.");
    }
    catch(...)
    {
        BOOST_FAIL( "unexpected exception" );
    }
}
BOOST_AUTO_TEST_CASE( test_parallel_spreadsort_int_post_future2 )
{
    auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::threadpool_scheduler<
                                                                                boost::asynchronous::lockfree_queue<> >(6));
    std::vector<int> data;
    generate(data);
    // make a copy and execute in pool
    boost::future<std::vector<int>> fu = boost::asynchronous::post_future(
                scheduler,
                [data]() mutable {return boost::asynchronous::parallel_spreadsort2(std::move(data),std::less<int>(),1500);});
    try
    {
        std::vector<int> res = std::move(fu.get());
        std::sort(data.begin(),data.end(),std::less<int>());
        BOOST_CHECK_MESSAGE(std::is_sorted(res.begin(),res.end(),std::less<int>()),"parallel_sort did not sort.");
        BOOST_CHECK_MESSAGE(res == data,"parallel_sort gave a wrong value.");
    }
    catch(...)
    {
        BOOST_FAIL( "unexpected exception" );
    }
}
BOOST_AUTO_TEST_CASE( test_parallel_spreadsort_int_post_future_dist )
{
    auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::threadpool_scheduler<
                                                                                boost::asynchronous::lockfree_queue<> >(6));
    std::vector<int> data;
    generate(data);
    // make a copy and execute in pool
    boost::future<std::vector<int>> fu = boost::asynchronous::post_future(
                scheduler,
                [data]() mutable {return boost::asynchronous::parallel_spreadsort(std::move(data),increasing_sort_subtask(),1500);});
    try
    {
        std::vector<int> res = std::move(fu.get());
        std::sort(data.begin(),data.end(),std::less<int>());
        BOOST_CHECK_MESSAGE(std::is_sorted(res.begin(),res.end(),std::less<int>()),"parallel_sort did not sort.");
        BOOST_CHECK_MESSAGE(res == data,"parallel_sort gave a wrong value.");
    }
    catch(...)
    {
        BOOST_FAIL( "unexpected exception" );
    }
}
