// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2017
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <iostream>
#include <vector>
#include <memory>
#include <future>
#include <random>

#include <boost/thread/future.hpp> // for wait_for_all

#include <boost/asynchronous/scheduler/stealing_multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/composite_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/diagnostics/any_loggable.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/algorithm/parallel_sort.hpp>

using namespace std;
#define LOOP_COUNT 10000

void test_callable_multiqueue_threadpool_scheduler_lockfree(long tpsize,long queue_size)
{
    boost::asynchronous::any_shared_scheduler_proxy<> scheduler =  boost::asynchronous::make_shared_scheduler_proxy<
            boost::asynchronous::multiqueue_threadpool_scheduler<
                    boost::asynchronous::lockfree_queue<>
                >>(tpsize,queue_size);
    // set processor affinity to improve cache usage. We start at core 0, until tpsize-1
    //scheduler.processor_bind({{0,tpsize}});

    auto start = std::chrono::high_resolution_clock::now();
    std::vector<std::future<void>> fus;
    for (auto i=0; i< LOOP_COUNT; ++i)
    {
        auto fu = boost::asynchronous::post_future(scheduler,
                   []()mutable
                   {
                   });
        fus.emplace_back(std::move(fu));
    }
    auto post_time = (std::chrono::nanoseconds(std::chrono::high_resolution_clock::now() - start).count() / 1000);
    boost::wait_for_all(fus.begin(), fus.end());
    std::cout << "test_callable_multiqueue_threadpool_scheduler_lockfree, average post in us: " << post_time / LOOP_COUNT <<std::endl;
}

void test_loggable_multiqueue_threadpool_scheduler_lockfree(long tpsize,long queue_size)
{
    typedef boost::asynchronous::any_loggable servant_job;

    boost::asynchronous::any_shared_scheduler_proxy<servant_job> scheduler =  boost::asynchronous::make_shared_scheduler_proxy<
            boost::asynchronous::multiqueue_threadpool_scheduler<
                    boost::asynchronous::lockfree_queue<servant_job>
                >>(tpsize,queue_size);
    // set processor affinity to improve cache usage. We start at core 0, until tpsize-1
    //scheduler.processor_bind({{0,tpsize}});

    auto start = std::chrono::high_resolution_clock::now();
    std::vector<std::future<void>> fus;
    for (auto i=0; i< LOOP_COUNT; ++i)
    {
        auto fu = boost::asynchronous::post_future(scheduler,
                   []()mutable
                   {
                   },"loooooooooooooooooonnnnnnnnnnnnnnngggggggggggggggggg",0);
        fus.emplace_back(std::move(fu));
    }
    auto post_time = (std::chrono::nanoseconds(std::chrono::high_resolution_clock::now() - start).count() / 1000);
    boost::wait_for_all(fus.begin(), fus.end());
    std::cout << "test_loggable_multiqueue_threadpool_scheduler_lockfree, average post in us: " << post_time / LOOP_COUNT <<std::endl;
}

void test_loggable_composite_threadpool_scheduler_lockfree(long tpsize,long queue_size)
{
    typedef boost::asynchronous::any_loggable servant_job;

    boost::asynchronous::any_shared_scheduler_proxy<servant_job> sub1 =  boost::asynchronous::make_shared_scheduler_proxy<
            boost::asynchronous::stealing_multiqueue_threadpool_scheduler<
                    boost::asynchronous::lockfree_queue<servant_job>
                >>(tpsize/2,queue_size);
    boost::asynchronous::any_shared_scheduler_proxy<servant_job> sub2 =  boost::asynchronous::make_shared_scheduler_proxy<
            boost::asynchronous::stealing_multiqueue_threadpool_scheduler<
                    boost::asynchronous::lockfree_queue<servant_job>
                >>(tpsize/2,queue_size);
    auto scheduler =
            boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::composite_threadpool_scheduler<servant_job>> (sub1,sub2);

    auto start = std::chrono::high_resolution_clock::now();
    std::vector<std::future<void>> fus;
    for (auto i=0; i< LOOP_COUNT; ++i)
    {
        auto fu = boost::asynchronous::post_future(scheduler,
                   []()mutable
                   {
                   },"loooooooooooooooooonnnnnnnnnnnnnnngggggggggggggggggg",i%2);
        fus.emplace_back(std::move(fu));
    }
    auto post_time = (std::chrono::nanoseconds(std::chrono::high_resolution_clock::now() - start).count() / 1000);
    boost::wait_for_all(fus.begin(), fus.end());
    std::cout << "test_loggable_composite_threadpool_scheduler_lockfree, average post in us: " << post_time / LOOP_COUNT <<std::endl;
}

