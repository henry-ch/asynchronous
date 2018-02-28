

// Boost.Asynchronous library
//  Copyright (C) Christophe Henry, Tobias Holl 2018
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <functional>
#include <type_traits>
#include <vector>
#include <set>
#include <numeric>
#include <future>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_for.hpp>
#include <boost/asynchronous/algorithm/parallel_iota.hpp>
#include <boost/asynchronous/algorithm/parallel_reduce.hpp>
#include <boost/asynchronous/algorithm/then.hpp>

#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

namespace
{
// main thread id
boost::thread::id main_thread_id;
bool servant_dtor=false;

struct SumFunctor
{
    int operator()(const int& a, const int& b)
    {
        return a + b;
    }
};

struct ExpectedState
{
    ExpectedState m_flag(int expected) const
    {
        auto copy = ExpectedState(*this);
        copy.use_m_flag = true;
        copy.storage_m_flag = expected;
        return copy;
    }

    ExpectedState m_data(int index, int expected) const
    {
        auto copy = ExpectedState(*this);
        copy.storage_m_data[index] = expected;
        return copy;
    }

    ExpectedState result(int expected) const
    {
        auto copy = ExpectedState(*this);
        copy.use_result = true;
        copy.storage_result = expected;
        return copy;
    }

    bool use_m_flag = false;
    int storage_m_flag;

    std::map<int, int> storage_m_data;

    bool use_result = false;
    int storage_result;
};

struct Holder
{
    std::vector<int> m_data;
    int m_flag;
};

struct TestCase
{
    // Inner functions will turn all 10000 values into 4s
    template <bool FunctorReturnsContinuation, bool InnerReturnsVoid, bool OuterReturnsVoid>
    static boost::asynchronous::detail::callback_continuation<void> inner(Holder *servant, typename std::enable_if<InnerReturnsVoid>::type* = nullptr)
    {
        return boost::asynchronous::parallel_for(
            servant->m_data.begin(),
            servant->m_data.end(),
            [](const int& i)
            {
               const_cast<int&>(i) += 3;
            },
            1500
        );
    }

    template <bool FunctorReturnsContinuation, bool InnerReturnsVoid, bool OuterReturnsVoid>
    static boost::asynchronous::detail::callback_continuation<std::vector<int>> inner(Holder *servant, typename std::enable_if<!InnerReturnsVoid>::type* = nullptr)
    {
        return boost::asynchronous::parallel_for(
            std::move(servant->m_data),
            [](const int& i)
            {
               const_cast<int&>(i) += 3;
            },
            1500
        );
    }

    // Each of these returns a functor, and the matching test
    template <bool FunctorReturnsContinuation, bool InnerReturnsVoid, bool OuterReturnsVoid>
    static std::pair<std::function<void()>, ExpectedState> functor(Holder *servant, typename std::enable_if<!FunctorReturnsContinuation && InnerReturnsVoid && OuterReturnsVoid>::type* = nullptr)
    {
        return std::pair<std::function<void()>, ExpectedState>(
            [servant]() mutable {
                servant->m_flag = std::accumulate(servant->m_data.begin(), servant->m_data.end(), 0);
            },
            ExpectedState().m_data(0, 4).m_data(9999, 4).m_flag(40000)
        );
    }

    template <bool FunctorReturnsContinuation, bool InnerReturnsVoid, bool OuterReturnsVoid>
    static std::pair<std::function<int()>, ExpectedState> functor(Holder *servant, typename std::enable_if<!FunctorReturnsContinuation && InnerReturnsVoid && !OuterReturnsVoid>::type* = nullptr)
    {
        return std::pair<std::function<int()>, ExpectedState>(
            [servant]() mutable
            {
                return std::accumulate(servant->m_data.begin(), servant->m_data.end(), 0);
            },
            ExpectedState().m_data(0, 4).m_data(9999, 4).result(40000)
        );
    }

    template <bool FunctorReturnsContinuation, bool InnerReturnsVoid, bool OuterReturnsVoid>
    static std::pair<std::function<void(std::vector<int>&&)>, ExpectedState> functor(Holder *servant, typename std::enable_if<!FunctorReturnsContinuation && !InnerReturnsVoid && OuterReturnsVoid>::type* = nullptr)
    {
        return std::pair<std::function<void(std::vector<int>&&)>, ExpectedState>(
            [servant](std::vector<int>&& data) mutable
            {
                servant->m_flag = std::accumulate(data.begin(), data.end(), 0);
            },
            ExpectedState().m_flag(40000)
        );
    }

    template <bool FunctorReturnsContinuation, bool InnerReturnsVoid, bool OuterReturnsVoid>
    static std::pair<std::function<int(std::vector<int>&&)>, ExpectedState> functor(Holder *servant, typename std::enable_if<!FunctorReturnsContinuation && !InnerReturnsVoid && !OuterReturnsVoid>::type* = nullptr)
    {
        return std::pair<std::function<int(std::vector<int>&&)>, ExpectedState>(
            [servant](std::vector<int>&& data) mutable
            {
                return std::accumulate(data.begin(), data.end(), 0);
            },
            ExpectedState().result(40000)
        );
    }

