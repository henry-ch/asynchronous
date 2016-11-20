//----------------------------------------------------------------------------
/// @file benchmark.cpp
/// @brief Benchmark of several sort methods with different objects
///
/// @author Copyright (c) 2016 Francisco José Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanying file LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt )
///
///         This program use for comparison purposes, the Threading Building
///         Blocks which license is the GNU General Public License, version 2
///         as  published  by  the  Free Software Foundation.
///
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#define BOOST_ASYNCHRONOUS_USE_BOOST_SORT

#include <algorithm>
#include <iostream>
#include <random>
#include <stdlib.h>
#include <vector>

#include <omp.h>
#include <parallel/algorithm>
#include <parallel/basic_iterator.h>
#include <parallel/features.h>
#include <parallel/parallel.h>

#include <boost/sort/parallel/sort.hpp>
#include <boost/sort/spreadsort/spreadsort.hpp>

#include "int_array.hpp"
#include <time_measure.hpp>
#include <file_vector.hpp>
#include <boost/smart_ptr/shared_array.hpp>

#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/algorithm/parallel_sort.hpp>
#include <boost/asynchronous/algorithm/parallel_quicksort.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#define NELEM 100000000
#define NMAXSTRING 10000000

using namespace std;
namespace bsp_util = boost::sort::parallel::detail::util;
namespace bsort = boost::sort::parallel;

using bsp_util::time_point;
using bsp_util::now;
using bsp_util::subtract_time;
using bsp_util::fill_vector_uint64;
using bsp_util::write_file_uint64;

void Generator_sorted(void);
void Generator_reverse_sorted(void);
void Generator_uint64(void);
void Generator_string(void);

template <class IA>
void Generator(uint64_t N);

template <class IA, class compare>
int Test(std::vector<IA> &B, compare comp = compare());

template <class IA>
int Test_spreadsort(std::vector<IA> &B);

int main(int argc, char *argv[])
{    
    cout << "\n\n";
    cout << "************************************************************\n";
    cout << "**                                                        **\n";
    cout << "**     B O O S T :: S O R T :: P A R A L L E L            **\n";
    cout << "**                                                        **\n";
    cout << "**               B E N C H M A R K                        **\n";
    cout << "**                                                        **\n";
    cout << "************************************************************\n";
    cout << std::endl;
    std::cout.flush();
    int code = system("lscpu");
    std::cout.flush();
    cout << "\n";

    Generator_sorted();
    Generator_reverse_sorted();
    Generator_uint64();
    Generator_string();

    cout << "=============================================================\n";
    cout << "=            OBJECT COMPARISON                              =\n";
    cout << "=          ---------------------                            =\n";
    cout << "=                                                           =\n";
    cout << "= The objects are arrays of 64 bits numbers                 =\n";
    cout << "= They are compared in two ways :                           =\n";
    cout << "= (H) Heavy : The comparison is the sum of all the numbers  =\n";
    cout << "=             of the array                                  =\n";
    cout << "= (L) Light : The comparison is with the first element of   =\n";
    cout << "=             the array, as a key                           =\n";
    cout << "=                                                           =\n";
    cout << "============================================================\n";
    cout << "\n\n";
    Generator<int_array<1>>(NELEM);
    Generator<int_array<2>>(NELEM >> 1);
    Generator<int_array<4>>(NELEM >> 2);
    Generator<int_array<8>>(NELEM >> 3);
    Generator<int_array<16>>(NELEM >> 4);
    Generator<int_array<32>>(NELEM >> 5);
    Generator<int_array<64>>(NELEM >> 6);

    return code;
}

