// Boost.Asynchronous library
//  Copyright (C) Christophe Henry, Tobias Holl 2014
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <iostream>
#include <vector>
#include <limits>

#include <boost/range/irange.hpp>
#include <boost/asynchronous/callable_any.hpp>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_reduce.hpp>
#include <boost/asynchronous/algorithm/parallel_for.hpp>
#include <boost/asynchronous/algorithm/invoke.hpp>
#include <boost/asynchronous/helpers/lazy_irange.hpp>

#define COUNT 100000000L
#define THREAD_COUNT 6
#define STEP_SIZE COUNT/THREAD_COUNT/100

namespace {
struct pi {
    double operator()(long n) {
        return ((double) ((((int) n) % 2 == 0) ? 1 : -1)) / ((double) (2 * n + 1));
    }
};

double serial_pi()
{
    double res = 0.0;
    for (long i = 0; i < COUNT; ++i) {
        res += ((double) ((((long) i) % 2 == 0) ? 1 : -1)) / ((double) (2 * i + 1));
    }
    return res * 4.0;
}
struct Servant : boost::asynchronous::trackable_servant<>
{
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               boost::asynchronous::create_shared_scheduler_proxy(
                                                   new boost::asynchronous::multiqueue_threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<> >(THREAD_COUNT)))
    {}

    boost::shared_future<double> calc_pi()
    {
        std::cout << "start calculating PI" << std::endl;
        // we need a promise to inform caller when we're done
        boost::shared_ptr<boost::promise<double> > aPromise(new boost::promise<double>);
        boost::shared_future<double> fu = aPromise->get_future();
        // start long tasks
        post_callback(
                   []()
                   {
                        auto mult = [](double a) { return a * 4.0; };
                        auto sum = [](double a, double b) { return a + b; };
                        return boost::asynchronous::invoke(
                                    boost::asynchronous::parallel_reduce(
                                        boost::asynchronous::lazy_irange(0L, COUNT, pi()), sum, STEP_SIZE),
                                    mult);
                   },// work
                   [aPromise](boost::asynchronous::expected<double> res){
                        aPromise->set_value(res.get());
                   }// callback functor.
        );
        return fu;
    }
};


class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant> {
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    // caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER(calc_pi)
};
}

void parallel_pi()
{
    std::cout.precision(std::numeric_limits< double >::digits10 +2);
    std::cout << "calculating pi, serial way" << std::endl;
    typename boost::chrono::high_resolution_clock::time_point start;
    typename boost::chrono::high_resolution_clock::time_point stop;
    start = boost::chrono::high_resolution_clock::now();
    double res = serial_pi();
    stop = boost::chrono::high_resolution_clock::now();
    std::cout << "PI = " << res << std::endl;
    std::cout << "serial_pi took in us:"
              <<  (boost::chrono::nanoseconds(stop - start).count() / 1000) <<"\n" <<std::endl;

    std::cout << "calculating pi, parallel way" << std::endl;
    auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                            new boost::asynchronous::single_thread_scheduler<
                                 boost::asynchronous::lockfree_queue<> >);
    {
        ServantProxy proxy(scheduler);
        start = boost::chrono::high_resolution_clock::now();
        boost::shared_future<boost::shared_future<double> > fu = proxy.calc_pi();
        boost::shared_future<double> resfu = fu.get();
        res = resfu.get();
        stop = boost::chrono::high_resolution_clock::now();
        std::cout << "PI = " << res << std::endl;
        std::cout << "parallel_pi took in us:"
                  <<  (boost::chrono::nanoseconds(stop - start).count() / 1000) <<"\n" <<std::endl;
    }
}
