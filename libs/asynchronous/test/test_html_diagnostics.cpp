﻿// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2024
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <algorithm>
#include <fstream>
#include <iostream>
#include <type_traits>
#include <thread>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_for.hpp>
#include <boost/asynchronous/algorithm/parallel_reduce.hpp>

#include <boost/asynchronous/diagnostics/formatter.hpp>
#include <boost/asynchronous/diagnostics/html_formatter.hpp>

#include <boost/asynchronous/helpers/recursive_future_get.hpp>

#include <boost/test/unit_test.hpp>

namespace
{

#define MOD 1000000009
#define EXPONENT 42

    // 2 ** 24 items in the example servant
#define DATA_EXPONENT 24

    typedef boost::asynchronous::any_loggable job;

    class Servant : public boost::asynchronous::trackable_servant<job, job>
    {
    public:
        Servant(boost::asynchronous::any_weak_scheduler<job> scheduler, boost::asynchronous::any_shared_scheduler_proxy<job> pool)
            : boost::asynchronous::trackable_servant<job, job>(scheduler, pool)
            , m_promise(new std::promise<void>)
        {
            m_data = std::vector<long long>(1 << DATA_EXPONENT);
            std::iota(m_data.begin(), m_data.end(), 0);
        }

        void on_callback(long long /*result*/)
        {
            m_promise->set_value();
        }

        std::future<void> foo()
        {
            std::future<void> fu = m_promise->get_future();

            auto fn = [](long long const& i)
                {
                    long long& j = const_cast<long long&>(i);
                    long long k = j;
                    for (int m = 0; m < EXPONENT; ++m) {
                        j = (j * k) % MOD;
                    }
                };
            auto reduction = [](long long const& a, long long const& b) {
                return (a + b) % MOD;
                };

            post_callback(
                [this, fn, reduction]() mutable {
                    // Capturing 'this' is evil, but we'll do it anyways, because this is just a test.
                    // Also, if everything works, the servant is guaranteed to stay alive until the callback finishes.
                    // With C++14, we would instead capture [data=std::move(this->m_data), fn=std::move(fn), reduction=std::move(reduction)]
                    auto continuation = boost::asynchronous::parallel_for<decltype(this->m_data),
                    decltype(fn),
                    job>(
                        std::move(this->m_data),
                        std::move(fn),
                        8192,
                        "parallel_for",
                        0);
            return boost::asynchronous::parallel_reduce<decltype(continuation),
                decltype(reduction),
                job>(
                    std::move(continuation),
                    std::move(reduction),
                    8192,
                    "parallel_reduce",
                    0);
                },
                [this](boost::asynchronous::expected<long long> res) {
                    this->on_callback(res.get());
                },
                "foo",
                0,
                0
            );
            return fu;
        }
    private:
        std::shared_ptr<std::promise<void> > m_promise;
        std::vector<long long> m_data;
    };


    class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy, Servant, job, 20000>
    {
    public:
        template <class Scheduler, class Pool>
        ServantProxy(Scheduler s, Pool p) :
            boost::asynchronous::servant_proxy<ServantProxy, Servant, job, 20000>(s, p)
        {}

        BOOST_ASYNC_FUTURE_MEMBER_LOG(foo, "foo", 0);
        BOOST_ASYNC_SERVANT_POST_CTOR_LOG("ctor", 0);
        BOOST_ASYNC_SERVANT_POST_DTOR_LOG("dtor", 0);
    };

}

BOOST_AUTO_TEST_CASE(test_html_diagnostics_call)
{
    // Create schedulers
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
        boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::lockfree_queue<job>>>(std::string("Servant")); // scheduler name
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<
        boost::asynchronous::multiqueue_threadpool_scheduler<
        boost::asynchronous::lockfree_queue<job>>>(
            boost::thread::hardware_concurrency(),            // number of threads
            std::string("Threadpool"),                        // scheduler name
            20);                                              // queue parameter

    auto formatter_scheduler = boost::asynchronous::make_shared_scheduler_proxy<
        boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::lockfree_queue<job>>>
        (std::string("Formatter scheduler"));        // scheduler name


    typedef boost::asynchronous::html_formatter::formatter<> html_formatter_t;

    boost::asynchronous::formatter_proxy<html_formatter_t> formatter(formatter_scheduler,
        pool,
        boost::asynchronous::make_scheduler_interfaces(scheduler, pool, formatter_scheduler));

    {
        // Create proxy
        ServantProxy proxy(scheduler, pool);
        auto fu = proxy.foo();

        // Sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Wait for the task to finish
        boost::asynchronous::recursive_future_get(std::move(fu));

        // Output intermediate statistics
        auto formatted = boost::asynchronous::recursive_future_get(formatter.format());

        // Clear schedulers
        formatter.clear_schedulers();

        BOOST_CHECK_MESSAGE(!formatted.empty(), "no html diagnostics");

    }

}