void Generator_sorted(void)
{
    vector<uint64_t> A;

    A.reserve(NELEM);
    cout << "  " << NELEM << " uint64_t elements already sorted\n";
    cout << "=================================================\n";
    A.clear();
    for (size_t i = 0; i < NELEM; ++i) A.push_back(i);
    Test<uint64_t, std::less<uint64_t>>(A);
    Test_spreadsort(A);
    cout << std::endl;
}
void Generator_reverse_sorted(void)
{
    vector<uint64_t> A;

    A.reserve(NELEM);
    cout << "  " << NELEM << " uint64_t elements reverse sorted\n";
    cout << "=================================================\n";
    A.clear();
    for (size_t i = NELEM; i > 0; --i) A.push_back(i);
    Test<uint64_t, std::less<uint64_t>>(A);
    Test_spreadsort(A);
    cout << std::endl;
}
void Generator_uint64(void)
{
    vector<uint64_t> A;
    A.reserve(NELEM);
    cout << "  " << NELEM << " uint64_t elements randomly filled\n";
    cout << "=================================================\n";
    A.clear();
    if (fill_vector_uint64("input.bin", A, NELEM) != 0) {
        std::cout << "Error in the input file\n";
        return;
    };
    Test<uint64_t, std::less<uint64_t>>(A);
    Test_spreadsort(A);
    cout << std::endl;
}
void Generator_string(void)
{
    cout << "  " << NMAXSTRING << " strings randomly filled\n";
    cout << "===============================================\n";
    std::vector<std::string> A;
    A.reserve(NMAXSTRING);
    A.clear();
    if (bsp_util::fill_vector_string("input.bin", A, NMAXSTRING) != 0) {
        std::cout << "Error in the input file\n";
        return;
    };
    Test<std::string, std::less<std::string>>(A);
    Test_spreadsort(A);
    cout << std::endl;
};

template <class IA>
void Generator(uint64_t N)
{
    bsp_util::uint64_file_generator gen("input.bin");
    vector<IA> A;
    A.reserve(N);

    cout << N << " elements of size " << sizeof(IA) << " randomly filled \n";
    cout << "=============================================\n";
    gen.reset();
    A.clear();
    for (uint32_t i = 0; i < N; i++) A.emplace_back(IA::generate(gen));
    cout << "\n  H E A V Y   C O M P A R I S O N\n";
    cout << "====================================\n";
    Test(A, H_comp<IA>());
    cout << "\n  L I G H T   C O M P A R I S O N \n";
    cout << "=======================================\n";
    Test(A, L_comp<IA>());
    cout << std::endl;
};

