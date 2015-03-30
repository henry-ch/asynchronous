
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
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/test/unit_test.hpp>

namespace
{
bool void_task_done=false;

struct cont_task : public boost::asynchronous::continuation_task<int>
{
    cont_task()=default;
    cont_task(cont_task&&)=default;
    cont_task& operator=(cont_task&&)=default;
    cont_task(cont_task const&)=delete;
    cont_task& operator=(cont_task const&)=delete;

    void operator()()const
    {
        boost::asynchronous::continuation_result<int> task_res = this_task_result();
        task_res.set_value(42);
    }
};
struct void_cont_task : public boost::asynchronous::continuation_task<void>
{
    void_cont_task()=default;
    void_cont_task(void_cont_task&&)=default;
    void_cont_task& operator=(void_cont_task&&)=default;
    void_cont_task(void_cont_task const&)=delete;
    void_cont_task& operator=(void_cont_task const&)=delete;

    void operator()()const
    {
        boost::asynchronous::continuation_result<void> task_res = this_task_result();
        void_task_done = true;
        task_res.set_value();
    }
};
}


BOOST_AUTO_TEST_CASE( test_post_future_continuation )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                        boost::asynchronous::threadpool_scheduler<
                                boost::asynchronous::lockfree_queue<>>>(1);
    boost::future<int> fui = boost::asynchronous::post_future(scheduler,
                         []()
                         {
                              // a top-level continuation is the first one in a recursive serie.
                              // Its result will be passed to callback
                              return boost::asynchronous::top_level_continuation<int>(cont_task());
                          });
    int res = fui.get();
    BOOST_CHECK_MESSAGE(42 == res,"post_future_continuation<int> returned wrong value.");
}

BOOST_AUTO_TEST_CASE( test_post_future_void_continuation )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                        boost::asynchronous::threadpool_scheduler<
                                boost::asynchronous::lockfree_queue<>>>(1);
    boost::future<void> fu = boost::asynchronous::post_future(scheduler,
                         []()
                         {
                              // a top-level continuation is the first one in a recursive serie.
                              // Its result will be passed to callback
                              return boost::asynchronous::top_level_continuation<void>(void_cont_task());
                          });
    fu.get();
    BOOST_CHECK_MESSAGE(void_task_done,"post_future_continuation<void> not done.");
}