namespace
{
struct Servant : boost::asynchronous::trackable_servant<>
{
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler,long tpsize,long queue_size)
        : boost::asynchronous::trackable_servant<>(scheduler
        , boost::asynchronous::make_shared_scheduler_proxy<
            boost::asynchronous::multiqueue_threadpool_scheduler<
                  boost::asynchronous::lockfree_queue<>>>(tpsize,queue_size))
    {}
    std::future<void> foo()
    {
        std::shared_ptr<std::promise<void> > p(new std::promise<void>);
        std::future<void> fu = p->get_future();
        auto start = std::chrono::high_resolution_clock::now();
        for (auto i=0; i< LOOP_COUNT; ++i)
        {
            post_callback(
                   []()mutable{},
                   [this,p](boost::asynchronous::expected<void>)
                   {
                        if(++cpt_ == LOOP_COUNT)
                            p->set_value();
                   },
                   "loooooooooooooooooonnnnnnnnnnnnnnngggggggggggggggggg",1,1
            );
        }
        auto post_time = (std::chrono::nanoseconds(std::chrono::high_resolution_clock::now() - start).count() / 1000);
        std::cout << "test_servant_post_time, average post in us: " << post_time / LOOP_COUNT <<std::endl;
        return fu;
    }
    std::future<void> foo2()
    {
        std::shared_ptr<std::promise<void> > p(new std::promise<void>);
        std::future<void> fu = p->get_future();
        std::vector<std::shared_ptr<std::vector<int>>> datas;
        for (auto i=0; i< LOOP_COUNT; ++i)
        {
            std::shared_ptr<std::vector<int>> data = std::make_shared<std::vector<int>>(10000,1);
            std::mt19937 mt(static_cast<unsigned int>(std::time(nullptr)));
            std::uniform_int_distribution<> dis(0, 1000);
            std::generate(data->begin(), data->end(), std::bind(dis, std::ref(mt)));
            datas.push_back(data);
        }
        auto start = std::chrono::high_resolution_clock::now();
        for (auto i=0; i< LOOP_COUNT; ++i)
        {
            post_callback(
                   [data=datas[i]]()mutable
                   {                        
                        //std::sort(data.begin(),data.end(),std::less<int>());
                        return boost::asynchronous::parallel_sort(data->begin(),data->end(),std::less<int>(),1500);
                   },
                   [this,p,data=datas[i]](boost::asynchronous::expected<void>)
                   {
                        if(++cpt_ == LOOP_COUNT)
                            p->set_value();
                   },
                   "loooooooooooooooooonnnnnnnnnnnnnnngggggggggggggggggg",1,1
            );
        }
        auto post_time = (std::chrono::nanoseconds(std::chrono::high_resolution_clock::now() - start).count() / 1000);
        std::cout << "test_servant_post_time_load, average post in us: " << post_time / LOOP_COUNT <<std::endl;
        return fu;
    }
    int cpt_ =0;
};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s,long tpsize,long queue_size):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s,tpsize, queue_size)
    {}
    // caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER(foo)
    BOOST_ASYNC_FUTURE_MEMBER(foo2)
};

typedef boost::asynchronous::any_loggable servant_job;
struct Servant2 : boost::asynchronous::trackable_servant<servant_job,servant_job>
{
    Servant2(boost::asynchronous::any_weak_scheduler<servant_job> scheduler,long tpsize,long queue_size)
        : boost::asynchronous::trackable_servant<servant_job,servant_job>(scheduler
        , boost::asynchronous::make_shared_scheduler_proxy<
            boost::asynchronous::multiqueue_threadpool_scheduler<
                  boost::asynchronous::lockfree_queue<servant_job>>>(tpsize,queue_size))
    {}
    std::future<void> foo()
    {
        std::shared_ptr<std::promise<void> > p(new std::promise<void>);
        std::future<void> fu = p->get_future();
        auto start = std::chrono::high_resolution_clock::now();
        for (auto i=0; i< LOOP_COUNT; ++i)
        {
            post_callback(
                   [](){},
                   [this,p](boost::asynchronous::expected<void>)
                   {
                        if(++cpt_ == LOOP_COUNT)
                            p->set_value();
                   },
                   "loooooooooooooooooonnnnnnnnnnnnnnngggggggggggggggggg",1,1
            );
        }
        auto post_time = (std::chrono::nanoseconds(std::chrono::high_resolution_clock::now() - start).count() / 1000);
        std::cout << "test_servant_post_time_log, average post in us: " << post_time / LOOP_COUNT <<std::endl;
        return fu;
    }
    int cpt_ =0;
};
class ServantProxy2 : public boost::asynchronous::servant_proxy<ServantProxy2,Servant2,servant_job>
{
public:
    template <class Scheduler>
    ServantProxy2(Scheduler s,long tpsize,long queue_size):
        boost::asynchronous::servant_proxy<ServantProxy2,Servant2,servant_job>(s,tpsize, queue_size)
    {}
    // caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER(foo)
};
}

void test_servant_post_time(long tpsize,long queue_size)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                            boost::asynchronous::single_thread_scheduler<
                                 boost::asynchronous::lockfree_queue<>>>();
    ServantProxy proxy(scheduler,tpsize,queue_size);
    auto fu = proxy.foo();
    fu.get().get();
}
void test_servant_post_time_load(long tpsize,long queue_size)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                            boost::asynchronous::single_thread_scheduler<
                                 boost::asynchronous::lockfree_queue<>>>();
    ServantProxy proxy(scheduler,tpsize,queue_size);
    auto fu = proxy.foo2();
    fu.get().get();
}
void test_servant_post_time_log(long tpsize,long queue_size)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                            boost::asynchronous::single_thread_scheduler<
                                 boost::asynchronous::lockfree_queue<servant_job>>>();
    ServantProxy2 proxy(scheduler,tpsize,queue_size);
    auto fu = proxy.foo();
    fu.get().get();
}
int main( int argc, const char *argv[] )
{
    long tpsize = (argc>1) ? strtol(argv[1],0,0) : boost::thread::hardware_concurrency();
    long queue_size = (argc>2) ? strtol(argv[2],0,0) : 500;
    std::cout << "tpsize=" << tpsize << std::endl;
    std::cout << "queue size=" << queue_size << std::endl;

    test_callable_multiqueue_threadpool_scheduler_lockfree(tpsize,queue_size);
    test_loggable_multiqueue_threadpool_scheduler_lockfree(tpsize,queue_size);
    test_loggable_composite_threadpool_scheduler_lockfree(tpsize,queue_size);
    test_servant_post_time(tpsize,queue_size);
    test_servant_post_time_log(tpsize,queue_size);
    test_servant_post_time_load(tpsize,queue_size);
    return 0;
}