template <class IA, class compare>
int Test(std::vector<IA> &B, compare comp)
{ //---------------------------- begin --------------------------------
    double duration;
    time_point start, finish;
    std::vector<IA> A(B);

    //--------------------------------------------------------------------
    A = B;
    cout << "std::sort                            : ";
    start = now();
    std::sort(A.begin(), A.end(), comp);
    finish = now();
    duration = subtract_time(finish, start);
    cout << duration << " secs\n";

    A = B;
    cout << "Boost sort                           : ";
    start = now();
    bsort::sort(A.begin(), A.end(), comp);
    finish = now();
    duration = subtract_time(finish, start);
    cout << duration << " secs\n\n";

    A = B;
    cout << "std::stable_sort                     : ";
    start = now();
    std::stable_sort(A.begin(), A.end(), comp);
    finish = now();
    duration = subtract_time(finish, start);
    cout << duration << " secs\n";

    A = B;
    cout << "Boost stable sort                    : ";
    start = now();
    bsort::stable_sort(A.begin(), A.end(), comp);
    finish = now();
    duration = subtract_time(finish, start);
    cout << duration << " secs\n\n";

    A = B;
    cout << "OMP parallel sort                    : ";
    start = now();
    __gnu_parallel::sort(A.begin(), A.end(), comp);
    finish = now();
    duration = subtract_time(finish, start);
    cout << duration << " secs\n";

    A = B;
    cout << "Boost parallel sort                  : ";
    start = now();
    bsort::parallel_sort(A.begin(), A.end(), comp);
    finish = now();
    duration = subtract_time(finish, start);
    cout << duration << " secs\n\n";

    A = B;
    cout << "OMP parallel stable sort             : ";
    start = now();
    __gnu_parallel::stable_sort(A.begin(), A.end(), comp);
    finish = now();
    duration = subtract_time(finish, start);
    cout << duration << " secs\n";

    A = B;
    cout << "Boost sample sort                    : ";
    start = now();
    bsort::sample_sort(A.begin(), A.end(), comp);
    finish = now();
    duration = subtract_time(finish, start);
    cout << duration << " secs\n";

    A = B;
    cout << "Boost parallel stable sort           : ";
    start = now();
    bsort::parallel_stable_sort(A.begin(), A.end(), comp);
    finish = now();
    duration = subtract_time(finish, start);
    cout << duration << " secs\n\n";


    // Asynchronous scheduler
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<
                  boost::asynchronous::multiqueue_threadpool_scheduler<
                        boost::asynchronous::lockfree_queue<>,
                        boost::asynchronous::default_find_position< boost::asynchronous::sequential_push_policy>,
                        boost::asynchronous::no_cpu_load_saving
                    >>(boost::thread::hardware_concurrency(),boost::thread::hardware_concurrency() * 4);
    // set processor affinity to improve cache usage. We start at core 0
    pool.processor_bind(0);

    A = B;
    cout << "Asynchronous parallel sort           : ";
    start = now();
    boost::future<void> fu = boost::asynchronous::post_future(pool,
    [&A,&comp]()
    {
        return boost::asynchronous::parallel_sort(A.begin(), A.end(), comp, sizeof(IA)<=16 ? NELEM/64:4096*2,"",0);
    }
    ,"",0);
    fu.get();
    finish = now();
    duration = subtract_time(finish, start);
    cout << duration << " secs\n\n";

    A = B;
    cout << "Asynchronous parallel_quicksort      : ";
    start = now();
    fu = boost::asynchronous::post_future(pool,
    [&A,&comp]()
    {
        return boost::asynchronous::parallel_quicksort(A.begin(), A.end(), comp, sizeof(IA)<=16 && typeid(IA)!=typeid(std::string) ? NELEM/64:4096*2,
                                                       boost::thread::hardware_concurrency(),"",0);
    }
    ,"",0);
    fu.get();
    finish = now();
    duration = subtract_time(finish, start);
    cout << duration << " secs\n\n";

    A = B;
    cout << "Asynchronous parallel stable sort    : ";
    start = now();
    fu = boost::asynchronous::post_future(pool,
    [&A,&comp]()
    {
        return boost::asynchronous::parallel_stable_sort(A.begin(), A.end(), comp, sizeof(IA)<=16 && typeid(IA)!=typeid(std::string) ? NELEM/64:4096*2,"",0);
    }
    ,"",0);
    fu.get();
    finish = now();
    duration = subtract_time(finish, start);
    cout << duration << " secs\n\n";

    A = B;
    cout << "Asynchronous parallel_indirect_sort  : ";
    start = now();
    fu = boost::asynchronous::post_future(pool,
    [&A,&comp]()mutable
    {
        return boost::asynchronous::parallel_indirect_sort(A.begin(), A.end(), comp, 4096*2,"");
    }
    ,"",0);
    fu.get();
    finish = now();
    duration = subtract_time(finish, start);
    cout << duration << " secs\n\n";

    A = B;
    cout << "Asynchronous parallel_intro_sort     : ";
    start = now();
    fu = boost::asynchronous::post_future(pool,
    [&A,&comp]()mutable
    {
        return boost::asynchronous::parallel_intro_sort(A.begin(), A.end(), comp, sizeof(IA)<=16 && typeid(IA)!=typeid(std::string) ? NELEM/64:4096*2,"");
    }
    ,"",0);
    fu.get();
    finish = now();
    duration = subtract_time(finish, start);
    cout << duration << " secs\n\n";

    A = B;
    cout << "Asynchronous boost::stable sort      : ";
    start = now();
    fu = boost::asynchronous::post_future(pool,
    [&A,&comp]()
    {
        return boost::asynchronous::parallel_boost_stable_sort(A.begin(), A.end(), comp,sizeof(IA)<=16 && typeid(IA)!=typeid(std::string) ? NELEM/64:4096*2,"",0);
    }
    ,"",0);
    fu.get();
    finish = now();
    duration = subtract_time(finish, start);
    cout << duration << " secs\n\n";

    A = B;
    cout << "Asynchronous boost::spin sort        : ";
    start = now();
    fu = boost::asynchronous::post_future(pool,
    [&A,&comp]()
    {
        return boost::asynchronous::parallel_boost_spin_sort(A.begin(), A.end(), comp,sizeof(IA)<=16 && typeid(IA)!=typeid(std::string) ? NELEM/64:4096*2,"",0);
    }
    ,"",0);
    fu.get();
    finish = now();
    duration = subtract_time(finish, start);
    cout << duration << " secs\n\n";

    A = B;
    cout << "Asynchronous parallel_quick_spin_sort: ";
    start = now();
    fu = boost::asynchronous::post_future(pool,
    [&A,&comp]()
    {
        return boost::asynchronous::parallel_quick_spin_sort(A.begin(), A.end(), comp, sizeof(IA)<=16 && typeid(IA)!=typeid(std::string) ? NELEM/64:4096*2,
                                                             boost::thread::hardware_concurrency(),"",0);
    }
    ,"",0);
    fu.get();
    finish = now();
    duration = subtract_time(finish, start);
    cout << duration << " secs\n\n";

    A = B;
    cout << "Asynchronous quick_indirect_sort     : ";
    start = now();
    fu = boost::asynchronous::post_future(pool,
    [&A,&comp]()
    {
        return boost::asynchronous::parallel_quick_indirect_sort(A.begin(), A.end(), comp, sizeof(IA)<=16 && typeid(IA)!=typeid(std::string) ? NELEM/64:4096*2,
                                                                 boost::thread::hardware_concurrency(),"",0);
    }
    ,"",0);
    fu.get();
    finish = now();
    duration = subtract_time(finish, start);
    cout << duration << " secs\n\n";

    A = B;
    cout << "Asynchronous quick_intro_sort        : ";
    start = now();
    fu = boost::asynchronous::post_future(pool,
    [&A,&comp]()
    {
        return boost::asynchronous::parallel_quick_intro_sort(A.begin(), A.end(), comp, sizeof(IA)<=16 && typeid(IA)!=typeid(std::string) ? NELEM/64:4096*2,
                                                              boost::thread::hardware_concurrency(),"",0);
    }
    ,"",0);
    fu.get();
    finish = now();
    duration = subtract_time(finish, start);
    cout << duration << " secs\n\n";

    return 0;
};

