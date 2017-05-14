// Boost.Asynchronous library
//  Copyright (C) Christophe Henry, Tobias Holl 2014
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <vector>
#include <set>
#include <string>

#include <boost/asynchronous/callable_any.hpp>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_reduce.hpp>
#include <boost/asynchronous/algorithm/parallel_for.hpp>
#include "test_common.hpp"

#include <boost/test/unit_test.hpp>


using namespace boost::asynchronous::test;

namespace
{
// main thread id
boost::thread::id main_thread_id;
bool servant_dtor=false;

struct my_exception : virtual boost::exception, virtual std::exception
{
    virtual const char* what() const throw()
    {
        return "my_exception";
    }
};

std::vector<int> mkdata() {
    std::vector<int> data;
    for (int i = 1; i <= 100; ++i) data.push_back(i);
    return data;
}

struct some_cont_task : public boost::asynchronous::continuation_task<int>
{
    some_cont_task(std::vector<int> data,std::vector<int> data2 ):m_data(std::move(data)),m_data2(std::move(data2))
    {
    }
    void operator()()const
    {
        boost::asynchronous::continuation_result<int> task_res = this_task_result();
        boost::asynchronous::create_callback_continuation(
                        [task_res](std::tuple<boost::asynchronous::expected<int>,boost::asynchronous::expected<int> > res)
                        {
                            int r = std::get<0>(res).get() + std::get<1>(res).get();
                            task_res.set_value(r);
                        },
                        boost::asynchronous::parallel_reduce(std::move(this->m_data),
                                                                                   [](int const& a, int const& b)
                                                                                   {
                                                                                     return a + b;
                                                                                   },20),
                        boost::asynchronous::parallel_reduce(std::move(this->m_data2),
                                                                                   [](int const& a, int const& b)
                                                                                   {
                                                                                     return a + b;
                                                                                   },20)
        );
    }
    std::vector<int> m_data;
    std::vector<int> m_data2;
};

struct Servant : boost::asynchronous::trackable_servant<>
{
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<>>>(6))
        , m_data(mkdata())
        , m_data2(mkdata())
    {
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
        servant_dtor = true;
    }
    boost::shared_future<void> start_async_work()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work_range2 not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::shared_future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");

                    return boost::asynchronous::top_level_callback_continuation<int>(some_cont_task(std::move(m_data),std::move(m_data2)));
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<int> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        int r = res.get();
                        BOOST_CHECK_MESSAGE((r == 10100), ("result of parallel_reduce was " + std::to_string(r) + ", should have been 10100"));
                        // reset
                        m_data = mkdata();
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }

private:
    std::vector<int> m_data;
    std::vector<int> m_data2;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
#ifndef _MSC_VER
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work)
#else
    BOOST_ASYNC_FUTURE_MEMBER_1(start_async_work)
#endif
};

}

BOOST_AUTO_TEST_CASE( test_continuation_from_continuations )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::shared_future<boost::shared_future<void> > fuv = proxy.start_async_work();
        try
        {
            boost::shared_future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}



