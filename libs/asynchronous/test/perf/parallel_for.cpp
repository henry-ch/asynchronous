// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <algorithm>
#include <iostream>
#include <vector>

#include <boost/smart_ptr/shared_array.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/any_queue_container.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_for.hpp>
#include <boost/asynchronous/algorithm/parallel_generate.hpp>

using namespace std;

#define SIZE 100000000
#define LOOP 1

float Foo(float f)
{
    return std::cos(std::tan(f*3.141592654 + 2.55756 * 0.42));
}

boost::chrono::high_resolution_clock::time_point servant_time;
double servant_intern=0.0;
long tpsize = 12;
long tasks = 48;

boost::asynchronous::any_shared_scheduler_proxy<> scheduler;

void ParallelAsyncPostFuture(float a[], size_t n)
{
    long tasksize = SIZE / tasks;
    servant_time = boost::chrono::high_resolution_clock::now();
    auto fu = boost::asynchronous::post_future(scheduler,
               [a,n,tasksize]()
               {
                   return boost::asynchronous::parallel_for(a,a+n,
                                                            // first version
                                                            /*[](float& i)
                                                            {
                                                               i = Foo(i);
                                                            },*/
                                                            // second version
                                                            [](float* beg, float* end)
                                                            {
                                                               for(;beg!=end;++beg)
                                                               {
                                                                    *beg = Foo(*beg);
                                                               }
                                                            },tasksize,"",0);
               });
    fu.get();
    servant_intern += (boost::chrono::nanoseconds(boost::chrono::high_resolution_clock::now() - servant_time).count() / 1000);
}



void test(void(*pf)(float [], size_t ))
{
    boost::shared_array<float> a (new float[SIZE]);
    /* in case we have a huge input or compßlicated generate function
     long tasksize = SIZE / tasks;
     auto fu = boost::asynchronous::post_future(
                scheduler,
                [&]{return boost::asynchronous::parallel_generate(a.get(), a.get()+SIZE,rand,tasksize);});
    fu.get();*/
    std::generate(a.get(), a.get()+SIZE,rand);
    (*pf)(a.get(),SIZE);
}

int main( int argc, const char *argv[] ) 
{   
    tpsize = (argc>1) ? strtol(argv[1],0,0) : boost::thread::hardware_concurrency();
    tasks = (argc>2) ? strtol(argv[2],0,0) : 500;
    std::cout << "tpsize=" << tpsize << std::endl;
    std::cout << "tasks=" << tasks << std::endl;

    scheduler =  boost::asynchronous::make_shared_scheduler_proxy<
            boost::asynchronous::multiqueue_threadpool_scheduler<
                    boost::asynchronous::lockfree_queue<>,
                    boost::asynchronous::default_find_position< boost::asynchronous::sequential_push_policy>,
                    //boost::asynchronous::default_save_cpu_load<10,80000,1000>
                    boost::asynchronous::no_cpu_load_saving
                >>(tpsize,tasks/tpsize);
    // set processor affinity to improve cache usage. We start at core 0, until tpsize-1
    scheduler.processor_bind(0);

    for (int i=0;i<LOOP;++i)
    {     
        test(ParallelAsyncPostFuture);
    }
    printf ("%24s: time = %.1f usec\n","parallel async cb intern", servant_intern);
    return 0;
}
