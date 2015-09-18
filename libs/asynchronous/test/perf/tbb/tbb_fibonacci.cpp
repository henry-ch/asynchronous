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

/* Example program that computes Fibonacci numbers in different ways.
   Arguments are: [ Number [Threads [Repeats]]]
   The defaults are Number=500 Threads=1:4 Repeats=1.

   The point of this program is to check that the library is working properly.
   Most of the computations are deliberately silly and not expected to
   show any speedup on multiprocessors.
*/

// enable assertions
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <utility>
#include "tbb/task.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/tick_count.h"
#include "tbb/blocked_range.h"
#include "tbb/concurrent_vector.h"
#include "tbb/concurrent_queue.h"
#include "tbb/concurrent_hash_map.h"
#include "tbb/parallel_while.h"
#include "tbb/parallel_for.h"
#include "tbb/parallel_reduce.h"
#include "tbb/parallel_scan.h"
#include "tbb/pipeline.h"
#include "tbb/atomic.h"
#include "tbb/mutex.h"
#include "tbb/spin_mutex.h"
#include "tbb/queuing_mutex.h"
#include "tbb/tbb_thread.h"

using namespace std;
using namespace tbb;

long CutOff=10;

long SerialFib( long n ) {
    if( n<2 )
        return n;
    else
        return SerialFib(n-1)+SerialFib(n-2);
}
//class FibTask: public task {
//public:
//    const long n;
//    long* const sum;
//    FibTask( long n_, long* sum_ ) :
//        n(n_), sum(sum_)
//    {}
//    task* execute() {      // Overrides virtual function task::execute
//        if( n<CutOff ) {
//            *sum = SerialFib(n);
//        } else {
//            long x, y;
//            FibTask& a = *new( allocate_child() ) FibTask(n-1,&x);
//            FibTask& b = *new( allocate_child() ) FibTask(n-2,&y);
//            // Set ref_count to 'two children plus one for the wait".
//            set_ref_count(3);
//            // Start b running.
//            spawn( b );
//            // Start a running and wait for all children (a and b).
//            spawn_and_wait_for_all(a);
//            // Do the sum
//            *sum = x+y;
//        }
//        return NULL;
//    }
//};

struct FibContinuation: public task {
    long* const sum;
    long x, y;
    FibContinuation( long* sum_ ) : sum(sum_) {}
    task* execute() {
        *sum = x+y;
        return NULL;
    }
};
class FibTask: public task {
public:
    const long n;
    long* const sum;
    FibTask( long n_, long* sum_ ) :
        n(n_), sum(sum_)
    {}
    task* execute() {      // Overrides virtual function task::execute
        if( n<CutOff ) {
            *sum = SerialFib(n);
        } else {
            FibContinuation& c =
                            *new( allocate_continuation() ) FibContinuation(sum);

                        FibTask& a = *new( c.allocate_child() ) FibTask(n-2,&c.x);
                        FibTask& b = *new( c.allocate_child() ) FibTask(n-1,&c.y);
                        // Set ref_count to "two children".
                        c.set_ref_count(2);
                        spawn( b );
                        // spawn( a ); This line removed
                        // return NULL; This line removed
                        return &a;
        }
        return NULL;
    }
};

long ParallelFib( long n ) {
    long sum;
    FibTask& a = *new(task::allocate_root()) FibTask(n,&sum);
    task::spawn_root_and_wait(a);
    return sum;
}

//! program entry
int main(int argc, char* argv[])
{
    tbb::task_scheduler_init init;
    long fib = (argc>2) ? strtol(argv[1],0,0) : 48;
    CutOff = (argc>2) ? strtol(argv[2],0,0) : 30;
    std::cout << "fib=" << fib << std::endl;
    std::cout << "cutoff=" << CutOff << std::endl;

//    tbb::tick_count t0;
//    t0 = tbb::tick_count::now();
//    long res1 = SerialFib(fib);
//    double time1 = (tbb::tick_count::now()-t0).seconds()*1000;
//    printf ("%24s: time = %.1f msec\n", "serial" ,time1);
//    std::cout << "serial fib= " << res1 << std::endl;

    tbb::tick_count t1;
    t1 = tbb::tick_count::now();
    long res2 = ParallelFib(fib);
    double time2 = (tbb::tick_count::now()-t1).seconds()*1000;
    printf ("%24s: time = %.1f msec\n", "para" ,time2);
    std::cout << "oara fib= " << res2 << std::endl;
    return 0;
}

