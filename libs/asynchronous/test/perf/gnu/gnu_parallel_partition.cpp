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
#include "tbb/tick_count.h"

#include <parallel/basic_iterator.h>
#include <parallel/features.h>
#include <parallel/parallel.h>
#include <parallel/algorithm>

#include <boost/smart_ptr/shared_array.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>

using namespace std;

#define LOOP 1
//#define NELEM 200000000
//#define SORTED_TYPE uint32_t

//#define NELEM 10000000
//#define SORTED_TYPE std::string

#define NELEM 100000000
#define SORTED_TYPE float

SORTED_TYPE compare_with = NELEM/2;

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

void Parallelsort( SORTED_TYPE a[], size_t n ) {
    __gnu_parallel::partition(a , a+NELEM,[](SORTED_TYPE const& i)
    {
       // cheap version
       //return i < compare_with;
       // expensive version
       return i/(i*i)/i/i < compare_with/(compare_with * compare_with)/compare_with/compare_with;
    }  );
}

double test_sorted_elements(void(*pf)(SORTED_TYPE [], size_t ))
{
    boost::shared_array<SORTED_TYPE> a (new SORTED_TYPE[NELEM]);
    for ( uint32_t i = 0 ; i < NELEM ; ++i)
    {
        *(a.get()+i) = test_cast<uint32_t,SORTED_TYPE>( i+NELEM) ;
    }
    tbb::tick_count t0;
    t0 = tbb::tick_count::now();
    (*pf)(a.get(),NELEM);
    auto res = (tbb::tick_count::now()-t0).seconds()*1000;
    return res;
}
double test_random_elements_many_repeated(void(*pf)(SORTED_TYPE [], size_t ))
{
    boost::shared_array<SORTED_TYPE> a (new SORTED_TYPE[NELEM]);
    for ( uint32_t i = 0 ; i < NELEM ; ++i)
    {
        *(a.get()+i) = test_cast<uint32_t,SORTED_TYPE>(rand() % 10000) ;
    }
    tbb::tick_count t0;
    t0 = tbb::tick_count::now();
    (*pf)(a.get(),NELEM);
    auto res = (tbb::tick_count::now()-t0).seconds()*1000;
    return res;
}
double test_random_elements_few_repeated(void(*pf)(SORTED_TYPE [], size_t ))
{
    boost::shared_array<SORTED_TYPE> a (new SORTED_TYPE[NELEM]);
    for ( uint32_t i = 0 ; i < NELEM ; ++i)
    {
        *(a.get()+i) = test_cast<uint32_t,SORTED_TYPE>(rand()) ;
    }
    tbb::tick_count t0;
    t0 = tbb::tick_count::now();
    (*pf)(a.get(),NELEM);
    auto res = (tbb::tick_count::now()-t0).seconds()*1000;
    return res;
}
double test_random_elements_quite_repeated(void(*pf)(SORTED_TYPE [], size_t ))
{
    boost::shared_array<SORTED_TYPE> a (new SORTED_TYPE[NELEM]);
    for ( uint32_t i = 0 ; i < NELEM ; ++i)
    {
        *(a.get()+i) = test_cast<uint32_t,SORTED_TYPE>(rand() % (NELEM/2)) ;
    }
    tbb::tick_count t0;
    t0 = tbb::tick_count::now();
    (*pf)(a.get(),NELEM);
    auto res = (tbb::tick_count::now()-t0).seconds()*1000;
    return res;
}
double test_reversed_sorted_elements(void(*pf)(SORTED_TYPE [], size_t ))
{
    boost::shared_array<SORTED_TYPE> a (new SORTED_TYPE[NELEM]);
    for ( uint32_t i = 0 ; i < NELEM ; ++i)
    {
        *(a.get()+i) = test_cast<uint32_t,SORTED_TYPE>((NELEM<<1) -i) ;
    }
    tbb::tick_count t0;
    t0 = tbb::tick_count::now();
    (*pf)(a.get(),NELEM);
    auto res = (tbb::tick_count::now()-t0).seconds()*1000;
    return res;
}
double test_equal_elements(void(*pf)(SORTED_TYPE [], size_t ))
{
    boost::shared_array<SORTED_TYPE> a (new SORTED_TYPE[NELEM]);
    for ( uint32_t i = 0 ; i < NELEM ; ++i)
    {
        *(a.get()+i) = test_cast<uint32_t,SORTED_TYPE>(NELEM) ;
    }
    tbb::tick_count t0;
    t0 = tbb::tick_count::now();
    (*pf)(a.get(),NELEM);
    auto res = (tbb::tick_count::now()-t0).seconds()*1000;
    return res;
}
int main( int , const char *[] )
{

    double t2=0.0;
    for (int i=0;i<LOOP;++i)
    {
        t2 += test_random_elements_many_repeated(Parallelsort);
    }
    printf ("%40s: time = %.1f msec\n","test_random_elements_many_repeated", t2);

    t2=0.0;
    for (int i=0;i<LOOP;++i)
    {
        t2 += test_random_elements_few_repeated(Parallelsort);
    }
    printf ("%40s: time = %.1f msec\n","test_random_elements_few_repeated", t2);

    t2=0.0;
    for (int i=0;i<LOOP;++i)
    {
        t2 += test_random_elements_quite_repeated(Parallelsort);
    }
    printf ("%40s: time = %.1f msec\n","test_random_elements_quite_repeated", t2);

    t2=0.0;
    for (int i=0;i<LOOP;++i)
    {
        t2 += test_sorted_elements(Parallelsort);
    }
    printf ("%40s: time = %.1f msec\n","test_sorted_elements", t2);

    t2=0.0;
    for (int i=0;i<LOOP;++i)
    {
        t2 += test_reversed_sorted_elements(Parallelsort);
    }
    printf ("%40s: time = %.1f msec\n","test_reversed_sorted_elements", t2);

    t2=0.0;
    for (int i=0;i<LOOP;++i)
    {
        t2 += test_equal_elements(Parallelsort);
    }
    printf ("%40s: time = %.1f msec\n","test_equal_elements", t2);

    return 0;
}
