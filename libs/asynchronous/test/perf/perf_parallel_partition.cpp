// Boost.Asynchronous library
//  Copyright (C) Christophe Henry, Tobias Holl 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <vector>
#include <memory>

#include <boost/smart_ptr/shared_array.hpp>

#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/any_queue_container.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_partition.hpp>

#include <boost/asynchronous/helpers/random_provider.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include <boost/asynchronous/algorithm/parallel_fill.hpp>
#include <boost/asynchronous/algorithm/parallel_generate.hpp>
#include <boost/asynchronous/algorithm/parallel_iota.hpp>

using namespace std;


#define LOOP 1

#define NELEM 100000000
#define SORTED_TYPE float

//#define NELEM 10000000
//#define SORTED_TYPE std::string

typename boost::chrono::high_resolution_clock::time_point servant_time;
double servant_intern=0.0;
long tpsize = 12;
long tasks = 500;

boost::asynchronous::any_shared_scheduler_proxy<> scheduler;

SORTED_TYPE compare_with = NELEM/2;

void ParallelAsyncPostCb(std::vector<SORTED_TYPE>& a)
{
    std::shared_ptr<std::vector<SORTED_TYPE>> vec = std::make_shared<std::vector<SORTED_TYPE>>(a);
    servant_time = boost::chrono::high_resolution_clock::now();
    auto fu = boost::asynchronous::post_future(
                scheduler,
                [vec]()mutable{
                         return boost::asynchronous::parallel_partition(std::move(*vec),
                                                                        [](SORTED_TYPE const& i)
                                                                        {
                                                                           // cheap version
                                                                           //return i < compare_with;
                                                                           // expensive version
                                                                           return i/(i*i)/i/i < compare_with/(compare_with * compare_with)/compare_with/compare_with;
                                                                        },tpsize,"",0);
                       },
                "",0);
    auto res_pair = std::move(fu.get());
    servant_intern += (boost::chrono::nanoseconds(boost::chrono::high_resolution_clock::now() - servant_time).count() / 1000000);

    {
        auto seq_start = boost::chrono::high_resolution_clock::now();
        std::partition(a.begin(),a.end(),[](SORTED_TYPE const& i)
        {
            // cheap version
            //return i < compare_with;
            // expensive version
            return i/(i*i)/i/i < compare_with/(compare_with * compare_with)/compare_with/compare_with;
        });
        auto seq_stop = boost::chrono::high_resolution_clock::now();
        double seq_time = (boost::chrono::nanoseconds(seq_stop - seq_start).count() / 1000000);
        printf ("\n%50s: time = %.1f msec\n","sequential", seq_time);
    }
}
void test_sorted_elements(void(*pf)(std::vector<SORTED_TYPE>& ))
{
    std::vector<SORTED_TYPE> a(NELEM);
    auto fu = boost::asynchronous::post_future(
                scheduler,
                [&]{
                    return boost::asynchronous::parallel_iota(a.begin(), a.end(), NELEM, 1024);
                });
    fu.get();
    (*pf)(a);
}
void test_random_elements_many_repeated(void(*pf)(std::vector<SORTED_TYPE>& ))
{
    std::vector<SORTED_TYPE> a(NELEM);
    auto fu = boost::asynchronous::post_future(
                scheduler,
                [&]{
                    return boost::asynchronous::parallel_generate(
                                a.begin(), a.end(),
                                []{
                                    boost::random::uniform_int_distribution<> distribution(0, 9999); // 9999 is inclusive, rand() % 10000 was exclusive
                                    return boost::asynchronous::random_provider<boost::random::mt19937>::generate(distribution);
                                }, 1024);
                });
    fu.get();
    (*pf)(a);
}
void test_random_elements_few_repeated(void(*pf)(std::vector<SORTED_TYPE>& ))
{
    std::vector<SORTED_TYPE> a(NELEM);
    auto fu = boost::asynchronous::post_future(
                scheduler,
                [&]{
                    return boost::asynchronous::parallel_generate(
                                a.begin(), a.end(),
                                []{
                                    return boost::asynchronous::random_provider<boost::random::mt19937>::generate();
                                }, 1024);
                });
    fu.get();
    (*pf)(a);
}
void test_random_elements_quite_repeated(void(*pf)(std::vector<SORTED_TYPE>& ))
{
    std::vector<SORTED_TYPE> a(NELEM);
    auto fu = boost::asynchronous::post_future(
                scheduler,
                [&]{
                    return boost::asynchronous::parallel_generate(
                                a.begin(), a.end(),
                                []{
                                    boost::random::uniform_int_distribution<> distribution(0, NELEM/2 - 1);
                                    return boost::asynchronous::random_provider<boost::random::mt19937>::generate(distribution);
                                }, 1024);
                });
    fu.get();
    (*pf)(a);
}
void test_reversed_sorted_elements(void(*pf)(std::vector<SORTED_TYPE>& ))
{
    std::vector<SORTED_TYPE> a(NELEM);
    auto fu = boost::asynchronous::post_future(
                scheduler,
                [&]{
                    return boost::asynchronous::parallel_for(
                                boost::asynchronous::parallel_iota(std::move(a), 0, 1024),
                                [](SORTED_TYPE& i){
                                    i = (NELEM * 2) - i;
                                },
                                1024);
                });
    a = std::move(fu.get());
    (*pf)(a);
}
void test_equal_elements(void(*pf)(std::vector<SORTED_TYPE>& ))
{
    std::vector<SORTED_TYPE> a(NELEM);
    auto fu = boost::asynchronous::post_future(
                scheduler,
                [&]{
                    return boost::asynchronous::parallel_fill(a.begin(), a.end(), NELEM, 1024);
                });
    fu.get();
    (*pf)(a);
}
int main( int argc, const char *argv[] )
{
    tpsize = (argc>1) ? strtol(argv[1],0,0) : boost::thread::hardware_concurrency();
    tasks = (argc>2) ? strtol(argv[2],0,0) : 500;
    std::cout << "tasks=" << tasks << std::endl;
    std::cout << "tpsize=" << tpsize << std::endl;

    scheduler = boost::asynchronous::make_shared_scheduler_proxy<
            boost::asynchronous::multiqueue_threadpool_scheduler<
                    boost::asynchronous::lockfree_queue<>,
                    boost::asynchronous::default_find_position< boost::asynchronous::sequential_push_policy>,
                boost::asynchronous::default_save_cpu_load<10,80000,1000>
                //boost::asynchronous::no_cpu_load_saving
                >>(tpsize);
    // set processor affinity to improve cache usage. We start at core 0, until tpsize-1
    scheduler.processor_bind(0);

    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {
        test_random_elements_many_repeated(ParallelAsyncPostCb);
    }
    printf ("%50s: time = %.1f msec\n","test_random_elements_many_repeated", servant_intern);

    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {
        test_random_elements_few_repeated(ParallelAsyncPostCb);
    }
    printf ("%50s: time = %.1f msec\n","test_random_elements_few_repeated", servant_intern);

    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {
        test_random_elements_quite_repeated(ParallelAsyncPostCb);
    }
    printf ("%50s: time = %.1f msec\n","test_random_elements_quite_repeated", servant_intern);

    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {
        test_sorted_elements(ParallelAsyncPostCb);
    }
    printf ("%50s: time = %.1f msec\n","test_sorted_elements", servant_intern);

    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {
        test_reversed_sorted_elements(ParallelAsyncPostCb);
    }
    printf ("%50s: time = %.1f msec\n","test_reversed_sorted_elements", servant_intern);

    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {
        test_equal_elements(ParallelAsyncPostCb);
    }
    printf ("%50s: time = %.1f msec\n","test_equal_elements", servant_intern);

    std::cout << std::endl;

    return 0;
}
