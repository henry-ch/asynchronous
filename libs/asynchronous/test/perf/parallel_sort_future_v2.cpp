// Boost.Asynchronous library
//  Copyright (C) Christophe Henry, Tobias Holl 2015
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

#include <boost/asynchronous/helpers/lazy_irange.hpp>
#include <boost/asynchronous/algorithm/parallel_copy.hpp>
#include <boost/asynchronous/algorithm/parallel_fill.hpp>
#include <boost/asynchronous/algorithm/parallel_generate.hpp>

#include <boost/asynchronous/helpers/random_provider.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

using namespace std;

#define LOOP 1

struct LongOne
{
    LongOne(uint32_t i = 0)
        :data(10,0)
    {
        data[0]=i;
    }
    LongOne& operator= (LongOne const& rhs)
    {
        data = rhs.data;
        return *this;
    }

    std::vector<long> data;
};
inline bool operator< (const LongOne& lhs, const LongOne& rhs){ return rhs.data[0] < lhs.data[0]; }
#define NELEM 1000000
#define SORTED_TYPE LongOne
#define NO_SPREADSORT

//#define NELEM 200000000
//#define SORTED_TYPE uint32_t

//#define NELEM 10000000
//#define SORTED_TYPE std::string

//#define NELEM 200000000
//#define SORTED_TYPE double

typename boost::chrono::high_resolution_clock::time_point servant_time;
double servant_intern=0.0;
long tpsize = 12;
long tasks = 48;

boost::asynchronous::any_shared_scheduler_proxy<> pool;

template <class T, class U>
typename boost::disable_if<boost::mpl::or_<boost::is_same<T,U>,boost::is_same<LongOne,U>>,U >::type
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
template <class T, class U>
typename boost::enable_if<boost::is_same<LongOne,U>,U >::type
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
    long tasksize = NELEM / tasks;
    servant_time = boost::chrono::high_resolution_clock::now();
    boost::future<std::vector<SORTED_TYPE>> fu = boost::asynchronous::post_future(pool,
    [a=std::move(a),n,tasksize]()mutable
    {
        return boost::asynchronous::parallel_sort2(std::move(a),SORT_FCT(),tasksize,"",0);
    }
    ,"",0);
    fu.wait();
    servant_intern += (boost::chrono::nanoseconds(boost::chrono::high_resolution_clock::now() - servant_time).count() / 1000000);           
}
void ParallelAsyncPostCbSpreadsort(std::vector<SORTED_TYPE> a, size_t n)
{
#ifndef NO_SPREADSORT
    long tasksize = NELEM / tasks;
    servant_time = boost::chrono::high_resolution_clock::now();
    boost::future<std::vector<SORTED_TYPE>> fu = boost::asynchronous::post_future(pool,
    [a=std::move(a),n,tasksize]()mutable
    {
        return boost::asynchronous::parallel_spreadsort2(std::move(a),SORT_FCT(),tasksize,"",0);
    }
    ,"",0);
    fu.wait();
    servant_intern += (boost::chrono::nanoseconds(boost::chrono::high_resolution_clock::now() - servant_time).count() / 1000000); 
#endif
}
    


