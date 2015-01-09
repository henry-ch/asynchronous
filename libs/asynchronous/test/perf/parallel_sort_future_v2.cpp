/*
    Copyright 2005-2013 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks.

    Threading Building Blocks is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    version 2 as published by the Free Software Foundation.

    Threading Building Blocks is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Threading Building Blocks; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    As a special exception, you may use this file as part of a free software
    library without restriction.  Specifically, if other files instantiate
    templates or use macros or inline functions from this file, or you compile
    this file and link it with other files to produce an executable, this
    file does not by itself cause the resulting executable to be covered by
    the GNU General Public License.  This exception does not however
    invalidate any other reasons why the executable file might be covered by
    the GNU General Public License.
*/


#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>

// The performance of this example can be significantly better when
// the objects are allocated by the scalable_allocator instead of the
// default "operator new".  The reason is that the scalable_allocator
// typically packs small objects more tightly than the default "operator new",
// resulting in a smaller memory footprint, and thus more efficient use of
// cache and virtual memory.  Also the scalable_allocator works faster for
// multi-threaded allocations.
//
// Pass stdmalloc as the 1st command line parameter to use the default "operator new"
// and see the performance difference.
#include <algorithm>
#include <iostream>
#include <vector>

#include <boost/smart_ptr/shared_array.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>

#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/any_queue_container.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_sort.hpp>

using namespace std;

#define LOOP 1

//#define NELEM 200000000
//#define SORTED_TYPE uint32_t

#define NELEM 10000000
#define SORTED_TYPE std::string

//#define NELEM 200000000
//#define SORTED_TYPE double

typename boost::chrono::high_resolution_clock::time_point servant_time;
double servant_intern=0.0;
long tpsize = 12;
long tasks = 48;

template <class T, class U>
typename boost::disable_if<boost::is_same<T,U>,U >::type
test_cast(T const& t)
{
    return boost::lexical_cast<U>(t);
}
template <class T, class U>
typename boost::enable_if<boost::is_same<T,U>,U >::type
test_cast(T const& t)
{
    return t;
}

//#define USE_SERIALIZABLE 
// we pretend to be serializable to use a different version of the sort algorithm
#ifdef USE_SERIALIZABLE
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
#define SORT_FCT increasing_sort_subtask
#else
#define SORT_FCT std::less<SORTED_TYPE>
#endif


void ParallelAsyncPostCb(std::vector<SORTED_TYPE> a, size_t n)
{
    auto pool = boost::asynchronous::create_shared_scheduler_proxy(
                new boost::asynchronous::multiqueue_threadpool_scheduler<
                        boost::asynchronous::lockfree_queue<>,
                        boost::asynchronous::default_find_position< boost::asynchronous::sequential_push_policy>,
                        boost::asynchronous::no_cpu_load_saving
                    >(tpsize,tasks));
    
    long tasksize = NELEM / tasks;
    servant_time = boost::chrono::high_resolution_clock::now();
    boost::future<std::vector<SORTED_TYPE>> fu = boost::asynchronous::post_future(pool,
    [a=std::move(a),n,tasksize]()mutable
    {
        return boost::asynchronous::parallel_sort_move(std::move(a),SORT_FCT(),tasksize,"",0);
    }
    ,"",0);
    fu.get();
    servant_intern += (boost::chrono::nanoseconds(boost::chrono::high_resolution_clock::now() - servant_time).count() / 1000000);           
}
void ParallelAsyncPostCbSpreadsort(std::vector<SORTED_TYPE> a, size_t n)
{
    auto pool = boost::asynchronous::create_shared_scheduler_proxy(
                new boost::asynchronous::multiqueue_threadpool_scheduler<
                        boost::asynchronous::lockfree_queue<>,
                        boost::asynchronous::default_find_position< boost::asynchronous::sequential_push_policy>,
                        boost::asynchronous::no_cpu_load_saving
                    >(tpsize,tasks));
    
    long tasksize = NELEM / tasks;
    servant_time = boost::chrono::high_resolution_clock::now();
    boost::future<std::vector<SORTED_TYPE>> fu = boost::asynchronous::post_future(pool,
    [a=std::move(a),n,tasksize]()mutable
    {
        return boost::asynchronous::parallel_spreadsort_move(std::move(a),SORT_FCT(),tasksize,"",0);
    }
    ,"",0);
    fu.get();
    servant_intern += (boost::chrono::nanoseconds(boost::chrono::high_resolution_clock::now() - servant_time).count() / 1000000); 
}
    


