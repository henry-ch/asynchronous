<html><head>
      <meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">
   <link rel="stylesheet" href="boostbook.css" type="text/css"><meta name="generator" content="DocBook XSL-NS Stylesheets V1.75.2"></head><body bgcolor="white" text="black" link="#0000FF" vlink="#840084" alink="#0000FF"><div class="book" title="Boost Asynchronous"><div class="titlepage"><div><div><h1 class="title"><a name="d0e2"></a>Asynchronous</h1></div><div><div class="author"><h3 class="author">Christophe Henry</h3><code class="email">&lt;<a class="email" href="mailto:christophe.j.henry@gmail.com">christophe.j.henry@gmail.com</a>&gt;</code></div></div><div><p class="copyright">Copyright &copy; 2015 
                <span> Distributed under the Boost Software License, Version 1.0. (See
                    accompanying file LICENSE_1_0.txt or copy at <a class="link" href="http://www.boost.org/LICENSE_1_0.txt" target="_top">http://www.boost.org/LICENSE_1_0.txt</a> ) </span>
            </p></div></div><hr></div><div class="toc"><p><b>Table of Contents</b></p><dl><dt><span class="preface"><a href="#d0e22">Introduction</a></span></dt><dt><span class="part"><a href="#d0e164">I. Concepts</a></span></dt><dd><dl><dt><span class="chapter"><a href="#d0e167">1. Related designs: std::async, Active Object, Proactor</a></span></dt><dd><dl><dt><span class="sect1"><a href="#d0e170">std::async</a></span></dt><dt><span class="sect1"><a href="#d0e200">N3558 / N3650</a></span></dt><dt><span class="sect1"><a href="#d0e221">Active Object</a></span></dt><dt><span class="sect1"><a href="#d0e246">Proactor</a></span></dt></dl></dd><dt><span class="chapter"><a href="#d0e260">2. Features of Boost.Asynchronous</a></span></dt><dd><dl><dt><span class="sect1"><a href="#d0e263">Thread world</a></span></dt><dt><span class="sect1"><a href="#d0e274">Better Architecture</a></span></dt><dt><span class="sect1"><a href="#d0e284">Shutting down</a></span></dt><dt><span class="sect1"><a href="#d0e291">Object lifetime</a></span></dt><dt><span class="sect1"><a href="#d0e314">Servant Proxies</a></span></dt><dt><span class="sect1"><a href="#d0e322">Interrupting</a></span></dt><dt><span class="sect1"><a href="#d0e337">Diagnostics</a></span></dt><dt><span class="sect1"><a href="#d0e349">Continuations</a></span></dt><dt><span class="sect1"><a href="#d0e365">Want more power? What about extra machines?</a></span></dt><dt><span class="sect1"><a href="#d0e377">Parallel algorithms</a></span></dt><dt><span class="sect1"><a href="#d0e413">Task Priority</a></span></dt><dt><span class="sect1"><a href="#d0e420">Integrating with Boost.Asio</a></span></dt><dt><span class="sect1"><a href="#d0e430">Integrating with Qt</a></span></dt><dt><span class="sect1"><a href="#d0e435">Work Stealing</a></span></dt><dt><span class="sect1"><a href="#d0e440">Extending the library</a></span></dt><dt><span class="sect1"><a href="#d0e458">Design Diagrams</a></span></dt></dl></dd></dl></dd><dt><span class="part"><a href="#d0e495">II. User Guide</a></span></dt><dd><dl><dt><span class="chapter"><a href="#d0e498">3. Using Asynchronous</a></span></dt><dd><dl><dt><span class="sect1"><a href="#d0e501">Definitions</a></span></dt><dd><dl><dt><span class="sect2"><a href="#d0e504">Scheduler</a></span></dt><dt><span class="sect2"><a href="#d0e509">Thread World (also known as Appartment)</a></span></dt><dt><span class="sect2"><a href="#d0e514">Weak Scheduler</a></span></dt><dt><span class="sect2"><a href="#d0e519">Trackable Servant</a></span></dt><dt><span class="sect2"><a href="#d0e524">Queue</a></span></dt><dt><span class="sect2"><a href="#d0e529">Servant Proxy</a></span></dt><dt><span class="sect2"><a href="#d0e534">Scheduler Shared Proxy</a></span></dt><dt><span class="sect2"><a href="#d0e539">Posting</a></span></dt></dl></dd><dt><span class="sect1"><a href="#d0e544">Hello, asynchronous world</a></span></dt><dt><span class="sect1"><a href="#d0e562">A servant proxy</a></span></dt><dt><span class="sect1"><a href="#d0e597">Using a threadpool from within a servant</a></span></dt><dt><span class="sect1"><a href="#d0e645">A servant using another servant proxy</a></span></dt><dt><span class="sect1"><a href="#d0e693">Interrupting tasks</a></span></dt><dt><span class="sect1"><a href="#d0e725">Logging tasks</a></span></dt><dt><span class="sect1"><a href="#d0e794">Generating HTML diagnostics</a></span></dt><dt><span class="sect1"><a href="#d0e815">Queue container with priority</a></span></dt><dt><span class="sect1"><a href="#d0e952">Multiqueue Schedulers' priority</a></span></dt><dt><span class="sect1"><a href="#d0e959">Threadpool Schedulers with several queues</a></span></dt><dt><span class="sect1"><a href="#d0e986">Composite Threadpool Scheduler</a></span></dt><dd><dl><dt><span class="sect2"><a href="#d0e989">Usage</a></span></dt><dt><span class="sect2"><a href="#d0e1078">Priority</a></span></dt></dl></dd><dt><span class="sect1"><a href="#d0e1087">More flexibility in dividing servants among threads</a></span></dt><dt><span class="sect1"><a href="#d0e1106">Processor binding</a></span></dt><dt><span class="sect1"><a href="#d0e1116">asio_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e1224">Timers</a></span></dt><dd><dl><dt><span class="sect2"><a href="#d0e1240">Constructing a timer</a></span></dt></dl></dd><dt><span class="sect1"><a href="#d0e1273">Continuation tasks</a></span></dt><dd><dl><dt><span class="sect2"><a href="#d0e1279">General</a></span></dt><dt><span class="sect2"><a href="#d0e1372">Logging</a></span></dt><dt><span class="sect2"><a href="#d0e1442">Creating a variable number of tasks for a continuation</a></span></dt><dt><span class="sect2"><a href="#d0e1534">Creating a continuation from a simple functor</a></span></dt></dl></dd><dt><span class="sect1"><a href="#d0e1566">Future-based continuations</a></span></dt><dt><span class="sect1"><a href="#d0e1640">Distributing work among machines</a></span></dt><dd><dl><dt><span class="sect2"><a href="#d0e1776">A distributed, parallel Fibonacci</a></span></dt><dt><span class="sect2"><a href="#d0e1945">Example: a hierarchical network</a></span></dt></dl></dd><dt><span class="sect1"><a href="#d0e1998">Picking your archive</a></span></dt><dt><span class="sect1"><a href="#d0e2022">Parallel Algorithms (Christophe Henry / Tobias Holl)</a></span></dt><dd><dl><dt><span class="sect2"><a href="#d0e2991">Finding the best cutoff</a></span></dt><dt><span class="sect2"><a href="#d0e3039">parallel_for</a></span></dt><dt><span class="sect2"><a href="#d0e3228">parallel_for_each</a></span></dt><dt><span class="sect2"><a href="#d0e3278">parallel_all_of</a></span></dt><dt><span class="sect2"><a href="#d0e3326">parallel_any_of</a></span></dt><dt><span class="sect2"><a href="#d0e3374">parallel_none_of</a></span></dt><dt><span class="sect2"><a href="#d0e3422">parallel_equal</a></span></dt><dt><span class="sect2"><a href="#d0e3471">parallel_mismatch</a></span></dt><dt><span class="sect2"><a href="#d0e3520">parallel_find_end</a></span></dt><dt><span class="sect2"><a href="#d0e3575">parallel_find_first_of</a></span></dt><dt><span class="sect2"><a href="#d0e3630">parallel_adjacent_find</a></span></dt><dt><span class="sect2"><a href="#d0e3676">parallel_lexicographical_compare</a></span></dt><dt><span class="sect2"><a href="#d0e3725">parallel_search</a></span></dt><dt><span class="sect2"><a href="#d0e3780">parallel_search_n</a></span></dt><dt><span class="sect2"><a href="#d0e3832">parallel_scan</a></span></dt><dt><span class="sect2"><a href="#d0e3905">parallel_inclusive_scan</a></span></dt><dt><span class="sect2"><a href="#d0e3968">parallel_exclusive_scan</a></span></dt><dt><span class="sect2"><a href="#d0e4031">parallel_copy</a></span></dt><dt><span class="sect2"><a href="#d0e4079">parallel_copy_if</a></span></dt><dt><span class="sect2"><a href="#d0e4118">parallel_move</a></span></dt><dt><span class="sect2"><a href="#d0e4169">parallel_fill</a></span></dt><dt><span class="sect2"><a href="#d0e4217">parallel_transform</a></span></dt><dt><span class="sect2"><a href="#d0e4281">parallel_generate</a></span></dt><dt><span class="sect2"><a href="#d0e4332">parallel_remove_copy / parallel_remove_copy_if</a></span></dt><dt><span class="sect2"><a href="#d0e4381">parallel_replace /
                        parallel_replace_if</a></span></dt><dt><span class="sect2"><a href="#d0e4450">parallel_reverse</a></span></dt><dt><span class="sect2"><a href="#d0e4498">parallel_swap_ranges</a></span></dt><dt><span class="sect2"><a href="#d0e4535">parallel_transform_inclusive_scan</a></span></dt><dt><span class="sect2"><a href="#d0e4583">parallel_transform_exclusive_scan</a></span></dt><dt><span class="sect2"><a href="#d0e4631">parallel_is_partitioned</a></span></dt><dt><span class="sect2"><a href="#d0e4673">parallel_partition</a></span></dt><dt><span class="sect2"><a href="#d0e4730">parallel_stable_partition</a></span></dt><dt><span class="sect2"><a href="#d0e4799">parallel_partition_copy</a></span></dt><dt><span class="sect2"><a href="#d0e4845">parallel_is_sorted</a></span></dt><dt><span class="sect2"><a href="#d0e4887">parallel_is_reverse_sorted</a></span></dt><dt><span class="sect2"><a href="#d0e4929">parallel_iota</a></span></dt><dt><span class="sect2"><a href="#d0e4977">parallel_reduce</a></span></dt><dt><span class="sect2"><a href="#d0e5057">parallel_inner_product</a></span></dt><dt><span class="sect2"><a href="#d0e5128">parallel_partial_sum</a></span></dt><dt><span class="sect2"><a href="#d0e5195">parallel_merge</a></span></dt><dt><span class="sect2"><a href="#d0e5243">parallel_invoke</a></span></dt><dt><span class="sect2"><a href="#d0e5313">if_then_else</a></span></dt><dt><span class="sect2"><a href="#d0e5353">parallel_geometry_intersection_of_x</a></span></dt><dt><span class="sect2"><a href="#d0e5415">parallel_geometry_union_of_x</a></span></dt><dt><span class="sect2"><a href="#d0e5477">parallel_union</a></span></dt><dt><span class="sect2"><a href="#d0e5518">parallel_intersection</a></span></dt><dt><span class="sect2"><a href="#d0e5559">parallel_find_all</a></span></dt><dt><span class="sect2"><a href="#d0e5637">parallel_extremum</a></span></dt><dt><span class="sect2"><a href="#d0e5687">parallel_count /
                        parallel_count_if</a></span></dt><dt><span class="sect2"><a href="#d0e5761">parallel_sort / parallel_stable_sort /
                        parallel_spreadsort / parallel_sort_inplace / parallel_stable_sort_inplace /
                        parallel_spreadsort_inplace</a></span></dt><dt><span class="sect2"><a href="#d0e5864">parallel_partial_sort</a></span></dt><dt><span class="sect2"><a href="#d0e5903">parallel_quicksort /
                        parallel_quick_spreadsort</a></span></dt><dt><span class="sect2"><a href="#d0e5945">parallel_nth_element</a></span></dt></dl></dd><dt><span class="sect1"><a href="#d0e5987">Parallel containers</a></span></dt></dl></dd><dt><span class="chapter"><a href="#d0e6375">4. Tips.</a></span></dt><dd><dl><dt><span class="sect1"><a href="#d0e6378">Which protections you get, which ones you don't.</a></span></dt><dt><span class="sect1"><a href="#d0e6416">No cycle, ever</a></span></dt><dt><span class="sect1"><a href="#d0e6425">No "this" within a task.</a></span></dt></dl></dd><dt><span class="chapter"><a href="#d0e6434">5. Design examples</a></span></dt><dd><dl><dt><span class="sect1"><a href="#d0e6439">A state machine managing a succession of tasks</a></span></dt><dt><span class="sect1"><a href="#d0e6484">A layered application</a></span></dt><dt><span class="sect1"><a href="#d0e6541">Boost.Meta State Machine and Asynchronous behind a Qt User Interface </a></span></dt></dl></dd></dl></dd><dt><span class="part"><a href="#d0e6636">III. Reference</a></span></dt><dd><dl><dt><span class="chapter"><a href="#d0e6639">6. Queues</a></span></dt><dd><dl><dt><span class="sect1"><a href="#d0e6647">threadsafe_list</a></span></dt><dt><span class="sect1"><a href="#d0e6664">lockfree_queue</a></span></dt><dt><span class="sect1"><a href="#d0e6688">lockfree_spsc_queue</a></span></dt><dt><span class="sect1"><a href="#d0e6713">lockfree_stack</a></span></dt></dl></dd><dt><span class="chapter"><a href="#d0e6732">7. Schedulers</a></span></dt><dd><dl><dt><span class="sect1"><a href="#d0e6737">single_thread_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e6815">multiple_thread_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e6890">threadpool_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e6966">multiqueue_threadpool_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e7040">stealing_threadpool_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e7112">stealing_multiqueue_threadpool_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e7184">composite_threadpool_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e7241">asio_scheduler</a></span></dt></dl></dd><dt><span class="chapter"><a href="#d0e7294">8. Performance tests</a></span></dt><dd><dl><dt><span class="sect1"><a href="#d0e7297">asynchronous::vector</a></span></dt><dt><span class="sect1"><a href="#d0e7511">Sort</a></span></dt><dt><span class="sect1"><a href="#d0e7912">parallel_scan</a></span></dt><dt><span class="sect1"><a href="#d0e7996">parallel_stable_partition</a></span></dt><dt><span class="sect1"><a href="#d0e8387">parallel_for</a></span></dt></dl></dd><dt><span class="chapter"><a href="#d0e8472">9. Compiler, linker, settings</a></span></dt><dd><dl><dt><span class="sect1"><a href="#d0e8475">C++ 11</a></span></dt><dt><span class="sect1"><a href="#d0e8480">Supported compilers</a></span></dt><dt><span class="sect1"><a href="#d0e8498">Supported targets</a></span></dt><dt><span class="sect1"><a href="#d0e8505">Linking</a></span></dt><dt><span class="sect1"><a href="#d0e8510">Compile-time switches</a></span></dt></dl></dd></dl></dd></dl></div><div class="list-of-tables"><p><b>List of Tables</b></p><dl><dt>3.1. <a href="#d0e2052">Non-modifying Algorithms, in boost/asynchronous/algorithm</a></dt><dt>3.2. <a href="#d0e2303">Modifying Algorithms, in boost/asynchronous/algorithm</a></dt><dt>3.3. <a href="#d0e2494">Partitioning Operations, in boost/asynchronous/algorithm</a></dt><dt>3.4. <a href="#d0e2565">Sorting Operations, in boost/asynchronous/algorithm</a></dt><dt>3.5. <a href="#d0e2732">Numeric Algorithms in boost/asynchronous/algorithm</a></dt><dt>3.6. <a href="#d0e2803">Algorithms Operating on Sorted Sequences in
                        boost/asynchronous/algorithm</a></dt><dt>3.7. <a href="#d0e2838">Minimum/maximum operations in boost/asynchronous/algorithm</a></dt><dt>3.8. <a href="#d0e2873">Miscellaneous Algorithms in boost/asynchronous/algorithm</a></dt><dt>3.9. <a href="#d0e2920">(Boost) Geometry Algorithms in boost/asynchronous/algorithm/geometry
                        (compatible with boost geometry 1.58). Experimental and tested only with
                        polygons.</a></dt><dt>3.10. <a href="#d0e6032">#include
                        &lt;boost/asynchronous/container/vector.hpp&gt;</a></dt><dt>3.11. <a href="#d0e6303">#include
                        &lt;boost/asynchronous/container/vector.hpp&gt;</a></dt><dt>7.1. <a href="#d0e6782">#include
                            &lt;boost/asynchronous/scheduler/single_thread_scheduler.hpp&gt;</a></dt><dt>7.2. <a href="#d0e6857">#include
                            &lt;boost/asynchronous/scheduler/single_thread_scheduler.hpp&gt;</a></dt><dt>7.3. <a href="#d0e6931">#include
                            &lt;boost/asynchronous/scheduler/threadpool_scheduler.hpp&gt;</a></dt><dt>7.4. <a href="#d0e7005">#include
                            &lt;boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp&gt;</a></dt><dt>7.5. <a href="#d0e7077">#include
                            &lt;boost/asynchronous/scheduler/stealing_threadpool_scheduler.hpp&gt;</a></dt><dt>7.6. <a href="#d0e7149">#include
                            &lt;boost/asynchronous/stealing_multiqueue_threadpool_scheduler.hpp&gt;</a></dt><dt>7.7. <a href="#d0e7208">#include
                            &lt;boost/asynchronous/scheduler/composite_threadpool_scheduler.hpp&gt;</a></dt><dt>7.8. <a href="#d0e7259">#include
                            &lt;boost/asynchronous/extensions/asio/asio_scheduler.hpp&gt;</a></dt><dt>8.1. <a href="#d0e7316">Performance of asynchronous::vector members using 4 threads</a></dt><dt>8.2. <a href="#d0e7381">Performance of asynchronous::vector members using 8 threads</a></dt><dt>8.3. <a href="#d0e7446">Performance of asynchronous::vector members Xeon Phi 3120A 57 Cores /
                            228 Threads</a></dt><dt>8.4. <a href="#d0e7530">Sorting 200000000 uint32_t</a></dt><dt>8.5. <a href="#d0e7633">Sorting 200000000 double</a></dt><dt>8.6. <a href="#d0e7736">Sorting 200000000 std::string</a></dt><dt>8.7. <a href="#d0e7839">Sorting 10000000 objects containing 10 longs</a></dt><dt>8.8. <a href="#d0e7931">Performance of parallel_scan vs serial scan on a i7 / Xeon Phi
                            Knight's Corner</a></dt><dt>8.9. <a href="#d0e8015">Partitioning 100000000 floats on Core i7-5960X 8 Cores / 8 Threads
                            (16 Threads bring no added value)</a></dt><dt>8.10. <a href="#d0e8089">Partitioning 100000000 floats on Core i7-5960X 4 Cores / 4
                            Threads</a></dt><dt>8.11. <a href="#d0e8163">Partitioning 100000000 floats on Xeon Phi 3120A 57 Cores / 228
                            Threads</a></dt><dt>8.12. <a href="#d0e8237">Partitioning 100000000 floats on Xeon Phi 3120A 57 Cores / 114
                            Threads</a></dt><dt>8.13. <a href="#d0e8311">Partitioning 100000000 floats on Xeon Phi 3120A 57 Cores / 10
                            Threads</a></dt><dt>8.14. <a href="#d0e8406">Performance of parallel_for on a i7 / Xeon Phi Knight's
                            Corner</a></dt></dl></div><div class="preface" title="Introduction"><div class="titlepage"><div><div><h2 class="title"><a name="d0e22"></a>Introduction</h2></div></div></div><p>
            <span class="underline">Note</span>: Asynchronous is not part of the Boost
            library. It is planed to be offered for Review at the beginning of 2016. </p><p>Asynchronous is first of all an architecture tool. It allows organizing a complete
            application into Thread Worlds, each world having the possibility to use the same or
            different threadpools for long-lasting tasks. The library provides an implementation of
            the Active Object pattern, extended to allow many Active Objects to live in the same
            World. It provides several Threadpools and many parallel algorithms making use of it.
            And most important of all, it allows simple, blocking-free asynchronous programming
            based on thread-safe callbacks.</p><p>This is particularly relevant for Designers who often have headaches bringing the
            notion of threads into class diagrams. These usually do not mix well. Asynchronous
            solves this problems: it allows representing a Thread World as a Component or Package,
            objects of this Component or Package living into the corresponding thread.</p><p>Why do we care? Herb Sutter wrote <a class="link" href="http://www.gotw.ca/publications/concurrency-ddj.htm" target="_top">in an
                article</a> "The Free Lunch Is Over", meaning that developpers will be forced to
            learn to develop multi-threaded applications. The reason is that we now get our extra
            power in the form of more cores. The problem is: multithreading is hard! It's full of
            ugly beasts waiting hidden for our mistakes: races, deadlocks, crashes, all kinds of
            subtle timing-dependent bugs. Worse yet, these bugs are hard to find because they are
            never reproducible when we are looking for them, which leaves us with backtrace
            analysis, and this is when we are lucky enough to have a backtrace in the first
            place.</p><p>This is not even the only danger. CPUs are a magnitude faster than memory, I/O
            operations, network communications, which all stall our programms and degrade our
            performance, which means long sessions with coverage or analysis tools.</p><p>Trying to solve these problems with tools of the past (mutexes, programmer-managed
            threads) is a dead-end. It's just too hard. This is where Boost Asynchronous is helping.
            Let us forget what mutexes, atomics and races are!</p><p>There are existing solutions for asynchronous or parallel programming. To name a
            few:</p><p>
            </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>std/boost::async.</p></li><li class="listitem"><p>Intel TBB.</p></li><li class="listitem"><p>N3428.</p></li></ul></div><p>
        </p><p>TBB is a wonderful parallel library. But it's not asynchronous as one needs to wait
            for the end of a parallel call.</p><p>std::async will return a future. But what to do with it? Wait for it? This would be
            synchronous. Collect them and then wait for all? This would also be synchronous. Collect
            them, do something else, then check whether they are ready? This would be wasted
            opportunity for more calculations.</p><p>To solve these problems, NB3428 is an attempt at continuations. Let's have a quick
            look at code using futures and .then (taken from N3428):</p><p>
            </p><pre class="programlisting">future&lt;int&gt; f1 = async([]() { return 123; });
future&lt;string&gt; f2 = f1.then([](future&lt;int&gt; f) {return f.get().to_string();}); // here .get() won&#8217;t block
f2.get(); // just a "small get" at the end?</pre><p>
        </p><p>Saying that there is only a "small get" at the end is, for an application with
            real-time constraints, equivalent to saying at a lockfree conference something like
            "what is all the fuss about? Can't we just add a small lock at the end?". Just try
            it...</p><p>Worse yet, it clutters the code, makes it hard to debug and understand. The author,
            also being the author of Boost Meta State Machine sees no way to use this paradigm with
            state machines.</p><p>Asynchronous supports this programming model too, though it is advised to use it only
            for simple programs, quick prototyping unit tests, or as a step to the more powerful
            tools offered by the library. std::async can be replaced by
            boost::asynchronous::post_future:</p><pre class="programlisting">auto pool = boost::asynchronous::make_shared_scheduler_proxy&lt;
                  boost::asynchronous::multiqueue_threadpool_scheduler&lt;
                        boost::asynchronous::lockfree_queue&lt;&gt;&gt;&gt;(8); // create a pool with 8 threads
std::future&lt;int&gt; fu = boost::asynchronous::post_future(pool,
    []()
    {
        return 123;
    });
f1.get();</pre><p>Instead of an ugly future.then, Asynchronous supports continuations as coded into the
            task itself. We will see later how to do it. For the moment, here is a quick example.
            Let's say we want to modify a vector in parallel, then reduce it, also in parallel,
            without having to write synchronization points:</p><pre class="programlisting">std::future&lt;int&gt; fu = boost::asynchronous::post_future(pool, // pool as before
    [this]()
    {
        return boost::asynchronous::parallel_reduce(                   // reduce will be done in parallel after for
            boost::asynchronous::parallel_for(std::move(this-&gt;m_data), // our data, a std::vector&lt;int&gt; will be moved, transformed, then reduced and eventually destroyed
                                              [](int&amp; i)
                                              {
                                                  i += 2;              // transform all elements in parallel
                                              }, 1024),                // cutoff (when to go sequential. Will be explained later)
            [](int const&amp; a, int const&amp; b)                             // reduce function
            {
                return a + b;
            }, 1024);                                                  // reduce cutoff
    });
int res = fu.get();</pre><p>But this is just the beginning. It is not really asynchronous. More important, Boost
            Asynchronous is a library which can play a great role in making a thread-correct
            architecture. To achieve this, it offers tools for asynchronous designs: ActiveObject,
            safe callbacks, threadpools, servants, proxies, queues, algorithms, etc. </p><p>Consider the following example showing us why we need an architecture tool:</p><p>
            </p><pre class="programlisting">struct Bad : public boost::signals::trackable
{
   int foo();
};
boost::shared_ptr&lt;Bad&gt; b;
future&lt;int&gt; f = async([b](){return b-&gt;foo()});          </pre><p>
        </p><p>Now we have the ugly problem of not knowing in which thread Bad will be destroyed. And
            as it's pretty hard to have a thread-safe destructor, we find ourselves with a race
            condition in it. </p><p>Asynchronous programming has the advantage of allowing to design of code, which is
            nonblocking and single-threaded while still utilizing parallel hardware at full
            capacity. And all this while forgetting what a mutex is. </p><p>This brings us to a central point of Asynchronous: if we build a system with strict
            real-time constraints, there is no such thing as a small blocking get(). We need to be
            able to react to any event in the system in a timely manner. And we can't afford to have
            lots of functions potentially waiting too long everywhere in our code. Therefore,
            .then() is only good for an application of a few hundreds of lines. What about using a
            timed_wait instead? Nope. This just limits the amount of time we waste waiting. Either
            we wait too long before handling an error or result, or we wait not enough and we poll.
            In any case, while waiting, our thread cannot react to other events and wastes
            time.</p><p>An image being more worth than thousand words, the following story will explain in a
            few minutes what Asynchronous is about. Consider some fast-food restaurant:</p><p><span class="inlinemediaobject"><img src="pics/Proactor1.jpg"></span>
        </p><p>This restaurant has a single employee, Worker, who delivers burgers through a burger
            queue and drinks. A Customer comes. Then another, who waits until the first customer is
            served.</p><p><span class="inlinemediaobject"><img src="pics/Proactor2.jpg"></span></p><p>To keep customers happy by reducing waiting time, the restaurant owner hires a second
            employee:</p><p><span class="inlinemediaobject"><img src="pics/Proactor3.jpg"></span></p><p>Unfortunately, this brings chaos in the restaurant. Sometimes, employes fight to get a
            burger to their own customer first:</p><p><span class="inlinemediaobject"><img src="pics/Proactor-RC.jpg"></span></p><p>And sometimes, they stay in each other's way:</p><p><span class="inlinemediaobject"><img src="pics/Proactor-DL.jpg"></span></p><p>This clearly is a not an optimal solution. Not only the additional employee brings
            additional costs, but both employees now spend much more time waiting. It also is not a
            scalable solution if even more customers want to eat because it's lunch-time right now.
            Even worse, as they fight for resources and stay in each other's way, the restaurant now
            serves people less fast than before. Customers flee and the restaurant gets bankrupt. A
            sad story, isn't it? To avoid this, the owner decides to go asynchronous. He keeps a
            single worker, who runs in zero time from cash desk to cash desk:</p><p><span class="inlinemediaobject"><img src="pics/Proactor-async.jpg"></span></p><p>The worker never waits because it would increase customer's waiting time. Instead, he
            runs from cash desks to the burger queue, beverage machine using a self-made strategy: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>ask what the customer wants and keep an up-to-date information of the
                        customer's state.</p></li><li class="listitem"><p>if we have another customer at a desk, ask what he wants. For both
                        customers, remember the state of the order (waiting for customer choice,
                        getting food, getting drink, delivering, getting payment, etc.)</p></li><li class="listitem"><p>as soon as some new state is detected (customer choice, burger in the
                        queue, drink ready), handle it.</p></li><li class="listitem"><p>priorities are defined: start the longest-lasting tasks first, serve
                        angry-looking customers first, etc.</p></li></ul></div><p>The following diagram shows us the busy and really really fast worker in
            action:</p><p><span class="inlinemediaobject"><img src="pics/Proactor-async2.jpg"></span></p><p>Of course the owner needs a worker who runs fast, and has a pretty good memory so he
            can remember what customers are waiting for. </p><p>This is what Asynchronous is for. A worker (thread) runs as long as there are waiting
            customers, following a precisely defined algorithm, and lots of state machines to manage
            the asynchronous behaviour. In case of customers, we could have a state machine: Waiting
            -&gt; PickingMenu -&gt; WaitingForFood -&gt; Paying.</p><p>We also need some queues (Burger queue, Beverage glass positioning) and some
            Asynchronous Operation Processor (for example a threadpool made of workers in the
            kitchen), event of different types (Drinks delivery). Maybe we also want some work
            stealing (someone in the kitchen serving drinks as he has no more burger to prepare. He
            will be slower than the machine, but still bring some time gain).</p><p><span class="bold"><strong>To make this work, the worker must not block, never,
                ever</strong></span>. And whatever he's doing has to be as fast as possible, otherwise
            the whole process stalls.</p></div><div class="part" title="Part&nbsp;I.&nbsp;Concepts"><div class="titlepage"><div><div><h1 class="title"><a name="d0e164"></a>Part&nbsp;I.&nbsp;Concepts</h1></div></div></div><div class="toc"><p><b>Table of Contents</b></p><dl><dt><span class="chapter"><a href="#d0e167">1. Related designs: std::async, Active Object, Proactor</a></span></dt><dd><dl><dt><span class="sect1"><a href="#d0e170">std::async</a></span></dt><dt><span class="sect1"><a href="#d0e200">N3558 / N3650</a></span></dt><dt><span class="sect1"><a href="#d0e221">Active Object</a></span></dt><dt><span class="sect1"><a href="#d0e246">Proactor</a></span></dt></dl></dd><dt><span class="chapter"><a href="#d0e260">2. Features of Boost.Asynchronous</a></span></dt><dd><dl><dt><span class="sect1"><a href="#d0e263">Thread world</a></span></dt><dt><span class="sect1"><a href="#d0e274">Better Architecture</a></span></dt><dt><span class="sect1"><a href="#d0e284">Shutting down</a></span></dt><dt><span class="sect1"><a href="#d0e291">Object lifetime</a></span></dt><dt><span class="sect1"><a href="#d0e314">Servant Proxies</a></span></dt><dt><span class="sect1"><a href="#d0e322">Interrupting</a></span></dt><dt><span class="sect1"><a href="#d0e337">Diagnostics</a></span></dt><dt><span class="sect1"><a href="#d0e349">Continuations</a></span></dt><dt><span class="sect1"><a href="#d0e365">Want more power? What about extra machines?</a></span></dt><dt><span class="sect1"><a href="#d0e377">Parallel algorithms</a></span></dt><dt><span class="sect1"><a href="#d0e413">Task Priority</a></span></dt><dt><span class="sect1"><a href="#d0e420">Integrating with Boost.Asio</a></span></dt><dt><span class="sect1"><a href="#d0e430">Integrating with Qt</a></span></dt><dt><span class="sect1"><a href="#d0e435">Work Stealing</a></span></dt><dt><span class="sect1"><a href="#d0e440">Extending the library</a></span></dt><dt><span class="sect1"><a href="#d0e458">Design Diagrams</a></span></dt></dl></dd></dl></div><div class="chapter" title="Chapter&nbsp;1.&nbsp;Related designs: std::async, Active Object, Proactor"><div class="titlepage"><div><div><h2 class="title"><a name="d0e167"></a>Chapter&nbsp;1.&nbsp;Related designs: std::async, Active Object, Proactor</h2></div></div></div><div class="toc"><p><b>Table of Contents</b></p><dl><dt><span class="sect1"><a href="#d0e170">std::async</a></span></dt><dt><span class="sect1"><a href="#d0e200">N3558 / N3650</a></span></dt><dt><span class="sect1"><a href="#d0e221">Active Object</a></span></dt><dt><span class="sect1"><a href="#d0e246">Proactor</a></span></dt></dl></div><div class="sect1" title="std::async"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e170"></a>std::async</h2></div><div><h3 class="subtitle">What is wrong with it</h3></div></div></div><p>The following code is a classical use of std::async as it can be found in
                    articles, books, etc.</p><pre class="programlisting">std::future&lt;int&gt; f = std::async([](){return 42;}); // executes asynchronously
int res = f.get(); // wait for result, block until ready</pre><p>It looks simple, easy to use, and everybody can get it. The problem is, well,
                    that it's not really asynchronous. True, our lambda will execute in another
                    thread. Actually, it's not even guaranteed either. But then, what do we do with
                    our future? Do we poll it? Or call get() as in the example? But then we will
                    block, right? And if we block, are we still asynchronous? If we block, we cannot
                    react to any event happening in our system any more, we are unresponsive for a
                    while (are we back to the old times of freezing programs, the old time before
                    threads?). We also probably miss some opportunities to fully use our hardware as
                    we could be doing something more useful at the same time, as in our fast-food
                    example. And diagnostics are looking bad too as we are blocked and cannot
                    deliver any. What is left to us is polling. And if we get more and more futures,
                    do we carry a bag of them with us at any time and check them from time to time?
                    Do we need some functions to, at a given point, wait for all futures or any of
                    them to be ready? </p><p>Wait, yes they exist, <code class="code">wait_for_all</code> and
                    <code class="code">wait_for_any</code>... </p><p>And what about this example from an online documentation?</p><p>
                    </p><pre class="programlisting">{ 
   std::async(std::launch::async, []{ f(); }); 
   std::async(std::launch::async, []{ g(); });
}</pre><p>
                </p><p>Every std::async returns you a future, a particularly mean one which blocks
                    upon destruction. This means that the second line will not execute until f()
                    completes. Now this is not only not asynchronous, it's also much slower than
                    calling sequentially f and g while doing the same.</p><p>No, really, this does not look good. Do we have alternatives?</p></div><div class="sect1" title="N3558 / N3650"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e200"></a>N3558 / N3650</h2></div></div></div><p>Of course it did not go unnoticed that std::async has some limitations. And so
                    do we see some tries to save it instead of giving it up. Usually, it goes around
                    the lines of blocking, but later.</p><p>
                    </p><pre class="programlisting">future&lt;int&gt; f1 = async([]() { return 123; }); 
future&lt;string&gt; f2 = f1.then([](future&lt;int&gt; f) 
{ 
  return f.get().to_string(); // here .get() won&#8217;t block 
});
// and here?
string s= f2.get();</pre><p>
                </p><p>The idea is to make std::async more asynchronous (this already just sounds
                    bad) by adding something (.then) to be called when the asynchronous action
                    finishes. It still does not fly:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>at some point, we will have to block, thus ending our asynchronous
                                behavior</p></li><li class="listitem"><p>This works only for very small programs. Do we imagine a 500k
                                lines program built that way?</p></li></ul></div><p>And what about the suggestion of adding new keywords, async and await, as in
                    N3650? Nope. First because, as await suggests, someone will need, at some point,
                    to block waiting. Second because as we have no future, we also lose our polling
                    option.</p></div><div class="sect1" title="Active Object"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e221"></a>Active Object</h2></div><div><h3 class="subtitle">Design</h3></div></div></div><p><span class="inlinemediaobject"><img src="pics/ActiveObject.jpg"></span></p><p>This simplified diagram shows a possible design variation of an Active Object
                    pattern.</p><p>A thread-unsafe Servant is hidden behind a Proxy, which offers the same
                    members as the Servant itself. This Proxy is called by clients and delivers a
                    future object, which will, at some later point, contain the result of the
                    corresponding member called on the servant. The Proxy packs a MethodRequest
                    corresponding to a Servant call into the ActivationQueue. The Scheduler waits
                    permanently for MethodRequests in the queue, dequeues them, and executes them.
                    As only one scheduler waits for requests, it serializes access to the Servant,
                    thus providing thread-safety.</p><p>However, this pattern presents some liabilities:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>Performance overhead: depending on the system, data moving and
                                context switching can be a performance drain.</p></li><li class="listitem"><p>Memory overhead: for every Servant, a thread has to be created,
                                consuming resources.</p></li><li class="listitem"><p>Usage: getting a future gets us back to the non-asynchronous
                                behaviour we would like to avoid.</p></li></ul></div></div><div class="sect1" title="Proactor"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e246"></a>Proactor</h2></div><div><h3 class="subtitle">Design</h3></div></div></div><p><span class="inlinemediaobject"><img src="pics/Proactor.jpg"></span></p><p>This is the design pattern behind Boost.Asio. See: <a class="link" href="http://www.boost.org/doc/libs/1_57_0/doc/html/boost_asio/overview/core/async.html" target="_top">Boost.Asio documentation</a> for a full explanation. Boost Asynchronous
                    is very similar. It supports enqueueing asynchronous operations and waiting for
                    callbacks, offering extensions: safe callbacks, threadpools, proxies,
                    etc.</p></div></div><div class="chapter" title="Chapter&nbsp;2.&nbsp;Features of Boost.Asynchronous"><div class="titlepage"><div><div><h2 class="title"><a name="d0e260"></a>Chapter&nbsp;2.&nbsp;Features of Boost.Asynchronous</h2></div></div></div><div class="toc"><p><b>Table of Contents</b></p><dl><dt><span class="sect1"><a href="#d0e263">Thread world</a></span></dt><dt><span class="sect1"><a href="#d0e274">Better Architecture</a></span></dt><dt><span class="sect1"><a href="#d0e284">Shutting down</a></span></dt><dt><span class="sect1"><a href="#d0e291">Object lifetime</a></span></dt><dt><span class="sect1"><a href="#d0e314">Servant Proxies</a></span></dt><dt><span class="sect1"><a href="#d0e322">Interrupting</a></span></dt><dt><span class="sect1"><a href="#d0e337">Diagnostics</a></span></dt><dt><span class="sect1"><a href="#d0e349">Continuations</a></span></dt><dt><span class="sect1"><a href="#d0e365">Want more power? What about extra machines?</a></span></dt><dt><span class="sect1"><a href="#d0e377">Parallel algorithms</a></span></dt><dt><span class="sect1"><a href="#d0e413">Task Priority</a></span></dt><dt><span class="sect1"><a href="#d0e420">Integrating with Boost.Asio</a></span></dt><dt><span class="sect1"><a href="#d0e430">Integrating with Qt</a></span></dt><dt><span class="sect1"><a href="#d0e435">Work Stealing</a></span></dt><dt><span class="sect1"><a href="#d0e440">Extending the library</a></span></dt><dt><span class="sect1"><a href="#d0e458">Design Diagrams</a></span></dt></dl></div><div class="sect1" title="Thread world"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e263"></a>Thread world</h2></div><div><h3 class="subtitle">Extending Active Objects with more servants within a thread
                    context</h3></div></div></div><p>A commonly cited drawback of Active Objects is that they are awfully
                    expensive. A thread per object is really a waste of ressources.
                    Boost.Asynchronous extends this concept by allowing an unlimited number of
                    objects to live within a single thread context, thus amortizing the costs. It
                    even provides a way for n Active Objects to share m threads while still being
                    called single thread. This allows tuning thread usage.</p><p>As many objects are potentially living in a thread context, none should be
                    allowed to process long-lasting tasks as it would reduce reactivity of the whole
                    component. In this aspect, Asynchronous' philosophy is closer to a
                    Proactor.</p><p>As long-lasting tasks do happen, Boost.Asynchronous provides several
                    implementations of threadpools and the needed infrastructure to make it safe to
                    post work to threadpools and get aynchronously a safe callback. It also provides
                    safe mechanisms to shutdown Thread worlds and threadpools.</p></div><div class="sect1" title="Better Architecture"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e274"></a>Better Architecture</h2></div></div></div><p>We all learned in our design books that a software should be organized into
                    layers. This is, however, easier said than done, single-threaded, but much worse
                    when layers are having their own threads. Let's say, layer A is on top and
                    basing itself on layer B. A creates B and keeps it alive as long as it lives
                    itself. A and B are each composed of hundreds of classes / objects. Our standard
                    communication is A =&gt; B, meaning A gives orders to B, which executes them. This
                    is the theory. Unfortunately, B needs to give answers, usually delayed, to A.
                    Unfortunately, A and B live in different threads. This means mutexes. Ouch. Now
                    we are forced to check every class of A and protect it. Worse, the object of A
                    getting an answer might have long been destroyed. Ouch again. What to do? We
                    could keep the object of A alive in the callback of B. But then we have a
                    dependency B -&gt; A. Ouch again, bad design. We can also hide the dependency using
                    some type erasure mechanism. We still have a logical one as B keeps its owner,
                    A, alive. Then, we can use a weak_ptr so that B does not keep A alive. But when
                    we lock, we do keep A alive. It's for a short time, but what if A is shutting
                    down? It's lost, our layered design is broken.</p><p>Asynchronous is more that a library providing a better std::async or some
                    parallel algorithms, it's first of all an architectural tool. In the above case,
                    we will decide that every layer will live in its own thread(s), called
                    schedulers in Asynchronous language. Deciding in which thread an object "lives"
                    is a key point of a good design. Then the top layer, A, will make a request to
                    B, aking a future as a result, or much better, providing a callback.
                    Asynchronous offers a callback safe in two ways: thread-safe and checking the
                    lifetime of the callback target. This callback is provided by
                        <code class="code">make_safe_callback</code>. This simple tool is a major help in making
                    a safe and efficient design.</p></div><div class="sect1" title="Shutting down"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e284"></a>Shutting down</h2></div></div></div><p>Shutting down a thread turns out to be harder in practice than expected, as
                    shown by several posts of surprise on the Boost mailing lists when Boost.Thread
                    tried to match the C++ Standard. Asynchronous hides all these ugly details. What
                    users see is a scheduler proxy object, which can be shared by any number of
                    objects, and running any number of threads, managed by a scheduler. The
                    scheduler proxy object manages the lifetime of the scheduler. </p><p>When the last instance of the scheduler object is destroyed, the scheduler
                    thread is stopped. When the last instance of a scheduler proxy is destroyed, the
                    scheduler thread is joined. It's as simple as that. This makes threads shared
                    objects. </p></div><div class="sect1" title="Object lifetime"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e291"></a>Object lifetime</h2></div></div></div><p>There are subtle bugs when living in a multithreaded world. Consider the
                    following class:</p><p>
                    </p><pre class="programlisting">struct Unsafe
{
  void foo()
  {
    m_mutex.lock();
    // call private member
    m_mutex.unlock();
  }
private:
  void foobar()
  {
    //we are already locked when called, do something while locked
  }
  boost::mutex m_mutex;
};            </pre><p>
                </p><p>This is called a thread-safe interface pattern. Public members lock, private
                    do not. Simple enough. Unfortunately, it doesn't fly.</p><p>First one has the risk of deadlock if a private member calls a public one
                    while being called from another public member. If we forget to check one path of
                    execution within a class implementation, we get a deadlock. We'll have to test
                    every single path of execution to prove our code is correct. And this at every
                    commit.</p><p>Usually, for any complex class, where there's a mutex, there is a race or a
                    deadlock...</p><p>But even worse, the principle itself is not correct in C++. It supposes that a
                    class can protect itself. Well, no, it can't. Why? One cannot protect the
                    destructor. If the object (and the mutex) gets destroyed when a thread waits for
                    it in foo(), we get a crash or an exception. We can mitigate this with the use
                    of a shared_ptr, then we have no destructor call while someone waits for the
                    mutex. Unfortunately, we still have a risk of a signal, callback, etc. all those
                    things mixing badly with threads. And if we use too many shared_ptr's, we start
                    having lifetime issues or leaks. </p><p>There are more lifetime issues, even without mutexes or threads. If you have
                    ever used Boost.Asio, a common mistake and an easy one is when a callback is
                    called in the proactor thread after an asynchronous operation, but the object
                    called is long gone and the callback invalid. Asynchronous provides <span class="command"><strong><a class="command" href="#trackable_servant">trackable_servant</a></strong></span> which makes sure
                    that a callback is not called if the object which called the asynchronous
                    operation is gone. It also prevents a task posted in a threadpool to be called
                    if this condition occurs, which improves performance. Asynchronous also provides
                    a safe callback for use as Boost.Asio or similar asynchronous libraries.</p></div><div class="sect1" title="Servant Proxies"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e314"></a>Servant Proxies</h2></div></div></div><p>Asynchronous offers <code class="code">servant_proxy</code>, which makes the outside world
                    call members of a servant as if it was not living in an ActiveObject. It looks
                    like a thread-safe interface, but safe from deadlock and race conditions. </p></div><div class="sect1" title="Interrupting"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e322"></a>Interrupting</h2></div><div><h3 class="subtitle">Or how to catch back if you're drowning. </h3></div></div></div><p>Let's say you posted so many tasks to your threadpool that all your cores are
                    full, still, your application is slipping more and more behind plan. You need to
                    give up some tasks to catch back a little.</p><p>Asynchronous can give us an interruptible cookie when we post a task to a
                    scheduler, and we can use it to <span class="command"><strong><a class="command" href="#interrupting_tasks">stop a
                        posted task</a></strong></span>. If not running yet, the task will not start, if
                    running, it will stop at the next interruption point, in the sense of the <a class="link" href="http://www.boost.org/doc/libs/1_54_0/doc/html/thread/thread_management.html#thread.thread_management.tutorial.interruption" target="_top">Boost.Thread documentation</a>. Diagnostics will show that the task was
                    interrupted.</p></div><div class="sect1" title="Diagnostics"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e337"></a>Diagnostics</h2></div></div></div><p>Finding out how good your software is doing is not an easy task. Developers
                    are notoriously bad at it. You need to add lots of logging to find out which
                    function call takes too long and becomes a bottleneck. Finding out the minimum
                    required hardware to run your application is even harder.</p><p>Asynchronous design helps here too. By logging the required time and the
                    frequency of tasks, it is easy to find out how many cores are needed.
                    Bottlenecks can be found by logging what the Thread world is doing and how long.
                    Finally, designing the asynchronous Thread world as state machines and logging
                    state changes will allow a better understanding of your system and make visible
                    potential for concurrency. Even for non-parallel algorithms, finding out, using
                    a state machine, the earliest point a task can be thrown to a threadpool will
                    give some low-hanging-fruit concurrency. Throw enough tasks to the threadpool
                    and manage this with a state machine and you might use your cores with little
                    effort. Parallelization can then be used later on by logging which tasks are
                    worth parallelized.</p><p>Asynchronous offers <span class="command"><strong><a class="command" href="#html_diags">tools generating nice HTML outputs</a></strong></span> for every schedulers,
                    including waiting and execution times of tasks, histograms, etc.</p></div><div class="sect1" title="Continuations"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e349"></a>Continuations</h2></div></div></div><p>Callbacks are great when you have a complex flow of operations which require a
                    state machine for management, however there are cases where callbacks are not an
                    ideal solution. Either because your application would require a constant
                    switching of context between single-threaded and parallel schedulers, or because
                    the single-threaded scheduler might be busy, which would delay completion of the
                    algorithm. A known example of this is a parallel fibonacci. In this case, one
                    can register a <span class="command"><strong><a class="command" href="#continuations">continuation</a></strong></span>,
                    which is to be executed upon completion of one or several tasks. </p><p>This mechanism is flexible so that you can use it with futures coming from
                    another library, thus removing any need for a
                        <code class="code">wait_for_all(futures...)</code> or a
                        <code class="code">wait_for_any(futures...)</code>.</p></div><div class="sect1" title="Want more power? What about extra machines?"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e365"></a>Want more power? What about extra machines?</h2></div></div></div><p>What to do if your threadpools are using all of your cores but there simply
                    are not enough cores for the job? Buy more cores? Unfortunately, the number of
                    cores a single-machine can use is limited, unless you have unlimited money. A
                    dual 6-core Xeon, 24 threads with hyperthreading will cost much more than 2 x
                    6-core i7, and will usually have a lesser clock frequency and an older
                    architecture. </p><p>The solution could be: start with the i7, then if you need more power, add
                    some more machines which will steal jobs from your threadpools using <span class="command"><strong><a class="command" href="#distributing">TCP</a></strong></span>. This can be done quite easily with
                    Asynchronous.</p><p>Want to build your own hierarchical network of servers? It's hard to make it
                    easier.</p></div><div class="sect1" title="Parallel algorithms"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e377"></a>Parallel algorithms</h2></div></div></div><p>The library also comes with <span class="command"><strong><a class="command" href="#parallel_algos">non-blocking
                        algorithms</a></strong></span> with iterators or ranges, partial support for TCP,
                    which fit well in the asynchronous system, with more to come. If you want to
                    contribute some more, be welcome. At the moment, the library offers:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>most STL algorithms</p></li><li class="listitem"><p>parallel_for / parallel_for_each</p></li><li class="listitem"><p>parallel_reduce</p></li><li class="listitem"><p>parallel_extremum</p></li><li class="listitem"><p>parallel_find_all</p></li><li class="listitem"><p>parallel_invoke</p></li><li class="listitem"><p>parallel_sort , parallel_quicksort</p></li><li class="listitem"><p>parallel_scan</p></li><li class="listitem"><p>parallelized Boost.Geometry algorithms for polygons
                                (parallel_union, parallel_intersection,
                                parallel_geometry_intersection_of_x,
                                parallel_geometry_union_of_x)</p></li></ul></div></div><div class="sect1" title="Task Priority"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e413"></a>Task Priority</h2></div></div></div><p>Asynchronous offers this possibility for all schedulers at low performance
                    cost. This means you not only have the possibility to influence task execution
                    order in a threadpool but also in Active Objects.</p><p>This is achieved by posting a task to the queue with the corresponding
                    priority. It is also possible to get it even more fine-grained by using a
                    sequence of queues, etc.</p></div><div class="sect1" title="Integrating with Boost.Asio"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e420"></a>Integrating with Boost.Asio</h2></div></div></div><p>Asynchronous offers a Boost.Asio based <span class="command"><strong><a class="command" href="#asio_scheduler">scheduler</a></strong></span> allowing you to easily write
                    a Servant using Asio, or an Asio based threadpool. An advantage is that you get
                    safe callbacks and easily get your Asio application to scale. Writing a server
                    has never been easier.</p><p>Asynchronous also uses Boost.Asio to provide a timer with callbacks.</p></div><div class="sect1" title="Integrating with Qt"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e430"></a>Integrating with Qt</h2></div></div></div><p>What about getting the power of Asynchronous within a Qt application? Use
                    Asynchronous' threadpools, algorithms and other cool features easily.</p></div><div class="sect1" title="Work Stealing"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e435"></a>Work Stealing</h2></div></div></div><p>Work stealing is supported both within the threads of a threadpool but also
                    between different threadpools. Please have a look at Asynchronous' composite
                    scheduler.</p></div><div class="sect1" title="Extending the library"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e440"></a>Extending the library</h2></div></div></div><p>Asynchronous has been written with the design goal of allowing anybody to
                    extend the library. In particular, the authors are hoping to be offered the
                    following extensions:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>More schedulers, threadpools</p></li><li class="listitem"><p>Queues</p></li><li class="listitem"><p>Parallel algorithms</p></li><li class="listitem"><p>Integration with other libraries</p></li></ul></div></div><div class="sect1" title="Design Diagrams"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e458"></a>Design Diagrams</h2></div></div></div><p><span class="inlinemediaobject"><img src="pics/AsynchronousDesign.jpg"></span></p><p>This diagram shows an overview of the design behind Asynchronous. One or more
                    Servant objects live in a single-theaded world, communicating with the outside
                    world only through one or several queues, from which the single-threaded
                    scheduler pops tasks. Tasks are pushed by calling a member on a proxy
                    object.</p><p>Like an Active Object, a client uses a proxy (a shared object type), which
                    offers the same members as the real servant, with the same parameters, the only
                    difference being the return type, a std::future&lt;R&gt;, with R being the return
                    type of the servant's member. All calls to a servant from the client side are
                    posted, which includes the servant constructor and destructor. When the last
                    instance of a servant is destroyed, be it used inside the Thread world or
                    outside, the servant destructor is posted.</p><p>any_shared_scheduler is the part of the Active Object scheduler living inside
                    the Thread world. Servants do not hold it directly but hold an
                    any_weak_scheduler instead. The library will use it to create a posted callback
                    when a task executing in a worker threadpool is completed.</p><p>Shutting down a Thread world is done automatically by not needing it. It
                    happens in the following order:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>While a servant proxy is alive, no shutdown</p></li><li class="listitem"><p>When the last servant proxy goes out of scope, the servant
                                destructor is posted.</p></li><li class="listitem"><p>if jobs from servants are running in a threadpool, they get a
                                chance to stop earlier by running into an interruption point or will
                                not even start.</p></li><li class="listitem"><p>threadpool(s) is (are) shut down.</p></li><li class="listitem"><p>The Thread world scheduler is stopped and its thread
                                terminates.</p></li><li class="listitem"><p>The last instance of any_shared_scheduler_proxy goes out of scope
                                with the last servant proxy and joins.</p></li></ul></div><p>
                </p><p>It is usually accepted that threads are orthogonal to an OO design and
                    therefore are hard to manage as they don't belong to an object. Asynchronous
                    comes close to this: threads are not directly used, but instead owned by a
                    scheduler, in which one creates objects and tasks.</p></div></div></div><div class="part" title="Part&nbsp;II.&nbsp;User Guide"><div class="titlepage"><div><div><h1 class="title"><a name="d0e495"></a>Part&nbsp;II.&nbsp;User Guide</h1></div></div></div><div class="toc"><p><b>Table of Contents</b></p><dl><dt><span class="chapter"><a href="#d0e498">3. Using Asynchronous</a></span></dt><dd><dl><dt><span class="sect1"><a href="#d0e501">Definitions</a></span></dt><dd><dl><dt><span class="sect2"><a href="#d0e504">Scheduler</a></span></dt><dt><span class="sect2"><a href="#d0e509">Thread World (also known as Appartment)</a></span></dt><dt><span class="sect2"><a href="#d0e514">Weak Scheduler</a></span></dt><dt><span class="sect2"><a href="#d0e519">Trackable Servant</a></span></dt><dt><span class="sect2"><a href="#d0e524">Queue</a></span></dt><dt><span class="sect2"><a href="#d0e529">Servant Proxy</a></span></dt><dt><span class="sect2"><a href="#d0e534">Scheduler Shared Proxy</a></span></dt><dt><span class="sect2"><a href="#d0e539">Posting</a></span></dt></dl></dd><dt><span class="sect1"><a href="#d0e544">Hello, asynchronous world</a></span></dt><dt><span class="sect1"><a href="#d0e562">A servant proxy</a></span></dt><dt><span class="sect1"><a href="#d0e597">Using a threadpool from within a servant</a></span></dt><dt><span class="sect1"><a href="#d0e645">A servant using another servant proxy</a></span></dt><dt><span class="sect1"><a href="#d0e693">Interrupting tasks</a></span></dt><dt><span class="sect1"><a href="#d0e725">Logging tasks</a></span></dt><dt><span class="sect1"><a href="#d0e794">Generating HTML diagnostics</a></span></dt><dt><span class="sect1"><a href="#d0e815">Queue container with priority</a></span></dt><dt><span class="sect1"><a href="#d0e952">Multiqueue Schedulers' priority</a></span></dt><dt><span class="sect1"><a href="#d0e959">Threadpool Schedulers with several queues</a></span></dt><dt><span class="sect1"><a href="#d0e986">Composite Threadpool Scheduler</a></span></dt><dd><dl><dt><span class="sect2"><a href="#d0e989">Usage</a></span></dt><dt><span class="sect2"><a href="#d0e1078">Priority</a></span></dt></dl></dd><dt><span class="sect1"><a href="#d0e1087">More flexibility in dividing servants among threads</a></span></dt><dt><span class="sect1"><a href="#d0e1106">Processor binding</a></span></dt><dt><span class="sect1"><a href="#d0e1116">asio_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e1224">Timers</a></span></dt><dd><dl><dt><span class="sect2"><a href="#d0e1240">Constructing a timer</a></span></dt></dl></dd><dt><span class="sect1"><a href="#d0e1273">Continuation tasks</a></span></dt><dd><dl><dt><span class="sect2"><a href="#d0e1279">General</a></span></dt><dt><span class="sect2"><a href="#d0e1372">Logging</a></span></dt><dt><span class="sect2"><a href="#d0e1442">Creating a variable number of tasks for a continuation</a></span></dt><dt><span class="sect2"><a href="#d0e1534">Creating a continuation from a simple functor</a></span></dt></dl></dd><dt><span class="sect1"><a href="#d0e1566">Future-based continuations</a></span></dt><dt><span class="sect1"><a href="#d0e1640">Distributing work among machines</a></span></dt><dd><dl><dt><span class="sect2"><a href="#d0e1776">A distributed, parallel Fibonacci</a></span></dt><dt><span class="sect2"><a href="#d0e1945">Example: a hierarchical network</a></span></dt></dl></dd><dt><span class="sect1"><a href="#d0e1998">Picking your archive</a></span></dt><dt><span class="sect1"><a href="#d0e2022">Parallel Algorithms (Christophe Henry / Tobias Holl)</a></span></dt><dd><dl><dt><span class="sect2"><a href="#d0e2991">Finding the best cutoff</a></span></dt><dt><span class="sect2"><a href="#d0e3039">parallel_for</a></span></dt><dt><span class="sect2"><a href="#d0e3228">parallel_for_each</a></span></dt><dt><span class="sect2"><a href="#d0e3278">parallel_all_of</a></span></dt><dt><span class="sect2"><a href="#d0e3326">parallel_any_of</a></span></dt><dt><span class="sect2"><a href="#d0e3374">parallel_none_of</a></span></dt><dt><span class="sect2"><a href="#d0e3422">parallel_equal</a></span></dt><dt><span class="sect2"><a href="#d0e3471">parallel_mismatch</a></span></dt><dt><span class="sect2"><a href="#d0e3520">parallel_find_end</a></span></dt><dt><span class="sect2"><a href="#d0e3575">parallel_find_first_of</a></span></dt><dt><span class="sect2"><a href="#d0e3630">parallel_adjacent_find</a></span></dt><dt><span class="sect2"><a href="#d0e3676">parallel_lexicographical_compare</a></span></dt><dt><span class="sect2"><a href="#d0e3725">parallel_search</a></span></dt><dt><span class="sect2"><a href="#d0e3780">parallel_search_n</a></span></dt><dt><span class="sect2"><a href="#d0e3832">parallel_scan</a></span></dt><dt><span class="sect2"><a href="#d0e3905">parallel_inclusive_scan</a></span></dt><dt><span class="sect2"><a href="#d0e3968">parallel_exclusive_scan</a></span></dt><dt><span class="sect2"><a href="#d0e4031">parallel_copy</a></span></dt><dt><span class="sect2"><a href="#d0e4079">parallel_copy_if</a></span></dt><dt><span class="sect2"><a href="#d0e4118">parallel_move</a></span></dt><dt><span class="sect2"><a href="#d0e4169">parallel_fill</a></span></dt><dt><span class="sect2"><a href="#d0e4217">parallel_transform</a></span></dt><dt><span class="sect2"><a href="#d0e4281">parallel_generate</a></span></dt><dt><span class="sect2"><a href="#d0e4332">parallel_remove_copy / parallel_remove_copy_if</a></span></dt><dt><span class="sect2"><a href="#d0e4381">parallel_replace /
                        parallel_replace_if</a></span></dt><dt><span class="sect2"><a href="#d0e4450">parallel_reverse</a></span></dt><dt><span class="sect2"><a href="#d0e4498">parallel_swap_ranges</a></span></dt><dt><span class="sect2"><a href="#d0e4535">parallel_transform_inclusive_scan</a></span></dt><dt><span class="sect2"><a href="#d0e4583">parallel_transform_exclusive_scan</a></span></dt><dt><span class="sect2"><a href="#d0e4631">parallel_is_partitioned</a></span></dt><dt><span class="sect2"><a href="#d0e4673">parallel_partition</a></span></dt><dt><span class="sect2"><a href="#d0e4730">parallel_stable_partition</a></span></dt><dt><span class="sect2"><a href="#d0e4799">parallel_partition_copy</a></span></dt><dt><span class="sect2"><a href="#d0e4845">parallel_is_sorted</a></span></dt><dt><span class="sect2"><a href="#d0e4887">parallel_is_reverse_sorted</a></span></dt><dt><span class="sect2"><a href="#d0e4929">parallel_iota</a></span></dt><dt><span class="sect2"><a href="#d0e4977">parallel_reduce</a></span></dt><dt><span class="sect2"><a href="#d0e5057">parallel_inner_product</a></span></dt><dt><span class="sect2"><a href="#d0e5128">parallel_partial_sum</a></span></dt><dt><span class="sect2"><a href="#d0e5195">parallel_merge</a></span></dt><dt><span class="sect2"><a href="#d0e5243">parallel_invoke</a></span></dt><dt><span class="sect2"><a href="#d0e5313">if_then_else</a></span></dt><dt><span class="sect2"><a href="#d0e5353">parallel_geometry_intersection_of_x</a></span></dt><dt><span class="sect2"><a href="#d0e5415">parallel_geometry_union_of_x</a></span></dt><dt><span class="sect2"><a href="#d0e5477">parallel_union</a></span></dt><dt><span class="sect2"><a href="#d0e5518">parallel_intersection</a></span></dt><dt><span class="sect2"><a href="#d0e5559">parallel_find_all</a></span></dt><dt><span class="sect2"><a href="#d0e5637">parallel_extremum</a></span></dt><dt><span class="sect2"><a href="#d0e5687">parallel_count /
                        parallel_count_if</a></span></dt><dt><span class="sect2"><a href="#d0e5761">parallel_sort / parallel_stable_sort /
                        parallel_spreadsort / parallel_sort_inplace / parallel_stable_sort_inplace /
                        parallel_spreadsort_inplace</a></span></dt><dt><span class="sect2"><a href="#d0e5864">parallel_partial_sort</a></span></dt><dt><span class="sect2"><a href="#d0e5903">parallel_quicksort /
                        parallel_quick_spreadsort</a></span></dt><dt><span class="sect2"><a href="#d0e5945">parallel_nth_element</a></span></dt></dl></dd><dt><span class="sect1"><a href="#d0e5987">Parallel containers</a></span></dt></dl></dd><dt><span class="chapter"><a href="#d0e6375">4. Tips.</a></span></dt><dd><dl><dt><span class="sect1"><a href="#d0e6378">Which protections you get, which ones you don't.</a></span></dt><dt><span class="sect1"><a href="#d0e6416">No cycle, ever</a></span></dt><dt><span class="sect1"><a href="#d0e6425">No "this" within a task.</a></span></dt></dl></dd><dt><span class="chapter"><a href="#d0e6434">5. Design examples</a></span></dt><dd><dl><dt><span class="sect1"><a href="#d0e6439">A state machine managing a succession of tasks</a></span></dt><dt><span class="sect1"><a href="#d0e6484">A layered application</a></span></dt><dt><span class="sect1"><a href="#d0e6541">Boost.Meta State Machine and Asynchronous behind a Qt User Interface </a></span></dt></dl></dd></dl></div><div class="chapter" title="Chapter&nbsp;3.&nbsp;Using Asynchronous"><div class="titlepage"><div><div><h2 class="title"><a name="d0e498"></a>Chapter&nbsp;3.&nbsp;Using Asynchronous</h2></div></div></div><div class="toc"><p><b>Table of Contents</b></p><dl><dt><span class="sect1"><a href="#d0e501">Definitions</a></span></dt><dd><dl><dt><span class="sect2"><a href="#d0e504">Scheduler</a></span></dt><dt><span class="sect2"><a href="#d0e509">Thread World (also known as Appartment)</a></span></dt><dt><span class="sect2"><a href="#d0e514">Weak Scheduler</a></span></dt><dt><span class="sect2"><a href="#d0e519">Trackable Servant</a></span></dt><dt><span class="sect2"><a href="#d0e524">Queue</a></span></dt><dt><span class="sect2"><a href="#d0e529">Servant Proxy</a></span></dt><dt><span class="sect2"><a href="#d0e534">Scheduler Shared Proxy</a></span></dt><dt><span class="sect2"><a href="#d0e539">Posting</a></span></dt></dl></dd><dt><span class="sect1"><a href="#d0e544">Hello, asynchronous world</a></span></dt><dt><span class="sect1"><a href="#d0e562">A servant proxy</a></span></dt><dt><span class="sect1"><a href="#d0e597">Using a threadpool from within a servant</a></span></dt><dt><span class="sect1"><a href="#d0e645">A servant using another servant proxy</a></span></dt><dt><span class="sect1"><a href="#d0e693">Interrupting tasks</a></span></dt><dt><span class="sect1"><a href="#d0e725">Logging tasks</a></span></dt><dt><span class="sect1"><a href="#d0e794">Generating HTML diagnostics</a></span></dt><dt><span class="sect1"><a href="#d0e815">Queue container with priority</a></span></dt><dt><span class="sect1"><a href="#d0e952">Multiqueue Schedulers' priority</a></span></dt><dt><span class="sect1"><a href="#d0e959">Threadpool Schedulers with several queues</a></span></dt><dt><span class="sect1"><a href="#d0e986">Composite Threadpool Scheduler</a></span></dt><dd><dl><dt><span class="sect2"><a href="#d0e989">Usage</a></span></dt><dt><span class="sect2"><a href="#d0e1078">Priority</a></span></dt></dl></dd><dt><span class="sect1"><a href="#d0e1087">More flexibility in dividing servants among threads</a></span></dt><dt><span class="sect1"><a href="#d0e1106">Processor binding</a></span></dt><dt><span class="sect1"><a href="#d0e1116">asio_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e1224">Timers</a></span></dt><dd><dl><dt><span class="sect2"><a href="#d0e1240">Constructing a timer</a></span></dt></dl></dd><dt><span class="sect1"><a href="#d0e1273">Continuation tasks</a></span></dt><dd><dl><dt><span class="sect2"><a href="#d0e1279">General</a></span></dt><dt><span class="sect2"><a href="#d0e1372">Logging</a></span></dt><dt><span class="sect2"><a href="#d0e1442">Creating a variable number of tasks for a continuation</a></span></dt><dt><span class="sect2"><a href="#d0e1534">Creating a continuation from a simple functor</a></span></dt></dl></dd><dt><span class="sect1"><a href="#d0e1566">Future-based continuations</a></span></dt><dt><span class="sect1"><a href="#d0e1640">Distributing work among machines</a></span></dt><dd><dl><dt><span class="sect2"><a href="#d0e1776">A distributed, parallel Fibonacci</a></span></dt><dt><span class="sect2"><a href="#d0e1945">Example: a hierarchical network</a></span></dt></dl></dd><dt><span class="sect1"><a href="#d0e1998">Picking your archive</a></span></dt><dt><span class="sect1"><a href="#d0e2022">Parallel Algorithms (Christophe Henry / Tobias Holl)</a></span></dt><dd><dl><dt><span class="sect2"><a href="#d0e2991">Finding the best cutoff</a></span></dt><dt><span class="sect2"><a href="#d0e3039">parallel_for</a></span></dt><dt><span class="sect2"><a href="#d0e3228">parallel_for_each</a></span></dt><dt><span class="sect2"><a href="#d0e3278">parallel_all_of</a></span></dt><dt><span class="sect2"><a href="#d0e3326">parallel_any_of</a></span></dt><dt><span class="sect2"><a href="#d0e3374">parallel_none_of</a></span></dt><dt><span class="sect2"><a href="#d0e3422">parallel_equal</a></span></dt><dt><span class="sect2"><a href="#d0e3471">parallel_mismatch</a></span></dt><dt><span class="sect2"><a href="#d0e3520">parallel_find_end</a></span></dt><dt><span class="sect2"><a href="#d0e3575">parallel_find_first_of</a></span></dt><dt><span class="sect2"><a href="#d0e3630">parallel_adjacent_find</a></span></dt><dt><span class="sect2"><a href="#d0e3676">parallel_lexicographical_compare</a></span></dt><dt><span class="sect2"><a href="#d0e3725">parallel_search</a></span></dt><dt><span class="sect2"><a href="#d0e3780">parallel_search_n</a></span></dt><dt><span class="sect2"><a href="#d0e3832">parallel_scan</a></span></dt><dt><span class="sect2"><a href="#d0e3905">parallel_inclusive_scan</a></span></dt><dt><span class="sect2"><a href="#d0e3968">parallel_exclusive_scan</a></span></dt><dt><span class="sect2"><a href="#d0e4031">parallel_copy</a></span></dt><dt><span class="sect2"><a href="#d0e4079">parallel_copy_if</a></span></dt><dt><span class="sect2"><a href="#d0e4118">parallel_move</a></span></dt><dt><span class="sect2"><a href="#d0e4169">parallel_fill</a></span></dt><dt><span class="sect2"><a href="#d0e4217">parallel_transform</a></span></dt><dt><span class="sect2"><a href="#d0e4281">parallel_generate</a></span></dt><dt><span class="sect2"><a href="#d0e4332">parallel_remove_copy / parallel_remove_copy_if</a></span></dt><dt><span class="sect2"><a href="#d0e4381">parallel_replace /
                        parallel_replace_if</a></span></dt><dt><span class="sect2"><a href="#d0e4450">parallel_reverse</a></span></dt><dt><span class="sect2"><a href="#d0e4498">parallel_swap_ranges</a></span></dt><dt><span class="sect2"><a href="#d0e4535">parallel_transform_inclusive_scan</a></span></dt><dt><span class="sect2"><a href="#d0e4583">parallel_transform_exclusive_scan</a></span></dt><dt><span class="sect2"><a href="#d0e4631">parallel_is_partitioned</a></span></dt><dt><span class="sect2"><a href="#d0e4673">parallel_partition</a></span></dt><dt><span class="sect2"><a href="#d0e4730">parallel_stable_partition</a></span></dt><dt><span class="sect2"><a href="#d0e4799">parallel_partition_copy</a></span></dt><dt><span class="sect2"><a href="#d0e4845">parallel_is_sorted</a></span></dt><dt><span class="sect2"><a href="#d0e4887">parallel_is_reverse_sorted</a></span></dt><dt><span class="sect2"><a href="#d0e4929">parallel_iota</a></span></dt><dt><span class="sect2"><a href="#d0e4977">parallel_reduce</a></span></dt><dt><span class="sect2"><a href="#d0e5057">parallel_inner_product</a></span></dt><dt><span class="sect2"><a href="#d0e5128">parallel_partial_sum</a></span></dt><dt><span class="sect2"><a href="#d0e5195">parallel_merge</a></span></dt><dt><span class="sect2"><a href="#d0e5243">parallel_invoke</a></span></dt><dt><span class="sect2"><a href="#d0e5313">if_then_else</a></span></dt><dt><span class="sect2"><a href="#d0e5353">parallel_geometry_intersection_of_x</a></span></dt><dt><span class="sect2"><a href="#d0e5415">parallel_geometry_union_of_x</a></span></dt><dt><span class="sect2"><a href="#d0e5477">parallel_union</a></span></dt><dt><span class="sect2"><a href="#d0e5518">parallel_intersection</a></span></dt><dt><span class="sect2"><a href="#d0e5559">parallel_find_all</a></span></dt><dt><span class="sect2"><a href="#d0e5637">parallel_extremum</a></span></dt><dt><span class="sect2"><a href="#d0e5687">parallel_count /
                        parallel_count_if</a></span></dt><dt><span class="sect2"><a href="#d0e5761">parallel_sort / parallel_stable_sort /
                        parallel_spreadsort / parallel_sort_inplace / parallel_stable_sort_inplace /
                        parallel_spreadsort_inplace</a></span></dt><dt><span class="sect2"><a href="#d0e5864">parallel_partial_sort</a></span></dt><dt><span class="sect2"><a href="#d0e5903">parallel_quicksort /
                        parallel_quick_spreadsort</a></span></dt><dt><span class="sect2"><a href="#d0e5945">parallel_nth_element</a></span></dt></dl></dd><dt><span class="sect1"><a href="#d0e5987">Parallel containers</a></span></dt></dl></div><div class="sect1" title="Definitions"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e501"></a>Definitions</h2></div></div></div><div class="sect2" title="Scheduler"><div class="titlepage"><div><div><h3 class="title"><a name="d0e504"></a>Scheduler</h3></div></div></div><p>Object having 0..n threads, executing jobs or callbacks. Stops owned
                        threads when destroyed.</p></div><div class="sect2" title="Thread World (also known as Appartment)"><div class="titlepage"><div><div><h3 class="title"><a name="d0e509"></a>Thread World (also known as Appartment)</h3></div></div></div><p>A "thread world" is a world defined by a (single threaded) scheduler and
                        all the objects which have been created, are living and destroyed within
                        this context. It is usually agreed on that objects and threads do not mix
                        well. Class diagrams fail to display both as these are orthogonal concepts.
                        Asynchronous solves this by organizing objects into worlds, each living
                        within a thread. This way, life cycles issues and the question of thread
                        access to objects are solved. It is similar to the Active Object pattern,
                        but with n Objects living within a thread. </p></div><div class="sect2" title="Weak Scheduler"><div class="titlepage"><div><div><h3 class="title"><a name="d0e514"></a>Weak Scheduler</h3></div></div></div><p>A weak_ptr to a shared scheduler. Does not keep the Scheduler
                        alive.</p></div><div class="sect2" title="Trackable Servant"><div class="titlepage"><div><div><h3 class="title"><a name="d0e519"></a>Trackable Servant</h3></div></div></div><p>Object living in a (single-threaded) scheduler, starting tasks and
                        handling callbacks.</p></div><div class="sect2" title="Queue"><div class="titlepage"><div><div><h3 class="title"><a name="d0e524"></a>Queue</h3></div></div></div><p>Holds jobs for a scheduler to execute. Used as communication mean between
                        Schedulers / Worlds</p></div><div class="sect2" title="Servant Proxy"><div class="titlepage"><div><div><h3 class="title"><a name="d0e529"></a>Servant Proxy</h3></div></div></div><p>A thread-safe object looking like a Servant and serializing calls to
                        it.</p></div><div class="sect2" title="Scheduler Shared Proxy"><div class="titlepage"><div><div><h3 class="title"><a name="d0e534"></a>Scheduler Shared Proxy</h3></div></div></div><p>Object holding a scheduler and interfacing with it. The last instance
                        joins the threads of the scheduler.</p></div><div class="sect2" title="Posting"><div class="titlepage"><div><div><h3 class="title"><a name="d0e539"></a>Posting</h3></div></div></div><p>Enqueueing a job into a Scheduler's queue.</p></div></div><div class="sect1" title="Hello, asynchronous world"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e544"></a>Hello, asynchronous world</h2></div></div></div><p>The following code shows a very basic usage (a complete example <a class="link" href="examples/example_post_future.cpp" target="_top">here</a>), this is not
                    really asynchronous yet:</p><pre class="programlisting">#include &lt;boost/asynchronous/scheduler/threadpool_scheduler.hpp&gt;
#include &lt;boost/asynchronous/queue/lockfree_queue.hpp&gt;
#include &lt;boost/asynchronous/scheduler_shared_proxy.hpp&gt;
#include &lt;boost/asynchronous/post.hpp&gt;
struct void_task
{
    void operator()()const
    {
        std::cout &lt;&lt; "void_task called" &lt;&lt; std::endl;
    }
};
struct int_task
{
    int operator()()const
    {
        std::cout &lt;&lt; "int_task called" &lt;&lt; std::endl;
        return 42;
    }
};  

// create a threadpool scheduler with 3 threads and communicate with it using a threadsafe_list
// we use auto as it is easier than boost::asynchronous::any_shared_scheduler_proxy&lt;&gt;
auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                            new boost::asynchronous::threadpool_scheduler&lt;
                                boost::asynchronous::lockfree_queue&lt;&gt; &gt;(3));
// post a simple task and wait for execution to complete
std::future&lt;void&gt; fuv = boost::asynchronous::post_future(scheduler, void_task());
fuv.get();
// post a simple task and wait for result
std::future&lt;int&gt; fui = boost::asynchronous::post_future(scheduler, int_task());
int res = fui.get();
  </pre><p>Of course this works with C++11 lambdas:</p><pre class="programlisting">auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                            new boost::asynchronous::threadpool_scheduler&lt;
                                boost::asynchronous::lockfree_queue&lt;&gt; &gt;(3));
// post a simple task and wait for execution to complete
std::future&lt;void&gt; fuv = boost::asynchronous::post_future(scheduler, [](){std::cout &lt;&lt; "void lambda" &lt;&lt; std::endl;});
fuv.get();
// post a simple task and wait for result
std::future&lt;int&gt; fui = boost::asynchronous::post_future(scheduler, [](){std::cout &lt;&lt; "int lambda" &lt;&lt; std::endl;return 42;});
int res = fui.get();   </pre><p>boost::asynchronous::post_future posts a piece of work to a threadpool
                    scheduler with 3 threads and using a lockfree_queue. We get a std::future&lt;the
                    type of the task return type&gt;.</p><p>This looks like much std::async, but we're just getting started. Let's move on
                    to something more asynchronous.</p></div><div class="sect1" title="A servant proxy"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e562"></a>A servant proxy</h2></div></div></div><p>We now want to create a single-threaded scheduler, populate it with some
                    servant(s), and exercise some members of the servant from an outside thread. We
                    first need a servant:</p><pre class="programlisting">struct Servant
{
    Servant(int data): m_data(data){}
    int doIt()const
    {
        std::cout &lt;&lt; "Servant::doIt with m_data:" &lt;&lt; m_data &lt;&lt; std::endl;
        return 5;
    }
    void foo(int&amp; i)const
    {
        std::cout &lt;&lt; "Servant::foo with int:" &lt;&lt; i &lt;&lt; std::endl;
        i = 100;
    }
    void foobar(int i, char c)const
    {
        std::cout &lt;&lt; "Servant::foobar with int:" &lt;&lt; i &lt;&lt; " and char:" &lt;&lt; c &lt;&lt;std::endl;
    }
    int m_data;
}; </pre><p>We now create a proxy type to be used in other threads:</p><pre class="programlisting">class ServantProxy : public boost::asynchronous::servant_proxy&lt;ServantProxy,<span class="bold"><strong>Servant</strong></span>&gt;
{
public:
    // forwarding constructor. Scheduler to servant_proxy, followed by arguments to Servant.
    template &lt;class Scheduler&gt;
    ServantProxy(Scheduler s, int data):
        boost::asynchronous::servant_proxy&lt;ServantProxy,<span class="bold"><strong>Servant</strong></span>&gt;(s, data)
    {}
    // the following members must be available "outside"
    // foo and foobar, just as a post (no interesting return value)
    BOOST_ASYNC_POST_MEMBER(<span class="bold"><strong>foo</strong></span>)
    BOOST_ASYNC_POST_MEMBER(<span class="bold"><strong>foobar</strong></span>)
    // for doIt, we'd like a future
    BOOST_ASYNC_FUTURE_MEMBER(<span class="bold"><strong>doIt</strong></span>)
};</pre><p>Let's use our newly defined proxy:</p><pre class="programlisting">int something = 3;
{
    auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                    new boost::asynchronous::single_thread_scheduler&lt;
                          boost::asynchronous::lockfree_queue&lt;&gt; &gt;);

    {
        // arguments (here 42) are forwarded to Servant's constructor
        ServantProxy proxy(scheduler,42);
        // post a call to foobar, arguments are forwarded.
        proxy.foobar(1,'a');
        // post a call to foo. To avoid races, the reference is ignored.
        proxy.foo(something);
        // post and get a future because we're interested in the result.
        std::future&lt;int&gt; fu = proxy.doIt();
        std::cout&lt;&lt; "future:" &lt;&lt; fu.get() &lt;&lt; std::endl;
    }// here, Servant's destructor is posted and waited for
}// scheduler is gone, its thread has been joined
std::cout&lt;&lt; "something:" &lt;&lt; something &lt;&lt; std::endl; // something was not changed as it was passed by value. You could use a boost::ref if this is not desired.</pre><p>We can call members on the proxy, almost as if they were called on Servant.
                    The library takes care of the posting and forwarding the arguments. When
                    required, a future is returned. Stack unwinding works, and when the servant
                    proxy goes out of scope, the servant destructor is posted. When the scheduler
                    goes out of scope, its thread is stopped and joined. The queue is processed
                    completely first. Of course, as many servants as desired can be created in this
                    scheduler context. Please have a look at <a class="link" href="examples/example_simple_servant.cpp" target="_top">the complete
                    example</a>.</p></div><div class="sect1" title="Using a threadpool from within a servant"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e597"></a>Using a threadpool from within a servant</h2></div></div></div><p>If you remember the principles of Asynchronous, blocking a single-thread
                    scheduler is taboo as it blocks the thread doing all the management of a system.
                    But what to do when one needs to execute long tasks? Asynchronous provides a
                    whole set of threadpools. A servant posts something to a threadpool, provides a
                    callback, then gets a result. Wait a minute. Callback? Is this not
                    thread-unsafe? Why not threadpools with futures, like usual? Because in a
                    perfectly asynchronous world, waiting for a future means blocking a servant
                    scheduler. One would argue that it is possible not to block on the future, and
                    instead ask if there is a result. But frankly, polling is not a nice solution
                    either.</p><p>And what about thread-safety? Asynchronous takes care of this. A callback is
                    never called from a threadpool, but instead posted back to the queue of the
                    scheduler which posted the work. All the servant has to do, is to do nothing and
                    wait until the callback is executed. Note that this is not the same as a
                    blocking wait, the servant can still react to events.</p><p>Clearly, this brings some new challenges as the flow of control gets harder to
                    follow. This is why a servant is often written using state machines. The
                    (biased) author suggests to have a look at the <a class="link" href="http://www.boost.org/doc/libs/1_59_0/libs/msm/doc/HTML/index.html" target="_top"> Meta State Machine library </a> , which plays nicely with
                    Asynchronous.</p><p>But what about the usual proactor issues (crashes) when the servant has long
                    been destroyed when the callback is posted. Gone. Asynchronous <span class="command"><strong><a name="trackable_servant"></a></strong></span><code class="code">trackable_servant</code> post_callback
                    ensures that a callback is not called if the servant is gone. Better even, if
                    the servant has been destroyed, an unstarted posted task will not be
                    executed.</p><p>What about another common issue? If one posts a task, say a lambda, which
                    captures a shared_ptr to an object per value, and this object is a
                    boost::signal? Then when the task object has been executed and is destroyed, one
                    could have a race on the signal deregistration. But again no. Asynchronous
                    ensures that a task created within a scheduler context gets destroyed in this
                    context.</p><p>This is about the best protection one can get. What Asynchronous cannot
                    protect from are self-made races within a task (if you post a task with a
                    pointer to the servant, you're on your own and have to protect your servant). A
                    good rule of thumb is to consider data passed to a task as moved or passed by
                    value. To support this, Asynchronous does not copy tasks but moves them.</p><p>Armed with these protections, let's give a try to a threadpool, starting with
                    the most basic one, <code class="code">threadpool_scheduler</code> (more to come):</p><pre class="programlisting">struct Servant : boost::asynchronous::trackable_servant&lt;&gt;
{
    Servant(boost::asynchronous::any_weak_scheduler&lt;&gt; scheduler)
        : boost::asynchronous::trackable_servant&lt;&gt;(scheduler,
                                               // <span class="bold"><strong>threadpool with 3 threads</strong></span> and a lockfree_queue
                                               boost::asynchronous::create_shared_scheduler_proxy(
                                                   new <span class="bold"><strong>boost::asynchronous::<span class="bold"><strong>threadpool_scheduler</strong></span></strong></span>&lt;
                                                           boost::asynchronous::lockfree_queue&lt;&gt; &gt;(<span class="bold"><strong>3</strong></span>))){}
    // call to this is posted and executes in our (safe) single-thread scheduler
    void start_async_work()
    {
       //ok, let's post some work and wait for an answer
       <span class="bold"><strong>post_callback</strong></span>(
                    [](){std::cout &lt;&lt; "Long Work" &lt;&lt; std::endl;}, // work, do not use "this" here
                    [/*this*/](boost::asynchronous::expected&lt;void&gt;){...}// callback. Safe to use "this" as callback is only called if Servant is alive
        );
    }
};</pre><p>We now have a servant, living in its own thread, which posts some long work to
                    a three-thread-threadpool and gets a callback, but only if still alive.
                    Similarly, the long work will be executed by the threadpool only if Servant is
                    alive by the time it starts. Everything else stays the same, one creates a proxy
                    for the servant and posts calls to its members, so we'll skip it for
                    conciseness, the complete example can be found <a class="link" href="examples/example_post_trackable_threadpool.cpp" target="_top">here</a>.</p></div><div class="sect1" title="A servant using another servant proxy"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e645"></a>A servant using another servant proxy</h2></div></div></div><p>Often, in a layered design, you'll need that a servant in a single-threaded
                    scheduler calls a member of a servant living in another one. And you'll want to
                    get a callback, not a future, because you absolutely refuse to block waiting for
                    a future (and you'll be very right of course!). Ideally, except for main(), you
                    won't want any of your objects to wait for a future. There is another
                    servant_proxy macro for this, <code class="code">BOOST_ASYNC_UNSAFE_MEMBER</code>(unsafe
                    because you get no thread-safety from if and you'll take care of this yourself,
                    or better, <code class="code">trackable_servant</code> will take care of it for you, as
                    follows):</p><p>
                    </p><pre class="programlisting">// Proxy for a basic servant 
class ServantProxy : public boost::asynchronous::servant_proxy&lt;ServantProxy,Servant&gt;
{
public:
    template &lt;class Scheduler&gt;
    ServantProxy(Scheduler s, int data):
        boost::asynchronous::servant_proxy&lt;ServantProxy,Servant&gt;(s, data)
    {}
    BOOST_ASYNC_UNSAFE_MEMBER(foo)
    BOOST_ASYNC_UNSAFE_MEMBER(foobar)
};   </pre><p>
                    </p><pre class="programlisting">// Servant using the first one
struct Servant2 : boost::asynchronous::trackable_servant&lt;&gt;
{
    Servant2(boost::asynchronous::any_weak_scheduler&lt;&gt; scheduler,ServantProxy worker)
        :boost::asynchronous::trackable_servant&lt;&gt;(scheduler)
        ,m_worker(worker) // the proxy allowing access to Servant
    void doIt()    
    {                 
         call_callback(m_worker.get_proxy(), // Servant's outer proxy, for posting tasks
                       m_worker.foo(), // what we want to call on Servant
                      // callback functor, when done.
                      [](boost::asynchronous::expected&lt;int&gt; result){...} );// expected&lt;return type of foo&gt; 
    }
};</pre><p>
                </p><p>Call of <code class="code">foo()</code> will be posted to <code class="code">Servant</code>'s scheduler,
                    and the callback lambda will be posted to <code class="code">Servant2</code> when completed.
                    All this thread-safe of course. Destruction is also safe. When
                        <code class="code">Servant2</code> goes out of scope, it will shutdown
                        <code class="code">Servant</code>'s scheduler, then will his scheduler be shutdown
                    (provided no more object is living there), and all threads joined. The <a class="link" href="examples/example_two_simple_servants.cpp" target="_top">complete example
                    </a> shows a few more calls too.</p><p>Asynchronous offers a different syntax to achieve the same result. Which one
                    you use is a matter of taste, both are equivalent. The second method is with
                    BOOST_ASYNC_MEMBER_UNSAFE_CALLBACK(_LOG if you need logging). It takes a
                    callback as argument, other arguments are forwarded. Combined with
                    <code class="code">make_safe_callback</code>, one gets the same effect (safe call) as above.</p><pre class="programlisting">// Proxy for a basic servant 
class ServantProxy : public boost::asynchronous::servant_proxy&lt;ServantProxy,Servant&gt;
{
public:
    template &lt;class Scheduler&gt;
    ServantProxy(Scheduler s, int data):
        boost::asynchronous::servant_proxy&lt;ServantProxy,Servant&gt;(s, data)
    {}
    BOOST_ASYNC_MEMBER_UNSAFE_CALLBACK(foo) // say, foo takes an int as argument
};   </pre><pre class="programlisting">// Servant using the first one
struct Servant2 : boost::asynchronous::trackable_servant&lt;&gt;
{
    Servant2(boost::asynchronous::any_weak_scheduler&lt;&gt; scheduler,ServantProxy worker)
        :boost::asynchronous::trackable_servant&lt;&gt;(scheduler)
        ,m_worker(worker) // the proxy allowing access to Servant
    void doIt()    
    {                 
         m_worker.foo(make_safe_callback([](boost::asynchronous::expected&lt;void&gt; res) // expected&lt;return type of foo&gt; 
                                        {/* callback code*/}), 
                      42 /* arguments of foo*/); 
    }
};</pre></div><div class="sect1" title="Interrupting tasks"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e693"></a><span class="command"><strong><a name="interrupting_tasks"></a></strong></span>Interrupting tasks</h2></div></div></div><p>Let's imagine that a manager object (a state machine for example) posted some
                    long-lasting work to a threadpool, but this long-lasting work really takes too
                    long. As we are in an asynchronous world and non-blocking, the manager object
                    realizes there is a problem and decides the task must be stopped otherwise the
                    whole application starts failing some real-time constraints (how would we do if
                    we were blocked, waiting for a future?). This is made possible by using another
                    form of posting, getting a handle, on which one can require interruption. As
                    Asynchronous does not kill threads, we'll use one of Boost.Thread predefined
                    interruption points. Supposing we have well-behaved tasks, they will be
                    interrupted at the next interruption point if they started, or if they did not
                    start yet because they are waiting in a queue, then they will never start. In
                    this <a class="link" href="examples/example_interrupt.cpp" target="_top">example</a>, we have
                    very little to change but the post call. We use <code class="code">interruptible_post_callback</code>
                    instead of <code class="code">post_callback</code>. We get an <code class="code">any_interruptible object</code>, which offers a
                    single <code class="code">interrupt()</code> member.</p><pre class="programlisting">struct Servant : boost::asynchronous::trackable_servant&lt;&gt;
{
     ... // as usual
    void start_async_work()
    {
        // start long interruptible tasks
        // we get an interruptible handler representing the task
        <span class="bold"><strong>boost::asynchronous::any_interruptible</strong></span> interruptible =
        <span class="bold"><strong>interruptible_post_callback</strong></span>(
                // interruptible task
               [](){
                    std::cout &lt;&lt; "Long Work" &lt;&lt; std::endl;
                    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));}, // sleep is an interrupting point
               // callback functor.
               [](boost::asynchronous::expected&lt;void&gt; ){std::cout &lt;&lt; "Callback will most likely not be called" &lt;&lt; std::endl;}
        );
        // let the task start (not sure but likely)
        // if it had no time to start, well, then it will never.
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        // actually, we changed our mind and want to interrupt the task
        interruptible.<span class="bold"><strong>interrupt()</strong></span>;
        // the callback will likely never be called as the task was interrupted
    }
};                </pre></div><div class="sect1" title="Logging tasks"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e725"></a><span class="command"><strong><a name="logging_tasks"></a></strong></span>Logging tasks</h2></div></div></div><p>Developers are notoriously famous for being bad at guessing which part of
                    their code is inefficient. This is bad in itself, but even worse for a control
                    class like our post-callback servant as it reduces responsiveness. Knowing how
                    long a posted tasks or a callback lasts is therefore very useful. Knowing how
                    long take tasks executing in the threadpools is also essential to plan what
                    hardware one needs for an application(4 cores? Or 100?). We need to know what
                    our program is doing. Asynchronous provides logging per task to help there.
                    Let's have a look at some code. It's also time to start using our template
                    parameters for <code class="code">trackable_servant</code>, in case you wondered why they are
                    here.</p><pre class="programlisting">// we will be using loggable jobs internally
typedef boost::asynchronous::any_loggable&lt;std::chrono::high_resolution_clock&gt; <span class="bold"><strong>servant_job</strong></span>;
// the type of our log
typedef std::map&lt;std::string,std::list&lt;boost::asynchronous::diagnostic_item&lt;std::chrono::high_resolution_clock&gt; &gt; &gt; <span class="bold"><strong>diag_type</strong></span>;

// we log our scheduler and our threadpool scheduler (both use servant_job)
struct Servant : boost::asynchronous::trackable_servant&lt;<span class="bold"><strong>servant_job</strong></span>,<span class="bold"><strong>servant_job</strong></span>&gt;
{
    Servant(boost::asynchronous::any_weak_scheduler&lt;servant_job&gt; scheduler) //servant_job is our job type
        : boost::asynchronous::trackable_servant&lt;<span class="bold"><strong>servant_job,servant_job</strong></span>&gt;(scheduler,
                                               boost::asynchronous::create_shared_scheduler_proxy(
                                                   // threadpool with 3 threads and a simple threadsafe_list queue
                                                   // Furthermore, it logs posted tasks
                                                   new boost::asynchronous::threadpool_scheduler&lt;
                                                           //servant_job is our job type
                                                           boost::asynchronous::lockfree_queue&lt; <span class="bold"><strong>servant_job</strong></span> &gt; &gt;(3))){}
    void start_async_work()
    {
         post_callback(
               // task posted to threadpool
               [](){...}, // will return an int
               [](boost::asynchronous::expected&lt;int&gt; res){...},// callback functor.
               // the task / callback name for logging
               <span class="bold"><strong>"int_async_work"</strong></span>
        );
    }
    // we happily provide a way for the outside world to know what our threadpool did.
    // get_worker is provided by trackable_servant and gives the proxy of our threadpool
    diag_type get_diagnostics() const
    {
        return (*get_worker()).get_diagnostics();
    }
};</pre><p>The proxy is also slightly different, using a _LOG macro and an argument
                    representing the name of the task.</p><pre class="programlisting">class ServantProxy : public boost::asynchronous::servant_proxy&lt;ServantProxy,Servant,servant_job&gt;
{
public:
    template &lt;class Scheduler&gt;
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy&lt;ServantProxy,Servant,servant_job&gt;(s)
    {}
    // the _LOG macros do the same as the others, but take an extra argument, the logged task name
    BOOST_ASYNC_FUTURE_MEMBER<span class="bold"><strong>_LOG</strong></span>(start_async_work,<span class="bold"><strong>"proxy::start_async_work"</strong></span>)
    BOOST_ASYNC_FUTURE_MEMBER<span class="bold"><strong>_LOG</strong></span>(get_diagnostics,<span class="bold"><strong>"proxy::get_diagnostics"</strong></span>)
};               </pre><p> We now can get diagnostics from both schedulers, the single-threaded and the
                    threadpool (as external code has no access to it, we ask Servant to help us
                    there through a get_diagnostics() member).</p><pre class="programlisting">// create a scheduler with logging
auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::single_thread_scheduler&lt;
                                    boost::asynchronous::lockfree_queue&lt;servant_job&gt; &gt;);
// create a Servant                    
ServantProxy proxy(scheduler); 
...
// let's ask the single-threaded scheduler what it did.
diag_type single_thread_sched_diag = scheduler.get_diagnostics(); 
for (auto mit = single_thread_sched_diag.begin(); mit != single_thread_sched_diag.end() ; ++mit)
{
     std::cout &lt;&lt; "job type: " &lt;&lt; (*mit).first &lt;&lt; std::endl;
     for (auto jit = (*mit).second.begin(); jit != (*mit).second.end();++jit)
     {
          std::cout &lt;&lt; "job waited in us: " &lt;&lt; std::chrono::nanoseconds((*jit).get_started_time() - (*jit).<span class="bold"><strong>get_posted_time()</strong></span>).count() / 1000 &lt;&lt; std::endl;
          std::cout &lt;&lt; "job lasted in us: " &lt;&lt; std::chrono::nanoseconds((*jit).get_finished_time() - (*jit).<span class="bold"><strong>get_started_time()</strong></span>).count() / 1000 &lt;&lt; std::endl;
          std::cout &lt;&lt; "job interrupted? "  &lt;&lt; std::boolalpha &lt;&lt; (*jit).<span class="bold"><strong>is_interrupted()</strong></span> &lt;&lt; std::endl;
          std::cout &lt;&lt; "job failed? "  &lt;&lt; std::boolalpha &lt;&lt; (*jit).<span class="bold"><strong>is_failed()</strong></span> &lt;&lt; std::endl; // did this job throw an exception?
     }
}              </pre><p>It goes similarly with the threapool scheduler, with the slight difference
                    that we ask the Servant to deliver diagnostic information through a proxy
                    member. The <a class="link" href="examples/example_log.cpp" target="_top">complete example</a>
                    shows all this, plus an interrupted job.</p></div><div class="sect1" title="Generating HTML diagnostics"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e794"></a><span class="command"><strong><a name="html_diags"></a></strong></span>Generating HTML diagnostics</h2></div></div></div><p>We just saw how to programmatically get diagnostics from schedulers. This is
                    very useful, but nobody likes to do it manually, so the authors went the extra
                    mile and provide an HTML formatter for convenience. The <a class="link" href="examples/example_html_diagnostics.cpp" target="_top">included example</a>
                    shows how to use it. In this example, we have a Servant, living in its own
                    single-threaded scheduler called "Servant". It uses a threadpool call
                    "Threadpool". When the Servant's foo() method is called, it executes a
                    parallel_reduce(parallel_for(...)), or whatever you like. These operations are
                    named accordingly. We also create a third scheduler, called "Formatter
                    scheduler", which will be used by the formatter code. Yes, even this scheduler
                    will be logged too. The example creates a Servant, calls foo() on the proxy,
                    sleeps for a while (how long is passed to the example as argument), then
                    generates <a class="link" href="examples/in_progress.html" target="_top">a first output
                        statistics</a>. Depending on the sleep time, the parallel work might or
                    might not be finished, so this is an intermediate result.</p><p>We then wait for the tasks to finish, destroy the servant, so that its
                    destructor is logged too, and we generate a <a class="link" href="examples/final.html" target="_top">final diagnostics</a>.</p><p>The HTML pages display the statistics for all schedulers, including the
                    formatter. It shows with different colors the waiting times of tasks (called
                    Scheduling time), the execution times, successful or failed separately, and the
                    added total time for each task, with max min, average duration. One can also
                    display the full list of tasks and even histograms. As this is a lot of
                    information, it is possible to hide part of it using checkboxes.</p><p>One also gets the very useful information of how long are the different
                    scheduler queues, which gives a very good indication of how busy the system
                    is.</p></div><div class="sect1" title="Queue container with priority"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e815"></a>Queue container with priority</h2></div></div></div><p>Sometimes, all jobs posted to a scheduler do not have the same priority. For
                    threadpool schedulers, <code class="code">composite_threadpool_scheduler</code> is an option.
                    For a single-threaded scheduler, Asynchronous does not provide a priority queue
                    but a queue container, which itself contains any number of queues, of different
                    types if needed. This has several advantages:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>Priority is defined simply by posting to the queue with the
                                desired priority, so there is no need for expensive priority
                                algorithms.</p></li><li class="listitem"><p>Reduced contention if many threads of a threadpool post something
                                to the queue of a single-threaded scheduler. If no priority is
                                defined, one queue will be picked, according to a configurable
                                policy, reducing contention on a single queue.</p></li><li class="listitem"><p>It is possible to mix queues.</p></li><li class="listitem"><p>It is possible to build a queue container of queue containers,
                                etc.</p></li></ul></div><p>Note: This applies to any scheduler. We'll start with single-threaded
                    schedulers used by managing servants for simplicity, but it is possible to have
                    composite schedulers using queue containers for finest granularity and least
                    contention.</p><p>First, we need to create a single-threaded scheduler with several queues for
                    our servant to live in, for example, one threadsafe list and three lockfree
                    queues:</p><pre class="programlisting">boost::asynchronous::any_shared_scheduler_proxy&lt;&gt; scheduler = 
                           boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::single_thread_scheduler&lt;
                                        boost::asynchronous::any_queue_container&lt;&gt; &gt;
                        (boost::asynchronous::any_queue_container_config&lt;boost::asynchronous::<span class="bold"><strong>threadsafe_list</strong></span>&lt;&gt; &gt;(<span class="bold"><strong>1</strong></span>),
                         boost::asynchronous::any_queue_container_config&lt;boost::asynchronous::<span class="bold"><strong>lockfree_queue</strong></span>&lt;&gt; &gt;(<span class="bold"><strong>3</strong></span>,100)
                         ));</pre><p><code class="code">any_queue_container</code> takes as constructor arguments a variadic
                    sequence of <code class="code">any_queue_container_config</code>, with a queue type as
                    template argument, and in the constructor the number of objects of this queue
                    (in the above example, one <code class="code">threadsafe_list</code> and 3
                        <code class="code">lockfree_queue</code> instances, then the parameters that these queues
                    require in their constructor (100 is the capacity of the underlying
                        <code class="code">boost::lockfree_queue</code>). This means, that our
                        <code class="code">single_thread_scheduler</code> has 4 queues:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>a threadsafe_list at index 1</p></li><li class="listitem"><p>lockfree queues at indexes 2,3,4</p></li><li class="listitem"><p>&gt;= 4 means the queue with the least priority.</p></li><li class="listitem"><p>0 means "any queue" and is the default</p></li></ul></div><p>The scheduler will handle these queues as having priorities: as long as there
                    are work items in the first queue, take them, if there are no, try in the
                    second, etc. If all queues are empty, the thread gives up his time slice and
                    sleeps until some work item arrives. If no priority is defined by posting, a
                    queue will be chosen (by default randomly, but this can be configured with a
                    policy). This has the advantage of reducing contention of the queue, even when
                    not using priorities. The servant defines the priority of the tasks it provides.
                    While this might seem surprising, it is a design choice to avoid that the coder
                    using a servant proxy interface would have to think about it, as you will see in
                    the second listing. To define a priority for a servant proxy, there is a second
                    field in the macros:</p><pre class="programlisting">class ServantProxy : public boost::asynchronous::servant_proxy&lt;ServantProxy,Servant&gt;
{
public:
    template &lt;class Scheduler&gt;
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy&lt;ServantProxy,Servant&gt;(s)
    {}
    <span class="bold"><strong>BOOST_ASYNC_SERVANT_POST_CTOR(3)</strong></span>
    <span class="bold"><strong>BOOST_ASYNC_SERVANT_POST_DTOR(4)</strong></span>
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work,<span class="bold"><strong>1</strong></span>)
};</pre><p>BOOST_ASYNC_FUTURE_MEMBER and other similar macros can be given an optional
                    priority parameter, in this case 1, which is our threadsafe list. Notice how you
                    can then define the priority of the posted servant constructor and
                    destructor.</p><pre class="programlisting">ServantProxy proxy(scheduler);
std::future&lt;std::future&lt;int&gt;&gt; fu = proxy.start_async_work();</pre><p>Calling our proxy member stays unchanged because the macro defines the
                    priority of the call.</p><p>We also have an extended version of <code class="code">post_callback</code>, called by a
                    servant posting work to a threadpool:</p><pre class="programlisting">post_callback(
       [](){return 42;},// work
       [this](boost::asynchronous::expected&lt;int&gt; res){}// callback functor.
       ,"",
       <span class="bold"><strong>2</strong></span>, // work prio
       <span class="emphasis"><em>2</em></span>  // callback prio
);</pre><p>Note the two added priority values: the first one for the task posted to the
                    threadpool, the second for the priority of the callback posted back to the
                    servant scheduler. The string is the log name of the task, which we choose to
                    ignore here.</p><p>The priority is in any case an indication, the scheduler is free to ignore it
                    if not supported. In the <a class="link" href="examples/example_queue_container.cpp" target="_top">example</a>, the single threaded scheduler will honor the request, but
                    the threadpool has a normal queue and cannot honor the request, but a threadpool
                    with an <code class="code">any_queue_container</code> or a
                        <code class="code">composite_threadpool_scheduler</code> can. The <a class="link" href="examples/example_queue_container_log.cpp" target="_top">same example</a>
                    can be rewritten to also support logging.</p><p><code class="code">any_queue_container</code> has two template arguments. The first, the
                    job type, is as always by default, a callable (<code class="code">any_callable</code>) job.
                    The second is the policy which Asynchronous uses to find the desired queue for a
                    job. The default is <code class="code">default_find_position</code>, which is as described
                    above, 0 means any position, all other values map to a queue, priorities &gt;=
                    number of queues means last queue. Any position is by default random
                        (<code class="code">default_random_push_policy</code>), but you might pick
                        <code class="code">sequential_push_policy</code>, which keeps an atomic counter to post
                    jobs to queues in a sequential order.</p><p>If you plan to build a queue container of queue containers, you'll probably
                    want to provide your own policy.</p></div><div class="sect1" title="Multiqueue Schedulers' priority"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e952"></a>Multiqueue Schedulers' priority</h2></div></div></div><p>A multiqueue_... threadpool scheduler has a queue for each thread. This
                    reduces contention, making these faster than single queue schedulers, like
                    threadpool_scheduler. Furthermore, these schedulers support priority: the
                    priority given in post_future or post_callback is the (1-based) position of the
                    queue we want to post to. 0 means "any queue". A queue of priority 1 has a
                    higher priority than a queue with priority 2, etc. </p><p>Each queue is serving one thread, but threads steal from each other's queue,
                    according to the priority.</p></div><div class="sect1" title="Threadpool Schedulers with several queues"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e959"></a>Threadpool Schedulers with several queues</h2></div></div></div><p>A queue container has advantages (different queue types, priority for single
                    threaded schedulers) but also disadvantages (takes jobs from one end of the
                    queue, which means potential cache misses, more typing work). If you don't need
                    different queue types for a threadpool but want to reduce contention, multiqueue
                    schedulers are for you. A normal <code class="code">threadpool_scheduler</code> has x threads
                    and one queue, serving them. A <code class="code">multiqueue_threadpool_scheduler</code> has
                    x threads and x queues, each serving a worker thread. Each thread looks for work
                    in its queue. If it doesn't find any, it looks for work in the previous one,
                    etc. until it finds one or inspected all the queues. As all threads steal from
                    the previous queue, there is little contention. The construction of this
                    threadpool is very similar to the simple
                    <code class="code">threadpool_scheduler</code>:</p><pre class="programlisting">boost::asynchronous::any_shared_scheduler_proxy&lt;&gt; scheduler = 
    boost::asynchronous::create_shared_scheduler_proxy(
                // 4 threads and 4 lockfree queues of 10 capacity
                new boost::asynchronous::multiqueue_threadpool_scheduler&lt;boost::asynchronous::lockfree_queue&lt;&gt; &gt;(4,10));</pre><p>The first argument is the number of worker threads, which is at the same time
                    the number of queues. As for every scheduler, if the queue constructor takes
                    arguments, they come next and are forwarded to the queue.</p><p>This is the <span class="underline">advised</span> scheduler for
                    standard cases as it offers lesser contention and task stealing between the
                    queues it uses for task transfer.</p><p><span class="italic">Limitation:</span> these schedulers cannot have 0
                    thread like their single-queue counterparts.</p></div><div class="sect1" title="Composite Threadpool Scheduler"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e986"></a>Composite Threadpool Scheduler</h2></div></div></div><div class="sect2" title="Usage"><div class="titlepage"><div><div><h3 class="title"><a name="d0e989"></a>Usage</h3></div></div></div><p>When a project becomes more complex, having a single threadpool for the whole
                    application does not offer enough flexibility in load planning. It is pretty
                    hard to avoid either oversubscription (more busy threads than available hardware
                    threads) or undersubscription. One would need one big threadpool with exactly
                    the number of threads available in the hardware. Unfortunately, if we have a
                    hardware with, say 12 hardware threads, parallelizing some work using all 12
                    might be slowlier than using only 8. One would need different threadpools of
                    different number of threads for the application. This, however, has the serious
                    drawback that there is a risk that some threadpools will be in overload, while
                    others are out of work unless we have work stealing between different
                    threadpools.</p><p>The second issue is task priority. One can define priorities with several
                    queues or a queue container, but this ensures that only highest priority tasks
                    get executed if the system is coming close to overload. Ideally, it would be
                    great if we could decide how much compute power we give to each task
                    type.</p><p>This is what <code class="code">composite_threadpool_scheduler</code> solves. This pool
                        supports, like any other pool, the
                        <code class="code">any_shared_scheduler_proxy</code>concept so you can use it in place of
                        the ones we used so far. The pool is composed of other pools
                            (<code class="code">any_shared_scheduler_proxy</code> pools). It implements work
                        stealing between pools if a) the pools support it and b) the queue of a pool
                        also does. For example, we can create the following worker pool made of 3
                        sub-pools:</p><p>
                    </p><pre class="programlisting">// create a composite threadpool made of:
// a multiqueue_threadpool_scheduler, 1 thread, with a lockfree_queue of capacity 100. 
// This scheduler does not steal from other schedulers, but will lend its queue for stealing
boost::asynchronous::any_shared_scheduler_proxy&lt;&gt; tp = boost::asynchronous::create_shared_scheduler_proxy( 
               new boost::asynchronous::multiqueue_threadpool_scheduler&lt;boost::asynchronous::lockfree_queue&lt;&gt; &gt; (1,100));

// a stealing_multiqueue_threadpool_scheduler, 3 threads, each with a threadsafe_list
// this scheduler will steal from other schedulers if it can. In this case it will manage only with tp, not tp3
boost::asynchronous::any_shared_scheduler_proxy&lt;&gt; tp2 = boost::asynchronous::create_shared_scheduler_proxy( 
                    new boost::asynchronous::stealing_multiqueue_threadpool_scheduler&lt;boost::asynchronous::threadsafe_list&lt;&gt; &gt; (3));

// a multiqueue_threadpool_scheduler, 4 threads, each with a lockfree_spsc_queue of capacity 100
// this is safe because there will be no stealing as the queue does not support it, and only the servant single-thread scheduler will be the producer
boost::asynchronous::any_shared_scheduler_proxy&lt;&gt; tp3 = boost::asynchronous::create_shared_scheduler_proxy( 
               new boost::asynchronous::multiqueue_threadpool_scheduler&lt;boost::asynchronous::lockfree_spsc_queue&lt;&gt; &gt; (4,100));

// create a composite pool made of the 3 previous ones
boost::asynchronous::any_shared_scheduler_proxy&lt;&gt; tp_worker =
             boost::make_shared&lt;boost::asynchronous::composite_threadpool_scheduler&lt;&gt; &gt; (tp,tp2,tp3);
                    </pre><p>
                </p><p>We can use this pool:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>As a big worker pool. In this case, the priority argument we use
                                for posting refers to the (1-based) index of the subpool
                                (post_callback(func1,func2,"task name",<span class="bold"><strong>1</strong></span>,0);). "1" means post to the first pool. But
                                another pool could steal the work.</p></li><li class="listitem"><p>As a pool container, but different parts of the code will get to
                                see only the subpools. For example, the pools tp, tp2 and tp3 can
                                still be used independently as a worker pool. Calling
                                composite_threadpool_scheduler&lt;&gt;::get_scheduler(std::size_t
                                index_of_pool) will also give us the corresponding pool (1-based, as
                                always).</p></li></ul></div><p>Another example of why to use this pool is reusing threads allocated to an
                        asio-based communication for helping other schedulers. Addng an asio
                        scheduler to a composite pool will allow the threads of this scheduler to
                        help (steal) other pools when no communication is currently happening. </p><p>Stealing is done with priority. A stealing pool first tries to steal from the
                        first pool, then from the second, etc.</p><p>The <a class="link" href="examples/example_composite_threadpool.cpp" target="_top">following
                        example</a> shows a complete servant implementation, and the <span class="command"><strong><a class="command" href="#asio_scheduler">ASIO section</a></strong></span> will show how an ASIO
                    pool can steal.</p><p>The threadpool schedulers we saw so far are not stealing from other pools. The
                        single-queue schedulers are not stealing, and the multiqueue schedulers
                        steal from the queues of other threads of the same pool. The
                        scheduler-stealing schedulers usually indicate this by appending a
                            <code class="code">stealing_</code> to their name:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p><code class="code">stealing_threadpool_scheduler</code> is a
                                        <code class="code">threadpool_scheduler</code> which steals from other
                                    pools.</p></li><li class="listitem"><p><code class="code">stealing_multiqueue_threadpool_scheduler</code> is a
                                        <code class="code">multiqueue_threadpool scheduler</code> which steals
                                    from other pools.</p></li><li class="listitem"><p><code class="code">asio_scheduler steals</code>.</p></li></ul></div><p>The only difference with their not stealing equivalent is that they steal from
                        other schedulers. To achieve this, they need a composite_scheduler to tell
                        them from which schedulers they can steal.</p><p>Not all schedulers offer to be stolen from. A
                            <code class="code">single_thread_scheduler</code> does not as it would likely bring
                        race conditions to active objects.</p><p>Another interesting usage will be when planning for extra machines to help a
                        threadpool by processing some of the work: work can be stolen from a
                        threadpool by a <span class="command"><strong><a class="command" href="#distributing">tcp_server_scheduler</a></strong></span> from which other machines can get it.
                        Just pack both pools in a <code class="code">composite_threadpool_scheduler</code> and
                        you're ready to go.</p></div><div class="sect2" title="Priority"><div class="titlepage"><div><div><h3 class="title"><a name="d0e1078"></a>Priority</h3></div></div></div><p>A composite supports priority. The first pool passed in the constructor of
                        the composite pool has priority 1, the second 2, etc. 0 means "any pool" and
                        n where n &gt; number of pools will me modulo-ed.</p><p>Posting to this scheduler using post_future or post_callback using a given
                        priority will post to the according pool. If a pool supports stealing from
                        other pools (stealing_... pools), it will try to steal from other pools,
                        starting with the highest priority, but only if the to be stolen from pools
                        supports it. For example, we try to post to the first pool, callback to any
                        queue.</p><pre class="programlisting">post_callback(
               [](){},// work
               [this](boost::asynchronous::expected&lt;int&gt;){},// callback functor.
               "", // task and callback name
               1,  // work priority, highest
               0   // callback anywhere
);</pre></div></div><div class="sect1" title="More flexibility in dividing servants among threads"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e1087"></a>More flexibility in dividing servants among threads</h2></div></div></div><p>TODO example and code. We saw how to assign a servant or several servants to a
                    single thread scheduler. We can also create schedulers and divide servants among
                    them. This is very powerful but still has some constraints:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>We need to assign servants to schedulers while what we want is to
                                assign them to threads. We also have to consider how many schedulers
                                to create. This is not very flexible.</p></li><li class="listitem"><p>If a servant is taking too long, it blocks all other servants
                                living inside this thread context. This increases latency.</p></li></ul></div><p>We can increase the flexibility and reduce latency by using a
                        <code class="code">multiple_thread_scheduler</code>. This scheduler takes as first
                    argument a number of threads to use and a maximum number of client "worlds"
                    (clients living logically in the same thread context). What it does, is to
                    assign any of its threads to different client worlds, but only one thread can
                    service a world at a time. This means that the thread safety of servants is
                    preserved. At the same time, having any number of threads decreases latency
                    because if a servant keeps its thread busy, it does not block other servants
                    from being serviced. As we can choose the number of threads this scheduler will
                    use, we achieve very fine granularity in planing our thread resources.</p><p>Another interesting characteristics of this scheduler is that its threads
                    service its servants in order. If a thread serviced servant x, it next tries to
                    service servant x+1. This makes for good pipelining capabilities as it increases
                    the odds that task is koved from a pipeline stage to the next one by the same
                    thread and will be hot in its cache.</p></div><div class="sect1" title="Processor binding"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e1106"></a>Processor binding</h2></div></div></div><p>TODO example and code.On many systems, it can improve performance to bind
                    threads to a processor: better cache usage is likely as the OS does not move
                    threads from core to core. Mostly for threadpools this is an option you might
                    want to try.</p><p>Usage is very simple. One needs to call
                        <code class="code">processor_bind(core_index)</code> on a scheduler proxy. This function
                    takes a single argument, the core to which the first thread of the pool will be
                    bound. The second thread will be bound to core+1, etc.</p></div><div class="sect1" title="asio_scheduler"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e1116"></a><span class="command"><strong><a name="asio_scheduler"></a></strong></span>asio_scheduler</h2></div></div></div><p>Asynchronous supports the possibility to use Boost.Asio as a threadpool
                    provider. This has several advantages:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>asio_scheduler is delivered with a way to access Asio's io_service
                                from a servant object living inside the scheduler.</p></li><li class="listitem"><p>asio_scheduler handles the necessary work for creating a pool of
                                threads for multithreaded-multi-io_service communication.</p></li><li class="listitem"><p>asio_scheduler threads implement work-stealing from other
                                Asynchronous schedulers. This allows communication threads to help
                                other threadpools when no I/O communication is happening. This helps
                                reducing thread oversubscription.</p></li><li class="listitem"><p>One has all the usual goodies of Asynchronous: safe callbacks,
                                object tracking, servant proxies, etc.</p></li></ul></div><p>Let's create a simple but powerful example to illustrate its usage. We want to
                    create a TCP client, which connects several times to the same server, gets data
                    from it (in our case, the Boost license will do), then checks if the data is
                    coherent by comparing the results two-by-two. Of course, the client has to be
                    perfectly asynchronous and never block. We also want to guarantee some threads
                    for the communication and some for the calculation work. We also want to
                    communication threads to "help" by stealing some work if necessary.</p><p>Let's start by creating a TCP client using Boost.Asio. A slightly modified
                    version of the async TCP client from the Asio documentation will do. All we
                    change is pass it a callback which it will call when the requested data is
                    ready. We now pack it into an Asynchronous trackable servant:</p><pre class="programlisting">// Objects of this type are made to live inside an asio_scheduler,
// they get their associated io_service object from Thread Local Storage
struct AsioCommunicationServant : boost::asynchronous::trackable_servant&lt;&gt;
{
    AsioCommunicationServant(boost::asynchronous::any_weak_scheduler&lt;&gt; scheduler,
                             const std::string&amp; server, const std::string&amp; path)
        : boost::asynchronous::trackable_servant&lt;&gt;(scheduler)
        , m_client(*<span class="bold"><strong>boost::asynchronous::get_io_service&lt;&gt;()</strong></span>,server,path)
    {}
    void test(std::function&lt;void(std::string)&gt; cb)
    {
        // just forward call to asio asynchronous http client
        // the only change being the (safe) callback which will be called when http get is done
        m_client.request_content(cb);
    }
private:
    client m_client; //client is from Asio example
};</pre><p>The main noteworthy thing to notice is the call to <span class="bold"><strong>boost::asynchronous::get_io_service&lt;&gt;()</strong></span>, which, using
                    thread-local-storage, gives us the io_service associated with this thread (one
                    io_service per thread). This is needed by the Asio TCP client. Also noteworthy
                    is the argument to <code class="code">test()</code>, a callback when the data is available. </p><p>Wait a minute, is this not unsafe (called from an asio worker thread)? It is
                    but it will be made safe in a minute.</p><p>We now need a proxy so that this communication servant can be safely used by
                    others, as usual:</p><pre class="programlisting">class AsioCommunicationServantProxy: public boost::asynchronous::servant_proxy&lt;AsioCommunicationServantProxy,AsioCommunicationServant &gt;
{
public:
    // ctor arguments are forwarded to AsioCommunicationServant
    template &lt;class Scheduler&gt;
    AsioCommunicationServantProxy(Scheduler s,const std::string&amp; server, const std::string&amp; path):
        boost::asynchronous::servant_proxy&lt;AsioCommunicationServantProxy,AsioCommunicationServant &gt;(s,server,path)
    {}
    // we offer a single member for posting
    BOOST_ASYNC_POST_MEMBER(test)
};                   </pre><p>A single member, <code class="code">test</code>, is used in the proxy. The constructor
                    takes the server and relative path to the desired page. We now need a manager
                    object, which will trigger the communication, wait for data, check that the data
                    is coherent:</p><pre class="programlisting">struct Servant : boost::asynchronous::trackable_servant&lt;&gt;
{
    Servant(boost::asynchronous::any_weak_scheduler&lt;&gt; scheduler,const std::string&amp; server, const std::string&amp; path)
        : boost::asynchronous::trackable_servant&lt;&gt;(scheduler)
        , m_check_string_count(0)
    {
        // as worker we use a simple threadpool scheduler with 4 threads (0 would also do as the asio pool steals)
        auto worker_tp = boost::asynchronous::create_shared_scheduler_proxy(
                    new boost::asynchronous::threadpool_scheduler&lt;boost::asynchronous::lockfree_queue&lt;&gt; &gt; (4));

        // for tcp communication we use an asio-based scheduler with 3 threads
        auto asio_workers = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::asio_scheduler&lt;&gt;(3));

        // we create a composite pool whose only goal is to allow asio worker threads to steal tasks from the threadpool
        m_pools = boost::asynchronous::create_shared_scheduler_proxy(
                    new boost::asynchronous::composite_threadpool_scheduler&lt;&gt; (worker_tp,asio_workers));

        set_worker(worker_tp);
        // we create one asynchronous communication manager in each thread
        m_asio_comm.push_back(AsioCommunicationServantProxy(asio_workers,server,path));
        m_asio_comm.push_back(AsioCommunicationServantProxy(asio_workers,server,path));
        m_asio_comm.push_back(AsioCommunicationServantProxy(asio_workers,server,path));
    }
... //to be continued                 
                </pre><p>We create 3 pools:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>A worker pool for calculations (page comparisons)</p></li><li class="listitem"><p>An asio threadpool with 3 threads in which we create 3
                                communication objects.</p></li><li class="listitem"><p>A composite pool which binds both pools together into one stealing
                                unit. You could even set the worker pool to 0 thread, in which case
                                the worker will get its work done when the asio threads have nothing
                                to do. Only non- multiqueue schedulers support this. The worker pool
                                is now made to be the worker pool of this object using
                                    <code class="code">set_worker()</code>.</p></li></ul></div><p>We then create our communication objects inside the asio pool.</p><p><span class="underline">Note</span>: asio pools can steal from other
                    pools but not be stolen from. Let's move on to the most interesting part:</p><pre class="programlisting">void get_data()
{
    // provide this callback (executing in our thread) to all asio servants as task result. A string will contain the page
    std::function&lt;void(std::string)&gt; f =            
...
    m_asio_comm[0].test(make_safe_callback(f));
    m_asio_comm[1].test(make_safe_callback(f));
    m_asio_comm[2].test(make_safe_callback(f));
}</pre><p>We skip the body of f for the moment. f is a task which will be posted to each
                    communication servant so that they can do the same work:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>call the same http get on an asio servants</p></li><li class="listitem"><p>at each callback, check if we got all three callbacks</p></li><li class="listitem"><p>if yes, post some work to worker threadpool, compare the returned
                                strings (should be all the same)</p></li><li class="listitem"><p>if all strings equal as they should be, cout the page</p></li></ul></div><p>All this will be doine in a single functor. This functor is passed to each
                    communication servant, packed into a make_safe_callback, which, as its name
                    says, transforms the unsafe functor into one which posts this callback functor
                    to the manager thread and also tracks it to check if still alive at the time of
                    the callback. By calling <code class="code">test()</code>, we trigger the 3 communications,
                    and f will be called 3 times. The body of f is:</p><pre class="programlisting">std::function&lt;void(std::string)&gt; f =
                [this](std::string s)
                {
                   this-&gt;m_requested_data.push_back(s);
                   // poor man's state machine saying we got the result of our asio requests :)
                   if (this-&gt;m_requested_data.size() == 3)
                   {
                       // ok, this has really been called for all servants, compare.
                       // but it could be long, so we will post it to threadpool
                       std::cout &lt;&lt; "got all tcp data, parallel check it's correct" &lt;&lt; std::endl;
                       std::string s1 = this-&gt;m_requested_data[0];
                       std::string s2 = this-&gt;m_requested_data[1];
                       std::string s3 = this-&gt;m_requested_data[2];
                       // this callback (executing in our thread) will be called after each comparison
                       auto cb1 = [this,s1](boost::asynchronous::expected&lt;bool&gt; res)
                       {
                          if (res.get())
                              ++this-&gt;m_check_string_count;
                          else
                              std::cout &lt;&lt; "uh oh, the pages do not match, data not confirmed" &lt;&lt; std::endl;
                          if (this-&gt;m_check_string_count ==2)
                          {
                              // we started 2 comparisons, so it was the last one, data confirmed
                              std::cout &lt;&lt; "data has been confirmed, here it is:" &lt;&lt; std::endl;
                              std::cout &lt;&lt; s1;
                          }
                       };
                       auto cb2=cb1;
                       // post 2 string comparison tasks, provide callback where the last step will run
                       this-&gt;post_callback([s1,s2](){return s1 == s2;},std::move(cb1));
                       this-&gt;post_callback([s2,s3](){return s2 == s3;},std::move(cb2));
                   }
                };        
                </pre><p> We start by checking if this is the third time this functor is called (this,
                    the manager, is nicely serving as holder, kind of poor man's state machine
                    counting to 3). If yes, we prepare a call to the worker pool to compare the 3
                    returned strings 2 by 2 (cb1, cb2). Again, simple state machine, if the callback
                    is called twice, we are done comparing string 1 and 2, and 2 and 3, in which
                    case the page is confirmed and cout'ed. The last 2 lines trigger the work and
                    post to our worker pool (which is the threadpool scheduler, or, if stealing
                    happens, the asio pool) two comparison tasks and the callbacks.</p><p>Our manager is now ready, we still need to create for it a proxy so that it
                    can be called from the outside world asynchronously, then create it in its own
                    thread, as usual:</p><pre class="programlisting">class ServantProxy : public boost::asynchronous::servant_proxy&lt;ServantProxy,Servant&gt;
{
public:
    template &lt;class Scheduler&gt;
    ServantProxy(Scheduler s,const std::string&amp; server, const std::string&amp; path):
        boost::asynchronous::servant_proxy&lt;ServantProxy,Servant&gt;(s,server,path)
    {}
    // get_data is posted, no future, no callback
    BOOST_ASYNC_POST_MEMBER(get_data)
};
...              
auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::single_thread_scheduler&lt;
                                     boost::asynchronous::threadsafe_list&lt;&gt; &gt;);
{
   ServantProxy proxy(scheduler,"www.boost.org","/LICENSE_1_0.txt");
   // call member, as if it was from Servant
   proxy.get_data();
   // if too short, no problem, we will simply give up the tcp requests
   // this is simply to simulate a main() doing nothing but waiting for a termination request
   boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
}
                </pre><p> As usual, <a class="link" href="examples/example_asio_http_client.cpp" target="_top">here the
                        complete, ready-to-use example</a> and the implementation of the <a class="link" href="examples/asio/asio_http_async_client.hpp" target="_top">Boost.Asio HTTP
                        client</a>. </p></div><div class="sect1" title="Timers"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e1224"></a>Timers</h2></div></div></div><p>Very often, an Active Object servant acting as an asynchronous dispatcher will
                    post tasks which have to be done until a certain point in the future, or which
                    will start only at a later point. State machines also regularly make use of a
                    "time" event.</p><p>For this we need a timer, but a safe one:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>The timer callback has to be posted to the Active Object thread to
                                avoid races.</p></li><li class="listitem"><p>The timer callback shall not be called if the servant making the
                                request has been deleted (it can be an awfully long time until the
                                callback).</p></li></ul></div><p>Asynchronous itself has no timer, but Boost.Asio does, so the library provides
                    a wrapper around it and will allow us to create a timer using an
                    asio::io_service running in its own thread or in an asio threadpool, provided by
                    the library.</p><div class="sect2" title="Constructing a timer"><div class="titlepage"><div><div><h3 class="title"><a name="d0e1240"></a>Constructing a timer</h3></div></div></div><p>One first needs an <code class="code">asio_scheduler</code> with at least one
                        thread:</p><pre class="programlisting">boost::asynchronous::any_shared_scheduler_proxy&lt;&gt; asio_sched = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::asio_scheduler&lt;&gt;(1));               
                    </pre><p>The Servant living in its ActiveObject thread then creates a timer (as
                        attribute to keep it alive) using this scheduler and a timer value:</p><pre class="programlisting"> boost::asynchronous::asio_deadline_timer_proxy m_timer (asio_sched,boost::posix_time::milliseconds(1000));                   
                    </pre><p>It can now start the timer using <code class="code">trackable_servant</code> (its base
                            class)<code class="code">::async_wait</code>, passing it a functor call when timer
                        expires / is cancelled:</p><pre class="programlisting"> async_wait(m_timer,
            [](const ::boost::system::error_code&amp; err)
            {
                std::cout &lt;&lt; "timer expired? "&lt;&lt; std::boolalpha &lt;&lt; (bool)err &lt;&lt; std::endl; //true if expired, false if cancelled
            } 
            );                  </pre><p>Canceling or recreating the timer means destroying (and possibly
                        recreating) the timer object:</p><pre class="programlisting"> m_timer =  boost::asynchronous::asio_deadline_timer_proxy(get_worker(),boost::posix_time::milliseconds(1000));                                   
                    </pre><p>Alternatively, asio_deadline_timer_proxy offers a reset(duration) member,
                        which is more efficient than recreating a proxy. The <a class="link" href="examples/example_asio_deadline_timer.cpp" target="_top">following example
                        </a> displays a servant using an asio scheduler as a thread pool and
                        creating there its timer object. Note how the timer is created using the
                        worker scheduler of its owner.</p></div></div><div class="sect1" title="Continuation tasks"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e1273"></a><span class="command"><strong><a name="callback_continuations"></a></strong></span>Continuation tasks</h2></div></div></div><p>A common limitation of threadpools is support for recursive tasks: tasks start
                    other tasks, which start other tasks and wait for them to complete to do a merge
                    of the part-results. Unfortunately, all threads in the threadpool will soon be
                    busy waiting and no task will ever complete. One can achieve this with a
                    controller object or state machine in a single-threaded scheduler waiting for
                    callbacks, but for very small tasks, using callbacks might just be too
                    expensive. In such cases, Asynchronous provides continuations: a task executes,
                    does something then creates a continuation which will be excuted as soon as all
                    child tasks complete.</p><div class="sect2" title="General"><div class="titlepage"><div><div><h3 class="title"><a name="d0e1279"></a>General</h3></div></div></div><p>The Hello World of recursive tasks is a parallel fibonacci. The naive
                        algorithm creates a task calculating fib(n). For this it will start a fib(n-1)
                        and fib(n-2) and block until both are done. These tasks will start more tasks,
                        etc. until a cutoff number, at which point recursion stops and fibonacci is
                        calculated serially. This approach has some problems: to avoid thread explosion,
                        we would need fibers, which are not available in Boost at the time of this
                        writing. Even with fibers, tasks would block, which means interrupting them is
                        not possible, and a stack will have to be paid for both. Performance will also
                        suffer. Furthermore, blocking simply isn't part of the asynchronous philosophy
                        of the library. Let's have a look how callback continuation tasks let us
                        implement a parallel fibonacci.</p><p>First of all, we need a serial fibonacci when n is less than the cutoff. This
                        is a classical one:</p><pre class="programlisting"> long serial_fib( long n ) {
    if( n&lt;2 )
        return n;
    else
        return serial_fib(n-1)+serial_fib(n-2);
}</pre><p> We now need a recursive-looking fibonacci task: </p><pre class="programlisting">// our recursive fibonacci tasks. Needs to inherit continuation_task&lt;value type returned by this task&gt;
struct fib_task : public <span class="bold"><strong>boost::asynchronous::continuation_task&lt;long&gt;</strong></span>
{
    fib_task(long n,long cutoff):n_(n),cutoff_(cutoff){}
    // called inside of threadpool
    void operator()()const
    {
        // the result of this task, will be either set directly if &lt; cutoff, otherwise when taks is ready
        boost::asynchronous::<span class="bold"><strong>continuation_result&lt;long&gt; task_res = this_task_result()</strong></span>;
        if (n_&lt;cutoff_)
        {
            // n &lt; cutoff =&gt; execute immediately
            task_res.set_value(serial_fib(n_));
        }
        else
        {
            // n&gt;= cutoff, create 2 new tasks and when both are done, set our result (res(task1) + res(task2))
            boost::asynchronous::<span class="bold"><strong>create_callback_continuation</strong></span>(
                        // called when subtasks are done, set result of the calling task
                        [task_res](std::tuple&lt;boost::asynchronous::expected&lt;long&gt;,boost::asynchronous::expected&lt;long&gt; &gt; res) mutable
                        {
                            long r = std::get&lt;0&gt;(res).get() + std::get&lt;1&gt;(res).get();
                            task_res.set_value(r);
                        },
                        // recursive tasks
                        fib_task(n_-1,cutoff_),
                        fib_task(n_-2,cutoff_));
        }
    }
    long n_;
    long cutoff_;
};             </pre><p> Our task need to inherit
                        <code class="code">boost::asynchronous::continuation_task&lt;R&gt;</code> where R is the
                        returned type. This class provides us with <code class="code">this_task_result()</code> where
                        we set the task result. This is done either immediately if n &lt; cutoff (first
                        if clause), or (else clause) using a continuation.</p><p>If n&gt;= cutoff, we create a continuation. This is a sleeping task, which will
                        get activated when all required tasks complete. In this case, we have two
                        fibonacci sub tasks. The template argument is the return type of the
                        continuation. We create two sub-tasks, for n-1 and n-2 and when they complete,
                        the completion functor passed as first argument is called.</p><p>Note that <code class="code">boost::asynchronous::create_continuation</code> is a variadic
                        function, there can be any number of sub-tasks. The completion functor takes as
                        single argument a tuple of <code class="code">expected</code>, one for each subtask. The
                        template argument of the future is the template argument of
                        <code class="code">boost::asynchronous::continuation_task</code> of each subtask. In this
                        case, all are of type long, but it's not a requirement.</p><p>When this completion functor is called, we set our result to be result of
                        first task + result of second task. </p><p>The main particularity of this solution is that a task does not block until
                        sub-tasks complete but instead provides a functor to be called asynchronously as
                        soon as subtasks complete.</p><p>All what we still need to do is create the first task. In the tradition of
                        Asynchronous, we show it inside an asynchronous servant which posts the first
                        task and waits for a callback, but the same is of course possible using
                        <code class="code">post_future</code>:</p><pre class="programlisting">struct Servant : boost::asynchronous::trackable_servant&lt;&gt;
{
...
   void calc_fibonacci(long n,long cutoff)
   {
      post_callback(
            // work
            [n,cutoff]()
            {
                // a top-level continuation is the first one in a recursive serie.
                // Its result will be passed to callback
                <span class="bold"><strong>return</strong></span> boost::asynchronous::<span class="bold"><strong>top_level_callback_continuation&lt;long&gt;</strong></span>(fib_task(n,cutoff));
            },
            // callback with fibonacci result.
            [](boost::asynchronous::expected&lt;long&gt; res){...}// callback functor.
        );                                 
   }  
};          </pre><p> We call <code class="code">post_callback</code>, which, as usual, ensures that the
                        callback is posted to the right thread and the servant lifetime is tracked.
                        The posted task calls
                            <code class="code">boost::asynchronous::top_level_callback_continuation&lt;task-return-type&gt;</code>
                        to create the first, top-level continuation, passing it a first fib_task.
                        This is non-blocking, a special version of <code class="code">post_callback</code>
                        recognizes a continuation and will call its callback (with a
                            <code class="code">expected&lt;task-return-type&gt;</code>) only when the calculation is
                        finished, not when the "work" lambda returns. For this to work, <span class="bold"><strong>it is essential not to forget the return
                            statement</strong></span>. Without it, the compiler will unhappily remark
                        that an <code class="code">expected&lt;void&gt;</code> cannot be casted to an
                            <code class="code">expected&lt;long&gt;</code>, or worse if one expects an
                            <code class="code">expected&lt;void&gt;</code>, the callback would be called to
                        early.</p><p>As usual, calling get() on the expected is non-blocking, one gets either the
                        result or an exception if thrown by a task.</p><p>Please have a look at the <a class="link" href="examples/example_fibonacci.cpp" target="_top">complete example</a>.</p></div><div class="sect2" title="Logging"><div class="titlepage"><div><div><h3 class="title"><a name="d0e1372"></a>Logging</h3></div></div></div><p>What about logging? We don't want to give up this feature of course and
                        would like to know how long all these fib_task took to complete. This is
                        done through minor changes. As always we need a job:</p><pre class="programlisting">typedef boost::asynchronous::any_loggable&lt;std::chrono::high_resolution_clock&gt; servant_job;                                                 </pre><p> We give the logged name of the task in the constructor of fib_task, for
                        example fib_task_xxx:</p><pre class="programlisting">fib_task(long n,long cutoff)
        : boost::asynchronous::continuation_task&lt;long&gt;("fib_task_" + boost::lexical_cast&lt;std::string&gt;(n))
        ,n_(n),cutoff_(cutoff){}                                                </pre><p>And call <code class="code">boost::asynchronous::create_continuation_job</code> instead of
                        <code class="code">boost::asynchronous::create_continuation</code>:</p><pre class="programlisting">boost::asynchronous::<span class="bold"><strong>create_callback_continuation_job</strong></span>&lt;servant_job&gt;(
                        [task_res](std::tuple&lt;boost::asynchronous::expected&lt;long&gt;,boost::asynchronous::expected&lt;long&gt; &gt; res)
                        {
                            long r = std::get&lt;0&gt;(res).get() + std::get&lt;1&gt;(res).get();
                            task_res.set_value(r);
                        },
                        fib_task(n_-1,cutoff_),
                        fib_task(n_-2,cutoff_)
);                                              </pre><p> Inside the servant we might optionally want the version of post_callback with
                        name, and we need to use <code class="code">top_level_continuation_job</code> instead of
                        <code class="code">top_level_continuation</code>:</p><pre class="programlisting">post_callback(
              [n,cutoff]()
              {
                   return boost::asynchronous::<span class="bold"><strong>top_level_callback_continuation_job</strong></span>&lt;long,servant_job&gt;(fib_task(n,cutoff));
              },// work
              // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
              [this](boost::asynchronous::expected&lt;long&gt; res){...},// callback functor.
              <span class="bold"><strong>"calc_fibonacci"</strong></span>
        );                             
                </pre><p> The previous example has been <a class="link" href="examples/example_fibonacci_log.cpp" target="_top">rewritten with logs and a
                        display of all tasks</a> (beware, with higher fibonacci numbers, this can
                        become a long list).</p><p><span class="underline">Limitation</span>: in the current
                        implementation, tasks are logged, but the continuation callback is not. If it
                        might take long, one should post a (loggable) task.</p><p><span class="underline">Note</span>: to improve performance, the last
                        task passed to <span class="bold"><strong>create_callback_continuation(_job)</strong></span> is not posted but executed
                        directly so it will execute under the name of the task calling <span class="bold"><strong>create_callback_continuation(_job)</strong></span>.</p><p><span class="bold"><strong><span class="underline">Important note about
                        exception safety</span></strong></span>. The passed <span class="bold"><strong>expected</strong></span> contains either a result or an exception. Calling get()
                        will throw contained exceptions. You should catch it, in the continuation
                        callback and in the task itself. Asynchronous will handle the exception, but it
                        cannot set the <span class="bold"><strong>continuation_result</strong></span>, which will
                        never be set and the callback part of post_callback never called. This simple
                        example does not throw, so we save ourselves the cost, but more complicated
                        algorithms should take care of this.</p></div><div class="sect2" title="Creating a variable number of tasks for a continuation"><div class="titlepage"><div><div><h3 class="title"><a name="d0e1442"></a>Creating a variable number of tasks for a continuation</h3></div></div></div><p>It is sometimes not possible to know at compile-time the number of tasks
                        or even the types of tasks used in the creation of a continuation. In this
                        cases, Asynchronous provides more possibilities: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>Pack all subtasks of a same type into a std::vector, then pass
                                    it to <code class="code">create_callback_continuation or
                                        create_callback_continuation_job</code>. In this case, we
                                    know that these subtasks all have the same type, so our
                                    continuation is called with a
                                        <code class="code">vector&lt;expected&lt;return_type&gt;&gt;</code>:</p><pre class="programlisting">struct sub_task : public boost::asynchronous::continuation_task&lt;long&gt;
{
    // some task with long as result type
};
struct main_task : public boost::asynchronous::continuation_task&lt;long&gt;
{
  void operator()()
  {
     boost::asynchronous::continuation_result&lt;long&gt; task_res = this_task_result();
     <span class="bold"><strong>std::vector&lt;sub_task&gt;</strong></span> subs;
     subs.push_back(sub_task());
     subs.push_back(sub_task());
     subs.push_back(sub_task()); 
                                         
     boost::asynchronous::<span class="bold"><strong>create_callback_continuation</strong></span>(
          [task_res](<span class="bold"><strong>std::vector&lt;boost::asynchronous::expected&lt;long&gt;&gt;</strong></span> res)
          {
             long r = res[0].get() + res[1].get() + res[2].get();
             task_res.set_value(r);
          },
          <span class="bold"><strong>std::move(subs)</strong></span>);
   }
};</pre></li><li class="listitem"><p>If the subtasks have different type, but a common result type,
                                    we can pack them into a
                                    <code class="code">std::vector&lt;boost::asynchronous::any_continuation_task&lt;return_type&gt;&gt;</code>
                                    instead, the rest of the code staying the same:</p><pre class="programlisting"><span class="bold"><strong>#include &lt;boost/asynchronous/any_continuation_task.hpp&gt;</strong></span>

struct sub_task : public boost::asynchronous::continuation_task&lt;long&gt;
{
    // some task with long as result type
};
struct main_task2 : public boost::asynchronous::continuation_task&lt;long&gt;
{
    void operator()()
    {
        boost::asynchronous::continuation_result&lt;long&gt; task_res = this_task_result();
        <span class="bold"><strong>std::vector&lt;boost::asynchronous::any_continuation_task&lt;long&gt;&gt;</strong></span> subs;
        subs.push_back(sub_task());
        subs.push_back(sub_task2());
        subs.push_back(sub_task3());

        boost::asynchronous::<span class="bold"><strong>create_callback_continuation</strong></span>(
             [task_res](<span class="bold"><strong>std::vector&lt;boost::asynchronous::expected&lt;long&gt;&gt;</strong></span> res)
             {
                 long r = res[0].get() + res[1].get() + res[2].get();
                  task_res.set_value(r);
             },
             <span class="bold"><strong>std::move(subs)</strong></span>);
    }
};</pre></li><li class="listitem"><p>Of course, if we have continuations in the first place,
                                    returned by
                                        <code class="code">top_level_callback_continuation&lt;task-return-type&gt;</code>
                                    or
                                        <code class="code">top_level_callback_continuation&lt;task-return-type&gt;</code>,
                                    as all of Asynchronous' algorithms do, these can be packed into
                                    a vector as well:</p><pre class="programlisting">struct main_task3 : public boost::asynchronous::continuation_task&lt;long&gt;
{
    void operator()()
    {
        boost::asynchronous::continuation_result&lt;long&gt; task_res = this_task_result();
        <span class="bold"><strong>std::vector&lt;boost::asynchronous::detail::callback_continuation&lt;long&gt;&gt;</strong></span> subs;
        std::vector&lt;long&gt; data1(10000,1);
        std::vector&lt;long&gt; data2(10000,1);
        std::vector&lt;long&gt; data3(10000,1);
        subs.<span class="bold"><strong>push_back</strong></span>(boost::asynchronous::<span class="bold"><strong>parallel_reduce</strong></span>(std::move(data1),
                                                            [](long const&amp; a, long const&amp; b)
                                                            {
                                                              return a + b;
                                                            },1000));
        subs.<span class="bold"><strong>push_back</strong></span>(boost::asynchronous::<span class="bold"><strong>parallel_reduce</strong></span>(std::move(data2),
                                                            [](long const&amp; a, long const&amp; b)
                                                            {
                                                              return a + b;
                                                            },1000));
        subs.<span class="bold"><strong>push_back</strong></span>(boost::asynchronous::<span class="bold"><strong>parallel_reduce</strong></span>(std::move(data3),
                                                            [](long const&amp; a, long const&amp; b)
                                                            {
                                                              return a + b;
                                                            },1000));

        boost::asynchronous::<span class="bold"><strong>create_callback_continuation</strong></span>(
                        [task_res](<span class="bold"><strong>std::vector&lt;boost::asynchronous::expected&lt;long&gt;&gt;</strong></span> res)
                        {
                            long r = res[0].get() + res[1].get() + res[2].get();
                            task_res.set_value(r);
                        },
                        <span class="bold"><strong>std::move(subs)</strong></span>);
    }
};</pre></li></ul></div></div><div class="sect2" title="Creating a continuation from a simple functor"><div class="titlepage"><div><div><h3 class="title"><a name="d0e1534"></a>Creating a continuation from a simple functor</h3></div></div></div><p>For very simple tasks, it is in a post C++11 world annoying to have to
                        write a functor class like our above sub_task. For such cases, Asynchronous
                        provides a simple helper function:</p><p><code class="code">auto make_lambda_continuation_wrapper(functor f, std::string
                            const&amp; name="")</code> where auto will be a
                            <code class="code">continuation_task</code>. We can replace our first case above by a
                        more concise:</p><pre class="programlisting">struct main_task4 : public boost::asynchronous::continuation_task&lt;int&gt;
{
    void operator()()
    {
        // 15, 22,5 are of type int
        boost::asynchronous::continuation_result&lt;<span class="bold"><strong>int</strong></span>&gt; task_res = this_task_result();
        <span class="bold"><strong>std::vector&lt;boost::asynchronous::any_continuation_task&lt;int&gt;&gt;</strong></span> subs;
        subs.push_back(boost::asynchronous::<span class="bold"><strong>make_lambda_continuation_wrapper</strong></span>([](){return 15;}));
        subs.push_back(boost::asynchronous::<span class="bold"><strong>make_lambda_continuation_wrapper</strong></span>([](){return 22;}));
        subs.push_back(boost::asynchronous::<span class="bold"><strong>make_lambda_continuation_wrapper</strong></span>([](){return 5;}));

        boost::asynchronous::create_callback_continuation(
                        [task_res](std::vector&lt;boost::asynchronous::expected&lt;int&gt;&gt; res)
                        {
                            int r = res[0].get() + res[1].get() + res[2].get();
                            task_res.set_value(r);
                        },
                        <span class="bold"><strong>std::move(subs)</strong></span>);
    }
};</pre></div></div><div class="sect1" title="Future-based continuations"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e1566"></a><span class="command"><strong><a name="continuations"></a></strong></span>Future-based continuations</h2></div></div></div><p>The continuations shown above are the fastest offered by Asynchronous.
                    Sometimes, however, we are forced to use libraries returning us only a future.
                    In this case, Asynchronous also offers "simple" continuations, which are
                    future-based. Consider the following trivial example. We consider we have a
                    task, called sub_task. We will simulate the future-returning library using
                        <code class="code">post_future</code>. We want to divide our work between sub_task
                    instances, getting a callback when all complete. We can create a continuation
                    using these futures:</p><pre class="programlisting">// our main algo task. Needs to inherit continuation_task&lt;value type returned by this task&gt;
struct main_task : public boost::asynchronous::continuation_task&lt;long&gt;
{
    void operator()()const
    {
        // the result of this task
       <span class="bold"><strong> boost::asynchronous::continuation_result&lt;long&gt; task_res = this_task_result();</strong></span>

        // we start calculation, then while doing this we see new tasks which can be posted and done concurrently to us
        // when all are done, we will set the result
        // to post tasks, we need a scheduler
        boost::asynchronous::any_weak_scheduler&lt;&gt; weak_scheduler = boost::asynchronous::get_thread_scheduler&lt;&gt;();
        boost::asynchronous::any_shared_scheduler&lt;&gt; locked_scheduler = weak_scheduler.lock();
        if (!locked_scheduler.is_valid())
            // ok, we are shutting down, ok give up
            return;
        // simulate algo work
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        // let's say we just found a subtask
        std::future&lt;int&gt; fu1 = boost::asynchronous::post_future(locked_scheduler,sub_task());
        // simulate more algo work
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        // let's say we just found a subtask
        std::future&lt;int&gt; fu2 = boost::asynchronous::post_future(locked_scheduler,sub_task());
        // simulate algo work
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        // let's say we just found a subtask
        std::future&lt;int&gt; fu3 = boost::asynchronous::post_future(locked_scheduler,sub_task());

        // our algo is now done, wrap all and return
        boost::asynchronous::<span class="bold"><strong>create_continuation</strong></span>(
                    // called when subtasks are done, set our result
                    [task_res](std::tuple&lt;std::future&lt;int&gt;,std::future&lt;int&gt;,std::future&lt;int&gt; &gt; res)
                    {
                        try
                        {
                            long r = std::get&lt;0&gt;(res).get() + std::get&lt;1&gt;(res).get()+ std::get&lt;2&gt;(res).get();
                            <span class="bold"><strong>task_res.set_value(r);</strong></span>
                        }
                        catch(...)
                        {
                            <span class="bold"><strong>task_res.set_exception(std::current_exception());</strong></span>
                        }
                    },
                    // future results of recursive tasks
                    <span class="bold"><strong>std::move(fu1),std::move(fu2),std::move(fu3)</strong></span>);
    }
    };                                               </pre><p>Please have a look at <a class="link" href="examples/example_continuation_algo.cpp" target="_top">the complete
                    example</a></p><p>Our tasks starts by posting 3 instances of sub_task, each time getting a
                    future. We then call <span class="bold"><strong>create_continuation(_job)</strong></span>,
                    passing it the futures. When all futures are ready (have a value or an
                    exception), the callback is called, with 3 futures containing the result.</p><p>Advantage:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>can be used with any library returning a std::future</p></li></ul></div><p>Drawbacks:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>lesser performance</p></li><li class="listitem"><p>the thread calling <span class="bold"><strong>create_continuation(_job)</strong></span> polls until all futures
                                are set. If this thread is busy, the callback is delayed.</p></li></ul></div><p><span class="bold"><strong><span class="underline">Important
                        note</span></strong></span>: Like for the previous callback continuations,
                    tasks and continuation callbacks should catch exceptions.</p><p><span class="bold"><strong>create_continuation(_job)</strong></span> has a wider
                    interface. It can also take a vector of futures instead of a variadic
                    version, for example:</p><pre class="programlisting">// our main algo task. Needs to inherit continuation_task&lt;value type returned by this task&gt;
struct main_task : public boost::asynchronous::continuation_task&lt;long&gt;
{
    void operator()()const
    {
        // the result of this task
        boost::asynchronous::continuation_result&lt;long&gt; task_res = this_task_result();

        // we start calculation, then while doing this we see new tasks which can be posted and done concurrently to us
        // when all are done, we will set the result
        // to post tasks, we need a scheduler
        boost::asynchronous::any_weak_scheduler&lt;&gt; weak_scheduler = boost::asynchronous::get_thread_scheduler&lt;&gt;();
        boost::asynchronous::any_shared_scheduler&lt;&gt; locked_scheduler = weak_scheduler.lock();
        if (!locked_scheduler.is_valid())
            // ok, we are shutting down, ok give up
            return;
        // simulate algo work
        std::vector&lt;std::future&lt;int&gt; &gt; fus;
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        // let's say we just found a subtask
        std::future&lt;int&gt; fu1 = boost::asynchronous::post_future(locked_scheduler,sub_task());
        fus.emplace_back(std::move(fu1));
        // simulate more algo work
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        // let's say we just found a subtask
        std::future&lt;int&gt; fu2 = boost::asynchronous::post_future(locked_scheduler,sub_task());
        fus.emplace_back(std::move(fu2));
        // simulate algo work
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        // let's say we just found a subtask
        std::future&lt;int&gt; fu3 = boost::asynchronous::post_future(locked_scheduler,sub_task());
        fus.emplace_back(std::move(fu3));

        // our algo is now done, wrap all and return
        boost::asynchronous::<span class="bold"><strong>create_continuation</strong></span>(
                    // called when subtasks are done, set our result
                    [task_res](std::vector&lt;std::future&lt;int&gt;&gt; res)
                    {
                        try
                        {
                            long r = res[0].get() + res[1].get() + res[2].get();
                            task_res.set_value(r);
                        }
                        catch(...)
                        {
                            task_res.set_exception(std::current_exception());
                        }
                    },
                    // future results of recursive tasks
                    <span class="bold"><strong>std::move(fus)</strong></span>);
    }
    };                                            </pre><p>The drawback is that in this case, all futures must be of the same type.
                    Please have a look at <a class="link" href="examples/example_continuation_algo2.cpp" target="_top">the complete example</a></p></div><div class="sect1" title="Distributing work among machines"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e1640"></a><span class="command"><strong><a name="distributing"></a></strong></span>Distributing work among machines</h2></div></div></div><p>At the time of this writing, a core i7-3930K with 6 cores and 3.2 GHz will
                    cost $560, so say $100 per core. Not a bad deal, so you buy it. Unfortunately,
                    some time later you realize you need more power. Ok, there is no i7 with more
                    cores and an Extreme Edition will be quite expensive for only a little more
                    power so you decide to go for a Xeon. A 12-core E5-2697v2 2.7GHz will go for
                    almost $3000 which means $250 per core, and for this you also have a lesser
                    frequency. And if you need later even more power, well, it will become really
                    expensive. Can Asynchronous help us use more power for cheap, and at best, with
                    little work? It does, as you guess ;-)</p><p>Asynchronous provides a special pool, <code class="code">tcp_server_scheduler</code>, which
                    will behave like any other scheduler but will not execute work itself, waiting
                    instead for clients to connect and steal some work. The client execute the work
                    on behalf of the <code class="code">tcp_server_scheduler</code> and sends it back the
                    results. </p><p>For this to work, there is however a condition: jobs must be (boost)
                    serializable to be transferred to the client. So does the returned value.</p><p>Let's start with a <a class="link" href="examples/example_tcp_server.cpp" target="_top">simplest
                        example</a>:</p><pre class="programlisting">// notice how the worker pool has a different job type
struct Servant : boost::asynchronous::trackable_servant&lt;boost::asynchronous::any_callable,<span class="bold"><strong>boost::asynchronous::any_serializable</strong></span>&gt;
{
  Servant(boost::asynchronous::any_weak_scheduler&lt;&gt; scheduler)
        : boost::asynchronous::trackable_servant&lt;boost::asynchronous::any_callable,<span class="bold"><strong>boost::asynchronous::any_serializable</strong></span>&gt;(scheduler)
  {
        // let's build our pool step by step. First we need a worker pool
        // possibly for us, and we want to share it with the tcp pool for its serialization work
        boost::asynchronous::any_shared_scheduler_proxy&lt;&gt; workers = boost::asynchronous::make_shared_scheduler_proxy&lt;
                                                                            boost::asynchronous::threadpool_scheduler&lt;boost::asynchronous::lockfree_queue&lt;&gt;&gt;&gt;(3);

        // we use a tcp pool using the 3 worker threads we just built
        // our server will listen on "localhost" port 12345
        auto pool= boost::asynchronous::make_shared_scheduler_proxy&lt;
                    boost::asynchronous::tcp_server_scheduler&lt;
                            boost::asynchronous::lockfree_queue&lt;boost::asynchronous::any_serializable&gt;&gt;&gt;
                                (workers,"localhost",12345);
        // and this will be the worker pool for post_callback
        set_worker(pool);
  }
};</pre><p>We start by creating a worker pool. The <code class="code">tcp_server_scheduler</code> will
                    delegate to this pool all its serialization / deserialization work. For maximum
                    scalability we want this work to happen in more than one thread.</p><p>Note that our job type is no more a simple callable, it must be
                    (de)serializable too (<span class="bold"><strong>boost::asynchronous::any_serializable</strong></span>).</p><p>Then we need a <code class="code">tcp_server_scheduler</code> listening on, in this case,
                    localhost, port 12345. We now have a functioning worker pool and choose to use
                    it as our worker pool so that we do not execute jobs ourselves (other
                    configurations will be shown soon). Let's exercise our new pool. We first need a
                    task to be executed remotely:</p><pre class="programlisting">struct dummy_tcp_task : public boost::asynchronous::<span class="bold"><strong>serializable_task</strong></span>
{
    dummy_tcp_task(int d):boost::asynchronous::<span class="bold"><strong>serializable_task</strong></span>(<span class="bold"><strong>"dummy_tcp_task"</strong></span>),m_data(d){}
    template &lt;class Archive&gt;
    void <span class="bold"><strong>serialize</strong></span>(Archive &amp; ar, const unsigned int /*version*/)
    {
        ar &amp; m_data;
    }
    int operator()()const
    {
        std::cout &lt;&lt; "dummy_tcp_task operator(): " &lt;&lt; m_data &lt;&lt; std::endl;
        boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
        std::cout &lt;&lt; "dummy_tcp_task operator() finished" &lt;&lt; std::endl;
        return m_data;
    }
    int m_data;
};</pre><p>This is a minimum task, only sleeping. All it needs is a
                        <code class="code">serialize</code> member to play nice with Boost.Serialization and it
                    must inherit <code class="code">serializable_task</code>. Giving the task a name is essential
                    as it will allow the client to deserialize it. Let's post to our TCP worker pool
                    some of the tasks, wait for a client to pick them and use the results:</p><pre class="programlisting">// start long tasks in threadpool (first lambda) and callback in our thread
for (int i =0 ;i &lt; 10 ; ++i)
{
    std::cout &lt;&lt; "call post_callback with i: " &lt;&lt; i &lt;&lt; std::endl;
    post_callback(
           dummy_tcp_task(i),
           // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
           [this](boost::asynchronous::expected&lt;int&gt; res){
                  try{
                        this-&gt;on_callback(res.get());
                  }
                  catch(std::exception&amp; e)
                  {
                       std::cout &lt;&lt; "got exception: " &lt;&lt; e.what() &lt;&lt; std::endl;
                       this-&gt;on_callback(0);
                  }
            }// callback functor.
    );
}</pre><p>We post 10 tasks to the pool. For each task we will get, at some later
                    undefined point (provided some clients are around), a result in form of a
                    (ready) expected, possibly an exception if one was thrown by the task.</p><p>Notice it is safe to use <code class="code">this</code> in the callback lambda as it will
                    be only called if the servant still exists.</p><p>We still need a client to execute the task, this is pretty straightforward (we
                    will extend it soon):</p><pre class="programlisting">int main(int argc, char* argv[])
{
    std::string server_address = (argc&gt;1) ? argv[1]:"localhost";
    std::string server_port = (argc&gt;2) ? argv[2]:"12346";
    int threads = (argc&gt;3) ? strtol(argv[3],0,0) : 4;
    cout &lt;&lt; "Starting connecting to " &lt;&lt; server_address &lt;&lt; " port " &lt;&lt; server_port &lt;&lt; " with " &lt;&lt; threads &lt;&lt; " threads" &lt;&lt; endl;

    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy&lt;boost::asynchronous::asio_scheduler&lt;&gt;&gt;()
    {
        std::function&lt;void(std::string const&amp;,boost::asynchronous::tcp::server_reponse,std::function&lt;void(boost::asynchronous::tcp::client_request const&amp;)&gt;)&gt; 
        executor=
        [](std::string const&amp; task_name,boost::asynchronous::tcp::server_reponse resp,
           std::function&lt;void(boost::asynchronous::tcp::client_request const&amp;)&gt; when_done)
        {
            if (task_name=="dummy_tcp_task")
            {
                dummy_tcp_task t(0);
                boost::asynchronous::tcp::<span class="bold"><strong>deserialize_and_call_task</strong></span>(t,resp,when_done);
            }
            else
            {
                std::cout &lt;&lt; "unknown task! Sorry, don't know: " &lt;&lt; task_name &lt;&lt; std::endl;
                throw boost::asynchronous::tcp::transport_exception("unknown task");
            }
        };

        auto pool = boost::asynchronous::make_shared_scheduler_proxy&lt;
                          boost::asynchronous::threadpool_scheduler&lt;
                            boost::asynchronous::lockfree_queue&lt;boost::asynchronous::any_serializable&gt;&gt;&gt;(threads);
        boost::asynchronous::tcp::<span class="bold"><strong>simple_tcp_client_proxy proxy</strong></span>(scheduler,pool,server_address,server_port,executor,
                                                                    0/*ms between calls to server*/);
        std::future&lt;std::future&lt;void&gt; &gt; fu = proxy.run();
        std::future&lt;void&gt; fu_end = fu.get();
        fu_end.get();
    }
    return 0;
}</pre><p>We start by taking as command-line arguments the server address and port and
                    the number of threads the client will use to process stolen work from the
                    server. </p><p>We create a single-threaded <code class="code">asio_scheduler</code> for the communication
                    (in our case, this is sufficient, your case might vary) to the server.</p><p>The client then defines an executor function. This function will be called
                    when work is stolen by the client. As Asynchronous does not know what the work
                    type is, we will need to "help" by creating an instance of the task using its
                    name. Calling <code class="code">deserialize_and_call_task</code> will, well, deserialize the
                    task data into our dummy task, then call it. We also choose to return an
                    exception is the task is not known to us.</p><p>Next, we need a pool of threads to execute the work. Usually, you will want
                    more than one thread as we want to use all our cores.</p><p>The simplest client that Asynchronous offers is a
                        <code class="code">simple_tcp_client_proxy</code> proxy. We say simple, because it is
                    only a client. Later on, we will see a more powerful tool.
                        <code class="code">simple_tcp_client_proxy</code> will require the asio pool for
                    communication, the server address and port, our executor and a parameter telling
                    it how often it should try to steal work from a server.</p><p>We are now done, the client will run until killed.</p><p>Let's sum up what we got in these few lines of code:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>a pool behaving like any other pool, which can be stolen
                                from</p></li><li class="listitem"><p>a server which does no work itself, but still scales well as
                                serialization is using whatever threads it is given</p></li><li class="listitem"><p>a trackable servant working with <code class="code">post_callback</code>, like
                                always</p></li><li class="listitem"><p>a multithreaded client, which can be tuned precisely to use a
                                given pool for the communication and another (or the same btw.) for
                                work processing.</p></li></ul></div><p>Interestingly, we have a very versatile client. It is possible to reuse the
                    work processing and communication pools, within the same client application, for
                    a different <code class="code">simple_tcp_client_proxy</code> which would be connecting to another
                    server.</p><p>The server is also quite flexible. It scales well and can handle as many
                    clients as one wishes.</p><p>This is only the beginning of our distributed chapter.</p><div class="sect2" title="A distributed, parallel Fibonacci"><div class="titlepage"><div><div><h3 class="title"><a name="d0e1776"></a>A distributed, parallel Fibonacci</h3></div></div></div><p>Lets's revisit our parallel Fibonacci example. We realize that with higher
                        Fibonacci numbers, our CPU power doesn't suffice any more. We want to
                        distribute it among several machines while our main machine still does some
                        calculation work. To do this, we'll start with our previous example, and
                        rewrite our Fibonacci task to make it distributable.</p><p>We remember that we first had to call
                            <code class="code">boost::asynchronous::top_level_continuation</code> in our
                        post_callback to make Asynchronous aware of the later return value. The
                        difference now is that even this one-liner lambda could be serialized and
                        sent away, so we need to make it a <code class="code">serializable_task</code>:</p><pre class="programlisting">struct serializable_fib_task : public boost::asynchronous::<span class="bold"><strong>serializable_task</strong></span>
{
    serializable_fib_task(long n,long cutoff):boost::asynchronous::<span class="bold"><strong>serializable_task("serializable_fib_task")</strong></span>,n_(n),cutoff_(cutoff){}
    template &lt;class Archive&gt;
    <span class="bold"><strong>void serialize(Archive &amp; ar, const unsigned int /*version*/)</strong></span>
    {
        ar &amp; n_;
        ar &amp; cutoff_;
    }
    auto operator()()const
        -&gt; decltype(boost::asynchronous::top_level_continuation_log&lt;long,boost::asynchronous::any_serializable&gt;
                    (tcp_example::fib_task(long(0),long(0))))
    {
        auto cont =  boost::asynchronous::top_level_continuation_job&lt;long,boost::asynchronous::any_serializable&gt;
                (tcp_example::fib_task(n_,cutoff_));
        return cont;
    }
    long n_;
    long cutoff_;
};</pre><p>We need to make our task serializable and give it a name so that the client
                        application can recognize it. We also need a serialize member, as required
                        by Boost.Serialization. And we need an operator() so that the task can be
                        executed. There is in C++11 an ugly decltype, but C++14 will solve this if
                        your compiler supports it. We also need a few changes in our Fibonacci
                        task:</p><pre class="programlisting">// our recursive fibonacci tasks. Needs to inherit continuation_task&lt;value type returned by this task&gt;
struct fib_task : public boost::asynchronous::continuation_task&lt;long&gt;
                <span class="bold"><strong>, public boost::asynchronous::serializable_task</strong></span>
{
    fib_task(long n,long cutoff)
        :  boost::asynchronous::continuation_task&lt;long&gt;()
        <span class="bold"><strong>, boost::asynchronous::serializable_task("serializable_sub_fib_task")</strong></span>
        ,n_(n),cutoff_(cutoff)
    {
    }
    <span class="bold"><strong>template &lt;class Archive&gt;
    void save(Archive &amp; ar, const unsigned int /*version*/)const
    {
        ar &amp; n_;
        ar &amp; cutoff_;
    }
    template &lt;class Archive&gt;
    void load(Archive &amp; ar, const unsigned int /*version*/)
    {
        ar &amp; n_;
        ar &amp; cutoff_;
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()</strong></span>
    void operator()()const
    {
        // the result of this task, will be either set directly if &lt; cutoff, otherwise when taks is ready
        boost::asynchronous::continuation_result&lt;long&gt; task_res = this_task_result();
        if (n_&lt;cutoff_)
        {
            // n &lt; cutoff =&gt; execute ourselves
            task_res.set_value(serial_fib(n_));
        }
        else
        {
            // n&gt;= cutoff, create 2 new tasks and when both are done, set our result (res(task1) + res(task2))
            boost::asynchronous::create_callback_continuation_job&lt;boost::asynchronous::any_serializable&gt;(
                        // called when subtasks are done, set our result
                        [task_res](std::tuple&lt;std::future&lt;long&gt;,std::future&lt;long&gt; &gt; res)
                        {
                            long r = std::get&lt;0&gt;(res).get() + std::get&lt;1&gt;(res).get();
                            task_res.set_value(r);
                        },
                        // recursive tasks
                        fib_task(n_-1,cutoff_),
                        fib_task(n_-2,cutoff_));
        }
    }
    long n_;
    long cutoff_;
};</pre><p>The few changes are highlighted. The task needs to be a serializable task with
                        its own name in the constructor, and it needs serialization members. That's
                        it, we're ready to distribute!</p><p>As we previously said, we will reuse our previous TCP example, using
                            <code class="code">serializable_fib_task</code> as the main posted task. This gives
                        us <a class="link" href="examples/example_tcp_server_fib.cpp" target="_top">this example</a>.</p><p>But wait, we promised that our server would itself do some calculation
                        work, and we use as worker pool only a <code class="code">tcp_server_scheduler</code>.
                        Right, let's do it now, throwing in a few more goodies. We need a worker
                        pool, with as many threads as we are willing to offer:</p><pre class="programlisting">// we need a pool where the tasks execute
auto <span class="bold"><strong>pool</strong></span> = boost::asynchronous::create_shared_scheduler_proxy(
                    new boost::asynchronous::<span class="bold"><strong>threadpool_scheduler</strong></span>&lt;
                    boost::asynchronous::lockfree_queue&lt;boost::asynchronous::any_serializable&gt; &gt;(<span class="bold"><strong>threads</strong></span>));</pre><p>This pool will get the fibonacci top-level task we will post, then, if our
                        clients connect after we start, it will get the first sub-tasks. </p><p>To make it more interesting, let's offer our server to also be a job
                        client. This way, we can build a cooperation network: the server offers
                        fibonacci tasks, but also tries to steal some, thus increasing homogenous
                        work distribution. We'll talk more about this in the next chapter.</p><pre class="programlisting">// a client will steal jobs in this pool
auto cscheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::<span class="bold"><strong>asio_scheduler</strong></span>&lt;&gt;);
// jobs we will support
std::function&lt;void(std::string const&amp;,boost::asynchronous::tcp::server_reponse,
                   std::function&lt;void(boost::asynchronous::tcp::client_request const&amp;)&gt;)&gt; executor=
        [](std::string const&amp; task_name,boost::asynchronous::tcp::server_reponse resp,
           std::function&lt;void(boost::asynchronous::tcp::client_request const&amp;)&gt; when_done)
        {
            if (task_name=="serializable_sub_fib_task")
            {
                tcp_example::fib_task fib(0,0);
                boost::asynchronous::tcp::<span class="bold"><strong>deserialize_and_call_callback_continuation_task</strong></span>(fib,resp,when_done);
            }
            else if (task_name=="serializable_fib_task")
            {
                tcp_example::serializable_fib_task fib(0,0);
                boost::asynchronous::tcp::<span class="bold"><strong>deserialize_and_call_top_level_callback_continuation_task</strong></span>(fib,resp,when_done);
            }
            // else whatever functor we support
            else
            {
                std::cout &lt;&lt; "unknown task! Sorry, don't know: " &lt;&lt; task_name &lt;&lt; std::endl;
                throw boost::asynchronous::tcp::transport_exception("unknown task");
            }
        };
boost::asynchronous::tcp::simple_tcp_client_proxy client_proxy(cscheduler,pool,server_address,server_port,executor,
                                                               10/*ms between calls to server*/);</pre><p>Notice how we use our worker pool for job serialization / deserialization.
                        Notice also how we check both possible stolen jobs.</p><p>We also introduce two new deserialization functions.
                            boost::asynchronous::tcp::<span class="bold"><strong>deserialize_and_call_task</strong></span> was used for normal tasks, we now
                        have boost::asynchronous::tcp::<span class="bold"><strong>deserialize_and_call_top_level_callback_continuation_task</strong></span>
                        for our top-level continuation task, and boost::asynchronous::tcp::<span class="bold"><strong>deserialize_and_call_callback_continuation_task</strong></span>
                        for the continuation-sub-task.</p><p>We now need to build our TCP server, which we decide will get only one
                        thread for task serialization. This ought to be enough, Fibonacci tasks have
                        little data (2 long).</p><pre class="programlisting">// we need a server
// we use a tcp pool using 1 worker
auto server_pool = boost::asynchronous::create_shared_scheduler_proxy(
                    new boost::asynchronous::threadpool_scheduler&lt;
                            boost::asynchronous::lockfree_queue&lt;&gt; &gt;(<span class="bold"><strong>1</strong></span>));

auto tcp_server= boost::asynchronous::create_shared_scheduler_proxy(
                    new boost::asynchronous::<span class="bold"><strong>tcp_server_scheduler</strong></span>&lt;
                            boost::asynchronous::lockfree_queue&lt;boost::asynchronous::any_serializable&gt;,
                            boost::asynchronous::any_callable,true&gt;
                                (server_pool,own_server_address,(unsigned int)own_server_port));</pre><p>We have a TCP server pool, as before, even a client to steal work ourselves,
                        but how do we get ourselves this combined pool, which executes some work or
                        gives some away? </p><p>Wait a minute, combined pool? Yes, a
                            <code class="code">composite_threadpool_scheduler</code> will do the trick. As we're
                        at it, we create a servant to coordinate the work, as we now always
                        do:</p><pre class="programlisting">// we need a composite for stealing
auto composite = boost::asynchronous::create_shared_scheduler_proxy
                (new boost::asynchronous::<span class="bold"><strong>composite_threadpool_scheduler</strong></span>&lt;boost::asynchronous::any_serializable&gt;
                          (<span class="bold"><strong>pool</strong></span>,<span class="bold"><strong>tcp_server</strong></span>));

// a single-threaded world, where Servant will live.
auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::single_thread_scheduler&lt;
                                     boost::asynchronous::lockfree_queue&lt;&gt; &gt;);
{
      ServantProxy proxy(scheduler,<span class="bold"><strong>pool</strong></span>);
      // result of BOOST_ASYNC_FUTURE_MEMBER is a shared_future,
      // so we have a shared_future of a shared_future(result of start_async_work)
      std::future&lt;std::future&lt;long&gt; &gt; fu = proxy.calc_fibonacci(fibo_val,cutoff);
      std::future&lt;long&gt; resfu = fu.get();
      long res = resfu.get();
}</pre><p>Notice how we give only the worker "pool" to the servant. This means, the
                        servant will post the top-level task to it, it will immediately be called
                        and create 2 Fibonacci tasks, which will create each one 2 more, etc. until
                        at some point a client connects and steals one, which will create 2 more,
                        etc.</p><p>The client will not steal directly from this pool, it will steal from the
                            <code class="code">tcp_server</code> pool, which, as long as a client request comes,
                        will steal from the worker pool, as they belong to the same composite. This
                        will continue until the composite is destroyed, or the work is done. For the
                        sake of the example, we do not give the composite as the Servant's worker
                        pool but keep it alive until the end of calculation. Please have a look at
                        the <a class="link" href="examples/example_tcp_server_fib2.cpp" target="_top">complete example</a>.</p><p>In this example, we start taking care of homogenous work distribution by
                        packing a client and a server in the same application. But we need a bit
                        more: our last client would steal work so fast, every 10ms that it would
                        starve the server or other potential client applications, so we're going to
                        tell it to only steal if the size of its work queues are under a certain
                        amount, which we will empirically determine, according to our hardware,
                        network speed, etc.</p><pre class="programlisting">int main(int argc, char* argv[])
{
    std::string server_address = (argc&gt;1) ? argv[1]:"localhost";
    std::string server_port = (argc&gt;2) ? argv[2]:"12346";
    int threads = (argc&gt;3) ? strtol(argv[3],0,0) : 4;
    // 1..n =&gt; check at regular time intervals if the queue is under the given size
    int job_getting_policy = (argc&gt;4) ? strtol(argv[4],0,0):0;
    cout &lt;&lt; "Starting connecting to " &lt;&lt; server_address &lt;&lt; " port " &lt;&lt; server_port &lt;&lt; " with " &lt;&lt; threads &lt;&lt; " threads" &lt;&lt; endl;

    auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                new boost::asynchronous::asio_scheduler&lt;&gt;);
    {
        std::function&lt;void(std::string const&amp;,boost::asynchronous::tcp::server_reponse,std::function&lt;void(boost::asynchronous::tcp::client_request const&amp;)&gt;)&gt; 
        executor=
        [](std::string const&amp; task_name,boost::asynchronous::tcp::server_reponse resp,
           std::function&lt;void(boost::asynchronous::tcp::client_request const&amp;)&gt; when_done)
        {
            if (task_name=="serializable_fib_task")
            {
                tcp_example::serializable_fib_task fib(0,0);
                boost::asynchronous::tcp::deserialize_and_call_top_level_callback_continuation_task(fib,resp,when_done);
            }
            else if (task_name=="serializable_sub_fib_task")
            {
                tcp_example::fib_task fib(0,0);
                boost::asynchronous::tcp::deserialize_and_call_callback_continuation_task(fib,resp,when_done);
            }
            else
            {
                std::cout &lt;&lt; "unknown task! Sorry, don't know: " &lt;&lt; task_name &lt;&lt; std::endl;
                throw boost::asynchronous::tcp::transport_exception("unknown task");
            }
        };

        // guarded_deque supports queue size
        auto pool = boost::asynchronous::create_shared_scheduler_proxy(
                        new boost::asynchronous::threadpool_scheduler&lt;
                            boost::asynchronous::<span class="bold"><strong>guarded_deque</strong></span>&lt;boost::asynchronous::any_serializable&gt; &gt;(threads));
        // more advanced policy
        // or <span class="bold"><strong>simple_tcp_client_proxy&lt;boost::asynchronous::tcp::queue_size_check_policy&lt;&gt;&gt;</strong></span> if your compiler can (clang)
        typename boost::asynchronous::tcp::<span class="bold"><strong>get_correct_simple_tcp_client_proxy</strong></span>&lt;boost::asynchronous::tcp::queue_size_check_policy&lt;&gt;&gt;::type proxy(
                        scheduler,pool,server_address,server_port,executor,
                        0/*ms between calls to server*/,
                        <span class="bold"><strong>job_getting_policy /* number of jobs we try to keep in queue */</strong></span>);
        // run forever
        std::future&lt;std::future&lt;void&gt; &gt; fu = proxy.run();
        std::future&lt;void&gt; fu_end = fu.get();
        fu_end.get();
    }
    return 0;
}</pre><p>The important new part is highlighted. <code class="code">simple_tcp_client_proxy</code>
                        gets an extra template argument, <code class="code">queue_size_check_policy</code>, and a
                        new constructor argument, the number of jobs in the queue, under which the
                        client will try, every 10ms, to steal a job. Normally, that would be all,
                        but g++ (up to 4.7 at least) is uncooperative and requires an extra level of
                        indirection to get the desired client proxy. Otherwise, there is no
                        change.</p><p>Notice that our standard lockfree queue offers no size() so we use a less
                        efficient guarded_deque.</p><p>You will find in the <a class="link" href="examples/simple_tcp_client.cpp" target="_top">complete example</a> a few other tasks which we will explain
                        shortly.</p><p>Let's stop a minute to think about what we just did. We built, with little
                        code, a complete framework for distributing tasks homogenously among
                        machines, by reusing standard component offered by the library: threadpools,
                        composite pools, clients, servers. If we really have client connecting or
                        not is secondary, all what can happen is that calculating our Fibonacci
                        number will last a little longer.</p><p>We also separate the task (Fibonacci) from the threadpool configuration,
                        from the network configuration, and from the control of the task (Servant),
                        leading us to highly reusable, extendable code.</p><p>In the next chapter, we will add a way to further distribute work among
                        not only machines, but whole networks. </p></div><div class="sect2" title="Example: a hierarchical network"><div class="titlepage"><div><div><h3 class="title"><a name="d0e1945"></a>Example: a hierarchical network</h3></div></div></div><p>We already distribute and parallelize work, so we can scale a great deal,
                        but our current model is one server, many clients, which means a potentially
                        high network load and a lesser scalability as more and more clients connect
                        to a server. What we want is a client/server combo application  where the
                        client steals and executes jobs and a server component of the same
                        application which steals jobs from the client on behalf of other clients.
                        What we want is to achieve something like this:</p><p><span class="inlinemediaobject"><img src="pics/TCPHierarchical.jpg"></span></p><p>We have our server application, as seen until now, called interestingly
                        ServerApplication on a machine called MainJobServer. This machine executes
                        work and offers at the same time a steal-from capability. We also have a
                        simple client called ClientApplication running on ClientMachine1, which
                        steals jobs and executes them itself without further delegating. We have
                        another client machine called ClientMachine2 on which
                        ClientServerApplication runs. This applications has two parts, a client
                        stealing jobs like ClientApplication and a server part stealing jobs from
                        the client part upon request. For example, another simple ClientApplication
                        running on ClientMachine2.1 connects to it and steals further jobs in case
                        ClientMachine2 is not executing them fast enough, or if ClientMachine2 is
                        only seen as a pass-through to move jobs execution to another network.
                        Sounds scalable. How hard is it to build? Not so hard, because in fact, we
                        already saw all we need to build this, so it's kind of a Lego game.</p><pre class="programlisting">int main(int argc, char* argv[])
{
    std::string server_address = (argc&gt;1) ? argv[1]:"localhost";
    std::string server_port = (argc&gt;2) ? argv[2]:"12345";
    std::string own_server_address = (argc&gt;3) ? argv[3]:"localhost";
    long own_server_port = (argc&gt;4) ? strtol(argv[4],0,0):12346;
    int threads = (argc&gt;5) ? strtol(argv[5],0,0) : 4;
    cout &lt;&lt; "Starting connecting to " &lt;&lt; server_address &lt;&lt; " port " &lt;&lt; server_port
         &lt;&lt; " listening on " &lt;&lt; own_server_address &lt;&lt; " port " &lt;&lt; own_server_port &lt;&lt; " with " &lt;&lt; threads &lt;&lt; " threads" &lt;&lt; endl;

// to be continued</pre><p>We take as arguments the address and port of the server we are going to steal
                        from, then our own address and port. We now need a client with its
                        communication asio scheduler and its threadpool for job execution.</p><pre class="programlisting">auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::<span class="bold"><strong>asio_scheduler</strong></span>&lt;&gt;);
    { //block start
        std::function&lt;void(std::string const&amp;,boost::asynchronous::tcp::server_reponse,
                           std::function&lt;void(boost::asynchronous::tcp::client_request const&amp;)&gt;)&gt; executor=
        [](std::string const&amp; task_name,boost::asynchronous::tcp::server_reponse resp,
           std::function&lt;void(boost::asynchronous::tcp::client_request const&amp;)&gt; when_done)
        {
            if (task_name=="serializable_fib_task")
            {
                tcp_example::serializable_fib_task fib(0,0);
                boost::asynchronous::tcp::deserialize_and_call_top_level_callback_continuation_task(fib,resp,when_done);
            }
            else if (task_name=="serializable_sub_fib_task")
            {
                tcp_example::fib_task fib(0,0);
                boost::asynchronous::tcp::deserialize_and_call_callback_continuation_task(fib,resp,when_done);
            }
            // else whatever functor we support
            else
            {
                std::cout &lt;&lt; "unknown task! Sorry, don't know: " &lt;&lt; task_name &lt;&lt; std::endl;
                throw boost::asynchronous::tcp::transport_exception("unknown task");
            }
        };
        // create pools
        // we need a pool where the tasks execute
        auto pool = boost::asynchronous::create_shared_scheduler_proxy(
                    new boost::asynchronous::<span class="bold"><strong>threadpool_scheduler</strong></span>&lt;
                            boost::asynchronous::lockfree_queue&lt;boost::asynchronous::any_serializable&gt; &gt;(<span class="bold"><strong>threads</strong></span>));
        boost::asynchronous::tcp::<span class="bold"><strong>simple_tcp_client_proxy client_proxy</strong></span>(scheduler,<span class="bold"><strong>pool</strong></span>,server_address,server_port,executor,
                                                                       10/*ms between calls to server*/);
// to be continued</pre><p>We now need a server to which more clients will connect, and a composite
                binding it to our worker pool:</p><pre class="programlisting">   // we need a server
   // we use a tcp pool using 1 worker
   auto server_pool = boost::asynchronous::create_shared_scheduler_proxy(
                    new boost::asynchronous::threadpool_scheduler&lt;
                            boost::asynchronous::lockfree_queue&lt;&gt; &gt;(1));
   auto tcp_server= boost::asynchronous::create_shared_scheduler_proxy(
                    new boost::asynchronous::<span class="bold"><strong>tcp_server_scheduler</strong></span>&lt;
                            boost::asynchronous::lockfree_queue&lt;boost::asynchronous::any_serializable&gt;,
                            boost::asynchronous::any_callable,true&gt;
                                (server_pool,own_server_address,(unsigned int)own_server_port));
   // we need a composite for stealing
   auto composite = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::<span class="bold"><strong>composite_threadpool_scheduler</strong></span>&lt;boost::asynchronous::any_serializable&gt;
                                                                   (<span class="bold"><strong>pool</strong></span>,<span class="bold"><strong>tcp_server</strong></span>));

   std::future&lt;std::future&lt;void&gt; &gt; fu = client_proxy.run();
   std::future&lt;void&gt; fu_end = fu.get();
   fu_end.get();
} //end block

 return 0;
 } //end main</pre><p>And we're done! The client part will steal jobs and execute them, while the
                        server part, bound to the client pool, will steal on sub-client-demand.
                        Please have a look at the <a class="link" href="examples/tcp_client_server.cpp" target="_top">
                            complete code</a>.</p></div></div><div class="sect1" title="Picking your archive"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e1998"></a>Picking your archive</h2></div></div></div><p>By default, Asynchronous uses a Boost Text archive (text_oarchive,
                    text_iarchive), which is simple and efficient enough for our Fibonacci example,
                    but inefficient for tasks holding more data.</p><p>Asynchronous supports any archive task, requires however a different job type
                    for this. At the moment, we can use a
                        <code class="code">portable_binary_oarchive</code>/<code class="code">portable_binary_iarchive</code>
                    by selecting <code class="code">any_bin_serializable</code> as job. If Boost supports more
                    archive types, support is easy to add.</p><p>The previous Fibonacci server example has been <a class="link" href="examples/example_tcp_server_fib2_bin.cpp" target="_top">rewritten</a> to use this
                    capability. The <a class="link" href="examples/simple_tcp_client_bin_archive.cpp" target="_top">client</a> has also been rewritten using this new job type.</p></div><div class="sect1" title="Parallel Algorithms (Christophe Henry / Tobias Holl)"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e2022"></a><span class="command"><strong><a name="parallel_algos"></a></strong></span>Parallel Algorithms (Christophe Henry / Tobias Holl)</h2></div></div></div><p>Asynchronous supports out of the box quite some asynchronous parallel
                    algorithms, as well as interesting combination usages. These algorithms are
                    callback-continuation-based. Some of these algorithms also support distributed
                    calculations as long as the user-provided functors are (meaning they must be
                    serializable).</p><p>What is the point of adding yet another set of parallel algorithms which can
                    be found elsewhere? Because truly asynchronous algorithms are hard to find. By
                    this we mean non-blocking. If one needs parallel algorithms, it's because they
                    could need long to complete. And if they take long, we really do not want to
                    block until it happens.</p><p>All of the algorithms are made for use in a worker threadpool. They represent
                    the work part of a <code class="code">post_callback</code>;</p><p>In the philosophy of Asynchronous, the programmer knows better the task size
                    where he wants to start parallelizing, so all these algorithms take a cutoff.
                    Work is cut into packets of this size.</p><p>All range algorithms also have a version taking a continuation as range
                    argument. This allows to combine algorithms functional way, for example this
                    (more to come):</p><pre class="programlisting">return <span class="bold"><strong>parallel_for</strong></span>(<span class="bold"><strong>parallel_for</strong></span>(<span class="bold"><strong>parallel_for</strong></span>(...)));</pre><p>Asynchronous implements the following algorithms. It is also indicated whether
                    the algorithm supports arguments passed as iterators, moved range, or
                    continuation. Indicated is also whether the algorithm is distributable.</p><div class="table"><a name="d0e2052"></a><p class="title"><b>Table&nbsp;3.1.&nbsp;Non-modifying Algorithms, in boost/asynchronous/algorithm</b></p><div class="table-contents"><table summary="Non-modifying Algorithms, in boost/asynchronous/algorithm" border="1"><colgroup><col><col><col><col><col></colgroup><thead><tr><th>Name</th><th>Description</th><th>Header</th><th>Input Parameters</th><th>Distributable</th></tr></thead><tbody><tr><td><span class="command"><strong><a class="command" href="#parallel_all_of">parallel_all_of</a></strong></span></td><td>checks if a predicate is true for all of the elements in
                                        a range </td><td>parallel_all_of.hpp</td><td>Iterators, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_any_of">parallel_any_of</a></strong></span></td><td>checks if a predicate is true for any of the elements in
                                        a range </td><td>parallel_any_of.hpp</td><td>Iterators, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_none_of">parallel_none_of</a></strong></span></td><td>checks if a predicate is true for none of the elements in
                                        a range </td><td>parallel_none_of.hpp</td><td>Iterators, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_for_each">parallel_for_each</a></strong></span></td><td>aplpies a functor to a range of elements and accumulates
                                        the result into the functor</td><td>parallel_for_each.hpp</td><td>Iterators</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_for">parallel_for</a></strong></span></td><td>applies a function to a range of elements</td><td>parallel_for.hpp</td><td>Iterators, moved range, continuation</td><td>Yes</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_count">parallel_count</a></strong></span></td><td>returns the number of elements satisfying specific
                                        criteria </td><td>parallel_count.hpp</td><td>Iterators, moved range, continuation</td><td>Yes</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_count">parallel_count_if</a></strong></span></td><td>returns the number of elements satisfying specific
                                        criteria </td><td>parallel_count_if.hpp</td><td>Iterators, moved range, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_equal">parallel_equal</a></strong></span></td><td>determines if two sets of elements are the same </td><td>parallel_equal.hpp</td><td>Iterators</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_mismatch">parallel_mismatch</a></strong></span></td><td>finds the first position where two ranges differ</td><td>parallel_mismatch.hpp</td><td>Iterators</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_find_all">parallel_find_all</a></strong></span></td><td>finds all the elements satisfying specific criteria </td><td>parallel_find_all.hpp</td><td>Iterators, moved range, continuation</td><td>Yes</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_find_end">parallel_find_end</a></strong></span></td><td>finds the last sequence of elements in a certain
                                        range</td><td>parallel_find_end.hpp</td><td>Iterators, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_find_first_of">parallel_find_first_of</a></strong></span></td><td>searches for any one of a set of elements</td><td>parallel_find_first_of.hpp</td><td>Iterators, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_adjacent_find">parallel_adjacent_find</a></strong></span></td><td>finds the first two adjacent items that are equal (or
                                        satisfy a given predicate)</td><td>parallel_adjacent_find.hpp</td><td>Iterators</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_lexicographical_compare">parallel_lexicographical_compare</a></strong></span></td><td>returns true if one range is lexicographically less than
                                        another</td><td>parallel_lexicographical_compare.hpp</td><td>Iterators</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_search">parallel_search</a></strong></span></td><td>searches for a range of elements</td><td>parallel_search.hpp</td><td>Iterators, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_search_n">parallel_search_n</a></strong></span></td><td>searches for a number consecutive copies of an element in
                                        a range</td><td>parallel_search_n.hpp</td><td>Iterators, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_scan">parallel_scan</a></strong></span></td><td>does a custom scan over a range of elements</td><td>parallel_scan.hpp</td><td>Iterators, moved range, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_inclusive_scan">parallel_inclusive_scan</a></strong></span></td><td>does an inclusive scan over a range of elements</td><td>parallel_inclusive_scan.hpp</td><td>Iterators, moved range, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_exclusive_scan">parallel_exclusive_scan</a></strong></span></td><td>does an exclusive scan over a range of elements</td><td>parallel_exclusive_scan.hpp</td><td>Iterators, moved range, continuation</td><td>No</td></tr></tbody></table></div></div><p><br class="table-break"></p><p></p><div class="table"><a name="d0e2303"></a><p class="title"><b>Table&nbsp;3.2.&nbsp;Modifying Algorithms, in boost/asynchronous/algorithm</b></p><div class="table-contents"><table summary="Modifying Algorithms, in boost/asynchronous/algorithm" border="1"><colgroup><col><col><col><col><col></colgroup><thead><tr><th>Name</th><th>Description</th><th>Header</th><th>Parameters taken</th><th>Distributable</th></tr></thead><tbody><tr><td><span class="command"><strong><a class="command" href="#parallel_copy">parallel_copy</a></strong></span></td><td>copies a range of elements to a new location</td><td>parallel_copy.hpp</td><td>Iterators, moved range, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_copy_if">parallel_copy_if</a></strong></span></td><td>copies a the elements to a new location for which the given
                                predicate is true. </td><td>parallel_copy_if.hpp</td><td>Iterators</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_move">parallel_move</a></strong></span></td><td>moves a range of elements to a new location</td><td>parallel_move.hpp</td><td>Iterators, moved range, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_fill">parallel_fill</a></strong></span></td><td>assigns a range of elements a certain value </td><td>parallel_fill.hpp</td><td>Iterators, moved range, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_transform">parallel_transform</a></strong></span></td><td>applies a function to a range of elements</td><td>parallel_transform.hpp</td><td>Iterators</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_generate">parallel_generate</a></strong></span></td><td>saves the result of a function in a range </td><td>parallel_generate.hpp</td><td>Iterators, moved range, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_remove_copy">parallel_remove_copy</a></strong></span></td><td>copies a range of elements that are not equal to a specific
                                value</td><td>parallel_remove_copy.hpp</td><td>Iterators</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_remove_copy">parallel_remove_copy_if</a></strong></span></td><td>copies a range of elements omitting those that satisfy specific
                                criteria</td><td>parallel_remove_copy.hpp</td><td>Iterators</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_replace">parallel_replace</a></strong></span></td><td>replaces all values with a specific value with another value </td><td>parallel_replace.hpp</td><td>Iterators, moved range, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_replace">parallel_replace_if</a></strong></span></td><td>replaces all values satisfying specific criteria with another value </td><td>parallel_replace.hpp</td><td>Iterators, moved range, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_reverse">parallel_reverse</a></strong></span></td><td>reverses the order of elements in a range </td><td>parallel_reverse.hpp</td><td>Iterators, moved range, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_swap_ranges">parallel_swap_ranges</a></strong></span></td><td>swaps two ranges of elements</td><td>parallel_swap_ranges.hpp</td><td>Iterators</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_transform_inclusive_scan">parallel_transform_inclusive_scan</a></strong></span></td><td>does an inclusive scan over a range of elements after applying a
                                function to each element </td><td>parallel_transform_inclusive_scan.hpp</td><td>Iterators</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_transform_exclusive_scan">parallel_transform_exclusive_scan</a></strong></span></td><td>does an exclusive scan over a range of elements after applying a
                                function to each element</td><td>parallel_transform_exclusive_scan.hpp</td><td>Iterators</td><td>No</td></tr></tbody></table></div></div><br class="table-break"><p></p><div class="table"><a name="d0e2494"></a><p class="title"><b>Table&nbsp;3.3.&nbsp;Partitioning Operations, in boost/asynchronous/algorithm</b></p><div class="table-contents"><table summary="Partitioning Operations, in boost/asynchronous/algorithm" border="1"><colgroup><col><col><col><col><col></colgroup><thead><tr><th>Name</th><th>Description</th><th>Header</th><th>Parameters taken</th><th>Distributable</th></tr></thead><tbody><tr><td><span class="command"><strong><a class="command" href="#parallel_is_partitioned">parallel_is_partitioned</a></strong></span></td><td>determines if the range is partitioned by the given predicate</td><td>parallel_is_partitioned.hpp</td><td>Iterators</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_partition">parallel_partition</a></strong></span></td><td>divides a range of elements into two groups</td><td>parallel_partition.hpp</td><td>Iterators, moved range, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_stable_partition">parallel_stable_partition</a></strong></span></td><td>divides elements into two groups while preserving their relative
                                order</td><td>parallel_stable_partition.hpp</td><td>Iterators, moved range, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_partition_copy">parallel_partition_copy</a></strong></span></td><td>copies a range dividing the elements into two groups</td><td>parallel_partition_copy.hpp</td><td>Iterators</td><td>No</td></tr></tbody></table></div></div><br class="table-break"><p></p><div class="table"><a name="d0e2565"></a><p class="title"><b>Table&nbsp;3.4.&nbsp;Sorting Operations, in boost/asynchronous/algorithm</b></p><div class="table-contents"><table summary="Sorting Operations, in boost/asynchronous/algorithm" border="1"><colgroup><col><col><col><col><col></colgroup><thead><tr><th>Name</th><th>Description</th><th>Header</th><th>Parameters taken</th><th>Distributable</th></tr></thead><tbody><tr><td><span class="command"><strong><a class="command" href="#parallel_is_sorted">parallel_is_sorted</a></strong></span></td><td>checks whether a range is sorted according to the given predicate </td><td>parallel_is_sorted.hpp</td><td>Iterators</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_is_reverse_sorted">parallel_is_reverse_sorted</a></strong></span></td><td>checks whether a range is reverse sorted according to the given
                                predicate</td><td>parallel_is_sorted.hpp</td><td>Iterators</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_sort">parallel_sort</a></strong></span></td><td>sorts a range according to the given predicate </td><td>parallel_sort.hpp</td><td>Iterators, moved range, continuation</td><td>Yes</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_sort">parallel_sort_inplace</a></strong></span></td><td>sorts a range according to the given predicate using inplace
                                merge</td><td>parallel_sort_inplace.hpp</td><td>Iterators, moved range, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_sort">parallel_spreadsort_inplace</a></strong></span></td><td>sorts a range according to the given predicate using a
                                Boost.Spreadsort algorithm and inplace merge</td><td>parallel_sort_inplace.hpp</td><td>Iterators, moved range, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_sort">parallel_stable_sort_inplace</a></strong></span></td><td>sorts a range of elements while preserving order between equal
                                elements using inplace merge</td><td>parallel_sort_inplace.hpp</td><td>Iterators, moved range, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_sort">parallel_spreadsort</a></strong></span></td><td>sorts a range according to the given predicate using a
                                Boost.Spreadsort algorithm</td><td>parallel_sort.hpp</td><td>Iterators, moved range, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_sort">parallel_stable_sort</a></strong></span></td><td>sorts a range of elements while preserving order between equal
                                elements</td><td>parallel_stable_sort.hpp</td><td>Iterators, moved range, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_partial_sort">parallel_partial_sort</a></strong></span></td><td>sorts the first N elements of a range </td><td>parallel_partial_sort.hpp</td><td>Iterators</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_quicksort">parallel_quicksort</a></strong></span></td><td>sorts a range according to the given predicate using a
                                quicksort</td><td>parallel_quicksort.hpp</td><td>Iterators</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_quicksort">parallel_quick_spreadsort</a></strong></span></td><td>sorts a range according to the given predicate using a quicksort and
                                a Boost.Spreadsort algorithm</td><td>parallel_quicksort.hpp</td><td>Iterators</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_nth_element">parallel_nth_element</a></strong></span></td><td>partially sorts the given range making sure that it is partitioned
                                by the given element </td><td>parallel_nth_element.hpp</td><td>Iterators</td><td>No</td></tr></tbody></table></div></div><br class="table-break"><p></p><div class="table"><a name="d0e2732"></a><p class="title"><b>Table&nbsp;3.5.&nbsp;Numeric Algorithms in boost/asynchronous/algorithm</b></p><div class="table-contents"><table summary="Numeric Algorithms in boost/asynchronous/algorithm" border="1"><colgroup><col><col><col><col><col></colgroup><thead><tr><th>Name</th><th>Description</th><th>Header</th><th>Parameters taken</th><th>Distributable</th></tr></thead><tbody><tr><td><span class="command"><strong><a class="command" href="#parallel_iota">parallel_iota</a></strong></span></td><td>fills a range with successive increments of the starting value</td><td>parallel_iota.hpp</td><td>Iterators, moved range</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_reduce">parallel_reduce</a></strong></span></td><td>sums up a range of elements </td><td>parallel_reduce.hpp</td><td>Iterators, moved range, continuation</td><td>Yes</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_inner_product">parallel_inner_product</a></strong></span></td><td>computes the inner product of two ranges of elements </td><td>parallel_inner_product.hpp</td><td>Iterators, moved range, continuation</td><td>Yes</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_partial_sum">parallel_partial_sum</a></strong></span></td><td>computes the partial sum of a range of elements </td><td>parallel_partial_sum.hpp</td><td>Iterators, moved range, continuation</td><td>No</td></tr></tbody></table></div></div><br class="table-break"><p></p><div class="table"><a name="d0e2803"></a><p class="title"><b>Table&nbsp;3.6.&nbsp;Algorithms Operating on Sorted Sequences in
                        boost/asynchronous/algorithm</b></p><div class="table-contents"><table summary="Algorithms Operating on Sorted Sequences in&#xA;                        boost/asynchronous/algorithm" border="1"><colgroup><col><col><col><col><col></colgroup><thead><tr><th>Name</th><th>Description</th><th>Header</th><th>Parameters taken</th><th>Distributable</th></tr></thead><tbody><tr><td><span class="command"><strong><a class="command" href="#parallel_merge">parallel_merge</a></strong></span></td><td>merges two sorted ranges </td><td>parallel_merge.hpp</td><td>Iterators</td><td>No</td></tr></tbody></table></div></div><br class="table-break"><p></p><div class="table"><a name="d0e2838"></a><p class="title"><b>Table&nbsp;3.7.&nbsp;Minimum/maximum operations in boost/asynchronous/algorithm</b></p><div class="table-contents"><table summary="Minimum/maximum operations in boost/asynchronous/algorithm" border="1"><colgroup><col><col><col><col><col></colgroup><thead><tr><th>Name</th><th>Description</th><th>Header</th><th>Parameters taken</th><th>Distributable</th></tr></thead><tbody><tr><td><span class="command"><strong><a class="command" href="#parallel_extremum">parallel_extremum</a></strong></span></td><td>returns an extremum(smaller/ greater) of the given values according
                                to a given predicate </td><td>parallel_extremum.hpp</td><td>Iterators, moved range, continuation</td><td>Yes</td></tr></tbody></table></div></div><br class="table-break"><p></p><div class="table"><a name="d0e2873"></a><p class="title"><b>Table&nbsp;3.8.&nbsp;Miscellaneous Algorithms in boost/asynchronous/algorithm</b></p><div class="table-contents"><table summary="Miscellaneous Algorithms in boost/asynchronous/algorithm" border="1"><colgroup><col><col><col><col><col></colgroup><thead><tr><th>Name</th><th>Description</th><th>Header</th><th>Parameters taken</th><th>Distributable</th></tr></thead><tbody><tr><td><span class="command"><strong><a class="command" href="#parallel_invoke">parallel_invoke</a></strong></span></td><td>invokes a variable number of operations in parallel</td><td>parallel_invoke.hpp</td><td>variadic sequence of functions</td><td>Yes</td></tr><tr><td><span class="command"><strong><a class="command" href="#if_then_else">if_then_else</a></strong></span></td><td>invokes algorithms based on the result of a function</td><td>if_then_else.hpp</td><td>if/then/else clauses</td><td>No</td></tr></tbody></table></div></div><br class="table-break"><p></p><div class="table"><a name="d0e2920"></a><p class="title"><b>Table&nbsp;3.9.&nbsp;(Boost) Geometry Algorithms in boost/asynchronous/algorithm/geometry
                        (compatible with boost geometry 1.58). Experimental and tested only with
                        polygons.</b></p><div class="table-contents"><table summary="(Boost) Geometry Algorithms in boost/asynchronous/algorithm/geometry&#xA;                        (compatible with boost geometry 1.58). Experimental and tested only with&#xA;                        polygons." border="1"><colgroup><col><col><col><col><col></colgroup><thead><tr><th>Name</th><th>Description</th><th>Header</th><th>Parameters taken</th><th>Distributable</th></tr></thead><tbody><tr><td><span class="command"><strong><a class="command" href="#parallel_geometry_intersection_of_x">parallel_geometry_intersection_of_x</a></strong></span></td><td>calculates the intersection of many geometries </td><td>parallel_geometry_intersection_of_x.hpp</td><td>Iterators, moved range, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_geometry_union_of_x">parallel_geometry_union_of_x</a></strong></span></td><td>combines many geometries which each other </td><td>parallel_geometry_union_of_x.hpp</td><td>Iterators, moved range, continuation</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_intersection">parallel_intersection</a></strong></span></td><td>calculates the intersection of two geometries</td><td>parallel_intersection.hpp</td><td>two geometries</td><td>No</td></tr><tr><td><span class="command"><strong><a class="command" href="#parallel_union">parallel_union</a></strong></span></td><td>combines two geometries which each other</td><td>parallel_union.hpp</td><td>two geometries</td><td>No</td></tr></tbody></table></div></div><br class="table-break"><p></p><div class="sect2" title="Finding the best cutoff"><div class="titlepage"><div><div><h3 class="title"><a name="d0e2991"></a>Finding the best cutoff</h3></div></div></div><p>The algorithms described above all make use of a cutoff, which is the
                        number of elements where the algorithm should stop going parallel and
                        execute sequentially. It is sometimes also named grain size in the
                        literrature. Finding the best value can often make quite a big difference in
                        execution time. Unfortunately, the best cutoff differs greatly between
                        different processors or even machines. To make this task easier,
                        Asynchronous provides a helper function, <span class="bold"><strong>find_best_cutoff</strong></span>, which helps finding the best cutoff. For
                        best results, it makes sense to use it at deployment time. <span class="bold"><strong>find_best_cutoff</strong></span> can be found in
                        boost/asynchronous/helpers.hpp.</p><pre class="programlisting">template &lt;class Func, class Scheduler&gt;
std::tuple&lt;std::size_t,std::vector&lt;std::size_t&gt;&gt; find_best_cutoff(Scheduler s, Func f,
                                                     std::size_t cutoff_begin,
                                                     std::size_t cutoff_end,
                                                     std::size_t steps,
                                                     std::size_t retries,
                                                     const std::string&amp; task_name="",
                                                     std::size_t prio=0 )</pre><p><span class="underline">Return value</span>: A tuple containing the
                        best cutoff and a std::vector containing the elapsed times of this best
                        cutoff.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>Scheduler s: the scheduler which will execute the algorithm we
                                    want to optimize</p></li><li class="listitem"><p>Func f: a unary predicate which will be called for every
                                    cutoff value. It must have a signature of the form
                                    Unspecified-Continuation f (std::size_t cutoff); which will
                                    return a continuation, which is what algorithms described in the
                                    next sections will return.</p></li><li class="listitem"><p>cutoff_begin, cutoff_end: the range of cutoffs to test</p></li><li class="listitem"><p>steps: step between two possible cutoff values. This is needed
                                    because testing every possible cutoff would take very
                                    long.</p></li><li class="listitem"><p>retries: how many times the same cutoff value will be used.
                                    Using retries will give us a better mean vakue for a given
                                    cutoff.</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div><p>Please have a look at <a class="link" href="examples/example_cutoff_sort.cpp" target="_top">this example finding the best cutoff for a parallel_sort</a>.</p></div><div class="sect2" title="parallel_for"><div class="titlepage"><div><div><h3 class="title"><a name="d0e3039"></a><span class="command"><strong><a name="parallel_for"></a></strong></span>parallel_for</h3></div></div></div><p>Applies a functor to every element of the range [beg,end) .</p><pre class="programlisting">template &lt;class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>void</strong></span>,Job&gt;
parallel_for(Iterator beg, Iterator end,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The parallel_for version taking iterators requires that the iterators stay
                        valid until completion. It is the programmer's job to ensure this.</p><p>The third argument is the predicate applied on each element of the
                        algorithm.</p><p>The fourth argument is the cutoff, meaning in this case the max. number of
                        elements of the input range in a task.</p><p>The optional fifth argument is the name of the tasks used for
                        logging.</p><p>The optional sixth argument is the priority of the tasks in the
                        pool.</p><p>The return value is a void continuation containing either nothing or an
                        exception if one was thrown from one of the tasks.</p><p>Example:</p><pre class="programlisting">struct Servant : boost::asynchronous::trackable_servant&lt;&gt;
{
    void start_async_work()
    {
        // start long tasks in threadpool (first lambda) and callback in our thread
        post_callback(
               [this](){
                        <span class="bold"><strong>return</strong></span> boost::asynchronous::<span class="bold"><strong>parallel_for</strong></span>(<span class="bold"><strong>this-&gt;m_data.begin(),this-&gt;m_data.end()</strong></span>,
                                                                 [](int&amp; i)
                                                                 {
                                                                    i += 2;
                                                                 },1500);
                      },// work
               // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
               [](boost::asynchronous::expected&lt;<span class="bold"><strong>void</strong></span>&gt; /*res*/){
                            ...
               }// callback functor.
        );
    }
    std::vector&lt;int&gt; m_data;
};

// same using post_future
std::future&lt;void&gt; fu = post_future(
                           [this](){
                            <span class="bold"><strong>return</strong></span> boost::asynchronous::<span class="bold"><strong>parallel_for</strong></span>(<span class="bold"><strong>this-&gt;m_data.begin(),this-&gt;m_data.end()</strong></span>,
                                                                 [](int&amp; i)
                                                                 {
                                                                    i += 2;
                                                                 },1500);});</pre><p>The most important parts are highlighted. Do not forget the return statement
                        as we are returning a continuation and we do not want the lambda to be
                        interpreted as a void lambda. The caller has responsibility of the input
                        data, given in the form of iterators. </p><p>The code will do following:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>start tasks in the current worker pool of max 1500 elements of
                                the input data</p></li><li class="listitem"><p>add 2 to each element in parallel</p></li><li class="listitem"><p>The parallel_for will return a continuation</p></li><li class="listitem"><p>The callback lambda will be called when all tasks complete.
                                The expected will be either set or contain an exception</p></li><li class="listitem"><p>If post_future is used, a future&lt;void&gt; will be
                                returned.</p></li></ul></div><p>Please have a look at <a class="link" href="examples/example_parallel_for.cpp" target="_top">the complete example</a>.</p><p>The functor can either be called for every single element, or for a range
                        of elements:
                        </p><pre class="programlisting">std::future&lt;void&gt; fu = post_future(
                           [this](){
                            <span class="bold"><strong>return</strong></span> boost::asynchronous::<span class="bold"><strong>parallel_for</strong></span>(<span class="bold"><strong>this-&gt;m_data.begin(),this-&gt;m_data.end()</strong></span>,
                                                                 [](std::vector&lt;int&gt;::iterator beg, std::vector&lt;int&gt;::iterator end)
                                                                 {
                                                                    for(;beg != end; ++beg)
                                                                    {
                                                                        *beg += 2;
                                                                    }
                                                                 },1500);});</pre><p>The second version takes a range per rvalue reference. This is signal
                        given to Asynchronous that it must take ownership of the range. The return
                        value is then a continuation of the given range type:</p><pre class="programlisting">template &lt;class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Range</strong></span>,Job&gt;
parallel_for(Range&amp;&amp; range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>A <code class="code">post_callback / post_future</code> will therefore get a
                        expected&lt;new range&gt;, for example:</p><pre class="programlisting">post_callback(
    []()
    {
       std::vector&lt;int&gt; data;
       return boost::asynchronous::parallel_for(std::move(data),
                                                      [](int&amp; i)
                                                      {
                                                        i += 2;
                                                      },1500);
    },
    ](<span class="bold"><strong>boost::asynchronous::expected&lt;std::vector&lt;int&gt;&gt;</strong></span> ){}
);</pre><p>In this case, the programmer does not need to ensure the container stays
                        valid, Asynchronous takes care of it.</p><p>The third version of this algorithm takes a range continuation instead of
                        a range as argument and will be invoked after the continuation is
                        ready.</p><pre class="programlisting">// version taking a continuation of a range as first argument
template &lt;class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>typename Range::return_type</strong></span>,Job&gt;
parallel_for(Range range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>This version allows chaining parallel calls. For example, it is now possible
                        to write:</p><pre class="programlisting">post_callback(
    []()
    {
       std::vector&lt;int&gt; data;
       return <span class="bold"><strong>parallel_for</strong></span>(<span class="bold"><strong>parallel_for</strong></span>(<span class="bold"><strong>parallel_for</strong></span>(
                                                            // executed first
                                                            std::move(data),
                                                            [](int&amp; i)
                                                            {
                                                               i += 2;
                                                            },1500),
                                              // executed second
                                              [](int&amp; i)
                                              {
                                                  i += 2;
                                              },1500),
                                 // executed third
                                 [](int&amp; i)
                                 {
                                      i += 2;
                                 },1500);
    },
    ](<span class="bold"><strong>boost::asynchronous::expected&lt;std::vector&lt;int&gt;&gt;</strong></span> ){} // callback
);</pre><p>This code will be executed as follows:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>the most inner parallel_for (parallel execution)</p></li><li class="listitem"><p>A kind of synchronization point will be done at this point
                                until the parallel_for completes</p></li><li class="listitem"><p>the middle parallel_for will be executed (parallel
                                execution)</p></li><li class="listitem"><p>A kind of synchronization point will be done at this point
                                until the parallel_for completes</p></li><li class="listitem"><p>the outer parallel_for will be executed (parallel
                                execution)</p></li><li class="listitem"><p>A kind of synchronization point will be done at this point
                                until the parallel_for completes</p></li><li class="listitem"><p>The callback will be called</p></li></ul></div><p>With "kind of synchronization point", we mean there will be no blocking
                        synchronization, it will just be waited until completion.</p><p>Finally, this algorithm has a distributed version. We need, as with our
                        Fibonacci example, a serializable sub-task which will be created as often as
                        required by our cutoff and which will handle a part of our range:</p><pre class="programlisting">struct dummy_parallel_for_subtask : public boost::asynchronous::serializable_task
{
    dummy_parallel_for_subtask(int d=0):boost::asynchronous::serializable_task(<span class="bold"><strong>"dummy_parallel_for_subtask"</strong></span>),m_data(d){}
    template &lt;class Archive&gt;
    void <span class="bold"><strong>serialize</strong></span>(Archive &amp; ar, const unsigned int /*version*/)
    {
        ar &amp; m_data;
    }
    void operator()(int&amp; i)const
    {
        i += m_data;
    }
    // some data, so we have something to serialize
    int m_data;
};</pre><p>We also need a serializable top-level task, creating sub-tasks:</p><pre class="programlisting">struct dummy_parallel_for_task : public boost::asynchronous::serializable_task
{
    dummy_parallel_for_task():boost::asynchronous::serializable_task(<span class="bold"><strong>"dummy_parallel_for_task"</strong></span>),m_data(1000000,1){}
    template &lt;class Archive&gt;
    void <span class="bold"><strong>serialize</strong></span>(Archive &amp; ar, const unsigned int /*version*/)
    {
        ar &amp; m_data;
    }
    auto operator()() -&gt; decltype(boost::asynchronous::parallel_for&lt;std::vector&lt;int&gt;,dummy_parallel_for_subtask,boost::asynchronous::any_serializable&gt;(
                                      std::move(std::vector&lt;int&gt;()),
                                      dummy_parallel_for_subtask(2),
                                      10))
    {
        <span class="bold"><strong>return boost::asynchronous::parallel_for</strong></span>
                &lt;std::vector&lt;int&gt;,<span class="bold"><strong>dummy_parallel_for_subtask</strong></span>,boost::asynchronous::any_serializable&gt;(
            std::move(m_data),
            dummy_parallel_for_subtask(2),
            10);
    }
    std::vector&lt;int&gt; m_data;
};</pre><p>We now post our top-level task inside a servant or use post_future:</p><pre class="programlisting">post_callback(
               dummy_parallel_for_task(),
               // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
               [this](boost::asynchronous::expected&lt;std::vector&lt;int&gt;&gt; res){
                 try
                 {
                    // do something
                 }
                 catch(std::exception&amp; e)
                 {
                    std::cout &lt;&lt; "got exception: " &lt;&lt; e.what() &lt;&lt; std::endl;
                 }
              }// end of callback functor.
);</pre><p>Please have a look at the <a class="link" href="examples/example_parallel_for_tcp.cpp" target="_top">complete server example</a>.</p></div><div class="sect2" title="parallel_for_each"><div class="titlepage"><div><div><h3 class="title"><a name="d0e3228"></a><span class="command"><strong><a name="parallel_for_each"></a></strong></span>parallel_for_each</h3></div></div></div><p>Applies a functor to every element of the range [beg,end). This functor
                        can save data. It is merged at different steps with other instances of this
                        functor. The algorithm returns the last merged instance.</p><pre class="programlisting">template &lt;class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Func</strong></span>,Job&gt;
<span class="bold"><strong>parallel_all_of</strong></span>(Iterator begin, Iterator end,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Return value</span>: A merged instance of a
                        functor of type Func.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end: the range of elements to search</p></li><li class="listitem"><p>func: a class / struct object with a:</p><p>
                                    </p><div class="itemizedlist"><ul class="itemizedlist" type="circle"><li class="listitem"><p>void operator()(const Type&amp; a)</p></li><li class="listitem"><p>void merge (Func const&amp; f)</p></li></ul></div><p>
                                </p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_all_of"><div class="titlepage"><div><div><h3 class="title"><a name="d0e3278"></a><span class="command"><strong><a name="parallel_all_of"></a></strong></span>parallel_all_of</h3></div></div></div><p>Checks if unary predicate p returns true for all elements in the range
                        [begin, end). </p><pre class="programlisting">// version for iterators
template &lt;class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>bool</strong></span>,Job&gt;
<span class="bold"><strong>parallel_all_of</strong></span>(Iterator begin, Iterator end,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for ranges returned as continuations
template &lt;class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>bool</strong></span>,Job&gt;
<span class="bold"><strong>parallel_all_of</strong></span>(Range range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Return value</span>: true if unary
                        predicate returns true for all elements in the range, false otherwise.
                        Returns true if the range is empty.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end: the range of elements to search</p><p>Or a continuation, coming from another algorithm.</p></li><li class="listitem"><p>func: unary predicate function. The signature of the function
                                    should be equivalent to the following: bool pred(const Type
                                    &amp;a);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_any_of"><div class="titlepage"><div><div><h3 class="title"><a name="d0e3326"></a><span class="command"><strong><a name="parallel_any_of"></a></strong></span>parallel_any_of</h3></div></div></div><p>Checks if unary predicate p returns true for at least one element in the
                        range [begin, end).</p><pre class="programlisting">// version for iterators
template &lt;class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>bool</strong></span>,Job&gt;
<span class="bold"><strong>parallel_any_of</strong></span>(Iterator begin, Iterator end,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for ranges returned as continuations
template &lt;class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>bool</strong></span>,Job&gt;
<span class="bold"><strong>parallel_any_of</strong></span>(Range range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Return value</span>: true if unary
                        predicate returns true for at least one element in the range, false
                        otherwise. Returns false if the range is empty.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end: the range of elements to search</p><p>Or a continuation, coming from another algorithm.</p></li><li class="listitem"><p>func: unary predicate function. The signature of the function
                                    should be equivalent to the following: bool pred(const Type
                                    &amp;a);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_none_of"><div class="titlepage"><div><div><h3 class="title"><a name="d0e3374"></a><span class="command"><strong><a name="parallel_none_of"></a></strong></span>parallel_none_of</h3></div></div></div><p>Checks if unary predicate p returns true for no elements in the range
                        [begin, end).</p><pre class="programlisting">// version for iterators
template &lt;class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>bool</strong></span>,Job&gt;
<span class="bold"><strong>parallel_none_of</strong></span>(Iterator begin, Iterator end,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for ranges returned as continuations
template &lt;class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>bool</strong></span>,Job&gt;
<span class="bold"><strong>parallel_none_of</strong></span>(Range range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Return value</span>: true if unary
                        predicate returns true for no elements in the range, false otherwise.
                        Returns true if the range is empty.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end: the range of elements to search</p><p>Or a continuation, coming from another algorithm.</p></li><li class="listitem"><p>func: unary predicate function. The signature of the function
                                    should be equivalent to the following: bool pred(const Type
                                    &amp;a);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_equal"><div class="titlepage"><div><div><h3 class="title"><a name="d0e3422"></a><span class="command"><strong><a name="parallel_equal"></a></strong></span>parallel_equal</h3></div></div></div><p>Checks if two ranges are equal.</p><pre class="programlisting">template &lt;class Iterator1,class Iterator2, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>bool</strong></span>,Job&gt;
<span class="bold"><strong>parallel_equal</strong></span>(Iterator1 begin1, Iterator1 end1,Iterator2 begin2, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

template &lt;class Iterator1,class Iterator2, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>bool</strong></span>,Job&gt;
<span class="bold"><strong>parallel_equal</strong></span>(Iterator1 begin1, Iterator1 end1,Iterator2 begin2,Func func, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Return value</span>:  If the length of the
                        range [first1, end1) does not equal the length of the range beginning at
                        begin2, returns false If the elements in the two ranges are equal, returns
                        true. Otherwise returns false. </p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin1, end1: the first range</p></li><li class="listitem"><p>begin2: the beginning of the second range</p></li><li class="listitem"><p>func: binary predicate function. The signature of the function
                                    should be equivalent to the following: bool pred(const Type
                                    &amp;a,const Type &amp;b);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_mismatch"><div class="titlepage"><div><div><h3 class="title"><a name="d0e3471"></a><span class="command"><strong><a name="parallel_mismatch"></a></strong></span>parallel_mismatch</h3></div></div></div><p>Returns the first mismatching pair of elements from two ranges: one
                        defined by [begin1, end1) and another starting at begin2.</p><pre class="programlisting">template &lt;class Iterator1,class Iterator2, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>std::pair&lt;Iterator1,Iterator2&gt;</strong></span>,Job&gt;
<span class="bold"><strong>parallel_mismatch</strong></span>(Iterator1 begin1, Iterator1 end1,Iterator2 begin2,Func func, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

template &lt;class Iterator1,class Iterator2, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>std::pair&lt;Iterator1,Iterator2&gt;</strong></span>,Job&gt;
<span class="bold"><strong>parallel_mismatch</strong></span>(Iterator1 begin1, Iterator1 end1,Iterator2 begin2, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Return value</span>: std::pair with
                        iterators to the first two non-equivalent elements. If no mismatches are
                        found when the comparison reaches end1 or end2, whichever happens first, the
                        pair holds the end iterator and the corresponding iterator from the other
                        range. </p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin1, end1: the first range</p></li><li class="listitem"><p>begin2: the beginning of the second range</p></li><li class="listitem"><p>func: binary predicate function. The signature of the function
                                    should be equivalent to the following: bool pred(const Type
                                    &amp;a,const Type &amp;b);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_find_end"><div class="titlepage"><div><div><h3 class="title"><a name="d0e3520"></a><span class="command"><strong><a name="parallel_find_end"></a></strong></span>parallel_find_end</h3></div></div></div><p>Searches for the last subsequence of elements [begin2, end2) in the range
                        [begin1, end1). [begin2, end2) can be replaced (3rd form) by a
                        continuation.</p><pre class="programlisting">template &lt;class Iterator1,class Iterator2, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Iterator1</strong></span>,Job&gt;
<span class="bold"><strong>parallel_find_end</strong></span>(Iterator1 begin1, Iterator1 end1,Iterator2 begin2,Iterator2 end2, Func func, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

template &lt;class Iterator1,class Iterator2, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Iterator1</strong></span>,Job&gt;
<span class="bold"><strong>parallel_find_end</strong></span>(Iterator1 begin1, Iterator1 end1,Iterator2 begin2,Iterator2 end2, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

template &lt;class Iterator1,class Range,class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Iterator1</strong></span>,Job&gt;
<span class="bold"><strong>parallel_find_end</strong></span>(Iterator1 begin1, Iterator1 end1,Range range, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Return value</span>: Iterator to the
                        beginning of last subsequence [begin2, end2) in range [begin1, end1). If
                        [begin2, end2) is empty or if no such subsequence is found, end1 is
                        returned.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin1, end1: the first range</p></li><li class="listitem"><p>begin2, end2 / Range: the subsequence to look for.</p></li><li class="listitem"><p>func: binary predicate function. The signature of the function
                                    should be equivalent to the following: bool pred(const Type
                                    &amp;a,const Type &amp;b);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_find_first_of"><div class="titlepage"><div><div><h3 class="title"><a name="d0e3575"></a><span class="command"><strong><a name="parallel_find_first_of"></a></strong></span>parallel_find_first_of</h3></div></div></div><p>Searches the range [begin1, end1) for any of the elements in the range
                        [begin2, end2). [begin2, end2) can be replaced (3rd form) by a
                        continuation.</p><pre class="programlisting">template &lt;class Iterator1,class Iterator2, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Iterator1</strong></span>,Job&gt;
<span class="bold"><strong>parallel_find_first_of</strong></span>(Iterator1 begin1, Iterator1 end1,Iterator2 begin2,Iterator2 end2, Func func, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

template &lt;class Iterator1,class Iterator2, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Iterator1</strong></span>,Job&gt;
<span class="bold"><strong>parallel_find_first_of</strong></span>(Iterator1 begin1, Iterator1 end1,Iterator2 begin2,Iterator2 end2, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

template &lt;class Iterator1,class Range,class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Iterator1</strong></span>,Job&gt;
<span class="bold"><strong>parallel_find_first_of</strong></span>(Iterator1 begin1, Iterator1 end1,Range range, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Return value</span>: Iterator to the first
                        element in the range [begin1, end1) that is equal to an element from the
                        range [begin2; end2). If no such element is found, end1 is returned.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin1, end1: the first range</p></li><li class="listitem"><p>begin2, end2 / Range: the subsequence to look for.</p></li><li class="listitem"><p>func: binary predicate function. The signature of the function
                                    should be equivalent to the following: bool pred(const Type
                                    &amp;a,const Type &amp;b);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_adjacent_find"><div class="titlepage"><div><div><h3 class="title"><a name="d0e3630"></a><span class="command"><strong><a name="parallel_adjacent_find"></a></strong></span>parallel_adjacent_find</h3></div></div></div><p>Searches the range  [begin, end) for two consecutive identical
                        elements.</p><pre class="programlisting">template &lt;class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Iterator</strong></span>,Job&gt;
<span class="bold"><strong>parallel_adjacent_find</strong></span>(Iterator begin, Iterator end,Func func, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

template &lt;class Iterator, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Iterator</strong></span>,Job&gt;
<span class="bold"><strong>parallel_adjacent_find</strong></span>(Iterator begin, Iterator end, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Return value</span>: an iterator to the
                        first of the first pair of identical elements, that is, the first iterator
                        it such that *it == *(it+1) for the second version or func(*it, *(it + 1))
                        != false for the first version. If no such elements are found, last is
                        returned.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end: the range of elements to examine</p></li><li class="listitem"><p>func: binary predicate function. The signature of the function
                                    should be equivalent to the following: bool pred(const Type
                                    &amp;a,const Type &amp;b);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_lexicographical_compare"><div class="titlepage"><div><div><h3 class="title"><a name="d0e3676"></a><span class="command"><strong><a name="parallel_lexicographical_compare"></a></strong></span>parallel_lexicographical_compare</h3></div></div></div><p>Checks if the first range [begin1, end1) is lexicographically less than
                        the second range [begin2, end2).</p><pre class="programlisting">template &lt;class Iterator1,class Iterator2, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>bool</strong></span>,Job&gt;
<span class="bold"><strong>parallel_lexicographical_compare</strong></span>(Iterator1 begin1, Iterator1 end1,Iterator2 begin2,Iterator2 end2, Func func, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

template &lt;class Iterator1,class Iterator2, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>bool</strong></span>,Job&gt;
<span class="bold"><strong>parallel_lexicographical_compare</strong></span>(Iterator1 begin1, Iterator1 end1,Iterator2 begin2,Iterator2 end2, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Return value</span>: true if the first
                        range is lexicographically less than the second.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin1, end1: the first range of elements to examine </p></li><li class="listitem"><p>begin2, end2: the second range of elements to examine.</p></li><li class="listitem"><p>func: binary predicate function. The signature of the function
                                    should be equivalent to the following: bool pred(const Type
                                    &amp;a,const Type &amp;b);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_search"><div class="titlepage"><div><div><h3 class="title"><a name="d0e3725"></a><span class="command"><strong><a name="parallel_search"></a></strong></span>parallel_search</h3></div></div></div><p>Searches for the first occurrence of the subsequence of elements [begin2,
                        end2)in the range [begin1, end1 - (end2 - end1)).</p><pre class="programlisting">template &lt;class Iterator1,class Iterator2, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Iterator1</strong></span>,Job&gt;
<span class="bold"><strong>parallel_search</strong></span>(Iterator1 begin1, Iterator1 end1,Iterator2 begin2,Iterator2 end2, Func func, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

template &lt;class Iterator1,class Iterator2, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Iterator1</strong></span>,Job&gt;
<span class="bold"><strong>parallel_search</strong></span>(Iterator1 begin1, Iterator1 end1,Iterator2 begin2,Iterator2 end2, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

template &lt;class Iterator1,class Range,class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Iterator1</strong></span>,Job&gt;
<span class="bold"><strong>parallel_search</strong></span>(Iterator1 begin1, Iterator1 end1,Range range, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Return value</span>: Iterator to the
                        beginning of first subsequence [begin2, end2) in the range [begin1, end1 -
                        (end2 - begin2)). If no such subsequence is found, end1 is returned. If
                        [begin2, end2) is empty, begin1 is returned. </p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin1, end1: the first range</p></li><li class="listitem"><p>begin2, end2 / Range: the subsequence to look for.</p></li><li class="listitem"><p>func: binary predicate function. The signature of the function
                                    should be equivalent to the following: bool pred(const Type
                                    &amp;a,const Type &amp;b);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_search_n"><div class="titlepage"><div><div><h3 class="title"><a name="d0e3780"></a><span class="command"><strong><a name="parallel_search_n"></a></strong></span>parallel_search_n</h3></div></div></div><p>Searches the range [begin, end) for the first sequence of count identical
                        elements, each equal to the given value value.</p><pre class="programlisting">template &lt;class Iterator1, class Size, class T, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Iterator1</strong></span>,Job&gt;
<span class="bold"><strong>parallel_search_n</strong></span>(Iterator1 begin1, Iterator1 end1, Size count, const T&amp; value, Func func, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

template &lt;class Iterator1, class Size, class T, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Iterator1</strong></span>,Job&gt;
<span class="bold"><strong>parallel_search_n</strong></span>(Iterator1 begin1, Iterator1 end1, Size count, const T&amp; value, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Return value</span>: Iterator to the
                        beginning of the found sequence in the range [begin1, end1). If no such
                        sequence is found, end1 is returned. </p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin1, end1: the first range</p></li><li class="listitem"><p>count: the length of the sequence to search for</p></li><li class="listitem"><p>value: the value of the elements to search for </p></li><li class="listitem"><p>func: binary predicate which returns &#8203;true if the elements
                                    should be treated as equal. The signature of the function should
                                    be equivalent to the following: bool pred(const Type
                                    &amp;a,const Type &amp;b);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_scan"><div class="titlepage"><div><div><h3 class="title"><a name="d0e3832"></a><span class="command"><strong><a name="parallel_scan"></a></strong></span>parallel_scan</h3></div></div></div><p>Computes all partial reductions of a collection. For every output
                        position, a reduction of the input up to that point is computed. This is the
                        basic, most flexible underlying implementation of parallel_exclusive_scan,
                        parallel_inclusive_scan, parallel_transform_exclusive_scan,
                        parallel_transform_inclusive_scan.</p><p>The operator, represented by the Combine function, must be
                        associative.</p><p>The algorithm works by doing two passes on the sequence: the first pass
                        uses the Reduce function, the second pass uses the result of Reduce in
                        Combine. Scan will output the result.</p><pre class="programlisting">// version for iterators
template &lt;class Iterator, class OutIterator, class T, class Reduce, class Combine, class Scan, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;OutIterator,Job&gt;
<span class="bold"><strong>parallel_scan</strong></span>(Iterator beg, Iterator end, OutIterator out, T init, Reduce r, Combine c, Scan s, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for moved ranges
template &lt;class Range, class OutRange, class T, class Reduce, class Combine, class Scan, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>std::pair&lt;Range,OutRange&gt;</strong></span>,Job&gt;
<span class="bold"><strong>parallel_scan</strong></span>(Range&amp;&amp; range,OutRange&amp;&amp; out_range,T init,Reduce r, Combine c, Scan s, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for a single moved range (in/out)
template &lt;class Range, class T, class Reduce, class Combine, class Scan, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_scan</strong></span>(Range&amp;&amp; range,T init,Reduce r, Combine c, Scan s, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for continuation
template &lt;class Range, class T, class Reduce, class Combine, class Scan, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Unspecified-Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_scan</strong></span>(Range range,T init,Reduce r, Combine c, Scan s, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end / range: the range of elements to scan.</p></li><li class="listitem"><p>out: the beginning of the output range.</p></li><li class="listitem"><p>out_range: the output range.</p></li><li class="listitem"><p>init: initial value, combined with the first scanned
                                    element.</p></li><li class="listitem"><p>Reduce: binary predicate which returns an accumulated value.
                                    The signature of the function should be equivalent to the
                                    following: Ret reduce(Iterator,Iterator); The type Ret must be
                                    such that an object of type Iterator can be dereferenced and
                                    assigned a value of type Ret.</p></li><li class="listitem"><p>Combine: binary predicate which combines two elements, like
                                    std::plus would do. The signature of the function should be
                                    equivalent to the following: Ret combine(const Type&amp;,const
                                    Type&amp;); The type Ret must be such that an object of type
                                    Iterator can be dereferenced and assigned a value of type
                                    Ret.</p></li><li class="listitem"><p>Scan: assigns the result to the out range / iterator. The
                                    signature of the function should be equivalent to the following:
                                    void scan(Iterator beg, Iterator end, OutIterator out, T
                                    init).</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li><li class="listitem"><p>Last version returns a Range of the type returned by
                                    continuation</p></li></ul></div></div><div class="sect2" title="parallel_inclusive_scan"><div class="titlepage"><div><div><h3 class="title"><a name="d0e3905"></a><span class="command"><strong><a name="parallel_inclusive_scan"></a></strong></span>parallel_inclusive_scan</h3></div></div></div><p>Computes all partial reductions of a collection. For every output
                        position, a reduction of the input up to that point is computed. The nth
                        output value is a reduction over the first n input values.</p><pre class="programlisting">// version for iterators
template &lt;class Iterator, class OutIterator, class T, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;OutIterator,Job&gt;
<span class="bold"><strong>parallel_inclusive_scan</strong></span>(Iterator beg, Iterator end, OutIterator out, T init, Func f, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for moved ranges
template &lt;class Range, class OutRange, class T, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>std::pair&lt;Range,OutRange&gt;</strong></span>,Job&gt;
<span class="bold"><strong>parallel_inclusive_scan</strong></span>(Range&amp;&amp; range,OutRange&amp;&amp; out_range, T init, Func f, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for a single moved range (in/out)
template &lt;class Range, class T, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_inclusive_scan</strong></span>(Range&amp;&amp; range,T init,Func f, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for continuation
template &lt;class Range, class T, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Unspecified-Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_inclusive_scan</strong></span>(Range range,T init,Func f, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end / range: the range of elements to scan.</p></li><li class="listitem"><p>out: the beginning of the output range.</p></li><li class="listitem"><p>out_range: the output range.</p></li><li class="listitem"><p>init: initial value, combined with the first scanned
                                element.</p></li><li class="listitem"><p>Func: binary operation function object that will be applied.
                                    The signature of the function should be equivalent to the
                                    following: Ret fun(const Type &amp;a, const Type &amp;b); The
                                    type Ret must be such that an object of type  OutIterator can be
                                    dereferenced and assigned a value of type Ret</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li><li class="listitem"><p>Last version returns a Range of the type returned by
                                continuation</p></li></ul></div></div><div class="sect2" title="parallel_exclusive_scan"><div class="titlepage"><div><div><h3 class="title"><a name="d0e3968"></a><span class="command"><strong><a name="parallel_exclusive_scan"></a></strong></span>parallel_exclusive_scan</h3></div></div></div><p>Computes all partial reductions of a collection. For every output
                        position, a reduction of the input up to that point is computed. The nth
                        output value is a reduction over the first n- input values.</p><pre class="programlisting">// version for iterators
template &lt;class Iterator, class OutIterator, class T, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;OutIterator,Job&gt;
<span class="bold"><strong>parallel_exclusive_scan</strong></span>(Iterator beg, Iterator end, OutIterator out, T init, Func f, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for moved ranges
template &lt;class Range, class OutRange, class T, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>std::pair&lt;Range,OutRange&gt;</strong></span>,Job&gt;
<span class="bold"><strong>parallel_exclusive_scan</strong></span>(Range&amp;&amp; range,OutRange&amp;&amp; out_range, T init, Func f, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for a single moved range (in/out)
template &lt;class Range, class T, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_exclusive_scan</strong></span>(Range&amp;&amp; range,T init,Func f, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for continuation
template &lt;class Range, class T, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Unspecified-Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_exclusive_scan</strong></span>(Range range,T init,Func f, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end / range: the range of elements to scan.</p></li><li class="listitem"><p>out: the beginning of the output range.</p></li><li class="listitem"><p>out_range: the output range.</p></li><li class="listitem"><p>init: initial value, combined with the first scanned
                                element.</p></li><li class="listitem"><p>Func: binary operation function object that will be applied.
                                The signature of the function should be equivalent to the
                                following: Ret fun(const Type &amp;a, const Type &amp;b); The
                                type Ret must be such that an object of type  OutIterator can be
                                dereferenced and assigned a value of type Ret</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li><li class="listitem"><p>Last version returns a Range of the type returned by
                                continuation</p></li></ul></div></div><div class="sect2" title="parallel_copy"><div class="titlepage"><div><div><h3 class="title"><a name="d0e4031"></a><span class="command"><strong><a name="parallel_copy"></a></strong></span>parallel_copy</h3></div></div></div><p>Copies the elements in the range, defined by [begin, end), to another
                        range beginning at result. The order of the elements that are not removed is
                        preserved.</p><pre class="programlisting">// version for iterators
template &lt;class Iterator,class ResultIterator, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>void</strong></span>,Job&gt;
<span class="bold"><strong>parallel_copy</strong></span>(Iterator begin, Iterator end,ResultIterator result, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for moved range
template &lt;class Range,class ResultIterator, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>void</strong></span>,Job&gt;
<span class="bold"><strong>parallel_copy</strong></span>(Range&amp;&amp; range, ResultIterator result, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for continuation
template &lt;class Range,class ResultIterator, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>void</strong></span>,Job&gt;
<span class="bold"><strong>parallel_copy</strong></span>(Range range,ResultIterator out, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end / range: the range of elements to copy </p></li><li class="listitem"><p>result: the beginning of the destination range.</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_copy_if"><div class="titlepage"><div><div><h3 class="title"><a name="d0e4079"></a><span class="command"><strong><a name="parallel_copy_if"></a></strong></span>parallel_copy_if</h3></div></div></div><p>Copies copies the elements for which a predicate returns true. The order
                        of the elements that are not removed is preserved.</p><pre class="programlisting">template &lt;class Iterator,class ResultIterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>ResultIterator</strong></span>,Job&gt;
<span class="bold"><strong>parallel_copy_if</strong></span>(Iterator begin, Iterator end,ResultIterator result,Func func, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end: the range of elements to copy </p></li><li class="listitem"><p>result: the beginning of the destination range.</p></li><li class="listitem"><p>func: unary predicate which returns &#8203;true for the required
                                    elements. The signature of the function should be equivalent to
                                    the following: bool pred(const Type &amp;a);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_move"><div class="titlepage"><div><div><h3 class="title"><a name="d0e4118"></a><span class="command"><strong><a name="parallel_move"></a></strong></span>parallel_move</h3></div></div></div><p>Moves the elements in the range [begin, end), to another range beginning
                        at d_first. After this operation the elements in the moved-from range will
                        still contain valid values of the appropriate type, but not necessarily the
                        same values as before the move.</p><pre class="programlisting">// version for iterators
template &lt;class Iterator,class ResultIterator, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>void</strong></span>,Job&gt;
<span class="bold"><strong>parallel_move</strong></span>(Iterator begin, Iterator end,ResultIterator result, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for moved range
template &lt;class Range,class ResultIterator, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_move</strong></span>(Range&amp;&amp; range, ResultIterator result, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for continuation
template &lt;class Range,class ResultIterator, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Unspecified-Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_move</strong></span>(Range range,ResultIterator out, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end / range: the range of elements to move </p></li><li class="listitem"><p>result: the beginning of the destination range.</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li><li class="listitem"><p>3rd version returns a Range of the type returned by
                                    continuation</p></li></ul></div></div><div class="sect2" title="parallel_fill"><div class="titlepage"><div><div><h3 class="title"><a name="d0e4169"></a><span class="command"><strong><a name="parallel_fill"></a></strong></span>parallel_fill</h3></div></div></div><p>Assigns the given value to the elements in the range [begin, end). </p><pre class="programlisting">// version for iterators
template &lt;class Iterator, class Value, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>void</strong></span>,Job&gt;
<span class="bold"><strong>parallel_fill</strong></span>(Iterator begin, Iterator end,const Value&amp; value, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for moved range
template &lt;class Range, class Value, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_fill</strong></span>(Range&amp;&amp; range, const Value&amp; value, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for continuation
template &lt;class Range, class Value, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Unspecified-Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_fill</strong></span>(Range range,const Value&amp; value, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end / range: the range of elements to fill </p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li><li class="listitem"><p>3rd version returns a Range of the type returned by
                                    continuation</p></li></ul></div></div><div class="sect2" title="parallel_transform"><div class="titlepage"><div><div><h3 class="title"><a name="d0e4217"></a><span class="command"><strong><a name="parallel_transform"></a></strong></span>parallel_transform</h3></div></div></div><p>Applies a given function to one or a variadic number of ranges and stores
                        the result in another range. </p><pre class="programlisting">// version for iterators, one range tranformed to another
template &lt;class Iterator1, class ResultIterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>ResultIterator</strong></span>,Job&gt;
<span class="bold"><strong>parallel_transform</strong></span>(Iterator1 begin1, Iterator1 end1, ResultIterator result, Func func, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for iterators, two ranges tranformed to another
template &lt;class Iterator1, class Iterator2, class ResultIterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>ResultIterator</strong></span>,Job&gt;
<span class="bold"><strong>parallel_transform</strong></span>(Iterator1 begin1, Iterator1 end1, Iterator2 begin2, ResultIterator result, Func func, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for any number of iterators (not with ICC)
template &lt;class ResultIterator, class Func, class Job, class Iterator, class... Iterators&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>ResultIterator</strong></span>,Job&gt;
<span class="bold"><strong>parallel_transform</strong></span>(ResultIterator result, Func func, Iterator begin, Iterator end, Iterators... iterators, Func func, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Return value</span>: Result iterator to the
                        element past the last element transformed.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin1, end1, begin2, end2, iterators: the range of input
                                    elements </p></li><li class="listitem"><p>result: the beginning of the destination range.</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li><li class="listitem"><p>3rd version takes a variadic number of input ranges</p></li><li class="listitem"><p>func in first version : unary predicate which returns a new
                                    value. The signature of the function should be equivalent to the
                                    following: Ret pred(const Type &amp;a); The type Ret must be
                                    such that an object of type ResultIterator can be dereferenced
                                    and assigned a value of type Ret</p></li><li class="listitem"><p>func in second version : binary predicate which returns a new
                                    value. The signature of the function should be equivalent to the
                                    following: Ret pred(const Type1 &amp;a,const Type2 &amp;b); The
                                    type Ret must be such that an object of type ResultIterator can
                                    be dereferenced and assigned a value of type Ret</p></li><li class="listitem"><p>func in third version : n-ary predicate which returns a new
                                    value. The signature of the function should be equivalent to the
                                    following: Ret pred(const Type1 &amp;a,const Type2
                                    &amp;b...,const TypeN &amp;n); The type Ret must be such that an
                                    object of type ResultIterator can be dereferenced and assigned a
                                    value of type Ret</p></li></ul></div></div><div class="sect2" title="parallel_generate"><div class="titlepage"><div><div><h3 class="title"><a name="d0e4281"></a><span class="command"><strong><a name="parallel_generate"></a></strong></span>parallel_generate</h3></div></div></div><p>Assigns each element in range [begin, end) a value generated by the given
                        function object func.  </p><pre class="programlisting">// version for iterators
template &lt;class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>void</strong></span>,Job&gt;
<span class="bold"><strong>parallel_generate</strong></span>(Iterator begin, Iterator end,Func func, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for moved range
template &lt;class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_generate</strong></span>(Range&amp;&amp; range, Func func, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for continuation
template &lt;class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Unspecified-Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_generate</strong></span>(Range range,Func func, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end / range: the range of elements to assign </p></li><li class="listitem"><p>func: unary predicate which returns &#8203;a new value. The
                                    signature of the function should be equivalent to the following:
                                    Ret pred(); The type Ret must be such that an object of type
                                    Iterator can be dereferenced and assigned a value of type
                                    Ret.</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li><li class="listitem"><p>3rd version returns a Range of the type returned by
                                continuation</p></li></ul></div></div><div class="sect2" title="parallel_remove_copy / parallel_remove_copy_if"><div class="titlepage"><div><div><h3 class="title"><a name="d0e4332"></a><span class="command"><strong><a name="parallel_remove_copy"></a></strong></span>parallel_remove_copy / parallel_remove_copy_if</h3></div></div></div><p>Copies elements from the range [begin, end), to another range beginning at
                        out, omitting the elements which satisfy specific criteria. The first
                        version ignores the elements that are equal to value, the second version
                        ignores the elements for which predicate func returns true. Source and
                        destination ranges cannot overlap.</p><pre class="programlisting">template &lt;class Iterator,class Iterator2, class Value, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Iterator2</strong></span>,Job&gt;
<span class="bold"><strong>parallel_remove_copy</strong></span>(Iterator begin, Iterator end,Iterator2 out,Value const&amp; value, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

template &lt;class Iterator,class Iterator2, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Iterator2</strong></span>,Job&gt;
<span class="bold"><strong>parallel_remove_copy_if</strong></span>(Iterator begin, Iterator end,Iterator2 out,Func func, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Return value</span>: Iterator to the
                        element past the last element copied.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end: the range of elements to copy </p></li><li class="listitem"><p>out: the beginning of the destination range.</p></li><li class="listitem"><p>func: unary predicate which returns &#8203;true for the required
                                    elements. The signature of the function should be equivalent to
                                    the following: bool pred(const Type &amp;a);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_replace / parallel_replace_if"><div class="titlepage"><div><div><h3 class="title"><a name="d0e4381"></a><span class="command"><strong><a name="parallel_replace"></a></strong></span>parallel_replace /
                        parallel_replace_if</h3></div></div></div><p>Replaces all elements satisfying specific criteria with new_value in the
                        range [begin, end). The replace version replaces the elements that are equal
                        to old_value, the replace_if version replaces elements for which predicate
                        func returns true. </p><pre class="programlisting">// version for iterators
template &lt;class Iterator, class T, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>void</strong></span>,Job&gt;
<span class="bold"><strong>parallel_replace</strong></span>(Iterator begin, Iterator end, const T&amp; new_value, const T&amp; new_value, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);
template &lt;class Iterator, class T, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>void</strong></span>,Job&gt;
<span class="bold"><strong>parallel_replace_if</strong></span>(Iterator begin, Iterator end,Func func, const T&amp; new_value, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for moved range
template &lt;class Range, class T, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_replace</strong></span>(Range&amp;&amp; range, const T&amp; old_value, const T&amp; new_value, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);
template &lt;class Range, class T, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_replace_if</strong></span>(Range&amp;&amp; range, Func func, const T&amp; new_value, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for continuation
template &lt;class Range, class T, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Unspecified-Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_replace</strong></span>(Range range, const T&amp; old_value, const T&amp; new_value, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);
template &lt;class Range, class T, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Unspecified-Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_replace_if</strong></span>(Range range, Func func, const T&amp; new_value, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end / range: the range of elements to modify </p></li><li class="listitem"><p>func: unary predicate which returns &#8203;true if the element value
                                    should be replaced. The signature of the predicate function
                                    should be equivalent to the following: bool pred(const Type
                                    &amp;a);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li><li class="listitem"><p>continuation version returns a Range of the type returned by
                                    continuation</p></li></ul></div></div><div class="sect2" title="parallel_reverse"><div class="titlepage"><div><div><h3 class="title"><a name="d0e4450"></a><span class="command"><strong><a name="parallel_reverse"></a></strong></span>parallel_reverse</h3></div></div></div><p>Reverses the order of the elements in the range [begin, end).</p><pre class="programlisting">// version for iterators
template &lt;class Iterator, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>void</strong></span>,Job&gt;
<span class="bold"><strong>parallel_reverse</strong></span>(Iterator begin, Iterator end, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for moved range
template &lt;class Range, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_reverse</strong></span>(Range&amp;&amp; range, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for continuation
template &lt;class Range, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Unspecified-Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_reverse</strong></span>(Range range, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end: the range of elements to reverse </p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li><li class="listitem"><p>continuation version returns a Range of the type returned by
                                    continuation</p></li></ul></div></div><div class="sect2" title="parallel_swap_ranges"><div class="titlepage"><div><div><h3 class="title"><a name="d0e4498"></a><span class="command"><strong><a name="parallel_swap_ranges"></a></strong></span>parallel_swap_ranges</h3></div></div></div><p>Exchanges elements between range [begin1, end1) and another range starting
                        at begin2..</p><pre class="programlisting">template &lt;class Iterator1,class Iterator2, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Iterator2</strong></span>,Job&gt;
<span class="bold"><strong>parallel_swap_ranges</strong></span>(Iterator1 begin1, Iterator1 end1,Iterator2 begin2, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Return value</span>: true if the range
                        [begin, end) is empty or is partitioned by p. false otherwise.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end: the first range of elements to swap </p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_transform_inclusive_scan"><div class="titlepage"><div><div><h3 class="title"><a name="d0e4535"></a><span class="command"><strong><a name="parallel_transform_inclusive_scan"></a></strong></span>parallel_transform_inclusive_scan</h3></div></div></div><p>Computes all partial reductions of a collection. For every output
                        position, a reduction of the input up to that point is computed. The nth
                        output value is a reduction over the first n input values. This algorithm
                        applies a transformation to the input element before scan. Fusing transform
                        and scan avoids having to process two passes on the element and saves
                        time.</p><pre class="programlisting">// version for iterators
template &lt;class Iterator, class OutIterator, class T, class Func, class Transform, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;OutIterator,Job&gt;
<span class="bold"><strong>parallel_transform_inclusive_scan</strong></span>(Iterator beg, Iterator end, OutIterator out, T init, Func f, Transform t, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end / range: the range of elements to scan.</p></li><li class="listitem"><p>out: the beginning of the output range.</p></li><li class="listitem"><p>out_range: the output range.</p></li><li class="listitem"><p>init: initial value, combined with the first scanned
                                element.</p></li><li class="listitem"><p>Func: binary operation function object that will be applied.
                                The signature of the function should be equivalent to the
                                following: Ret fun(const Type &amp;a, const Type &amp;b); The
                                type Ret must be such that an object of type OutIterator can be
                                dereferenced and assigned a value of type Ret</p></li><li class="listitem"><p>Transform: unary operation function object that will be
                                applied. The signature of the function should be equivalent to
                                the following: Ret fun(const Type &amp;a); The type Ret must be
                                such that an object of type OutIterator can be dereferenced and
                                assigned a value of type Ret</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li><li class="listitem"><p>Last version returns a Range of the type returned by
                                continuation</p></li></ul></div></div><div class="sect2" title="parallel_transform_exclusive_scan"><div class="titlepage"><div><div><h3 class="title"><a name="d0e4583"></a><span class="command"><strong><a name="parallel_transform_exclusive_scan"></a></strong></span>parallel_transform_exclusive_scan</h3></div></div></div><p>Computes all partial reductions of a collection. For every output
                        position, a reduction of the input up to that point is computed. The nth
                        output value is a reduction over the first n-1 input values. This algorithm
                        applies a transformation to the input element before scan. Fusing transform
                        and scan avoids having to process two passes on the element and saves
                        time.</p><pre class="programlisting">// version for iterators
template &lt;class Iterator, class OutIterator, class T, class Func, class Transform, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;OutIterator,Job&gt;
<span class="bold"><strong>parallel_transform_exclusive_scan</strong></span>(Iterator beg, Iterator end, OutIterator out, T init, Func f, Transform t, long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The algorithm requires that the iterators stay valid until completion. It
                        is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end / range: the range of elements to scan.</p></li><li class="listitem"><p>out: the beginning of the output range.</p></li><li class="listitem"><p>out_range: the output range.</p></li><li class="listitem"><p>init: initial value, combined with the first scanned
                                element.</p></li><li class="listitem"><p>Func: binary operation function object that will be applied.
                                The signature of the function should be equivalent to the
                                following: Ret fun(const Type &amp;a, const Type &amp;b); The
                                type Ret must be such that an object of type OutIterator can be
                                dereferenced and assigned a value of type Ret</p></li><li class="listitem"><p>Transform: unary operation function object that will be
                                applied. The signature of the function should be equivalent to
                                the following: Ret fun(const Type &amp;a); The type Ret must be
                                such that an object of type OutIterator can be dereferenced and
                                assigned a value of type Ret</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li><li class="listitem"><p>Last version returns a Range of the type returned by
                                continuation</p></li></ul></div></div><div class="sect2" title="parallel_is_partitioned"><div class="titlepage"><div><div><h3 class="title"><a name="d0e4631"></a><span class="command"><strong><a name="parallel_is_partitioned"></a></strong></span>parallel_is_partitioned</h3></div></div></div><p>Returns true if all elements in the range [begin, end) that satisfy the
                        predicate func appear before all elements that don't. Also returns true if
                        [begin, end) is empty.  </p><pre class="programlisting">template &lt;class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>bool</strong></span>,Job&gt;
<span class="bold"><strong>parallel_is_partitioned</strong></span>(Iterator begin, Iterator end,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Return value</span>: true if unary
                        predicate returns true for all elements in the range, false otherwise.
                        Returns true if the range is empty.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end: the range of elements to search</p><p>Or a continuation, coming from another algorithm.</p></li><li class="listitem"><p>func: unary predicate function. The signature of the function
                                should be equivalent to the following: bool pred(const Type
                                &amp;a);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_partition"><div class="titlepage"><div><div><h3 class="title"><a name="d0e4673"></a><span class="command"><strong><a name="parallel_partition"></a></strong></span>parallel_partition</h3></div></div></div><p>Reorders the elements in the range [begin, end) in such a way that all
                        elements for which the predicate func returns true precede the elements for
                        which predicate func returns false </p><pre class="programlisting">// version with iterators
template &lt;class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Iterator</strong></span>,Job&gt;
<span class="bold"><strong>parallel_partition</strong></span>(Iterator begin, Iterator end,Func func,const uint32_t thread_num = boost::thread::hardware_concurrency(),const std::string&amp; task_name="", std::size_t prio=0);

// version with moved range
template &lt;class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>std::pair&lt;Range,decltype(range.begin())&gt;</strong></span>,Job&gt;
<span class="bold"><strong>parallel_partition</strong></span>(Range&amp;&amp; range,Func func,const uint32_t thread_num = boost::thread::hardware_concurrency(),const std::string&amp; task_name="", std::size_t prio=0);

// version with continuation
template &lt;class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>std::pair&lt;Unspecified-Range,Unspecified-Range-Iterator&gt;</strong></span>,Job&gt;
<span class="bold"><strong>parallel_partition</strong></span>(Range&amp;&amp; range,Func func,const uint32_t thread_num = boost::thread::hardware_concurrency(),const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Return value</span>: an iterator to the
                        first element of the second group for the first version. A pair of a range
                        and an iterator to the first element of the second group of this range for
                        the second and third.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end: the range of elements to partition</p><p>Or a continuation, coming from another algorithm.</p></li><li class="listitem"><p>func: unary predicate function. The signature of the function
                                    should be equivalent to the following: bool pred(const Type
                                    &amp;a);</p></li><li class="listitem"><p>thread_num: this algorithm being not a divide and conquer, it
                                    requires the number of threads which will be used. By default
                                    boost::thread::hardware_concurrency() is used.</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_stable_partition"><div class="titlepage"><div><div><h3 class="title"><a name="d0e4730"></a><span class="command"><strong><a name="parallel_stable_partition"></a></strong></span>parallel_stable_partition</h3></div></div></div><p>Reorders the elements in the range [begin, end) in such a way that all
                        elements for which the predicate func returns true precede the elements for
                        which predicate func returns false. Relative order of the elements is
                        preserved. </p><pre class="programlisting">template &lt;class Iterator, class Iterator2, class Func,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Iterator2</strong></span>,Job&gt;
<span class="bold"><strong>parallel_stable_partition</strong></span>(Iterator begin, Iterator end, Iterator2 out, Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version taking ownership of the container to be sorted
template &lt;class Range, class Func,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>&lt;std::pair&lt;Range,decltype(range.begin())&gt;</strong></span>,Job&gt;
<span class="bold"><strong>parallel_stable_partition</strong></span>(Range&amp;&amp; range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version taking a continuation of a range as first argument
template &lt;class Range, class Func,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>std::pair&lt;Unspecified Range, Unspecified-Iterator&gt;</strong></span>,Job&gt;
<span class="bold"><strong>parallel_stable_partition</strong></span>(Range range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);
</pre><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Returns</span>:</p><p>
                        </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>an iterator to the first element of the second group (first
                                    version)</p></li><li class="listitem"><p>The partitioned range and an iterator to the first element of
                                    the second group  (second version)</p></li><li class="listitem"><p>The partitioned range returned from the continuation and an
                                    iterator to the first element of the second group  (second
                                    version)</p></li></ul></div><p>
                    </p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end: the range of elements to reorder.</p><p>Or range: a moved range. Returns the sorted moved
                                    range.</p><p>Or a continuation, coming from another algorithm. Returns the
                                    sorted range.</p></li><li class="listitem"><p>func:  unary predicate which returns &#8203;true if the element
                                    should be ordered before other elements. The signature of the
                                    predicate function should be equivalent to the following: bool
                                    func(const Type &amp;a); </p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_partition_copy"><div class="titlepage"><div><div><h3 class="title"><a name="d0e4799"></a><span class="command"><strong><a name="parallel_partition_copy"></a></strong></span>parallel_partition_copy</h3></div></div></div><p>Copies the elements from the range [begin, end) to two output ranges in
                        such a way that all elements for which the predicate func returns true are
                        in the first range and all the others are in the second range. Relative
                        order of the elements is preserved. </p><pre class="programlisting">template &lt;class Iterator, class OutputIt1, class OutputIt2, class Func,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>std::pair&lt;OutputIt1, OutputIt2&gt;</strong></span>,Job&gt;
<span class="bold"><strong>parallel_partition_copy</strong></span>(Iterator begin, Iterator end, OutputIt1 out_true, OutputIt2 out_false, Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Returns</span>: a pair of iterators to the
                        first element of the first and second group</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end: the range of elements to reorder.</p></li><li class="listitem"><p>out_true: the beginning of the range for which func returns
                                    true.</p></li><li class="listitem"><p>out_false: the beginning of the range for which func returns
                                    false.</p></li><li class="listitem"><p>func: unary predicate which returns &#8203;true if the element should be
                                    in the first range. The signature of the predicate function
                                    should be equivalent to the following: bool func(const Type
                                    &amp;a); </p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_is_sorted"><div class="titlepage"><div><div><h3 class="title"><a name="d0e4845"></a><span class="command"><strong><a name="parallel_is_sorted"></a></strong></span>parallel_is_sorted</h3></div></div></div><p>Checks if the elements in range [first, last) are sorted in ascending
                        order. It uses the given comparison function func to compare the elements. </p><pre class="programlisting">template &lt;class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>bool</strong></span>,Job&gt;
<span class="bold"><strong>parallel_is_sorted</strong></span>(Iterator begin, Iterator end,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Return value</span>: true if the elements
                        in the range are sorted in ascending order.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end: the range of elements to search</p><p>Or a continuation, coming from another algorithm.</p></li><li class="listitem"><p>func: binary predicate function. The signature of the function
                                    should be equivalent to the following: bool func(const Type
                                    &amp;a, const Type &amp;b);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_is_reverse_sorted"><div class="titlepage"><div><div><h3 class="title"><a name="d0e4887"></a><span class="command"><strong><a name="parallel_is_reverse_sorted"></a></strong></span>parallel_is_reverse_sorted</h3></div></div></div><p>Checks if the elements in range [first, last) are sorted in descending
                        order. It uses the given comparison function func to compare the elements. </p><pre class="programlisting">template &lt;class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>bool</strong></span>,Job&gt;
<span class="bold"><strong>parallel_is_reverse_sorted</strong></span>(Iterator begin, Iterator end,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Return value</span>: true if the elements
                        in the range are sorted in descending order.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end: the range of elements to search</p><p>Or a continuation, coming from another algorithm.</p></li><li class="listitem"><p>func: binary predicate function. The signature of the function
                                should be equivalent to the following: bool func(const Type
                                &amp;a, const Type &amp;b);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_iota"><div class="titlepage"><div><div><h3 class="title"><a name="d0e4929"></a><span class="command"><strong><a name="parallel_iota"></a></strong></span>parallel_iota</h3></div></div></div><p>Fills the range [begin, end) with sequentially increasing values, starting
                        with value and repetitively evaluating ++value</p><pre class="programlisting">// version for iterators
template &lt;class Iterator, class T, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>void</strong></span>,Job&gt;
<span class="bold"><strong>parallel_iota</strong></span>(Iterator beg, Iterator end,T const&amp; value,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version taking a moved range
template &lt;class Range, class T, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_iota</strong></span>(Range&amp;&amp; range,T const&amp; value,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Returns</span>: a void continuation for the
                        first version, a continuation containing the new range for the second
                        one.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>beg, end: the range of elements</p><p>Or range: a moved range.</p></li><li class="listitem"><p>T value:  initial value to store, the expression ++value must
                                be well-formed </p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_reduce"><div class="titlepage"><div><div><h3 class="title"><a name="d0e4977"></a><span class="command"><strong><a name="parallel_reduce"></a></strong></span>parallel_reduce</h3></div></div></div><p><span class="underline">Description</span>: Sums up elements of a
                        range using func.</p><pre class="programlisting">template &lt;class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>decltype(func(...))</strong></span>,Job&gt;
<span class="bold"><strong>parallel_reduce</strong></span>(Iterator beg, Iterator end,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

template &lt;class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>decltype(func(...))</strong></span>,Job&gt;
<span class="bold"><strong>parallel_reduce</strong></span>(Range&amp;&amp; range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version taking a continuation of a range as first argument
template &lt;class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>decltype(func(...))</strong></span>,Job&gt;
<span class="bold"><strong>parallel_reduce</strong></span>(Range range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>beg, end: the range of elements to sum</p><p>Or range: a moved range.</p><p>Or a continuation, coming from another algorithm.</p></li><li class="listitem"><p>func: binary operation function object that will be applied.
                                    The signature of the function should be equivalent to the
                                    following: Ret fun(const Type &amp;a, const Type &amp;b);</p><p>Alternatively, func might have a signature with a range: Ret
                                    fun(Iterator a, Iterator b);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div><p><span class="underline">Returns</span>:  the return value type of
                        calling func.</p><p><span class="underline">Example</span>:</p><p>
                        </p><pre class="programlisting">std::vector&lt;int&gt; data;
post_callback(
    [this]()
    {
       return boost::asynchronous::<span class="bold"><strong>parallel_reduce</strong></span>(this-&gt;data.begin(),this-&gt;data.end(),
                                                     [](int const&amp; a, int const&amp; b)
                                                     {
                                                         return a + b; // returns an int
                                                     },
                                                     1500);
    },
    ](<span class="bold"><strong>boost::asynchronous::expected&lt;int&gt;</strong></span> ){} // callback gets an int
);</pre><p>
                    </p><p>We also have a <a class="link" href="examples/example_parallel_reduce_tcp.cpp" target="_top">distributed version</a> as an example, which strictly looks like the parallel_for version.</p></div><div class="sect2" title="parallel_inner_product"><div class="titlepage"><div><div><h3 class="title"><a name="d0e5057"></a><span class="command"><strong><a name="parallel_inner_product"></a></strong></span>parallel_inner_product</h3></div></div></div><p><span class="underline">Description</span>: Computes inner product
                        (i.e. sum of products) of the range [begin1, end1) and another range
                        beginning at begin2.It uses op and red for these tasks respectively. </p><pre class="programlisting">template &lt;class Iterator1, class Iterator2, class BinaryOperation, class Reduce, class Value, class Enable, class T, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>T</strong></span>,Job&gt;
<span class="bold"><strong>parallel_inner_product</strong></span>(Iterator1 begin1, Iterator1 end1, Iterator2 begin2, BinaryOperation op, Reduce red, const Value&amp; value,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

template &lt;class Range1, class Range2, class BinaryOperation, class Reduce, class Value, class Enable, class T, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>T</strong></span>,Job&gt;
<span class="bold"><strong>parallel_inner_product</strong></span>(Range1 &amp;&amp; range1, Range2 &amp;&amp; range2, BinaryOperation op, Reduce red, const Value&amp; value,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version taking continuations of ranges as first and second argument
template &lt;class Continuation1, class Continuation2, class BinaryOperation, class Reduce, class Value, class Enable, class T, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>T</strong></span>,Job&gt;
<span class="bold"><strong>parallel_inner_product</strong></span>(Continuation1 &amp;&amp; cont1, Continuation2 &amp;&amp; cont2, BinaryOperation op, Reduce red, const Value&amp; value,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin1, end1: the first range of elements </p><p>Or range1: a moved range.</p><p>Or a cont1, coming from another algorithm.</p></li><li class="listitem"><p>begin2, end2: the second range of elements </p><p>Or range2: a moved range.</p><p>Or a cont2, coming from another algorithm.</p></li><li class="listitem"><p>op:  binary operation function object that will be applied.
                                    This "sum" function takes a value returned by op2 and the
                                    current value of the accumulator and produces a new value to be
                                    stored in the accumulator. </p></li><li class="listitem"><p>red: binary operation function object that will be applied.
                                    This "product" function takes one value from each range and
                                    produces a new value. The signature of the function should be
                                    equivalent to the following: Ret fun(const Type1 &amp;a, const
                                    Type2 &amp;b); </p></li><li class="listitem"><p>value: initial value of the sum of the products </p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div><p><span class="underline">Returns</span>:  the return value type of
                        calling func.</p></div><div class="sect2" title="parallel_partial_sum"><div class="titlepage"><div><div><h3 class="title"><a name="d0e5128"></a><span class="command"><strong><a name="parallel_partial_sum"></a></strong></span>parallel_partial_sum</h3></div></div></div><p><span class="underline">Description</span>: Computes the partial
                        sums of the elements in the subranges of the range [begin, end) and writes
                        them to the range beginning at out. It uses the given binary function func
                        to sum up the elements.</p><pre class="programlisting">// version for iterators
template &lt;class Iterator, class OutIterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>OutIterator</strong></span>,Job&gt;
<span class="bold"><strong>parallel_partial_sum</strong></span>(Iterator beg, Iterator end, OutIterator out, Func func, long cutoff, const std::string&amp; task_name="", std::size_t prio=0);

// version for moved ranges
template &lt;class Range, class OutRange, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>std::pair&lt;Range,OutRange&gt;</strong></span>,Job&gt;
<span class="bold"><strong>parallel_partial_sum</strong></span>(Range&amp;&amp; range,OutRange&amp;&amp; out_range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version for a single moved range (in/out) =&gt; will return the range as continuation
template &lt;class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_partial_sum</strong></span>(Range&amp;&amp; range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version taking a continuation of a range as first argument
template &lt;class Range, class OutIterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Unspecified-Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_partial_sum</strong></span>(Range range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end: the range of elements to sum</p><p>Or range: a moved range.</p><p>Or a continuation, coming from another algorithm.</p></li><li class="listitem"><p>out_range: a moved range which will be returned upon
                                    completion</p></li><li class="listitem"><p>func: binary operation function object that will be applied.
                                The signature of the function should be equivalent to the
                                following: Ret fun(const Type &amp;a, const Type &amp;b);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div><p><span class="underline">Returns</span>: Iterator to the element
                        past the last element written in the first version, an iterator and the
                        passed moved range in the second, the passed range in the third, a range
                        returned by the continuation in the fourth.</p></div><div class="sect2" title="parallel_merge"><div class="titlepage"><div><div><h3 class="title"><a name="d0e5195"></a><span class="command"><strong><a name="parallel_merge"></a></strong></span>parallel_merge</h3></div></div></div><p><span class="underline">Description</span>: Merges two sorted
                        ranges [begin1, end1) and [begin2, end2) into one sorted range beginning at
                        out. It uses the given comparison function func to compare the elements. For
                        equivalent elements in the original two ranges, the elements from the first
                        range (preserving their original order) precede the elements from the second
                        range (preserving their original order). The behavior is undefined if the
                        destination range overlaps either of the input ranges (the input ranges may
                        overlap each other). </p><pre class="programlisting">template &lt;class Iterator1, class Iterator2, class OutIterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>void</strong></span>,Job&gt;
<span class="bold"><strong>parallel_merge</strong></span>(Iterator1 begin1, Iterator1 end1, Iterator2 beg2, Iterator2 end2, OutIterator out, Func func, long cutoff, const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin1, end1:  the first range of elements to merge.</p></li><li class="listitem"><p>begin2, end2:  the second range of elements to merge.</p></li><li class="listitem"><p>out:  the beginning of the destination range </p></li><li class="listitem"><p>func: comparison function object (i.e. an object that satisfies
                                    the requirements of Compare) which returns &#8203;true if the first
                                    argument is less than (i.e. is ordered before) the second. The
                                    signature of the comparison function should be equivalent to the
                                    following: bool func(const Type1 &amp;a, const Type2 &amp;b); </p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div><p><span class="underline">Returns</span>: A void continuation.</p></div><div class="sect2" title="parallel_invoke"><div class="titlepage"><div><div><h3 class="title"><a name="d0e5243"></a><span class="command"><strong><a name="parallel_invoke"></a></strong></span>parallel_invoke</h3></div></div></div><p>parallel_invoke invokes a variadic list of predicates in parallel and
                        returns a (continuation of) tuple of expected containing the result of all
                        of them. Functors have to be wrapped within a <span class="bold"><strong>boost::asynchronous::to_continuation_task</strong></span>.</p><pre class="programlisting">template &lt;class Job, typename... Args&gt;
boost::asynchronous::detail::<span class="bold"><strong>callback_continuation</strong></span>&lt;std::tuple&lt;expected&lt;return type of args&gt;...&gt;,Job&gt;
<span class="bold"><strong>parallel_invoke</strong></span>(Args&amp;&amp;... args);</pre><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>args: functors to call.</p></li></ul></div><p><span class="underline">Returns</span>:  an expected of tuple of
                        expected containing the result of every called functor.</p><p><span class="underline">Example</span>:</p><p>Of course, the futures can have exceptions if exceptions are thrown, as in
                        the following example:</p><pre class="programlisting">post_callback(
               []()
               {
                   return boost::asynchronous::parallel_invoke&lt;boost::asynchronous::any_callable&gt;(
                                     boost::asynchronous::<span class="bold"><strong>to_continuation_task</strong></span>([](){throw my_exception();}), // void lambda
                                     boost::asynchronous::<span class="bold"><strong>to_continuation_task</strong></span>([](){return 42.0;}));         // double lambda
                },// work
                // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
                [this](boost::<span class="bold"><strong>asynchronous::expected</strong></span>&lt;std::<span class="bold"><strong>tuple</strong></span>&lt;<span class="bold"><strong>asynchronous::expected&lt;void&gt;,asynchronous::expected&lt;double&gt;</strong></span>&gt;&gt; res)
                {
                   try
                   {
                        auto t = res.get();
                        std::cout &lt;&lt; "got result: " &lt;&lt; (<span class="bold"><strong>std::get&lt;1&gt;</strong></span>(t)).get() &lt;&lt; std::endl;                // 42.0
                        std::cout &lt;&lt; "got exception?: " &lt;&lt; (<span class="bold"><strong>std::get&lt;0&gt;</strong></span>(t)).has_exception() &lt;&lt; std::endl;  // true, has exception
                    }
                    catch(std::exception&amp; e)
                    {
                        std::cout &lt;&lt; "got exception: " &lt;&lt; e.what() &lt;&lt; std::endl;
                     }
                }// callback functor.
);</pre><p>Notice the use of <span class="bold"><strong>to_continuation_task</strong></span> to
                        convert the lambdas in continuations.</p><p>As always, the callback lambda will be called when all tasks complete and
                        the futures are non-blocking.</p><p>Please have a look at the <a class="link" href="examples/example_parallel_invoke.cpp" target="_top">complete
                            example</a>.</p></div><div class="sect2" title="if_then_else"><div class="titlepage"><div><div><h3 class="title"><a name="d0e5313"></a><span class="command"><strong><a name="if_then_else"></a></strong></span>if_then_else</h3></div></div></div><p><span class="underline">Description</span>: Executes a then or an
                        else clause passed as continuations depending on an if clause. If clause is
                        a functor returning a bool. Then and Else clauses are functors returning a
                        continuation. Typically, if_then_else is called after an algorithm returning
                        a continuation for further processing.</p><pre class="programlisting">template &lt;class IfClause, class ThenClause, class ElseClause, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
Unspecified-Continuation-Type
<span class="bold"><strong>if_then_else</strong></span>((IfClause if_clause, ThenClause then_clause, ElseClause else_clause, const std::string&amp; task_name="");</pre><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>if_clause: unary predicate function which returns a bool. The
                                    signature of the function should be equivalent to the following:
                                    bool pred(const Type &amp;a); where Type is returned by a
                                    previous algorithm</p></li><li class="listitem"><p>then_clause: unary predicate function which returns a
                                    continuation. The signature of the function should be equivalent
                                    to the following: Unspecified-Continuation pred(const Type
                                    &amp;a); where Type is returned by a previous algorithm.</p></li><li class="listitem"><p>else_clause: unary predicate function which returns a
                                    continuation. The signature of the function should be equivalent
                                    to the following: Unspecified-Continuation pred(const Type
                                    &amp;a); where Type is returned by a previous algorithm. </p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li></ul></div><p><span class="underline">Returns</span>: A continuation of the type
                        returned by then_clause or else_clause. Both must return the same
                        continuation type.</p><p><span class="underline">Example:</span> the following code calls a
                        parallel_for (1) based on the result of another algorithm, a parallel_for
                        (4). Both then_clause (2) and else_clause (3) will return a continuation
                        representing a different parallel_for. The if-clause (5) makes the
                        decision.</p><pre class="programlisting">post_callback(
           // task
           [this](){
                    // the outer algorithm (1). Called with a continuation returned by if_then_else
                    return boost::asynchronous::parallel_for(
                                    boost::asynchronous::if_then_else(
                                        // if clause (5). Here, always true.
                                        [](std::vector&lt;int&gt; const&amp;){return true;},
                                        // then clause. Will be called, as if-clause (5) returns true. Returns a continuation
                                        [](std::vector&lt;int&gt; res)
                                        {
                                            std::vector&lt;unsigned int&gt; new_result(res.begin(),res.end());
                                            // This algorithm (2) will be called after (4)
                                            return boost::asynchronous::parallel_for(
                                                                std::move(new_result),
                                                                [](unsigned int const&amp; i)
                                                                {
                                                                   const_cast&lt;unsigned int&amp;&gt;(i) += 3;
                                                                },1500);
                                        },
                                        // else clause. Will NOT be called, as if-clause returns true. Returns a continuation
                                        [](std::vector&lt;int&gt; res)
                                        {
                                            std::vector&lt;unsigned int&gt; new_result(res.begin(),res.end());
                                            // This algorithm (3) would be called after (4) if (5) returned false.
                                            return boost::asynchronous::parallel_for(
                                                                std::move(new_result),
                                                                [](unsigned int const&amp; i)
                                                                {
                                                                   const_cast&lt;unsigned int&amp;&gt;(i) += 4;
                                                                },1500);
                                        }
                                    )
                                    // argument of if_then_else, a continuation (4)
                                    (
                                        boost::asynchronous::parallel_for(
                                                                         std::move(this-&gt;m_data),
                                                                         [](int const&amp; i)
                                                                         {
                                                                            const_cast&lt;int&amp;&gt;(i) += 2;
                                                                         },1500)
                                    ),
                        [](unsigned int const&amp; i)
                        {
                           const_cast&lt;unsigned int&amp;&gt;(i) += 1;
                        },1500
                    );
                    },
           // callback functor
           [](boost::asynchronous::expected&lt;std::vector&lt;unsigned int&gt;&gt; res){
                        auto modified_vec = res.get();
                        auto it = modified_vec.begin();
                        BOOST_CHECK_MESSAGE(*it == 7,"data[0] is wrong: "+ std::to_string(*it));
                        std::advance(it,100);
                        BOOST_CHECK_MESSAGE(*it == 7,"data[100] is wrong: "+ std::to_string(*it));
                        std::advance(it,900);
                        BOOST_CHECK_MESSAGE(*it == 7,"data[1000] is wrong: "+ std::to_string(*it));
                        std::advance(it,8999);
                        BOOST_CHECK_MESSAGE(*it == 7,"data[9999] is wrong: "+ std::to_string(*it));
                        auto r = std::accumulate(modified_vec.begin(),modified_vec.end(),0,[](int a, int b){return a+b;});
                        BOOST_CHECK_MESSAGE((r == 70000),
                                            ("result of parallel_for after if/else was " + std::to_string(r) +
                                             ", should have been 70000"));
           }
        );</pre></div><div class="sect2" title="parallel_geometry_intersection_of_x"><div class="titlepage"><div><div><h3 class="title"><a name="d0e5353"></a><span class="command"><strong><a name="parallel_geometry_intersection_of_x"></a></strong></span>parallel_geometry_intersection_of_x</h3></div></div></div><p>Calculate the intersection of any number of (Boost.Geometry) geometries.
                        The free function intersection calculates the spatial set theoretic
                        intersection of geometries. </p><p>
                        </p><pre class="programlisting">template &lt;class Iterator, class Range,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Iterator-Reference</strong></span>,Job&gt;
<span class="bold"><strong>parallel_geometry_intersection_of_x</strong></span>(Iterator beg, Iterator end,const std::string&amp; task_name="", std::size_t prio=0, long cutoff=300, long overlay_cutoff=1500, long partition_cutoff=80000);

template &lt;class Range, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Range-Begin-Reference</strong></span>,Job&gt;
<span class="bold"><strong>parallel_geometry_intersection_of_x</strong></span>(Range&amp;&amp; range,const std::string&amp; task_name="", std::size_t prio=0, long cutoff=300, long overlay_cutoff=1500, long partition_cutoff=80000);

// version taking a continuation of a range as first argument
template &lt;class Range, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Unspecified-Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_geometry_intersection_of_x</strong></span>(Range range,const std::string&amp; task_name="", std::size_t prio=0, long cutoff=300, long overlay_cutoff=1500, long partition_cutoff=80000);</pre><p>
                    </p><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Returns</span>: a geometry of the type
                        referenced by the iterator / contained in the range</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>beg, end: the range of input geometries</p><p>Or range: a moved range.</p><p>Or a continuation, coming from another algorithm.</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk of
                                geometries</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li><li class="listitem"><p>overlay_cutoff: performance tuning. Cutoff used for the
                                parallelization of the internal overlay algorithm</p></li><li class="listitem"><p>partition_cutoff: performance tuning. Cutoff used for the
                                parallelization of the internal partition algorithm</p></li></ul></div></div><div class="sect2" title="parallel_geometry_union_of_x"><div class="titlepage"><div><div><h3 class="title"><a name="d0e5415"></a><span class="command"><strong><a name="parallel_geometry_union_of_x"></a></strong></span>parallel_geometry_union_of_x</h3></div></div></div><p>Combines any number of (Boost.Geometry) geometries which each other. he
                        free function union calculates the spatial set theoretic union of
                        geometries.</p><p>
                        </p><pre class="programlisting">template &lt;class Iterator, class Range,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_geometry_union_of_x</strong></span>(Iterator beg, Iterator end,const std::string&amp; task_name="", std::size_t prio=0, long cutoff=300, long overlay_cutoff=1500, long partition_cutoff=80000);

template &lt;class Range, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_geometry_union_of_x</strong></span>(Range&amp;&amp; range,const std::string&amp; task_name="", std::size_t prio=0, long cutoff=300, long overlay_cutoff=1500, long partition_cutoff=80000);

// version taking a continuation of a range as first argument
template &lt;class Range, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Unspecified-Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_geometry_union_of_x</strong></span>(Range range,const std::string&amp; task_name="", std::size_t prio=0, long cutoff=300, long overlay_cutoff=1500, long partition_cutoff=80000);</pre><p>
                    </p><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Returns</span>: a sequence containing the
                        resulting geometries</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>beg, end: the range of geometries to combine</p><p>Or range: a moved range.</p><p>Or a continuation, coming from another algorithm.</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk of
                                    geometries</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li><li class="listitem"><p>overlay_cutoff: performance tuning. Cutoff used for the
                                    parallelization of the internal overlay algorithm</p></li><li class="listitem"><p>partition_cutoff: performance tuning. Cutoff used for the
                                    parallelization of the internal partition algorithm</p></li></ul></div></div><div class="sect2" title="parallel_union"><div class="titlepage"><div><div><h3 class="title"><a name="d0e5477"></a><span class="command"><strong><a name="parallel_union"></a></strong></span>parallel_union</h3></div></div></div><p> Combines two geometries which each other. </p><p>
                        </p><pre class="programlisting">template &lt;class Geometry1, class Geometry2, class Collection,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Collection</strong></span>,Job&gt;
<span class="bold"><strong>parallel_union</strong></span>(Geometry1 geometry1,Geometry2 geometry2,long overlay_cutoff,long partition_cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>
                    </p><p><span class="underline">Returns</span>: a geometry. Currently, Type
                        of Geometry1= Type of Geometry2 = Type of Collection</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>geometry1, geometry2: the geometries to combine</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li><li class="listitem"><p>overlay_cutoff: performance tuning. Cutoff used for the
                                    parallelization of the internal overlay algorithm</p></li><li class="listitem"><p>partition_cutoff: performance tuning. Cutoff used for the
                                    parallelization of the internal partition algorithm</p></li></ul></div></div><div class="sect2" title="parallel_intersection"><div class="titlepage"><div><div><h3 class="title"><a name="d0e5518"></a><span class="command"><strong><a name="parallel_intersection"></a></strong></span>parallel_intersection</h3></div></div></div><p>Calculate the intersection of two geometries.</p><p>
                        </p><pre class="programlisting">template &lt;class Geometry1, class Geometry2, class Collection,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Collection</strong></span>,Job&gt;
<span class="bold"><strong>parallel_intersection</strong></span>(Geometry1 geometry1,Geometry2 geometry2,long overlay_cutoff,long partition_cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>
                    </p><p><span class="underline">Returns</span>: a geometry. Currently, Type
                        of Geometry1= Type of Geometry2 = Type of Collection</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>geometry1, geometry2: the geometries to combine</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li><li class="listitem"><p>overlay_cutoff: performance tuning. Cutoff used for the
                                    parallelization of the internal overlay algorithm</p></li><li class="listitem"><p>partition_cutoff: performance tuning. Cutoff used for the
                                    parallelization of the internal partition algorithm</p></li></ul></div></div><div class="sect2" title="parallel_find_all"><div class="titlepage"><div><div><h3 class="title"><a name="d0e5559"></a><span class="command"><strong><a name="parallel_find_all"></a></strong></span>parallel_find_all</h3></div></div></div><p>Finds and copies into a returned container all elements of a range for
                        which a predicate returns true. </p><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>beg, end: the range of elements to search</p><p>Or range: a moved range.</p><p>Or a continuation, coming from another algorithm.</p></li><li class="listitem"><p>func: unary predicate function which returns true for searched
                                    elements. The signature of the function should be equivalent to
                                    the following: bool pred(const Type &amp;a);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div><p><span class="underline">Returns</span>: a container with the
                        searched elements. Default is a std::vector for the iterator version, a
                        range of the same type as the input range for the others.</p><pre class="programlisting">template &lt;class Iterator, class Func,
          <span class="bold"><strong>class ReturnRange=std::vector&lt;...&gt;</strong></span>,
          class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;ReturnRange,Job&gt;
<span class="bold"><strong>parallel_find_all</strong></span>(Iterator beg, Iterator end,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

template &lt;class Range, class Func, <span class="bold"><strong>class ReturnRange=Range</strong></span>, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;ReturnRange,Job&gt;
<span class="bold"><strong>parallel_find_all</strong></span>(Range&amp;&amp; range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version taking a continuation of a range as first argument
template &lt;class Range, class Func, <span class="bold"><strong>class ReturnRange=typename Range::return_type</strong></span>, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;ReturnRange,Job&gt;
<span class="bold"><strong>parallel_find_all</strong></span>(Range range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p><span class="underline">Example</span>:</p><p>
                        </p><pre class="programlisting"><span class="bold"><strong>std::vector&lt;int&gt;</strong></span> data;
post_callback(
    [this]()
    {
       return boost::asynchronous::<span class="bold"><strong>parallel_find_all</strong></span>(this-&gt;data.begin(),this-&gt;data.end(),
                                                     [](int i)
                                                     {
                                                         return (400 &lt;= i) &amp;&amp; (i &lt; 600);
                                                     },
                                                     1500);
    },
    ](<span class="bold"><strong>boost::asynchronous::expected&lt;std::vector&lt;int&gt;&gt;</strong></span> ){}
);</pre><p>
                    </p><p>Please have a look at the <a class="link" href="examples/example_parallel_find_all.cpp" target="_top">complete example</a>.</p></div><div class="sect2" title="parallel_extremum"><div class="titlepage"><div><div><h3 class="title"><a name="d0e5637"></a><span class="command"><strong><a name="parallel_extremum"></a></strong></span>parallel_extremum</h3></div></div></div><p>parallel_extremum finds an extremum (min/max) of a range given by a
                        predicate.</p><p>
                        </p><pre class="programlisting">template &lt;class Iterator, class Func,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;typename std::iterator_traits&lt;Iterator&gt;::value_type,Job&gt;
<span class="bold"><strong>parallel_extremum</strong></span>(Iterator beg, Iterator end,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

template &lt;class Iterator, class Func,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
decltype(boost::asynchronous::parallel_reduce(...))
<span class="bold"><strong>parallel_extremum</strong></span>(Range&amp;&amp; range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version taking a continuation of a range as first argument
template &lt;class Iterator, class Func,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
decltype(boost::asynchronous::parallel_reduce(...))
<span class="bold"><strong>parallel_extremum</strong></span>(Range range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>
                    </p><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>beg, end: the range of elements to search</p><p>Or range: a moved range.</p><p>Or a continuation, coming from another algorithm.</p></li><li class="listitem"><p>func: binary predicate function which returns true for
                                    searched elements. The signature of the function should be
                                    equivalent to the following: bool func(const Type &amp;a, const
                                    Type &amp;b);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div><p>Please have a look at the <a class="link" href="examples/example_parallel_extremum.cpp" target="_top">complete
                            example</a>.</p></div><div class="sect2" title="parallel_count / parallel_count_if"><div class="titlepage"><div><div><h3 class="title"><a name="d0e5687"></a><span class="command"><strong><a name="parallel_count"></a></strong></span>parallel_count /
                        parallel_count_if</h3></div></div></div><p>parallel_count counts the elements of a range satisfying a
                        predicate.</p><pre class="programlisting">template &lt;class Iterator, class T,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>long</strong></span>,Job&gt;
<span class="bold"><strong>parallel_count</strong></span>(Iterator beg, Iterator end,T const&amp; value,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);
template &lt;class Iterator, class Func,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>long</strong></span>,Job&gt;
<span class="bold"><strong>parallel_count_if</strong></span>(Iterator beg, Iterator end,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

template &lt;class Range, class T,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>long</strong></span>,Job&gt;
<span class="bold"><strong>parallel_count</strong></span>(Range&amp;&amp; range,T const&amp; value,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);
template &lt;class Range, class Func,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
<span class="bold"><strong>parallel_count_if</strong></span>(Range&amp;&amp; range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version taking a continuation of a range as first argument
template &lt;class Range, class T,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>long</strong></span>,Job&gt;
<span class="bold"><strong>parallel_count</strong></span>(Range range,T const&amp; value,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);
template &lt;class Range, class Func,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>long</strong></span>,Job&gt;
<span class="bold"><strong>parallel_count_if</strong></span>(Range range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>beg, end: the range of elements</p><p>Or range: a moved range.</p><p>Or a continuation, coming from another algorithm.</p></li><li class="listitem"><p>T value: the value to search for</p></li><li class="listitem"><p>func: unary predicate function which returns true for searched
                                    elements. The signature of the function should be equivalent to
                                    the following: bool func(const Type &amp;a);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div><p>Please have a look at the <a class="link" href="examples/example_parallel_count.cpp" target="_top">complete
                        example</a>.</p></div><div class="sect2" title="parallel_sort / parallel_stable_sort / parallel_spreadsort / parallel_sort_inplace / parallel_stable_sort_inplace / parallel_spreadsort_inplace"><div class="titlepage"><div><div><h3 class="title"><a name="d0e5761"></a><span class="command"><strong><a name="parallel_sort"></a></strong></span>parallel_sort / parallel_stable_sort /
                        parallel_spreadsort / parallel_sort_inplace / parallel_stable_sort_inplace /
                        parallel_spreadsort_inplace</h3></div></div></div><p>parallel_sort / parallel_stable_sort implement a parallel mergesort.
                        parallel_spreadsort is a parallel version of Boost.Spreadsort if
                        BOOST_ASYNCHRONOUS_USE_BOOST_SPREADSORT is defined. They all use a parallel
                        mergesort. For the sequential part, parallel_sort uses std::sort,
                        parallel_stable_sort uses std::stable_sort, parallel_spreadsort uses
                        Boost.Spreadsort.</p><p>parallel_sort_inplace / parallel_stable_sort_inplace /
                        parallel_spreadsort_inplace use an inplace merge to save memory, at the cost
                        of a performance penalty. For the sequential part, parallel_sort_inplace
                        uses std::sort, parallel_stable_sort_inplace uses std::stable_sort,
                        parallel_spreadsort_inplace uses Boost.Spreadsort.</p><pre class="programlisting">template &lt;class Iterator, class Func,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>void</strong></span>,Job&gt;
<span class="bold"><strong>parallel_sort</strong></span>(Iterator beg, Iterator end,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);
<span class="bold"><strong>parallel_stable_sort</strong></span>(Iterator beg, Iterator end,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);
<span class="bold"><strong>parallel_spreadsort</strong></span>(Iterator beg, Iterator end,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);
<span class="bold"><strong>parallel_sort_inplace</strong></span>(Iterator beg, Iterator end,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);
<span class="bold"><strong>parallel_stable_sort_inplace</strong></span>(Iterator beg, Iterator end,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);
<span class="bold"><strong>parallel_spreadsort_inplace</strong></span>(Iterator beg, Iterator end,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version taking ownership of the container to be sorted
template &lt;class Iterator, class Func,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_sort</strong></span>(Range&amp;&amp; range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);
<span class="bold"><strong>parallel_stable_sort</strong></span>(Range&amp;&amp; range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);
<span class="bold"><strong>parallel_spreadsort</strong></span>(Range&amp;&amp; range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);
<span class="bold"><strong>parallel_sort_inplace</strong></span>(Range&amp;&amp; range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);
<span class="bold"><strong>parallel_stable_sort_inplace</strong></span>(Range&amp;&amp; range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);
<span class="bold"><strong>parallel_spreadsort_inplace</strong></span>(Range&amp;&amp; range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

// version taking a continuation of a range as first argument
template &lt;class Iterator, class Func,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>Range</strong></span>,Job&gt;
<span class="bold"><strong>parallel_sort</strong></span>(Range range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);
<span class="bold"><strong>parallel_stable_sort</strong></span>(Range range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);
<span class="bold"><strong>parallel_spreadsort</strong></span>(Range range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);
<span class="bold"><strong>parallel_sort_inplace</strong></span>(Range range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);
<span class="bold"><strong>parallel_stable_sort_inplace</strong></span>(Range range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);
<span class="bold"><strong>parallel_spreadsort_inplace</strong></span>(Range range,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>beg, end: the range of elements. Returns nothing.</p><p>Or range: a moved range. Returns the sorted moved
                                    range.</p><p>Or a continuation, coming from another algorithm. Returns the
                                    sorted range.</p></li><li class="listitem"><p>func: binary predicate function which returns true for
                                    searched elements. The signature of the function should be
                                    equivalent to the following: bool func(const Type &amp;a, const
                                    Type &amp;b);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div><p>Please have a look at the <a class="link" href="examples/example_parallel_count.cpp" target="_top">complete
                        example</a>.</p></div><div class="sect2" title="parallel_partial_sort"><div class="titlepage"><div><div><h3 class="title"><a name="d0e5864"></a><span class="command"><strong><a name="parallel_partial_sort"></a></strong></span>parallel_partial_sort</h3></div></div></div><p>Rearranges elements such that the range [begin, middle) contains the
                        sorted middle - begin smallest elements in the range [begin, end). The order
                        of equal elements is not guaranteed to be preserved. The order of the
                        remaining elements in the range [begin, end) is unspecified. It uses the
                        given comparison function func. </p><pre class="programlisting">template &lt;class Iterator, class Func,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>void</strong></span>,Job&gt;
<span class="bold"><strong>parallel_partial_sort</strong></span>(Iterator begin, Iterator middle, Iterator end,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end: the range of elements.</p></li><li class="listitem"><p>middle: until where the range will be sorted</p></li><li class="listitem"><p>func: binary predicate function which returns true for
                                    searched elements. The signature of the function should be
                                    equivalent to the following: bool func(const Type &amp;a, const
                                    Type &amp;b);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_quicksort / parallel_quick_spreadsort"><div class="titlepage"><div><div><h3 class="title"><a name="d0e5903"></a><span class="command"><strong><a name="parallel_quicksort"></a></strong></span>parallel_quicksort /
                        parallel_quick_spreadsort</h3></div></div></div><p>Sorts the range [begin,end) using quicksort. The order of the remaining
                        elements in the range [begin, end) is unspecified. It uses the given
                        comparison function func. parallel_quicksort will use std::sort to sort when
                        the algorithm finishes partitioning. parallel_quick_spreadsort will use
                        Boost.Spreadsort for this.</p><pre class="programlisting">template &lt;class Iterator, class Func,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>void</strong></span>,Job&gt;
<span class="bold"><strong>parallel_quicksort</strong></span>(Iterator begin, Iterator end,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);

template &lt;class Iterator, class Func,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>void</strong></span>,Job&gt;
<span class="bold"><strong>parallel_quick_spreadsort</strong></span>(Iterator begin, Iterator end,Func func,long cutoff,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end: the range of elements.</p></li><li class="listitem"><p>func: binary predicate function which returns true for
                                    searched elements. The signature of the function should be
                                    equivalent to the following: bool func(const Type &amp;a, const
                                    Type &amp;b);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler
                                    diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div><div class="sect2" title="parallel_nth_element"><div class="titlepage"><div><div><h3 class="title"><a name="d0e5945"></a><span class="command"><strong><a name="parallel_nth_element"></a></strong></span>parallel_nth_element</h3></div></div></div><p>nth_element is a partial sorting algorithm that rearranges elements in
                        [begin, end) such that: The element pointed at by nth is changed to whatever
                        element would occur in that position if [begin, end) was sorted. All of the
                        elements before this new nth element are less than or equal to the elements
                        after the new nth element.  It uses the given comparison function func. </p><pre class="programlisting">template &lt;class Iterator, class Func,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB&gt;
boost::asynchronous::detail::callback_continuation&lt;<span class="bold"><strong>void</strong></span>,Job&gt;
<span class="bold"><strong>parallel_nth_element</strong></span>(Iterator begin, Iterator nth, Iterator end,Func func,long cutoff,const uint32_t thread_num = 1,const std::string&amp; task_name="", std::size_t prio=0);</pre><p>The version taking iterators requires that the iterators stay valid until
                        completion. It is the programmer's job to ensure this.</p><p><span class="underline">Parameters</span>: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>begin, end: andom access iterators defining the range sort.</p></li><li class="listitem"><p>nth: random access iterator defining the sort partition
                                    point</p></li><li class="listitem"><p>func: binary predicate function which returns true for
                                searched elements. The signature of the function should be
                                equivalent to the following: bool func(const Type &amp;a, const
                                Type &amp;b);</p></li><li class="listitem"><p>cutoff: the maximum size of a sequential chunk</p></li><li class="listitem"><p>thread_num: the number of threads in the pool executing the
                                    algorithm</p></li><li class="listitem"><p>task_name: the name displayed in the scheduler diagnostics</p></li><li class="listitem"><p>prio: task priority </p></li></ul></div></div></div><div class="sect1" title="Parallel containers"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e5987"></a>Parallel containers</h2></div></div></div><p>Any gain made by using a parallel algorithm can be reduced to nothing if the
                    calling codes spends most of its time creating a std::vector. Interestingly,
                    most parallel libraries provide parallel algorithms, but very few offer parallel
                    data structures. This is unfortunate because a container can be parallelized
                    with a great gain as long as the contained type either has a non-simple
                    constructor / destructor or simply is big enough, as our tests show (see
                    test/perf/perf_vector.cpp). Though memory allocating is not parallel,
                    constructors can be made so. Reallocating and resizing, adding elements can also
                    greatly benefit.</p><p>Asynchronous fills this gap by providing boost::asynchronous::vector. It can
                    be used like std::vector by default.</p><p>However, it can also be used as a parallel, synchronous type if provided a
                    threadpool. Apart from the construction, it looks and feels very much like a
                    std::vector. In this case, <span class="underline">it cannot be posted to its
                        own threadpool without releasing it</span> (see release_scheduler /
                    set_scheduler) as it would create a cycle, and therefore a possible deadlock. It
                    is defined in:</p><p>#include &lt;boost/asynchronous/container/vector.hpp&gt; </p><p>The vector supports the same constructors that std::vector, with as extra
                    parameters, the threadpool for parallel work, and a cutoff. Optionally, a name
                    used for logging and a threadpool priority can be given, for example:</p><pre class="programlisting">struct LongOne;
boost::asynchronous::any_shared_scheduler_proxy&lt;&gt; pool = 
               boost::asynchronous::make_shared_scheduler_proxy&lt;
                    boost::asynchronous::multiqueue_threadpool_scheduler&lt;
                            boost::asynchronous::lockfree_queue&lt;&gt;
                        &gt;&gt;(tpsize,tasks);

<span class="bold"><strong>boost::asynchronous::vector</strong></span>&lt;LongOne&gt; vec (pool,1024 /* cutoff */, /* std::vector ctor arguments */ 10000,LongOne()
                                            // optional, name for logging, priority in the threadpool
                                            , "vector", 1);                </pre><p>At this point, asynchronous::vector can be used like std::vector, with the
                    difference that constructor, destructor, operator=, assign, clear, push_back,
                    emplace_back, reserve, resize, erase, insert are executed in parallel in the
                    given threadpool.</p><p>The vector adds a few members compared to std::vector:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>release_scheduler(): removes the threadpool from vector. At this
                                point, the vector is no more parallel, but can live from within the
                                pool.</p></li><li class="listitem"><p>set_scheduler(): (re)sets scheduler, so that vector is again
                                parallel. At this point, the vector cannot live from within the
                                pool.</p></li><li class="listitem"><p>long get_cutoff() const: returns the cutoff as given in
                                constructor.</p></li><li class="listitem"><p>std::string get_name() const: the logged name, as given in the
                                constructor.</p></li><li class="listitem"><p>std::size_t get_prio()const: the priority, as given in the
                                constructor.</p></li></ul></div><p><a class="link" href="examples/example_vector.cpp" target="_top">This example</a> displays
                    some basic usage of vector.</p><div class="table"><a name="d0e6032"></a><p class="title"><b>Table&nbsp;3.10.&nbsp;#include
                        &lt;boost/asynchronous/container/vector.hpp&gt;</b></p><div class="table-contents"><table summary="#include&#xA;                        <boost/asynchronous/container/vector.hpp&gt;" border="1"><colgroup><col><col><col></colgroup><thead><tr><th>Public Member functions as in std::vector</th><th>Description</th><th>Parallel if threadpool?</th></tr></thead><tbody><tr><td>(constructor)</td><td>constructs the vector</td><td>Yes</td></tr><tr><td>(destructor)</td><td>destructs the vector</td><td>Yes</td></tr><tr><td>operator=</td><td>assigns values to the container</td><td>Yes</td></tr><tr><td>assign</td><td>assigns values to the container </td><td>Yes</td></tr><tr><td>get_allocator</td><td>returns the associated allocator </td><td>No</td></tr><tr><td>at</td><td>access specified element with bounds checking </td><td>No</td></tr><tr><td>operator[]</td><td>access specified element </td><td>No</td></tr><tr><td>front</td><td>access the first element </td><td>No</td></tr><tr><td>back</td><td>access the last element </td><td>No</td></tr><tr><td>data</td><td>direct access to the underlying array </td><td>No</td></tr><tr><td>begin / cbegin </td><td>returns an iterator to the beginning</td><td>No</td></tr><tr><td>end / cend </td><td>returns an iterator to the end </td><td>No</td></tr><tr><td>rbegin / crbegin </td><td>returns a reverse iterator to the beginning </td><td>No</td></tr><tr><td>rend / crend </td><td>returns a reverse iterator to the end </td><td>No</td></tr><tr><td>empty</td><td>checks whether the container is empty </td><td>No</td></tr><tr><td>size</td><td>returns the number of elements </td><td>No</td></tr><tr><td>max_size</td><td>returns the maximum possible number of elements</td><td>No</td></tr><tr><td>reserve</td><td>reserves storage </td><td>Yes</td></tr><tr><td>capacity</td><td>returns the number of elements that can be held in currently
                                    allocated storage </td><td>Yes</td></tr><tr><td>shrink_to_fit </td><td>reduces memory usage by freeing unused memory</td><td>Yes</td></tr><tr><td>clear</td><td>clears the contents </td><td>Yes</td></tr><tr><td>insert</td><td>inserts elements </td><td>Yes</td></tr><tr><td>emplace</td><td>constructs element in-place </td><td>Yes</td></tr><tr><td>erase</td><td>erases elements</td><td>Yes</td></tr><tr><td>push_back</td><td>adds an element to the end </td><td>Yes</td></tr><tr><td>emplace_back</td><td>constructs an element in-place at the end </td><td>Yes</td></tr><tr><td>pop_back</td><td>removes the last element</td><td>No</td></tr><tr><td>resize</td><td>changes the number of elements stored</td><td>Yes</td></tr><tr><td>swap</td><td>swaps the contents </td><td>No</td></tr><tr><td>operator==</td><td>lexicographically compares the values in the vector </td><td>Yes</td></tr><tr><td>operator!=</td><td>lexicographically compares the values in the vector </td><td>Yes</td></tr><tr><td>operator&lt; </td><td>lexicographically compares the values in the vector </td><td>Yes</td></tr><tr><td>operator&lt;= </td><td>lexicographically compares the values in the vector </td><td>Yes</td></tr><tr><td>operator&gt; </td><td>lexicographically compares the values in the vector </td><td>Yes</td></tr><tr><td>operator&gt;=</td><td>lexicographically compares the values in the vector </td><td>Yes</td></tr></tbody></table></div></div><br class="table-break"><p>All these members have the same signature as std::vector. Only some
                    constructors are new. First the standard ones:</p><pre class="programlisting">         vector();

explicit vector( const Allocator&amp; alloc );	

         vector( size_type count,
                 const T&amp; value = T(),
                 const Allocator&amp; alloc = Allocator());

template&lt; class InputIt &gt;
vector( InputIt first, InputIt last,
        const Allocator&amp; alloc = Allocator() );

vector( const vector&amp; other );

vector( vector&amp;&amp; other )

vector( std::initializer_list&lt;T&gt; init,
        const Allocator&amp; alloc = Allocator() );</pre><p>There are variants taking a scheduler making them a servant with parallel capabilities:</p><pre class="programlisting">explicit vector(boost::asynchronous::any_shared_scheduler_proxy&lt;Job&gt; scheduler,
                long cutoff,
                const std::string&amp; task_name="", 
                std::size_t prio=0,
                const Alloc&amp; alloc = Alloc());

explicit vector( boost::asynchronous::any_shared_scheduler_proxy&lt;Job&gt; scheduler,
                 long cutoff,
                 size_type count,
                 const std::string&amp; task_name="", 
                 std::size_t prio=0,
                 const Allocator&amp; alloc = Allocator());
	
explicit vector( boost::asynchronous::any_shared_scheduler_proxy&lt;Job&gt; scheduler,
                 long cutoff,
                 size_type count,
                 const T&amp; value = T(),
                 const std::string&amp; task_name="", 
                 std::size_t prio=0,
                 const Allocator&amp; alloc = Allocator());

         template&lt; class InputIt &gt;
         vector( boost::asynchronous::any_shared_scheduler_proxy&lt;Job&gt; scheduler,
                 long cutoff,
                 InputIt first, InputIt last,
                 const std::string&amp; task_name="", 
                 std::size_t prio=0,
                 const Allocator&amp; alloc = Allocator() );

         vector( boost::asynchronous::any_shared_scheduler_proxy&lt;Job&gt; scheduler,
                 long cutoff,
                 std::initializer_list&lt;T&gt; init,
                 const std::string&amp; task_name="", 
                 std::size_t prio=0,
                 const Allocator&amp; alloc = Allocator() );</pre><p>Some new members have also been added to handle the new functionality:</p><div class="table"><a name="d0e6303"></a><p class="title"><b>Table&nbsp;3.11.&nbsp;#include
                        &lt;boost/asynchronous/container/vector.hpp&gt;</b></p><div class="table-contents"><table summary="#include&#xA;                        <boost/asynchronous/container/vector.hpp&gt;" border="1"><colgroup><col><col><col></colgroup><thead><tr><th>Public Member functions as in std::vector</th><th>Description</th><th>Parallel if threadpool?</th></tr></thead><tbody><tr><td>void
                                    set_scheduler(any_shared_scheduler_proxy&lt;Job&gt;)</td><td>adds / replaces the scheduler pool</td><td>No</td></tr><tr><td>void release_scheduler()</td><td>Removes the scheduler pool. vector is now "standard"</td><td>No</td></tr><tr><td>long get_cutoff()const</td><td>returns cutoff</td><td>No</td></tr><tr><td>void set_cutoff(long)</td><td>sets cutoff </td><td>No</td></tr><tr><td>std::string get_name() const</td><td>returns vector name, used in task names</td><td>No</td></tr><tr><td>void set_name(std::string const&amp;)</td><td>sets vector name, used in task names</td><td>No</td></tr><tr><td>std::size_t get_prio()const</td><td>returns vector task priority in threadpool</td><td>No</td></tr><tr><td>void set_prio(std::size_t)</td><td>set vector task priority in threadpool</td><td>No</td></tr></tbody></table></div></div><br class="table-break"></div></div><div class="chapter" title="Chapter&nbsp;4.&nbsp;Tips."><div class="titlepage"><div><div><h2 class="title"><a name="d0e6375"></a>Chapter&nbsp;4.&nbsp;Tips.</h2></div></div></div><div class="toc"><p><b>Table of Contents</b></p><dl><dt><span class="sect1"><a href="#d0e6378">Which protections you get, which ones you don't.</a></span></dt><dt><span class="sect1"><a href="#d0e6416">No cycle, ever</a></span></dt><dt><span class="sect1"><a href="#d0e6425">No "this" within a task.</a></span></dt></dl></div><div class="sect1" title="Which protections you get, which ones you don't."><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e6378"></a>Which protections you get, which ones you don't.</h2></div></div></div><p>Asynchronous is doing much to protect developers from some ugly beasts around:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>(visible) threads</p></li><li class="listitem"><p>races</p></li><li class="listitem"><p>deadlocks</p></li><li class="listitem"><p>crashes at the end of an object lifetime</p></li></ul></div><p>It also helps parallelizing and improve performance by not blocking. It also
                    helps find out where bottlenecks and hidden possible performance gains
                    are.</p><p>There are, however, things for which it cannot help:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>cycles in design</p></li><li class="listitem"><p>C++ legal ways to work around the protections if one really
                                wants.</p></li><li class="listitem"><p>blocking on a future if one really wants.</p></li><li class="listitem"><p>using "this" captured in a task lambda.</p></li><li class="listitem"><p>writing a not clean task with pointers or references to data used
                                in a servant.</p></li></ul></div></div><div class="sect1" title="No cycle, ever"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e6416"></a>No cycle, ever</h2></div></div></div><p>This is one of the first things one learns in a design class. Cycles are
                    evil. Everybody knows it. And yet, designs are often made without care in a too
                    agile process, dependency within an application is not thought out carefully
                    enough and cycles happen. What we do learn in these classes is that cycles make
                    our code monolithic and not reusable. What we however do not learn is how bad,
                    bad, bad this is in face of threads. It becomes impossible to follow the flow of
                    information, resource usage, degradation of performance. But the worst of all,
                    it becomes almost impossible to prevent deadlocks and resource leakage.</p><p>Using Asynchronous will help write clean layered architectures. But it will
                    not replace carefully crafted designs, thinking before writing code and the
                    experience which make a good designer. Asynchronous will not be able to prevent
                    code having cycles in a design. </p><p>Fortunately, there is an easy solution: back to the basics, well-thought
                    designs before coding, writing diagrams, using a real development process (hint:
                    an agile "process" is not all this in the author's mind).</p></div><div class="sect1" title="No &#34;this&#34; within a task."><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e6425"></a>No "this" within a task.</h2></div></div></div><p>A very easy way to see if you are paving the way to a race even using
                    Asynchronous is to have a look at the captured variables of a lambda posted to a
                    threadpool. If you find "this", it's probably bad, unless you really know that
                    the single-thread code will do nothing. Apart from a simple application, this
                    will not be true. By extension, pointers, references, or even shared smart
                    pointers pointing to data living in a single-thread world is usually bad.</p><p>Experience shows that there are only two safe way to pass data to a posted
                    task: copy for basic types or types having a trivial destructor and move for
                    everything else. Keep to this rule and you will be safe.</p><p>On the other hand, "this" is okay in the capture list of a callback task as
                    Asynchronous will only call it if the servant is still alive.</p></div></div><div class="chapter" title="Chapter&nbsp;5.&nbsp;Design examples"><div class="titlepage"><div><div><h2 class="title"><a name="d0e6434"></a>Chapter&nbsp;5.&nbsp;Design examples</h2></div></div></div><div class="toc"><p><b>Table of Contents</b></p><dl><dt><span class="sect1"><a href="#d0e6439">A state machine managing a succession of tasks</a></span></dt><dt><span class="sect1"><a href="#d0e6484">A layered application</a></span></dt><dt><span class="sect1"><a href="#d0e6541">Boost.Meta State Machine and Asynchronous behind a Qt User Interface </a></span></dt></dl></div><p>This section shows some examples of strongly simplified designs using
                Asynchronous.</p><div class="sect1" title="A state machine managing a succession of tasks"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e6439"></a>A state machine managing a succession of tasks</h2></div></div></div><p>This example will show how the library allows a solution more powerful than
                    the proposed future.then() extension.</p><p>Futures returned by an asynchronous function like std::async have to be
                    get()-ed by the caller before a next task can be started. To overcome this
                    limitation, a solution is to add a then(some_functor) member to futures. The
                    idea is to chain several tasks without having to get() the first future. While
                    this provides some improvement, some serious limitations stay:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>A future still has to be waited for</p></li><li class="listitem"><p>The then-functor is executed in one of the worker threads</p></li><li class="listitem"><p>The then-functor has to be complete at the first call. By complete
                                we mean that it should not use any data from the caller thread to
                                avoid races.</p></li></ul></div><p>All this makes from the then functor a fire-and-forget one and prevents
                    reacting on changes happening between the first functor and the then
                    functor.</p><p>A superior solution exists using Asynchronous schedulers. <a class="link" href="examples/example_callback.cpp" target="_top">In this example</a>, we
                    define a Manager object, which lives in his single thread scheduler. This
                    Manager, a simplified state machine, starts a task when asked (calling start()
                    on its proxy). Upon completing the first task, the Manager chooses to start or
                    not the second part of the calculation (what would be done in future.then()). In
                    our example, an event cancels the second part (calling cancel()) so that it
                    never starts. </p><p>Notice in this example some important points:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>The time we choose to start or not the second part is done after
                                the first part completes, not before, which is a noticeable
                                improvement compared to future.then().</p></li><li class="listitem"><p>We have no race although no mutex is used and at least three
                                threads are implied (main / 2 std::thread / Manager's world /
                                threadpool). All events (start / cancel /completion of first task /
                                completion of second task) are done within the Manager thread
                                world.</p></li><li class="listitem"><p>A proxy object can be copied and the copies used safely in any
                                number of threads. The proxy is copied, the thread world no. We use
                                in our example 2 std::threads which both share the proxy (and share
                                control of the thread world's lifecycle) with the main thread and
                                access members of the servant, all safely. The last thread going
                                will cause the thread world to shutdown. </p></li><li class="listitem"><p>Thinking in general design, this is very powerful. Software is
                                usually designed in components, which one tries to make reusable.
                                This is usually difficult because of thread issues. This problem is
                                now gone. The component delimited by one (or several) proxy is safe,
                                completely reusable and its thread limits are well defined.</p></li><li class="listitem"><p>We have in this example only one servant and its proxy. It would
                                be easily possible to define any number of pair of those. In this
                                case, the last proxy destroyed would shut down the thread
                                world.</p></li></ul></div><p><a class="link" href="examples/example_callback_msm.cpp" target="_top">We can also write the same example using a real Boost.MSM state machine</a></p></div><div class="sect1" title="A layered application"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e6484"></a>A layered application</h2></div></div></div><p>A common design pattern for an application is organizing it into layers. We
                    suppose we are having three layers:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>TopLevel: what the user of the application is seeing</p></li><li class="listitem"><p>MiddleLevel: some internal business logic</p></li><li class="listitem"><p>LowLevel: communication layer</p></li></ul></div><p>This is a common design in lots of books. A top level layer receives user
                    commands , checks for their syntax, then delegates to a middle layer, composed
                    of business logic, checking whether the application is in a state to execute the
                    order. If it is, the low-level communication task is delegated to a low level
                    layer. </p><p>Of course this example is strongly simplified. A layer can be made of hundreds
                    of objects and thousands of lines of code.</p><p>What the books often ignore, however, are threads and lifecycle issues.
                    Non-trivial applications are likely to be running many threads. This is where
                    the problems start:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>Which layer is running into which thread?</p></li><li class="listitem"><p>How do to avoid races if each layer is running his own thread(s).
                                Usually, mutexes are used.</p></li><li class="listitem"><p>How to handle callbacks from lower layers, as these are likely to
                                be executed in the lower layer thread(s)</p></li><li class="listitem"><p>Lifecycles. Usually, each layer has responsibility of keeping his
                                lower layers alive. But how to handle destruction of higher-levels?
                                Callbacks might already be under way and they will soon meet a
                                destroyed mutex?</p></li></ul></div><p><span class="inlinemediaobject"><img src="pics/layers.jpg"></span></p><p>Asychronous provides a solution to these problems:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>Each layer is living in its own thread world (or sharing
                                one).</p></li><li class="listitem"><p>Asynchronous guarantees that each layer will be destroyed in turn:
                                LowLevel - MiddleLevel - TopLevel.</p></li><li class="listitem"><p>Asynchronous provides proxies to serialize outside calls into a
                                servant thread world.</p></li><li class="listitem"><p>Asynchronous provides safe callbacks: each servant can use
                                make_safe_calback, which guarantees execution in the servant thread
                                if and only if the servant is still alive.</p></li></ul></div><p><a class="link" href="examples/example_layers.cpp" target="_top">In this simplified
                        example</a>, each layer has its own thread world. Using the proxies
                    provided by the library, each servant is protected from races through calls from
                    their upper layer or the outside world (main). Servants are also protected from
                    callbacks from their lower layer by make_safe_callback.</p></div><div class="sect1" title="Boost.Meta State Machine and Asynchronous behind a Qt User Interface"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e6541"></a>Boost.Meta State Machine and Asynchronous behind a Qt User Interface </h2></div></div></div><p>We will now implement an application closer to a real-life case. The code is
                    to be found under asynchronous/libs/asynchronous/doc/examples/msmplayer/
                    .</p><p>The Boost.MSM documentation introduces a CD player implementation. This CD
                    player reacts to events coming from a hardware: opening of the player, adding a
                    disc, playing a song, increasing volume, etc. This logic is implemented in the
                    logic layer, which mostly delegates it to a state machine based on Boost.MSM.
                    The file playerlogic.cpp implements this state machine, which is an extension of
                    the ones found in the Boost.MSM documentation.</p><p>This logic is to be thought of as a reusable component, which must be
                    thread-safe, living in a clearly defined thread context. Asychronous handles
                    this: the PlayerLogic class is a servant, protected by a proxy,
                    PlayerLogicProxy, which in its constructor creates the necessary single-threaded
                    scheduler. At this point, we have a self-sufficient component.</p><p>Supposing that, like often in real-life, that the hardware is not ready while
                    our software would be ready for testing, we decide to build a Qt application,
                    acting as a hardware simulator. This also allows fast prototyping, early
                    testing, writing of training material and concept-checking by the different
                    stakeholders.</p><p>To achieve this, we write a QWidget-derived class named PlayerGui, a simple
                    interface simulating the controls which will be offered by the real CD player.
                    It implements IDisplay, the interface which the real CD player will provide. </p><p>The real hardware will also implement IHardware, an interface allowing control
                    of the buttons and motors of the player. Our simple PlayerGui will also
                    implement it for simplicity.</p><p>A Qt application is by definition asynchronous. Boost.Asynchronous provides
                    qt_servant, allowing a Qt object to make us of the library features (safe
                    callbacks, threadpools, etc.).</p><p>The application is straightforward: PlayerGui creates the logic of the
                    application, which means constructing a PlayerLogicProxy object. After the
                    construction, we have a usable, movable, perfectly asynchronous component, which
                    means being based on an almost 0 time run-to-completion implemented by the state
                    machine. The PlayerGui itself is also fully asynchronous: all actions it
                    triggers are posted into the logic component, and therefore non-blocking. After
                    the logic updates its internal states, it calls a provided safe callback, which
                    will update the status of all buttons. So we now have an asynchronous, non
                    blocking user interface delegating handling the hardware to an asynchronous, non
                    blocking logic layer:</p><p>
                    </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>PlayerGui(<a class="link" href="examples/msmplayer/playergui.h" target="_top">.h</a><a class="link" href="examples/msmplayer/playergui.cpp" target="_top">.cpp</a>): a QWidget providing a simple user interface with
                                buttons like the real hardware would. It inherits qt_servant to get
                                access to safe callbacks. It provides SafeDisplay, an implementation
                                of IDisplay, interface which the real CD player will also implement,
                                and SafeHardware, an implementation of IHardware, an interface which
                                the real hardware will implement. The "Safe" part is that the
                                callbacks being passed are resulting from make_safe_callback: they
                                will be executed within the Qt thread, only if the QWidget is still
                                alive.</p></li><li class="listitem"><p>PlayerLogic(<a class="link" href="examples/msmplayer/playerlogic.h" target="_top">.h</a><a class="link" href="examples/msmplayer/playerlogic.cpp" target="_top">.cpp</a>): the entry point to our logic layer: a
                                trackable_servant, hiden behind a servant_proxy. In our example, it
                                will delegate all logic work to the state machine.</p></li><li class="listitem"><p><a class="link" href="examples/msmplayer/playerlogic.cpp" target="_top">StateMachine</a>: a Boost.MSM state machine, implementing the whole CD
                                player logic.</p></li><li class="listitem"><p><a class="link" href="examples/msmplayer/idisplay.h" target="_top">IDisplay</a>: the user interface provided by the real player.</p></li><li class="listitem"><p><a class="link" href="examples/msmplayer/ihardware.h" target="_top">IHardware</a>: the interface provided by the real hardware (buttons,
                                motors, sensor, etc).</p></li></ul></div><p>
                </p><p>This example shows very important concepts of the Boost.MSM and Asynchronous
                    libraries in actions:</p><p>
                    </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>A state machine is based on run-to-completion: it is unstable
                                while processing an event and should move as fast as possible back
                                to a stable state.</p></li><li class="listitem"><p>To achieve this, the actions should be short. Whatever takes long
                                is posted to a thread pool. Our actions are costing only the cost of
                                a transition and posting.</p></li><li class="listitem"><p>Asynchronous provides the infrastructure needed by the state
                                machine: pools, safe callbacks, protection from external
                                threads.</p></li><li class="listitem"><p>A logic component is behaving like a simple, safe, moveable
                                object. All the application sees is a proxy object.</p></li><li class="listitem"><p>A Qt Object can also make use of thread pools, safe
                                callbacks.</p></li><li class="listitem"><p>Our application is completely asynchronous: it never ever blocks.
                                This is no small feat when we consider that we are controlling a
                                "real" hardware with huge response times compared to the speed of a
                                CPU.</p></li></ul></div><p>                   
                </p><p>Please have a closer look at the following implementation details:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p><a class="link" href="examples/msmplayer/playerlogic.h" target="_top">PlayerLogicProxy</a> has a future-free interface. Its members take a
                                callback. This is a sign there will be no blocking.</p></li><li class="listitem"><p>The state machine in PlayerLogic's state machine uses
                                post_callback for long tasks. In the callback, the next event
                                processing will start.</p></li><li class="listitem"><p>The UI (PlayerGui) is also non-blocking. We make use of callbacks
                                (look at the make_safe_callback calls). We therefore have a very
                                responsive UI.</p></li><li class="listitem"><p>PlayerGui::actionsHandler will set the new state of all buttons
                                each time the state machine updates its status.</p></li></ul></div></div></div></div><div class="part" title="Part&nbsp;III.&nbsp;Reference"><div class="titlepage"><div><div><h1 class="title"><a name="d0e6636"></a>Part&nbsp;III.&nbsp;Reference</h1></div></div></div><div class="toc"><p><b>Table of Contents</b></p><dl><dt><span class="chapter"><a href="#d0e6639">6. Queues</a></span></dt><dd><dl><dt><span class="sect1"><a href="#d0e6647">threadsafe_list</a></span></dt><dt><span class="sect1"><a href="#d0e6664">lockfree_queue</a></span></dt><dt><span class="sect1"><a href="#d0e6688">lockfree_spsc_queue</a></span></dt><dt><span class="sect1"><a href="#d0e6713">lockfree_stack</a></span></dt></dl></dd><dt><span class="chapter"><a href="#d0e6732">7. Schedulers</a></span></dt><dd><dl><dt><span class="sect1"><a href="#d0e6737">single_thread_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e6815">multiple_thread_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e6890">threadpool_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e6966">multiqueue_threadpool_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e7040">stealing_threadpool_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e7112">stealing_multiqueue_threadpool_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e7184">composite_threadpool_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e7241">asio_scheduler</a></span></dt></dl></dd><dt><span class="chapter"><a href="#d0e7294">8. Performance tests</a></span></dt><dd><dl><dt><span class="sect1"><a href="#d0e7297">asynchronous::vector</a></span></dt><dt><span class="sect1"><a href="#d0e7511">Sort</a></span></dt><dt><span class="sect1"><a href="#d0e7912">parallel_scan</a></span></dt><dt><span class="sect1"><a href="#d0e7996">parallel_stable_partition</a></span></dt><dt><span class="sect1"><a href="#d0e8387">parallel_for</a></span></dt></dl></dd><dt><span class="chapter"><a href="#d0e8472">9. Compiler, linker, settings</a></span></dt><dd><dl><dt><span class="sect1"><a href="#d0e8475">C++ 11</a></span></dt><dt><span class="sect1"><a href="#d0e8480">Supported compilers</a></span></dt><dt><span class="sect1"><a href="#d0e8498">Supported targets</a></span></dt><dt><span class="sect1"><a href="#d0e8505">Linking</a></span></dt><dt><span class="sect1"><a href="#d0e8510">Compile-time switches</a></span></dt></dl></dd></dl></div><div class="chapter" title="Chapter&nbsp;6.&nbsp;Queues"><div class="titlepage"><div><div><h2 class="title"><a name="d0e6639"></a>Chapter&nbsp;6.&nbsp;Queues</h2></div></div></div><div class="toc"><p><b>Table of Contents</b></p><dl><dt><span class="sect1"><a href="#d0e6647">threadsafe_list</a></span></dt><dt><span class="sect1"><a href="#d0e6664">lockfree_queue</a></span></dt><dt><span class="sect1"><a href="#d0e6688">lockfree_spsc_queue</a></span></dt><dt><span class="sect1"><a href="#d0e6713">lockfree_stack</a></span></dt></dl></div><p> Asynchronous provides a range of queues with different trade-offs. Use
                    <code class="code">lockfree_queue</code> as default for a quickstart with
                Asynchronous.</p><div class="sect1" title="threadsafe_list"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e6647"></a>threadsafe_list</h2></div></div></div><p>This queue is mostly the one presented in Anthony Williams' book, "C++
                    Concurrency In Action". It is made of a single linked list of nodes, with a
                    mutex at each end of the queue to minimize contention. It is reasonably fast and
                    of simple usage. It can be used in all configurations of pools.</p><p>Its constructor does not require any parameter forwarded from the
                    scheduler.</p><p>Stealing: from the same queue end as pop. Will be implemented better (from the
                    other end to reduce contention) in a future version.</p><p><span class="underline">Caution</span>: crashes were noticed with gcc
                    4.8 while 4.7 and clang 3.3 seemed ok though the compiler might be the reason.
                    For this reason, lockfree_queue is now the default queue.</p><p>Declaration:</p><pre class="programlisting">template&lt;class JOB = boost::asynchronous::any_callable&gt;
class threadsafe_list;</pre></div><div class="sect1" title="lockfree_queue"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e6664"></a>lockfree_queue</h2></div></div></div><p>This queue is a light wrapper around a <code class="code">boost::lockfree::queue</code>,
                    which gives lockfree behavior at the cost of an extra dynamic memory allocation.
                    Please use this container as default when starting with Asynchronous.</p><p>The container is faster than a <code class="code">threadsafe_list</code>, provided one
                    manages to set the queue size to an optimum value. A too small size will cause
                    expensive memory allocations, a too big size will significantly degrade
                    performance.</p><p>Its constructor takes optionally a default size forwarded from the
                    scheduler.</p><p>Stealing: from the same queue end as pop. Stealing from the other end is not
                    supported by <code class="code">boost::lockfree::queue</code>. It can be used in all
                    configurations of pools.</p><p>Declaration:</p><pre class="programlisting">template&lt;class JOB = boost::asynchronous::any_callable&gt;
class lockfree_queue;</pre></div><div class="sect1" title="lockfree_spsc_queue"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e6688"></a>lockfree_spsc_queue</h2></div></div></div><p>This queue is a light wrapper around a
                        <code class="code">boost::lockfree::spsc_queue</code>, which gives lockfree behavior at
                    the cost of an extra dynamic memory allocation. </p><p>Its constructor requires a default size forwarded from the scheduler.</p><p>Stealing: None. Stealing is not supported by
                        <code class="code">boost::lockfree::spsc_queue</code>. It can only be used
                    Single-Producer / Single-Consumer, which reduces its typical usage to a queue of
                    a <code class="code">multiqueue_threadpool_scheduler</code> as consumer, with a
                        <code class="code">single_thread_scheduler</code> as producer.</p><p>Declaration:</p><pre class="programlisting">template&lt;class JOB = boost::asynchronous::any_callable&gt;
class lockfree_spsc_queue;                </pre></div><div class="sect1" title="lockfree_stack"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e6713"></a>lockfree_stack</h2></div></div></div><p>This queue is a light wrapper around a <code class="code">boost::lockfree::stack</code>,
                    which gives lockfree behavior at the cost of an extra dynamic memory allocation.
                    This container creates a task inversion as the last posted tasks will be
                    executed first.</p><p>Its constructor requires a default size forwarded from the scheduler.</p><p>Stealing: from the same queue end as pop. Stealing from the other end is not
                    supported by <code class="code">boost::lockfree::stack</code>. It can be used in all
                    configurations of pools.</p><p>Declaration:</p><pre class="programlisting">template&lt;class JOB = boost::asynchronous::any_callable&gt;
class lockfree_stack;</pre></div></div><div class="chapter" title="Chapter&nbsp;7.&nbsp;Schedulers"><div class="titlepage"><div><div><h2 class="title"><a name="d0e6732"></a>Chapter&nbsp;7.&nbsp;Schedulers</h2></div></div></div><div class="toc"><p><b>Table of Contents</b></p><dl><dt><span class="sect1"><a href="#d0e6737">single_thread_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e6815">multiple_thread_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e6890">threadpool_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e6966">multiqueue_threadpool_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e7040">stealing_threadpool_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e7112">stealing_multiqueue_threadpool_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e7184">composite_threadpool_scheduler</a></span></dt><dt><span class="sect1"><a href="#d0e7241">asio_scheduler</a></span></dt></dl></div><p> There is no perfect scheduler. In any case it's a question of trade-off. Here are
                the schedulers offered by Asynchronous.</p><div class="sect1" title="single_thread_scheduler"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e6737"></a>single_thread_scheduler</h2></div></div></div><p>The scheduler of choice for all servants which are not thread-safe. Serializes
                    all calls to a single queue and executes them in order. Using
                        <code class="code">any_queue_container</code> as queue will however allow it to support
                    task priority.</p><p>This scheduler does not steal from other queues or pools, and does not get
                    stolen from to avoid races.</p><p>Declaration:</p><pre class="programlisting">template&lt;class Queue, class CPULoad&gt;
class single_thread_scheduler;               </pre><p>Creation:</p><pre class="programlisting">boost::asynchronous::any_shared_scheduler_proxy&lt;&gt; scheduler = 
    boost::asynchronous::make_shared_scheduler_proxy&lt;
         boost::asynchronous::<span class="bold"><strong>single_thread_scheduler</strong></span>&lt;
            boost::asynchronous::lockfree_queue&lt;&gt;&gt;&gt;();  

boost::asynchronous::any_shared_scheduler_proxy&lt;&gt; scheduler = 
    boost::asynchronous::make_shared_scheduler_proxy&lt;
       boost::asynchronous::<span class="bold"><strong>single_thread_scheduler</strong></span>&lt;
          boost::asynchronous::lockfree_queue&lt;&gt;&gt;&gt;(10); // size of queue
                </pre><p>Or, using logging:</p><pre class="programlisting">typedef boost::asynchronous::any_loggable&lt;std::chrono::high_resolution_clock&gt; <span class="bold"><strong>servant_job</strong></span>;

boost::asynchronous::any_shared_scheduler_proxy&lt;<span class="bold"><strong>servant_job</strong></span>&gt; scheduler = 
    boost::asynchronous::make_shared_scheduler_proxy&lt;
                                boost::asynchronous::single_thread_scheduler&lt;
                                     boost::asynchronous::threadsafe_list&lt;<span class="bold"><strong>servant_job</strong></span>&gt;&gt;&gt;();                                      
                
boost::asynchronous::any_shared_scheduler_proxy&lt;<span class="bold"><strong>servant_job</strong></span>&gt; scheduler = 
    boost::asynchronous::make_shared_scheduler_proxy&lt;
                                boost::asynchronous::single_thread_scheduler&lt;
                                     boost::asynchronous::lockfree_queue&lt;<span class="bold"><strong>servant_job</strong></span>&gt;&gt;&gt;(10); // size of queue</pre><p>
                    </p><div class="table"><a name="d0e6782"></a><p class="title"><b>Table&nbsp;7.1.&nbsp;#include
                            &lt;boost/asynchronous/scheduler/single_thread_scheduler.hpp&gt;</b></p><div class="table-contents"><table summary="#include&#xA;                            <boost/asynchronous/scheduler/single_thread_scheduler.hpp&gt;" border="1"><colgroup><col><col></colgroup><thead><tr><th>Characteristics</th><th>&nbsp;</th></tr></thead><tbody><tr><td>Number of threads</td><td>1</td></tr><tr><td>Can be stolen from?</td><td>No</td></tr><tr><td>Can steal from other threads in this pool?</td><td>N/A (only 1 thread)</td></tr><tr><td>Can steal from other threads in other pools?</td><td>No</td></tr></tbody></table></div></div><p><br class="table-break">
                </p></div><div class="sect1" title="multiple_thread_scheduler"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e6815"></a>multiple_thread_scheduler</h2></div></div></div><p>The scheduler is an extended version of single_thread_scheduler, where all
                    servants are operated by only one thread at a time, though not always the same
                    one. It creates a n (servants) to m (threads) dependency. The advantages of this
                    scheduler is that one long task will not block other servants, more flexibility
                    in distributing threads among servants, and better cache behaviour (a thread
                    tries to serve servants in order).</p><p>This scheduler does not steal from other queues or pools, and does not get
                    stolen from to avoid races.</p><p>Declaration:</p><pre class="programlisting">template&lt;class Queue, class CPULoad&gt;
class multiple_thread_scheduler;               </pre><p>Creation:</p><pre class="programlisting">boost::asynchronous::any_shared_scheduler_proxy&lt;&gt; scheduler = 
    boost::asynchronous::make_shared_scheduler_proxy&lt;
           boost::asynchronous::<span class="bold"><strong>multiple_thread_scheduler</strong></span>&lt;
              boost::asynchronous::lockfree_queue&lt;&gt;&gt;&gt;(n,m); // n: max number of servants, m: number of worker threads

boost::asynchronous::any_shared_scheduler_proxy&lt;&gt; scheduler = 
    boost::asynchronous::make_shared_scheduler_proxy&lt;
           boost::asynchronous::<span class="bold"><strong>multiple_thread_scheduler</strong></span>&lt;
              boost::asynchronous::lockfree_queue&lt;&gt;&gt;&gt;(n,m,10); // n: max number of servants, m: number of worker threads, 10: size of queue
 
                </pre><p>Or, using logging:</p><pre class="programlisting">typedef boost::asynchronous::any_loggable&lt;std::chrono::high_resolution_clock&gt; <span class="bold"><strong>servant_job</strong></span>;

boost::asynchronous::any_shared_scheduler_proxy&lt;<span class="bold"><strong>servant_job</strong></span>&gt; scheduler = 
    boost::asynchronous::make_shared_scheduler_proxy&lt;
                                boost::asynchronous::single_thread_scheduler&lt;
                                     boost::asynchronous::threadsafe_list&lt;<span class="bold"><strong>servant_job</strong></span>&gt;&gt;&gt;(n,m); // n: max number of servants, m: number of worker threads
                
boost::asynchronous::any_shared_scheduler_proxy&lt;<span class="bold"><strong>servant_job</strong></span>&gt; scheduler = 
    boost::asynchronous::make_shared_scheduler_proxy&lt;
                                boost::asynchronous::single_thread_scheduler&lt;
                                     boost::asynchronous::lockfree_queue&lt;<span class="bold"><strong>servant_job</strong></span>&gt;&gt;&gt;(n,m,10); // n: max number of servants, m: number of worker threads, 10: size of queue</pre><p>
                    </p><div class="table"><a name="d0e6857"></a><p class="title"><b>Table&nbsp;7.2.&nbsp;#include
                            &lt;boost/asynchronous/scheduler/single_thread_scheduler.hpp&gt;</b></p><div class="table-contents"><table summary="#include&#xA;                            <boost/asynchronous/scheduler/single_thread_scheduler.hpp&gt;" border="1"><colgroup><col><col></colgroup><thead><tr><th>Characteristics</th><th>&nbsp;</th></tr></thead><tbody><tr><td>Number of threads</td><td>1..n</td></tr><tr><td>Can be stolen from?</td><td>No</td></tr><tr><td>Can steal from other threads in this pool?</td><td>No</td></tr><tr><td>Can steal from other threads in other pools?</td><td>No</td></tr></tbody></table></div></div><p><br class="table-break">
                </p></div><div class="sect1" title="threadpool_scheduler"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e6890"></a>threadpool_scheduler</h2></div></div></div><p>The simplest and easiest threadpool using a single queue, though multiqueue
                    behavior could be done using <code class="code">any_queue_container</code>. The advantage is
                    that it allows the pool to be given 0 thread and only be stolen from. The cost
                    is a slight performance loss due to higher contention on the single
                    queue.</p><p>This pool does not steal from other pool's queues.</p><p>Use this pool as default for a quickstart with Asynchronous.</p><p>Declaration:</p><pre class="programlisting">template&lt;class Queue,class CPULoad&gt;
class threadpool_scheduler;</pre><p>Creation:</p><pre class="programlisting">boost::asynchronous::any_shared_scheduler_proxy&lt;&gt; scheduler = 
    boost::asynchronous::make_shared_scheduler_proxy&lt;
                    boost::asynchronous::threadpool_scheduler&lt;
                          boost::asynchronous::threadsafe_list&lt;&gt;&gt;&gt;(4); // 4 threads in pool  

boost::asynchronous::any_shared_scheduler_proxy&lt;&gt; scheduler = 
    boost::asynchronous::make_shared_scheduler_proxy&lt;
                    boost::asynchronous::threadpool_scheduler&lt;
                          boost::asynchronous::lockfree_queue&lt;&gt;&gt;&gt;(4,10); // size of queue=10, 4 threads in pool
                </pre><p>Or, using logging:</p><pre class="programlisting">typedef boost::asynchronous::any_loggable&lt;std::chrono::high_resolution_clock&gt; <span class="bold"><strong>servant_job</strong></span>;

boost::asynchronous::any_shared_scheduler_proxy&lt;<span class="bold"><strong>servant_job</strong></span>&gt; scheduler = 
    boost::asynchronous::make_shared_scheduler_proxy&lt;
                    boost::asynchronous::threadpool_scheduler&lt;
                          boost::asynchronous::threadsafe_list&lt;<span class="bold"><strong>servant_job</strong></span>&gt;&gt;&gt;(4); // 4 threads in pool                                      
                
boost::asynchronous::any_shared_scheduler_proxy&lt;<span class="bold"><strong>servant_job</strong></span>&gt; scheduler = 
    boost::asynchronous::make_shared_scheduler_proxy&lt;
                    boost::asynchronous::threadpool_scheduler&lt;
                          boost::asynchronous::lockfree_queue&lt;<span class="bold"><strong>servant_job</strong></span>&gt;&gt;&gt;(4,10); // size of queue=10, 4 threads in pool  </pre><p>
                    </p><div class="table"><a name="d0e6931"></a><p class="title"><b>Table&nbsp;7.3.&nbsp;#include
                            &lt;boost/asynchronous/scheduler/threadpool_scheduler.hpp&gt;</b></p><div class="table-contents"><table summary="#include&#xA;                            <boost/asynchronous/scheduler/threadpool_scheduler.hpp&gt;" border="1"><colgroup><col><col></colgroup><thead><tr><th>Characteristics</th><th>&nbsp;</th></tr></thead><tbody><tr><td>Number of threads</td><td><span class="bold"><strong>0</strong></span>-n</td></tr><tr><td>Can be stolen from?</td><td>Yes</td></tr><tr><td>Can steal from other threads in this pool?</td><td>N/A (only 1 queue)</td></tr><tr><td>Can steal from other threads in other pools?</td><td>No</td></tr></tbody></table></div></div><p><br class="table-break">
                </p></div><div class="sect1" title="multiqueue_threadpool_scheduler"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e6966"></a>multiqueue_threadpool_scheduler</h2></div></div></div><p>This is a <code class="code">threadpool_scheduler</code> with multiple queues to reduce
                    contention. On the other hand, this pool requires at least one thread.</p><p>This pool does not steal from other pool's queues though pool threads do steal
                    from each other's queues.</p><p>Declaration:</p><pre class="programlisting">template&lt;class Queue,class FindPosition=boost::asynchronous::default_find_position&lt; &gt;, class CPULoad &gt;
class multiqueue_threadpool_scheduler;</pre><p>Creation:</p><pre class="programlisting">boost::asynchronous::any_shared_scheduler_proxy&lt;&gt; scheduler = 
    boost::asynchronous::make_shared_scheduler_proxy&lt;
                    boost::asynchronous::multiqueue_threadpool_scheduler&lt;
                          boost::asynchronous::threadsafe_list&lt;&gt;&gt;&gt;(4); // 4 threads in pool  

boost::asynchronous::any_shared_scheduler_proxy&lt;&gt; scheduler = 
    boost::asynchronous::make_shared_scheduler_proxy&lt;
                    boost::asynchronous::multiqueue_threadpool_scheduler&lt;
                          boost::asynchronous::lockfree_queue&lt;&gt;&gt;&gt;(4,10); // size of queue=10, 4 threads in pool
                </pre><p>Or, using logging:</p><pre class="programlisting">typedef boost::asynchronous::any_loggable&lt;std::chrono::high_resolution_clock&gt; <span class="bold"><strong>servant_job</strong></span>;

boost::asynchronous::any_shared_scheduler_proxy&lt;<span class="bold"><strong>servant_job</strong></span>&gt; scheduler = 
    boost::asynchronous::make_shared_scheduler_proxy&lt;
                    boost::asynchronous::multiqueue_threadpool_scheduler&lt;
                          boost::asynchronous::threadsafe_list&lt;<span class="bold"><strong>servant_job</strong></span>&gt;&gt;&gt;(4); // 4 threads in pool                                      
                
boost::asynchronous::any_shared_scheduler_proxy&lt;<span class="bold"><strong>servant_job</strong></span>&gt; scheduler = 
    boost::asynchronous::make_shared_scheduler_proxy&lt;
                    boost::asynchronous::multiqueue_threadpool_scheduler&lt;
                          boost::asynchronous::lockfree_queue&lt;<span class="bold"><strong>servant_job</strong></span>&gt;&gt;&gt;(4,10); // size of queue=10, 4 threads in pool  </pre><p>
                    </p><div class="table"><a name="d0e7005"></a><p class="title"><b>Table&nbsp;7.4.&nbsp;#include
                            &lt;boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp&gt;</b></p><div class="table-contents"><table summary="#include&#xA;                            <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp&gt;" border="1"><colgroup><col><col></colgroup><thead><tr><th>Characteristics</th><th>&nbsp;</th></tr></thead><tbody><tr><td>Number of threads</td><td><span class="bold"><strong>1</strong></span>-n</td></tr><tr><td>Can be stolen from?</td><td>Yes</td></tr><tr><td>Can steal from other threads in this pool?</td><td>Yes</td></tr><tr><td>Can steal from other threads in other pools?</td><td>No</td></tr></tbody></table></div></div><p><br class="table-break">
                </p></div><div class="sect1" title="stealing_threadpool_scheduler"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e7040"></a>stealing_threadpool_scheduler</h2></div></div></div><p>This is a <code class="code">threadpool_scheduler</code> with the added capability to steal
                    from other pool's queues within a <code class="code">composite_threadpool_scheduler</code>.
                    Not used within a <code class="code">composite_threadpool_scheduler</code>, it is a standard
                        <code class="code">threadpool_scheduler</code>.</p><p>Declaration:</p><pre class="programlisting">template&lt;class Queue,class CPULoad, bool /* InternalOnly */ = true &gt;
class stealing_threadpool_scheduler;</pre><p>Creation if used within a <code class="code">composite_threadpool_scheduler</code>:</p><pre class="programlisting">boost::asynchronous::any_shared_scheduler_proxy&lt;&gt; scheduler = 
    boost::asynchronous::make_shared_scheduler_proxy&lt;
                    boost::asynchronous::stealing_threadpool_scheduler&lt;
                          boost::asynchronous::threadsafe_list&lt;&gt;&gt;&gt;(4); // 4 threads in pool</pre><p> However, if used stand-alone, which has little interest outside of unit
                    tests, we need to add a template parameter to inform it:</p><pre class="programlisting">boost::asynchronous::any_shared_scheduler_proxy&lt;&gt; scheduler = 
    boost::asynchronous::make_shared_scheduler_proxy&lt;
                    boost::asynchronous::stealing_threadpool_scheduler&lt;
                          boost::asynchronous::threadsafe_list&lt;&gt;<span class="bold"><strong>,true</strong></span> &gt;&gt;(4); // 4 threads in pool</pre><p>
                    </p><div class="table"><a name="d0e7077"></a><p class="title"><b>Table&nbsp;7.5.&nbsp;#include
                            &lt;boost/asynchronous/scheduler/stealing_threadpool_scheduler.hpp&gt;</b></p><div class="table-contents"><table summary="#include&#xA;                            <boost/asynchronous/scheduler/stealing_threadpool_scheduler.hpp&gt;" border="1"><colgroup><col><col></colgroup><thead><tr><th>Characteristics</th><th>&nbsp;</th></tr></thead><tbody><tr><td>Number of threads</td><td><span class="bold"><strong>0</strong></span>-n</td></tr><tr><td>Can be stolen from?</td><td>Yes</td></tr><tr><td>Can steal from other threads in this pool?</td><td>N/A (only 1 queue)</td></tr><tr><td>Can steal from other threads in other pools?</td><td>Yes</td></tr></tbody></table></div></div><p><br class="table-break">
                </p></div><div class="sect1" title="stealing_multiqueue_threadpool_scheduler"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e7112"></a>stealing_multiqueue_threadpool_scheduler</h2></div></div></div><p>This is a <code class="code">multiqueue_threadpool_scheduler</code> with the added
                    capability to steal from other pool's queues within a
                        <code class="code">composite_threadpool_scheduler</code> (of course, threads within this
                    pool do steal from each other queues, with higher priority). Not used within a
                        <code class="code">composite_threadpool_scheduler</code>, it is a standard
                        <code class="code">multiqueue_threadpool_scheduler</code>.</p><p>Declaration:</p><pre class="programlisting">template&lt;class Queue,class FindPosition=boost::asynchronous::default_find_position&lt; &gt;,class CPULoad, bool /* InternalOnly */= true  &gt;
class stealing_multiqueue_threadpool_scheduler;</pre><p>Creation if used within a <code class="code">composite_threadpool_scheduler</code>:</p><pre class="programlisting">boost::asynchronous::any_shared_scheduler_proxy&lt;&gt; scheduler = 
    boost::asynchronous::make_shared_scheduler_proxy&lt;
                    boost::asynchronous::stealing_multiqueue_threadpool_scheduler&lt;
                          boost::asynchronous::threadsafe_list&lt;&gt;&gt;&gt;(4); // 4 threads in pool  
                </pre><p> However, if used stand-alone, which has little interest outside of unit
                    tests, we need to add a template parameter to inform it:</p><pre class="programlisting">boost::asynchronous::any_shared_scheduler_proxy&lt;&gt; scheduler = 
    boost::asynchronous::make_shared_scheduler_proxy&lt;
                    boost::asynchronous::stealing_multiqueue_threadpool_scheduler&lt;
                          boost::asynchronous::threadsafe_list&lt;&gt;,boost::asynchronous::default_find_position&lt;&gt;,<span class="bold"><strong>true</strong></span>  &gt;&gt;(4); // 4 threads in pool  </pre><p>
                    </p><div class="table"><a name="d0e7149"></a><p class="title"><b>Table&nbsp;7.6.&nbsp;#include
                            &lt;boost/asynchronous/stealing_multiqueue_threadpool_scheduler.hpp&gt;</b></p><div class="table-contents"><table summary="#include&#xA;                            <boost/asynchronous/stealing_multiqueue_threadpool_scheduler.hpp&gt;" border="1"><colgroup><col><col></colgroup><thead><tr><th>Characteristics</th><th>&nbsp;</th></tr></thead><tbody><tr><td>Number of threads</td><td><span class="bold"><strong>1</strong></span>-n</td></tr><tr><td>Can be stolen from?</td><td>Yes</td></tr><tr><td>Can steal from other threads in this pool?</td><td>Yes</td></tr><tr><td>Can steal from other threads in other pools?</td><td>Yes</td></tr></tbody></table></div></div><p><br class="table-break">
                </p></div><div class="sect1" title="composite_threadpool_scheduler"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e7184"></a>composite_threadpool_scheduler</h2></div></div></div><p>This pool owns no thread by itself. Its job is to contain other pools,
                    accessible by the priority given by posting, and share all queues of its
                    subpools among them. Only the stealing_* pools and <code class="code">asio_scheduler</code>
                    will make use of this and steal from other pools though.</p><p>For creation we need to create other pool of stealing or not stealing, stolen
                    from or not, schedulers. stealing_xxx pools will try to steal jobs from other
                    pool of the same composite, but only if these schedulers support this. Other
                    threadpools will not steal but get stolen from.
                        <code class="code">single_thread_scheduler</code> will not steal or get stolen
                    from.</p><pre class="programlisting">// create a composite threadpool made of:
// a multiqueue_threadpool_scheduler, 0 thread
// This scheduler does not steal from other schedulers, but will lend its queues for stealing
auto tp = boost::asynchronous::make_shared_scheduler_proxy&lt;
                    boost::asynchronous::threadpool_scheduler&lt;boost::asynchronous::lockfree_queue&lt;&gt;&gt;&gt; (0,100);

// a stealing_multiqueue_threadpool_scheduler, 3 threads, each with a threadsafe_list
// this scheduler will steal from other schedulers if it can. In this case it will manage only with tp, not tp3
auto tp2 = boost::asynchronous::make_shared_scheduler_proxy&lt;
                    boost::asynchronous::stealing_multiqueue_threadpool_scheduler&lt;boost::asynchronous::threadsafe_list&lt;&gt;&gt;&gt; (3);

// composite pool made of the previous 2
auto tp_worker = boost::asynchronous::make_shared_scheduler_proxy&lt;<span class="bold"><strong>boost::asynchronous::composite_threadpool_scheduler&lt;&gt;&gt;(tp,tp2)</strong></span>; 
                </pre><p>Declaration:</p><pre class="programlisting">template&lt;class Job = boost::asynchronous::any_callable,
         class FindPosition=boost::asynchronous::default_find_position&lt; &gt;,
         class Clock = std::chrono::high_resolution_clock  &gt;
class composite_threadpool_scheduler;                 
                </pre><p>
                    </p><div class="table"><a name="d0e7208"></a><p class="title"><b>Table&nbsp;7.7.&nbsp;#include
                            &lt;boost/asynchronous/scheduler/composite_threadpool_scheduler.hpp&gt;</b></p><div class="table-contents"><table summary="#include&#xA;                            <boost/asynchronous/scheduler/composite_threadpool_scheduler.hpp&gt;" border="1"><colgroup><col><col></colgroup><thead><tr><th>Characteristics</th><th>&nbsp;</th></tr></thead><tbody><tr><td>Number of threads</td><td>0</td></tr><tr><td>Can be stolen from?</td><td>Yes</td></tr><tr><td>Can steal from other threads in this pool?</td><td>N/A</td></tr><tr><td>Can steal from other threads in other pools?</td><td>No</td></tr></tbody></table></div></div><p><br class="table-break">
                </p></div><div class="sect1" title="asio_scheduler"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e7241"></a>asio_scheduler</h2></div></div></div><p>This pool brings the infrastructure and access to io_service for an integrated
                    usage of Boost.Asio. Furthermore, if used withing a
                        <code class="code">composite_threadpool_scheduler</code>, it will steal jobs from other
                    pool's queues.</p><p>Declaration:</p><pre class="programlisting">template&lt;class FindPosition=boost::asynchronous::default_find_position&lt; boost::asynchronous::sequential_push_policy &gt;, class CPULoad &gt;
class asio_scheduler;</pre><p>Creation:</p><pre class="programlisting">boost::asynchronous::any_shared_scheduler_proxy&lt;&gt; scheduler = 
    boost::asynchronous::make_shared_scheduler_proxy&lt;
                    boost::asynchronous::asio_scheduler&lt;&gt;&gt;(4); // 4 threads in pool</pre><p>
                    </p><div class="table"><a name="d0e7259"></a><p class="title"><b>Table&nbsp;7.8.&nbsp;#include
                            &lt;boost/asynchronous/extensions/asio/asio_scheduler.hpp&gt;</b></p><div class="table-contents"><table summary="#include&#xA;                            <boost/asynchronous/extensions/asio/asio_scheduler.hpp&gt;" border="1"><colgroup><col><col></colgroup><thead><tr><th>Characteristics</th><th>&nbsp;</th></tr></thead><tbody><tr><td>Number of threads</td><td><span class="bold"><strong>1</strong></span>-n</td></tr><tr><td>Can be stolen from?</td><td>No*</td></tr><tr><td>Can steal from other threads in this pool?</td><td>Yes</td></tr><tr><td>Can steal from other threads in other pools?</td><td>Yes</td></tr></tbody></table></div></div><p><br class="table-break">
                </p></div></div><div class="chapter" title="Chapter&nbsp;8.&nbsp;Performance tests"><div class="titlepage"><div><div><h2 class="title"><a name="d0e7294"></a>Chapter&nbsp;8.&nbsp;Performance tests</h2></div></div></div><div class="toc"><p><b>Table of Contents</b></p><dl><dt><span class="sect1"><a href="#d0e7297">asynchronous::vector</a></span></dt><dt><span class="sect1"><a href="#d0e7511">Sort</a></span></dt><dt><span class="sect1"><a href="#d0e7912">parallel_scan</a></span></dt><dt><span class="sect1"><a href="#d0e7996">parallel_stable_partition</a></span></dt><dt><span class="sect1"><a href="#d0e8387">parallel_for</a></span></dt></dl></div><div class="sect1" title="asynchronous::vector"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e7297"></a>asynchronous::vector</h2></div></div></div><p><span class="underline">Test</span>:
                    libs/asynchonous/test/perf_vector.cpp. </p><p><span class="underline">Test</span> processor Core i7-5960X / Xeon Phi
                    3120A. </p><p><span class="underline">Compiler</span>: g++ 6.1, -O3, -std=c++14, link
                    with libtbbmalloc_proxy.</p><p>The test uses a vector of 10000000 elements, each element containing a
                    std::vector of 10 integers.</p><p>
                    </p><div class="table"><a name="d0e7316"></a><p class="title"><b>Table&nbsp;8.1.&nbsp;Performance of asynchronous::vector members using 4 threads</b></p><div class="table-contents"><table summary="Performance of asynchronous::vector members using 4 threads" border="1"><colgroup><col><col><col><col></colgroup><thead><tr><th>Member</th><th>std::vector</th><th>asynchronous::vector</th><th>speedup asynchronous / std</th></tr></thead><tbody><tr><td>Constructor</td><td>385ms</td><td>147ms</td><td>2.62</td></tr><tr><td>Copy</td><td>337ms</td><td>190ms</td><td>1.77</td></tr><tr><td>compare</td><td>81ms</td><td>43ms</td><td>1.88</td></tr><tr><td>clear</td><td>221ms</td><td>98ms</td><td>2.25</td></tr><tr><td>resize</td><td>394ms</td><td>276ms</td><td>1.43</td></tr></tbody></table></div></div><p><br class="table-break">
                    </p><div class="table"><a name="d0e7381"></a><p class="title"><b>Table&nbsp;8.2.&nbsp;Performance of asynchronous::vector members using 8 threads</b></p><div class="table-contents"><table summary="Performance of asynchronous::vector members using 8 threads" border="1"><colgroup><col><col><col><col></colgroup><thead><tr><th>Member</th><th>std::vector</th><th>asynchronous::vector</th><th>speedup asynchronous / std</th></tr></thead><tbody><tr><td>Constructor</td><td>380ms</td><td>90ms</td><td>4.22</td></tr><tr><td>Copy</td><td>306ms</td><td>120ms</td><td>2.55</td></tr><tr><td>compare</td><td>74ms</td><td>30ms</td><td>2.47</td></tr><tr><td>clear</td><td>176ms</td><td>54ms</td><td>3.26</td></tr><tr><td>resize</td><td>341ms</td><td>178ms</td><td>1.92</td></tr></tbody></table></div></div><p><br class="table-break">
                    </p><div class="table"><a name="d0e7446"></a><p class="title"><b>Table&nbsp;8.3.&nbsp;Performance of asynchronous::vector members Xeon Phi 3120A 57 Cores /
                            228 Threads</b></p><div class="table-contents"><table summary="Performance of asynchronous::vector members Xeon Phi 3120A 57 Cores /&#xA;                            228 Threads" border="1"><colgroup><col><col><col><col></colgroup><thead><tr><th>Member</th><th>std::vector</th><th>asynchronous::vector</th><th>speedup asynchronous / std</th></tr></thead><tbody><tr><td>Constructor</td><td>4175 ms</td><td>240 ms</td><td>17.4</td></tr><tr><td>Copy</td><td>5439 ms</td><td>389 ms</td><td>14</td></tr><tr><td>compare</td><td>4139 ms</td><td>43 ms</td><td>96</td></tr><tr><td>clear</td><td>2390 ms</td><td>39 ms</td><td>61.3</td></tr><tr><td>resize</td><td>5223 ms</td><td>222 ms</td><td>23.5</td></tr></tbody></table></div></div><p><br class="table-break">
                </p></div><div class="sect1" title="Sort"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e7511"></a>Sort</h2></div></div></div><p>This test will compare asynchronous:parallel_sort with TBB 4.3 parallel_sort.
                    16 threads used.</p><p><span class="underline">Test</span>:
                    libs/asynchonous/test/perf/parallel_sort_future_v1.cpp. TBB test:
                    libs/asynchronous/test/perf/tbb/tbb_parallel_sort.cpp</p><p><span class="underline">Test</span> processor Core i7-5960X. </p><p><span class="underline">Compiler</span>: g++ 6.1, -O3, -std=c++14, link
                    with libtbbmalloc_proxy.</p><p>The same test will be done using TBB then asynchronous + std::sort, then
                    asynchronous + boost::spreadsort. 
                    </p><div class="table"><a name="d0e7530"></a><p class="title"><b>Table&nbsp;8.4.&nbsp;Sorting 200000000 uint32_t</b></p><div class="table-contents"><table summary="Sorting 200000000 uint32_t" border="1"><colgroup><col><col><col><col><col><col></colgroup><thead><tr><th>Test</th><th>TBB</th><th>asynchronous</th><th>asynchronous + boost::spreadsort</th><th>asynchronous::parallel_sort2</th><th>asynchronous::parallel_sort2 + boost::spreadsort</th></tr></thead><tbody><tr><td>test_random_elements_many_repeated</td><td>2416 ms</td><td>1133 ms</td><td>561 ms</td><td>1425 ms</td><td>928 ms</td></tr><tr><td>test_random_elements_few_repeated</td><td>2408 ms</td><td>1301 ms</td><td>1002 ms</td><td>1694 ms</td><td>1385 ms</td></tr><tr><td>test_random_elements_quite_repeated</td><td>2341 ms</td><td>1298 ms</td><td>1027 ms</td><td>1687 ms</td><td>1344 ms</td></tr><tr><td>test_sorted_elements</td><td>22.5 ms</td><td>16 ms</td><td>16 ms</td><td>16 ms</td><td>16 ms</td></tr><tr><td>test_reversed_sorted_elements</td><td>554 ms</td><td>47 ms</td><td>47 ms</td><td>48 ms</td><td>48 ms</td></tr><tr><td>test_equal_elements</td><td>26 ms</td><td>16 ms</td><td>16 ms</td><td>17 ms</td><td>17 ms</td></tr></tbody></table></div></div><p><br class="table-break"></p><div class="table"><a name="d0e7633"></a><p class="title"><b>Table&nbsp;8.5.&nbsp;Sorting 200000000 double</b></p><div class="table-contents"><table summary="Sorting 200000000 double" border="1"><colgroup><col><col><col><col><col><col></colgroup><thead><tr><th>Test</th><th>TBB</th><th>asynchronous</th><th>asynchronous + boost::spreadsort</th><th>asynchronous::parallel_sort2</th><th>asynchronous::parallel_sort2 + boost::spreadsort</th></tr></thead><tbody><tr><td>test_random_elements_many_repeated</td><td>2504 ms</td><td>1446 ms</td><td>1133 ms</td><td>2173 ms</td><td>1919 ms</td></tr><tr><td>test_random_elements_few_repeated</td><td>2690 ms</td><td>1714 ms</td><td>1266 ms</td><td>2406 ms</td><td>2044 ms</td></tr><tr><td>test_random_elements_quite_repeated</td><td>2602 ms</td><td>1715 ms</td><td>1309 ms</td><td>2448 ms</td><td>2037 ms</td></tr><tr><td>test_sorted_elements</td><td>34 ms</td><td>32 ms</td><td>32 ms</td><td>32 ms</td><td>34 ms</td></tr><tr><td>test_reversed_sorted_elements</td><td>644 ms</td><td>95 ms</td><td>94 ms</td><td>95 ms</td><td>95 ms</td></tr><tr><td>test_equal_elements</td><td>34 ms</td><td>33 ms</td><td>32 ms</td><td>33 ms</td><td>32 ms</td></tr></tbody></table></div></div><br class="table-break"><div class="table"><a name="d0e7736"></a><p class="title"><b>Table&nbsp;8.6.&nbsp;Sorting 200000000 std::string</b></p><div class="table-contents"><table summary="Sorting 200000000 std::string" border="1"><colgroup><col><col><col><col><col><col></colgroup><thead><tr><th>Test</th><th>TBB</th><th>asynchronous</th><th>asynchronous + boost::spreadsort</th><th>asynchronous::parallel_sort2</th><th>asynchronous::parallel_sort2 + boost::spreadsort</th></tr></thead><tbody><tr><td>test_random_elements_many_repeated</td><td>891 ms</td><td>924 ms</td><td>791 ms</td><td>889 ms</td><td>777 ms</td></tr><tr><td>test_random_elements_few_repeated</td><td>1031 ms</td><td>1069 ms</td><td>906 ms</td><td>1053 ms</td><td>967 ms</td></tr><tr><td>test_random_elements_quite_repeated</td><td>929 ms</td><td>1000 ms</td><td>838 ms</td><td>998 ms</td><td>1003 ms</td></tr><tr><td>test_sorted_elements</td><td>11 ms</td><td>16 ms</td><td>16 ms</td><td>16 ms</td><td>32 ms</td></tr><tr><td>test_reversed_sorted_elements</td><td>265 ms</td><td>28 ms</td><td>28 ms</td><td>29 ms</td><td>38 ms</td></tr><tr><td>test_equal_elements</td><td>12 ms</td><td>4 ms</td><td>3 ms</td><td>3 ms</td><td>4 ms</td></tr></tbody></table></div></div><br class="table-break"><div class="table"><a name="d0e7839"></a><p class="title"><b>Table&nbsp;8.7.&nbsp;Sorting 10000000 objects containing 10 longs</b></p><div class="table-contents"><table summary="Sorting 10000000 objects containing 10 longs" border="1"><colgroup><col><col><col><col></colgroup><thead><tr><th>Test</th><th>TBB</th><th>asynchronous</th><th>asynchronous::parallel_sort2</th></tr></thead><tbody><tr><td>test_random_elements_many_repeated</td><td>869 ms</td><td>1007 ms</td><td>204 ms</td></tr><tr><td>test_random_elements_few_repeated</td><td>803 ms</td><td>887 ms</td><td>226 ms</td></tr><tr><td>test_random_elements_quite_repeated</td><td>810 ms</td><td>960 ms</td><td>175 ms</td></tr><tr><td>test_sorted_elements</td><td>22 ms</td><td>27 ms</td><td>2 ms</td></tr><tr><td>test_reversed_sorted_elements</td><td>338 ms</td><td>34 ms</td><td>3 ms</td></tr><tr><td>test_equal_elements</td><td>25 ms</td><td>23 ms</td><td>2 ms</td></tr></tbody></table></div></div><br class="table-break"></div><div class="sect1" title="parallel_scan"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e7912"></a>parallel_scan</h2></div></div></div><p><span class="underline">Test</span>:
                    libs/asynchonous/test/perf_scan.cpp. </p><p><span class="underline">Test</span> processor Core i7-5960X / Xeon Phi
                    3120A. </p><p><span class="underline">Compiler</span>: g++ 6.1, -O3, -std=c++14, link
                    with libtbbmalloc_proxy.</p><p>The test will exercise 10 times a parallel_scan on vector of 1000000
                    elements.</p><p>
                    </p><div class="table"><a name="d0e7931"></a><p class="title"><b>Table&nbsp;8.8.&nbsp;Performance of parallel_scan vs serial scan on a i7 / Xeon Phi
                            Knight's Corner</b></p><div class="table-contents"><table summary="Performance of parallel_scan vs serial scan on a i7 / Xeon Phi&#xA;                            Knight's Corner" border="1"><colgroup><col><col><col><col></colgroup><thead><tr><th>Member</th><th>parallel_scan</th><th>sequential scan</th><th>speedup parallel / serial</th></tr></thead><tbody><tr><td>i7-5960X (8 Cores / 16 Threads)</td><td>204 ms</td><td>812 ms</td><td>4</td></tr><tr><td>i7-5960X (8 Cores / 8 Threads)</td><td>305 ms</td><td>887 ms</td><td>2.9</td></tr><tr><td>Xeon Phi (57 Cores /228 Threads)</td><td>74 ms</td><td>423 ms</td><td>57</td></tr><tr><td>Xeon Phi (57 Cores /114 Threads)</td><td>143 ms</td><td>354 ms</td><td>25</td></tr><tr><td>Xeon Phi (57 Cores /10 Threads)</td><td>373 ms</td><td>350 ms</td><td>9.4</td></tr></tbody></table></div></div><p><br class="table-break">                    
                </p></div><div class="sect1" title="parallel_stable_partition"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e7996"></a>parallel_stable_partition</h2></div></div></div><p><span class="underline">Test</span>:
                    libs/asynchonous/test/perf_scan.cpp. </p><p><span class="underline">Test</span> processor Core i7-5960X / Xeon Phi
                    3120A. </p><p><span class="underline">Compiler</span>: g++ 6.1, -O3, -std=c++11, link
                    with libtbbmalloc_proxy.</p><p>The test will exercise a parallel_stable_partition on vector of 100000000
                    floats. As comparison, std::partition will be used.</p><p>
                    </p><div class="table"><a name="d0e8015"></a><p class="title"><b>Table&nbsp;8.9.&nbsp;Partitioning 100000000 floats on Core i7-5960X 8 Cores / 8 Threads
                            (16 Threads bring no added value)</b></p><div class="table-contents"><table summary="Partitioning 100000000 floats on Core i7-5960X 8 Cores / 8 Threads&#xA;                            (16 Threads bring no added value)" border="1"><colgroup><col><col><col><col></colgroup><thead><tr><th>Test</th><th>parallel</th><th>std::partition</th><th>speedup parallel / serial</th></tr></thead><tbody><tr><td>test_random_elements_many_repeated</td><td>187 ms</td><td>720 ms</td><td>3.87</td></tr><tr><td>test_random_elements_few_repeated</td><td>171 ms</td><td>1113 ms</td><td>6.5</td></tr><tr><td>test_random_elements_quite_repeated</td><td>172 ms</td><td>555 ms</td><td>3.22</td></tr><tr><td>test_sorted_elements</td><td>176 ms</td><td>1139 ms</td><td>6.5</td></tr><tr><td>test_reversed_sorted_elements</td><td>180 ms</td><td>1125 ms</td><td>6.25</td></tr><tr><td>test_equal_elements</td><td>168 ms</td><td>1121 ms</td><td>6.7</td></tr></tbody></table></div></div><p><br class="table-break">
                    </p><div class="table"><a name="d0e8089"></a><p class="title"><b>Table&nbsp;8.10.&nbsp;Partitioning 100000000 floats on Core i7-5960X 4 Cores / 4
                            Threads</b></p><div class="table-contents"><table summary="Partitioning 100000000 floats on Core i7-5960X 4 Cores / 4&#xA;                            Threads" border="1"><colgroup><col><col><col><col></colgroup><thead><tr><th>Test</th><th>parallel</th><th>std::partition</th><th>speedup parallel / serial</th></tr></thead><tbody><tr><td>test_random_elements_many_repeated</td><td>296 ms</td><td>720 ms</td><td>2.43</td></tr><tr><td>test_random_elements_few_repeated</td><td>301 ms</td><td>1113 ms</td><td>3.7</td></tr><tr><td>test_random_elements_quite_repeated</td><td>294 ms</td><td>555 ms</td><td>1.9</td></tr><tr><td>test_sorted_elements</td><td>287 ms</td><td>1139 ms</td><td>4</td></tr><tr><td>test_reversed_sorted_elements</td><td>288 ms</td><td>1125 ms</td><td>3.9</td></tr><tr><td>test_equal_elements</td><td>286 ms</td><td>1121 ms</td><td>3.9</td></tr></tbody></table></div></div><p><br class="table-break">
                    </p><div class="table"><a name="d0e8163"></a><p class="title"><b>Table&nbsp;8.11.&nbsp;Partitioning 100000000 floats on Xeon Phi 3120A 57 Cores / 228
                            Threads</b></p><div class="table-contents"><table summary="Partitioning 100000000 floats on Xeon Phi 3120A 57 Cores / 228&#xA;                            Threads" border="1"><colgroup><col><col><col><col></colgroup><thead><tr><th>Test</th><th>parallel</th><th>std::partition</th><th>speedup parallel / serial</th></tr></thead><tbody><tr><td>test_random_elements_many_repeated</td><td>88 ms</td><td>15944 ms</td><td>181</td></tr><tr><td>test_random_elements_few_repeated</td><td>80 ms</td><td>27186 ms</td><td>339</td></tr><tr><td>test_random_elements_quite_repeated</td><td>89 ms</td><td>16067 ms</td><td>180</td></tr><tr><td>test_sorted_elements</td><td>77 ms</td><td>26830 ms</td><td>348</td></tr><tr><td>test_reversed_sorted_elements</td><td>73 ms</td><td>27367 ms</td><td>374</td></tr><tr><td>test_equal_elements</td><td>82 ms</td><td>27464 ms</td><td>334</td></tr></tbody></table></div></div><p><br class="table-break">   
                    </p><div class="table"><a name="d0e8237"></a><p class="title"><b>Table&nbsp;8.12.&nbsp;Partitioning 100000000 floats on Xeon Phi 3120A 57 Cores / 114
                            Threads</b></p><div class="table-contents"><table summary="Partitioning 100000000 floats on Xeon Phi 3120A 57 Cores / 114&#xA;                            Threads" border="1"><colgroup><col><col><col><col></colgroup><thead><tr><th>Test</th><th>parallel</th><th>std::partition</th><th>speedup parallel / serial</th></tr></thead><tbody><tr><td>test_random_elements_many_repeated</td><td>152 ms</td><td>15944 ms</td><td>104</td></tr><tr><td>test_random_elements_few_repeated</td><td>129 ms</td><td>27186 ms</td><td>210</td></tr><tr><td>test_random_elements_quite_repeated</td><td>153 ms</td><td>16067 ms</td><td>105</td></tr><tr><td>test_sorted_elements</td><td>104 ms</td><td>26830 ms</td><td>258</td></tr><tr><td>test_reversed_sorted_elements</td><td>110 ms</td><td>27367 ms</td><td>249</td></tr><tr><td>test_equal_elements</td><td>114 ms</td><td>27464 ms</td><td>241</td></tr></tbody></table></div></div><p><br class="table-break">    
                    </p><div class="table"><a name="d0e8311"></a><p class="title"><b>Table&nbsp;8.13.&nbsp;Partitioning 100000000 floats on Xeon Phi 3120A 57 Cores / 10
                            Threads</b></p><div class="table-contents"><table summary="Partitioning 100000000 floats on Xeon Phi 3120A 57 Cores / 10&#xA;                            Threads" border="1"><colgroup><col><col><col><col></colgroup><thead><tr><th>Test</th><th>parallel</th><th>std::partition</th><th>speedup parallel / serial</th></tr></thead><tbody><tr><td>test_random_elements_many_repeated</td><td>1131 ms</td><td>15944 ms</td><td>14</td></tr><tr><td>test_random_elements_few_repeated</td><td>816 ms</td><td>27186 ms</td><td>33</td></tr><tr><td>test_random_elements_quite_repeated</td><td>1212 ms</td><td>16067 ms</td><td>13</td></tr><tr><td>test_sorted_elements</td><td>755 ms</td><td>26830 ms</td><td>35</td></tr><tr><td>test_reversed_sorted_elements</td><td>739 ms</td><td>27367 ms</td><td>37</td></tr><tr><td>test_equal_elements</td><td>798 ms</td><td>27464 ms</td><td>34</td></tr></tbody></table></div></div><p><br class="table-break">                        
                </p><p>The Xeon Phi speedups are quie surprising. The implementation of
                    std::partition seems inefficient on this platform.</p></div><div class="sect1" title="parallel_for"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e8387"></a>parallel_for</h2></div></div></div><p><span class="underline">Test</span>:
                    libs/asynchonous/test/parallel_for.cpp. </p><p><span class="underline">Test</span> processor Core i7-5960X / Xeon Phi
                    3120A. </p><p><span class="underline">Compiler</span>: g++ 6.1, -O3, -std=c++14, link
                    with libtbbmalloc_proxy.</p><p>The test will exercise 10 times a parallel_for with a dummy operation on a
                    vector of 100000000 elements.</p><p>
                    </p><div class="table"><a name="d0e8406"></a><p class="title"><b>Table&nbsp;8.14.&nbsp;Performance of parallel_for on a i7 / Xeon Phi Knight's
                            Corner</b></p><div class="table-contents"><table summary="Performance of parallel_for on a i7 / Xeon Phi Knight's&#xA;                            Corner" border="1"><colgroup><col><col><col></colgroup><thead><tr><th>Member</th><th>parallel_for</th><th>speedup parallel / serial</th></tr></thead><tbody><tr><td>i7-5960X (8 Cores / 16 Threads)</td><td>83 ms</td><td>5.9</td></tr><tr><td>i7-5960X (8 Cores / 8 Threads)</td><td>76 ms</td><td>6.4</td></tr><tr><td>i7-5960X (8 Cores / 4 Threads)</td><td>136 ms</td><td>3.6</td></tr><tr><td>i7-5960X (8 Cores / 1 Threads)</td><td>491 ms</td><td> -</td></tr><tr><td>Xeon Phi (57 Cores /228 Threads)</td><td>51 ms</td><td>42</td></tr><tr><td>Xeon Phi (57 Cores /114 Threads)</td><td>84 ms</td><td>25</td></tr><tr><td>Xeon Phi (57 Cores /10 Threads)</td><td>2124 ms</td><td> -</td></tr></tbody></table></div></div><p><br class="table-break">                    
                </p></div></div><div class="chapter" title="Chapter&nbsp;9.&nbsp;Compiler, linker, settings"><div class="titlepage"><div><div><h2 class="title"><a name="d0e8472"></a>Chapter&nbsp;9.&nbsp;Compiler, linker, settings</h2></div></div></div><div class="toc"><p><b>Table of Contents</b></p><dl><dt><span class="sect1"><a href="#d0e8475">C++ 11</a></span></dt><dt><span class="sect1"><a href="#d0e8480">Supported compilers</a></span></dt><dt><span class="sect1"><a href="#d0e8498">Supported targets</a></span></dt><dt><span class="sect1"><a href="#d0e8505">Linking</a></span></dt><dt><span class="sect1"><a href="#d0e8510">Compile-time switches</a></span></dt></dl></div><div class="sect1" title="C++ 11"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e8475"></a>C++ 11</h2></div></div></div><p>Asynchronous is C++11/14-only. Please check that your compiler has C++11
                    enabled (-std=c++0x or -std=c++11 in different versions of gcc). Usually, C++14
                    is recommended.</p></div><div class="sect1" title="Supported compilers"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e8480"></a>Supported compilers</h2></div></div></div><p>Asynchronous is tested and ok with: </p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>gcc: &gt;= 4.9</p></li><li class="listitem"><p>clang: &gt;= 3.5</p></li><li class="listitem"><p>VS2015 with a limitation: BOOST_ASYNC_FUTURE/POST_MEMBER_1(or _2
                                or _3) as variadic macros are not supported</p></li><li class="listitem"><p>Intel ICC &gt;= 13.</p></li></ul></div></div><div class="sect1" title="Supported targets"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e8498"></a>Supported targets</h2></div></div></div><p>Asynchronous has been tested on Linux and Windows PCs, Intel and AMD, with the
                    above compilers, and with mingw.</p><p>Asynchronous being based on Boost.Thread, can also work on Intel Xeon Phi with
                    a minor change: within Boost, all usage of boost::shared_ptr must be replaced by
                    std::shared_ptr. Strongly recommended is linking with tbbmalloc_proxy for better
                    performance.</p></div><div class="sect1" title="Linking"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e8505"></a>Linking</h2></div></div></div><p>Asynchronous is header-only, but requires Boost libraries which are not. One
                    should link with: boost_system, boost_thread, boost_chrono and boost_date_time
                    if logging is required</p></div><div class="sect1" title="Compile-time switches"><div class="titlepage"><div><div><h2 class="title" style="clear: both"><a name="d0e8510"></a>Compile-time switches</h2></div></div></div><p>The following symbols will, when defined, influence the behaviour of the library:</p><div class="itemizedlist"><ul class="itemizedlist" type="disc"><li class="listitem"><p>BOOST_ASYNCHRONOUS_DEFAULT_JOB replaces
                                boost::asynchronous::any_callable by the required job type.</p></li><li class="listitem"><p>BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS: forces Asynchronous to
                                only provide servant_proxy macros with all their arguments to avoid
                                accidental forgetting. Precisely:</p><div class="itemizedlist"><ul class="itemizedlist" type="circle"><li class="listitem"><p>BOOST_ASYNC_FUTURE_MEMBER /BOOST_ASYNC_POST_MEMBER
                                            require priority</p></li><li class="listitem"><p>BOOST_ASYNC_FUTURE_MEMBER_LOG /
                                            BOOST_ASYNC_POST_MEMBER_LOG require task name and
                                            priority</p></li><li class="listitem"><p>make_safe_callback requires name and priority</p></li><li class="listitem"><p>make_lambda_continuation_wrapper requires task
                                            name</p></li><li class="listitem"><p>parallel algorithms require task name and
                                            priority</p></li><li class="listitem"><p>asynchronous::vector requires as last arguments name
                                            and priority</p></li></ul></div></li><li class="listitem"><p>BOOST_ASYNCHRONOUS_NO_SAVING_CPU_LOAD: overrides default of
                                Asynchronous: schedulers will run at full speed. This can slightly
                                increase speed, at the cost of high CPU load.</p></li><li class="listitem"><p>BOOST_ASYNCHRONOUS_PRCTL_SUPPORT: Allows naming of threads if
                                sys/prctl is supported (Linux).</p></li><li class="listitem"><p>BOOST_ASYNCHRONOUS_USE_BOOST_SPREADSORT: in older Boost versions,
                                Spreasort was not included. This switch will provide
                                parallel_spreadsort, parallel_quick_spreadsort and
                                parallel_spreadsort_inplace</p></li></ul></div></div></div></div></div></body></html><!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
    "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" lang="en" xml:lang="en">
<body>
  <p> Copyright Christophe Henry, 2010</p>
    <p>Distributed under the Boost Software License, Version 1.0. (See 
    accompanying file <a href="../../LICENSE_1_0.txt">
    LICENSE_1_0.txt</a> or copy at
    <a href="http://www.boost.org/LICENSE_1_0.txt">www.boost.org/LICENSE_1_0.txt</a>)</p>

</body>
</html>