void test_sorted_elements(void(*pf)(std::vector<SORTED_TYPE>, size_t ))
{
    std::vector<SORTED_TYPE> a (NELEM);
    auto lazy = boost::asynchronous::lazy_irange(
                  0, NELEM,
                  [](uint32_t index) {
                      return test_cast<decltype(index), SORTED_TYPE>(index + NELEM);\
                  });
    auto fu = boost::asynchronous::post_future(
                pool,
                [&]{
                    return boost::asynchronous::parallel_copy(
                                lazy.begin(), lazy.end(),
                                a.begin(),
                                NELEM / tasks);
                });
    fu.get();
    /* serial version:
    for ( uint32_t i = 0 ; i < NELEM ; ++i)
    {
        a[i] = test_cast<uint32_t,SORTED_TYPE>( i+NELEM) ;
    }
     */
    (*pf)(std::move(a),NELEM);
}
void test_random_elements_many_repeated(void(*pf)(std::vector<SORTED_TYPE>, size_t ))
{
    std::vector<SORTED_TYPE> a (NELEM);
    auto fu = boost::asynchronous::post_future(
                pool,
                [&]{
                    return boost::asynchronous::parallel_generate(
                                a.begin(), a.end(),
                                []{
                                    boost::random::uniform_int_distribution<> distribution(0, 9999); // 9999 is inclusive, rand() % 10000 was exclusive
                                    uint32_t gen = boost::asynchronous::random_provider<boost::random::mt19937>::generate(distribution);
                                    return test_cast<uint32_t, SORTED_TYPE>(gen);
                                }, NELEM / tasks);
                });
    fu.get();
    (*pf)(std::move(a),NELEM);
}
void test_random_elements_few_repeated(void(*pf)(std::vector<SORTED_TYPE>, size_t ))
{
    std::vector<SORTED_TYPE> a (NELEM);
    auto fu = boost::asynchronous::post_future(
                pool,
                [&]{
                    return boost::asynchronous::parallel_generate(
                                a.begin(), a.end(),
                                []{
                                    uint32_t gen = boost::asynchronous::random_provider<boost::random::mt19937>::generate();
                                    return test_cast<uint32_t, SORTED_TYPE>(gen);
                                }, NELEM / tasks);
                });
    fu.get();
    (*pf)(std::move(a),NELEM);
}
void test_random_elements_quite_repeated(void(*pf)(std::vector<SORTED_TYPE>, size_t ))
{
    std::vector<SORTED_TYPE> a (NELEM);
    auto fu = boost::asynchronous::post_future(
                pool,
                [&]{
                    return boost::asynchronous::parallel_generate(
                                a.begin(), a.end(),
                                []{
                                    boost::random::uniform_int_distribution<> distribution(0, NELEM / 2 - 1);
                                    uint32_t gen = boost::asynchronous::random_provider<boost::random::mt19937>::generate(distribution);
                                    return test_cast<uint32_t, SORTED_TYPE>(gen);
                                }, NELEM / tasks);
                });
    fu.get();
    (*pf)(std::move(a),NELEM);
}
void test_reversed_sorted_elements(void(*pf)(std::vector<SORTED_TYPE>, size_t ))
{
    std::vector<SORTED_TYPE> a (NELEM);
    auto lazy = boost::asynchronous::lazy_irange(
                  0, NELEM,
                  [](uint32_t index) {
                      return test_cast<decltype(index), SORTED_TYPE>((NELEM << 1) - index);\
                  });
    auto fu = boost::asynchronous::post_future(
                pool,
                [&]{
                    return boost::asynchronous::parallel_copy(
                                lazy.begin(), lazy.end(),
                                a.begin(),
                                NELEM / tasks);
                });
    fu.get();
    /* serial version:
    for ( uint32_t i = 0 ; i < NELEM ; ++i)
    {
        a[i] = test_cast<uint32_t,SORTED_TYPE>((NELEM<<1) -i) ;
    }
     */
    (*pf)(std::move(a),NELEM);
}
void test_equal_elements(void(*pf)(std::vector<SORTED_TYPE>, size_t ))
{
    std::vector<SORTED_TYPE> a (NELEM);
    auto fu = boost::asynchronous::post_future(
                pool,
                [&]{
                    return boost::asynchronous::parallel_fill(a.begin(), a.end(), test_cast<uint32_t, SORTED_TYPE>(NELEM), NELEM / tasks);
                });
    fu.get();
    (*pf)(std::move(a),NELEM);
}
int main( int argc, const char *argv[] ) 
{           
    tpsize = (argc>1) ? strtol(argv[1],0,0) : boost::thread::hardware_concurrency();
    tasks = (argc>2) ? strtol(argv[2],0,0) : 500;
    std::cout << "tpsize=" << tpsize << std::endl;
    std::cout << "tasks=" << tasks << std::endl;   
    
    pool = boost::asynchronous::create_shared_scheduler_proxy(
                new boost::asynchronous::multiqueue_threadpool_scheduler<
                        boost::asynchronous::lockfree_queue<>,
                        boost::asynchronous::default_find_position< boost::asynchronous::sequential_push_policy>,
                        boost::asynchronous::no_cpu_load_saving
                    >(tpsize,tasks));

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