    template <bool FunctorReturnsContinuation, bool InnerReturnsVoid, bool OuterReturnsVoid>
    static std::pair<std::function<boost::asynchronous::detail::callback_continuation<void>()>, ExpectedState> functor(Holder *servant, typename std::enable_if<FunctorReturnsContinuation && InnerReturnsVoid && OuterReturnsVoid>::type* = nullptr)
    {
        return std::pair<std::function<boost::asynchronous::detail::callback_continuation<void>()>, ExpectedState>(
            [servant]() mutable
            {
                return boost::asynchronous::parallel_for(
                    servant->m_data.begin(),
                    servant->m_data.end(),
                    [](const int& i)
                    {
                       const_cast<int&>(i) += 3;
                    },
                    1500
                );
            },
            ExpectedState().m_data(0, 7).m_data(9999, 7)
        );
    }

    template <bool FunctorReturnsContinuation, bool InnerReturnsVoid, bool OuterReturnsVoid>
    static std::pair<std::function<boost::asynchronous::detail::callback_continuation<int>()>, ExpectedState> functor(Holder *servant, typename std::enable_if<FunctorReturnsContinuation && InnerReturnsVoid && !OuterReturnsVoid>::type* = nullptr)
    {
        return std::pair<std::function<boost::asynchronous::detail::callback_continuation<int>()>, ExpectedState>(
            [servant]() mutable
            {
                return boost::asynchronous::parallel_reduce(
                    servant->m_data.begin(),
                    servant->m_data.end(),
                    SumFunctor {},
                    1500
                );
            },
            ExpectedState().m_data(0, 4).m_data(9999, 4).result(40000)
        );
    }

    template <bool FunctorReturnsContinuation, bool InnerReturnsVoid, bool OuterReturnsVoid>
    static std::pair<std::function<boost::asynchronous::detail::callback_continuation<void>(std::vector<int>&&)>, ExpectedState> functor(Holder *servant, typename std::enable_if<FunctorReturnsContinuation && !InnerReturnsVoid && OuterReturnsVoid>::type* = nullptr)
    {
        return std::pair<std::function<boost::asynchronous::detail::callback_continuation<void>(std::vector<int>&&)>, ExpectedState>(
            [servant](std::vector<int>&& data) mutable
            {
                (void) data;
                servant->m_data = std::vector<int>(10000);
                return boost::asynchronous::parallel_iota(
                    servant->m_data.begin(),
                    servant->m_data.end(),
                    0,
                    1500
                );
            },
            ExpectedState().m_data(0, 0).m_data(9999, 9999)
        );
    }

    template <bool FunctorReturnsContinuation, bool InnerReturnsVoid, bool OuterReturnsVoid>
    static std::pair<std::function<boost::asynchronous::detail::callback_continuation<int>(std::vector<int>&&)>, ExpectedState> functor(Holder *servant, typename std::enable_if<FunctorReturnsContinuation && !InnerReturnsVoid && !OuterReturnsVoid>::type* = nullptr)
    {
        return std::pair<std::function<boost::asynchronous::detail::callback_continuation<int>(std::vector<int>&&)>, ExpectedState>(
            [servant](std::vector<int>&& data) mutable
            {
                return boost::asynchronous::parallel_reduce(
                    std::move(data),
                    SumFunctor {},
                    1500
                );
            },
            ExpectedState().result(40000)
        );
    }

    template <bool FunctorReturnsContinuation, bool InnerReturnsVoid, bool OuterReturnsVoid>
    static std::string task_name()
    {
        std::string name = "";
        name += FunctorReturnsContinuation ? "Continuation; " : "Value; ";
        name += InnerReturnsVoid           ? "InnerVoid; "    : "InnerValue; ";
        name += OuterReturnsVoid           ? "OuterVoid"      : "OuterValue";
        return name;
    }

    template <bool FunctorReturnsContinuation, bool InnerReturnsVoid, bool OuterReturnsVoid>
    static void verify(boost::asynchronous::expected<void> res, Holder *servant, ExpectedState es, typename std::enable_if<OuterReturnsVoid>::type* = nullptr)
    {
        res.get();
        if (es.use_m_flag)
            BOOST_CHECK_MESSAGE(es.storage_m_flag == servant->m_flag, "m_flag is wrong: was " + std::to_string(servant->m_flag) + ", expected " + std::to_string(es.storage_m_flag));
        for (const auto& pair : es.storage_m_data)
            BOOST_CHECK_MESSAGE(pair.second == servant->m_data[pair.first], "m_data[" + std::to_string(pair.first) + "] is wrong: was " + std::to_string(servant->m_data[pair.first]) + ", expected " + std::to_string(pair.second));
    }