template <class IA>
int Test_spreadsort(std::vector<IA> &B)
{
    double duration;
    time_point start, finish;
    std::vector<IA> A(B);

    //--------------------------------------------------------------------
    A = B;
    cout << "Boost spreadsort                     : ";
    start = now();
    boost::sort::spreadsort::spreadsort(A.begin(), A.end());
    finish = now();
    duration = subtract_time(finish, start);
    cout << duration << " secs\n";

    //--------------------------------------------------------------------
    // Asynchronous scheduler
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<
                  boost::asynchronous::multiqueue_threadpool_scheduler<
                        boost::asynchronous::lockfree_queue<>,
                        boost::asynchronous::default_find_position< boost::asynchronous::sequential_push_policy>,
                        boost::asynchronous::no_cpu_load_saving
                    >>(boost::thread::hardware_concurrency(),boost::thread::hardware_concurrency() * 4);
    // set processor affinity to improve cache usage. We start at core 0, until tpsize-1
    pool.processor_bind(0);

    A = B;
    cout << "Asynchronous parallel spreadsort     : ";
    start = now();
    auto fu = boost::asynchronous::post_future(pool,
    [&A]()
    {
        return boost::asynchronous::parallel_spreadsort(A.begin(), A.end(), std::less<IA>(), sizeof(IA)<=16 ? NELEM/64:4096*2,"",0);
    }
    ,"",0);
    fu.get();
    finish = now();
    duration = subtract_time(finish, start);
    cout << duration << " secs\n\n";

    A = B;
    cout << "Asynchronous parallel quickspreadsort: ";
    start = now();
    fu = boost::asynchronous::post_future(pool,
    [&A]()
    {
        return boost::asynchronous::parallel_quick_spreadsort(A.begin(), A.end(), std::less<IA>(), sizeof(IA)<=16 ? NELEM/64:4096*2,boost::thread::hardware_concurrency(),"",0);
    }
    ,"",0);
    fu.get();
    finish = now();
    duration = subtract_time(finish, start);
    cout << duration << " secs\n\n";


    return 0;
};