void test_sorted_elements(void(*pf)(std::vector<SORTED_TYPE>, size_t ))
{
    std::vector<SORTED_TYPE> a (NELEM);
    for ( uint32_t i = 0 ; i < NELEM ; ++i)
    {
        a[i] = test_cast<uint32_t,SORTED_TYPE>( i+NELEM) ;
    }
    (*pf)(std::move(a),NELEM);
}
void test_random_elements_many_repeated(void(*pf)(std::vector<SORTED_TYPE>, size_t ))
{
    std::vector<SORTED_TYPE> a (NELEM);
    for ( uint32_t i = 0 ; i < NELEM ; ++i)
    {
        a[i] = test_cast<uint32_t,SORTED_TYPE>(rand() % 10000) ;
    }
    (*pf)(std::move(a),NELEM);
}
void test_random_elements_few_repeated(void(*pf)(std::vector<SORTED_TYPE>, size_t ))
{
    std::vector<SORTED_TYPE> a (NELEM);
    for ( uint32_t i = 0 ; i < NELEM ; ++i)
    {
        a[i] = test_cast<uint32_t,SORTED_TYPE>(rand());
    }
    (*pf)(std::move(a),NELEM);
}
void test_random_elements_quite_repeated(void(*pf)(std::vector<SORTED_TYPE>, size_t ))
{
    std::vector<SORTED_TYPE> a (NELEM);
    for ( uint32_t i = 0 ; i < NELEM ; ++i)
    {
        a[i] = test_cast<uint32_t,SORTED_TYPE>(rand() % (NELEM/2)) ;
    }
    (*pf)(std::move(a),NELEM);
}
void test_reversed_sorted_elements(void(*pf)(std::vector<SORTED_TYPE>, size_t ))
{
    std::vector<SORTED_TYPE> a (NELEM);
    for ( uint32_t i = 0 ; i < NELEM ; ++i)
    {
        a[i] = test_cast<uint32_t,SORTED_TYPE>((NELEM<<1) -i) ;
    }
    (*pf)(std::move(a),NELEM);
}
void test_equal_elements(void(*pf)(std::vector<SORTED_TYPE>, size_t ))
{
    std::vector<SORTED_TYPE> a (NELEM);
    for ( uint32_t i = 0 ; i < NELEM ; ++i)
    {
        a[i] = test_cast<uint32_t,SORTED_TYPE>(NELEM) ;
    }
    (*pf)(std::move(a),NELEM);
}
int main( int argc, const char *argv[] ) 
{           
    tpsize = (argc>1) ? strtol(argv[1],0,0) : boost::thread::hardware_concurrency();
    tasks = (argc>2) ? strtol(argv[2],0,0) : 500;
    std::cout << "tpsize=" << tpsize << std::endl;
    std::cout << "tasks=" << tasks << std::endl;   
    
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
    
    // boost spreadsort
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {     
        test_random_elements_many_repeated(ParallelAsyncPostCbSpreadsort);
    }
    printf ("%50s: time = %.1f msec\n","Spreadsort: test_random_elements_many_repeated", servant_intern);
    
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {     
        test_random_elements_few_repeated(ParallelAsyncPostCbSpreadsort);
    }
    printf ("%50s: time = %.1f msec\n","Spreadsort: test_random_elements_few_repeated", servant_intern);
    
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {     
        test_random_elements_quite_repeated(ParallelAsyncPostCbSpreadsort);
    }
    printf ("%50s: time = %.1f msec\n","Spreadsort: test_random_elements_quite_repeated", servant_intern);
    
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {     
        test_sorted_elements(ParallelAsyncPostCbSpreadsort);
    }
    printf ("%50s: time = %.1f msec\n","Spreadsort: test_sorted_elements", servant_intern);
    
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {     
        test_reversed_sorted_elements(ParallelAsyncPostCbSpreadsort);
    }
    printf ("%50s: time = %.1f msec\n","Spreadsort: test_reversed_sorted_elements", servant_intern);
    
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {     
        test_equal_elements(ParallelAsyncPostCbSpreadsort);
    }
    printf ("%50s: time = %.1f msec\n","Spreadsort: test_equal_elements", servant_intern);
    return 0;
}