    template <bool FunctorReturnsContinuation, bool InnerReturnsVoid, bool OuterReturnsVoid>
    static void verify(boost::asynchronous::expected<int> res, Holder *servant, ExpectedState es, typename std::enable_if<!OuterReturnsVoid>::type* = nullptr)
    {
        int result = res.get();
        if (es.use_result)
            BOOST_CHECK_MESSAGE(es.storage_result == result, "result is wrong: was " + std::to_string(result) + ", expected " + std::to_string(es.storage_result));
        if (es.use_m_flag)
            BOOST_CHECK_MESSAGE(es.storage_m_flag == servant->m_flag, "m_flag is wrong: was " + std::to_string(servant->m_flag) + ", expected " + std::to_string(es.storage_m_flag));
        for (const auto& pair : es.storage_m_data)
            BOOST_CHECK_MESSAGE(pair.second == servant->m_data[pair.first], "m_data[" + std::to_string(pair.first) + "] is wrong: was " + std::to_string(servant->m_data[pair.first]) + ", expected " + std::to_string(pair.second));
    }

    template <bool FunctorReturnsContinuation, bool InnerReturnsVoid, bool OuterReturnsVoid>
    struct ReturnType
    {
        using type = typename std::conditional<OuterReturnsVoid, void, int>::type;
    };
};

struct Servant : boost::asynchronous::trackable_servant<>, public Holder
{
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                                   boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::threadpool_scheduler<
                                                      boost::asynchronous::lockfree_queue<>>>(6))
        , Holder { std::vector<int>(10000,1), 0 }
    {
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id != boost::this_thread::get_id(),"servant dtor not posted.");
        servant_dtor = true;
    }

    template <bool FunctorReturnsContinuation,
              bool InnerReturnsVoid,
              bool OuterReturnsVoid>
    std::future<void> automated_task()
    {
        std::string name = TestCase::task_name<FunctorReturnsContinuation, InnerReturnsVoid, OuterReturnsVoid>();
        BOOST_CHECK_MESSAGE(main_thread_id != boost::this_thread::get_id(), name + " not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();

        auto fvpair = TestCase::functor<FunctorReturnsContinuation, InnerReturnsVoid, OuterReturnsVoid>(this);
        auto functor = fvpair.first;
        auto verifier = fvpair.second;

        // start long tasks
        post_callback(
           [ids,functor,name,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");
                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");

                    return boost::asynchronous::then(
                        TestCase::inner<FunctorReturnsContinuation, InnerReturnsVoid, OuterReturnsVoid>(this),
                        functor,
                        name + "::then"
                    );
           },// work
           [aPromise,verifier,ids,this](boost::asynchronous::expected<typename TestCase::ReturnType<FunctorReturnsContinuation, InnerReturnsVoid, OuterReturnsVoid>::type> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");

                        TestCase::verify<FunctorReturnsContinuation, InnerReturnsVoid, OuterReturnsVoid>(res, this, verifier);

                        // reset
                        m_data = std::vector<int>(10000,1);
                        m_flag = 0;
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }

#define TASK_N(index) std::future<void> task_##index() { return automated_task<(((index - 1) & 4) > 0), (((index - 1) & 2) > 0), (((index - 1) & 1) > 0)>(); }
    TASK_N(1)
    TASK_N(2)
    TASK_N(3)
    TASK_N(4)
    TASK_N(5)
    TASK_N(6)
    TASK_N(7)
    TASK_N(8)
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
#ifndef _MSC_VER
    BOOST_ASYNC_FUTURE_MEMBER(task_1)
    BOOST_ASYNC_FUTURE_MEMBER(task_2)
    BOOST_ASYNC_FUTURE_MEMBER(task_3)
    BOOST_ASYNC_FUTURE_MEMBER(task_4)
    BOOST_ASYNC_FUTURE_MEMBER(task_5)
    BOOST_ASYNC_FUTURE_MEMBER(task_6)
    BOOST_ASYNC_FUTURE_MEMBER(task_7)
    BOOST_ASYNC_FUTURE_MEMBER(task_8)
#else
    BOOST_ASYNC_FUTURE_MEMBER_1(task_1)
    BOOST_ASYNC_FUTURE_MEMBER_1(task_2)
    BOOST_ASYNC_FUTURE_MEMBER_1(task_3)
    BOOST_ASYNC_FUTURE_MEMBER_1(task_4)
    BOOST_ASYNC_FUTURE_MEMBER_1(task_5)
    BOOST_ASYNC_FUTURE_MEMBER_1(task_6)
    BOOST_ASYNC_FUTURE_MEMBER_1(task_7)
    BOOST_ASYNC_FUTURE_MEMBER_1(task_8)
#endif
};

}

#define TEST_N(index) \
    BOOST_AUTO_TEST_CASE( test_then_##index ) \
    { \
        servant_dtor = false; \
        { \
            auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<boost::asynchronous::lockfree_queue<>>>(); \
            main_thread_id = boost::this_thread::get_id(); \
            ServantProxy proxy(scheduler); \
            auto fuv = proxy.task_##index(); \
            try \
            { \
                auto resfuv = fuv.get(); \
                resfuv.get(); \
            } \
            catch(...) \
            { \
                BOOST_FAIL( "Unexpected exception" ); \
            } \
        } \
        BOOST_CHECK_MESSAGE(servant_dtor, "servant dtor not called."); \
    }

TEST_N(1)
TEST_N(2)
TEST_N(3)
TEST_N(4)
TEST_N(5)
TEST_N(6)
TEST_N(7)
TEST_N(8)
