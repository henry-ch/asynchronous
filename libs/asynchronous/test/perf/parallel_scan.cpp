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
#include <boost/asynchronous/algorithm/parallel_scan.hpp>
#include <boost/asynchronous/algorithm/parallel_generate.hpp>

using namespace std;

#define SIZE 100000000
#define LOOP 1

boost::chrono::high_resolution_clock::time_point servant_time;
double servant_intern=0.0;
double serial_duration=0.0;

long tpsize = 12;
long tasks = 48;
using element=float;
using container = std::vector<element>;
using Iterator = std::vector<element>::iterator;

boost::asynchronous::any_shared_scheduler_proxy<> scheduler;

void generate(container& data, unsigned elements, unsigned /*dist*/)
{
    data = container(elements,1);
    //std::random_device rd;
    //std::mt19937 mt(rd());
    //std::mt19937 mt(static_cast<unsigned int>(std::time(nullptr)));
    //std::uniform_int_distribution<> dis(0, dist);
    auto fu = boost::asynchronous::post_future(
                scheduler,
                [&]{return boost::asynchronous::parallel_generate(data.begin(),data.end(),rand/*std::bind(dis, std::ref(mt))*/,1024);});
    fu.get();
    //std::generate(data.begin(), data.end(), std::bind(dis, std::ref(mt)));
}
template <class Iterator,class OutIterator,class T,class Combine>
OutIterator inclusive_scan(Iterator beg, Iterator end, OutIterator out, T init, Combine c)
{
    for (;beg != end; ++beg)
    {
        init = c(init,*beg);
        *out++ = init;
    }
    return out;
}

void ParallelAsyncPostFuture(Iterator beg, Iterator end, Iterator out, element init)
{    
    long tasksize = SIZE / tasks;
    servant_time = boost::chrono::high_resolution_clock::now();
    auto fu = boost::asynchronous::post_future(scheduler,
               [beg,end,out,init,tasksize]()
               {
                   return boost::asynchronous::parallel_scan(beg,end,out,init,
                                                             [](Iterator beg, Iterator end)
                                                             {
                                                               element r=0;
                                                               for (;beg != end; ++beg)
                                                               {
                                                                   r = r + *beg;
                                                               }
                                                               return r;
                                                             },
                                                             std::plus<element>(),
                                                             [](Iterator beg, Iterator end, Iterator out, element init) mutable
                                                             {
                                                               for (;beg != end; ++beg)
                                                               {
                                                                   init = *beg + init;
                                                                   *out++ = init;
                                                               };
                                                             },
                                                             tasksize,"",0);
               });
    fu.get();
    servant_intern += (boost::chrono::nanoseconds(boost::chrono::high_resolution_clock::now() - servant_time).count() / 1000);
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
        container data;
        container res(SIZE,0);
        generate(data,SIZE,SIZE/10);
        container data2 = data;
        container res2(SIZE,0);

        ParallelAsyncPostFuture(data.begin(),data.end(),res.begin(),0);

        // serial version
        boost::chrono::high_resolution_clock::time_point start = boost::chrono::high_resolution_clock::now();
        inclusive_scan(data2.begin(),data2.end(),res2.begin(),0,std::plus<element>());
        serial_duration += (boost::chrono::nanoseconds(boost::chrono::high_resolution_clock::now() - start).count() / 1000);


    }
    printf ("%24s: time = %.1f usec\n","parallel_scan", servant_intern);
    printf ("%24s: time = %.1f usec\n","serial_scan", serial_duration);
    return 0;
}
