// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_SCHEDULER_CPU_LOAD_POLICIES_HPP
#define BOOST_ASYNCHRONOUS_SCHEDULER_CPU_LOAD_POLICIES_HPP

#include <chrono>
#include <thread>

#include <boost/thread/thread.hpp>

namespace boost { namespace asynchronous
{

// no sleep, fastest reaction time, highest cpu load
struct no_cpu_load_saving
{
public:
    // all threads handled same way
    static std::size_t no_load_save_threads() {return 0;}
    // called each time a job is popped and executed
    void popped_job(bool=false)
    {
        // nothing to do
    }
    void loop_done_no_job(bool=false)
    {
        // nothing to do
    }
};

template <unsigned Loops=10, unsigned MinDurationUs=80000, unsigned SleepTimeUs=5000>
struct default_save_cpu_load
{
public:
    default_save_cpu_load():m_cpt_nojob(0),m_start( std::chrono::high_resolution_clock::now()){}
    // all threads handled same way
    static std::size_t no_load_save_threads() {return 0;}
    // called each time a job is popped and executed
    void popped_job(bool=false)
    {
        // reset counter and timer
        m_cpt_nojob=0;
        m_start = std::chrono::high_resolution_clock::now();
    }
    // called each time a loop on all queues is done and no job was found
    void loop_done_no_job(bool=false)
    {
        ++m_cpt_nojob;
        if (m_cpt_nojob >= Loops)
        {
            m_cpt_nojob = 0;
            auto elapsed = std::chrono::high_resolution_clock::now() - m_start;
            if(std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count() <= MinDurationUs)
            {
                std::this_thread::sleep_for( std::chrono::microseconds(SleepTimeUs) );
            }
            m_start = std::chrono::high_resolution_clock::now();
        }
    }
private:
    unsigned int m_cpt_nojob;
    std::chrono::high_resolution_clock::time_point m_start;
};

// will call no_cpu_load_saving for some threads and default_save_cpu_load for all others
template <unsigned NoSaveLoadThreads=1, unsigned Loops=10, unsigned MinDurationUs=80000, unsigned SleepTimeUs=5000>
struct some_save_cpu_load
{
    static std::size_t no_load_save_threads() {return NoSaveLoadThreads;}

    void popped_job(bool save_load_thread=true)
    {
        if(save_load_thread)
        {
            m_save.popped_job();
        }
        else
        {
            m_no_save.popped_job();
        }
    }
    void loop_done_no_job(bool save_load_thread=true)
    {
        if(save_load_thread)
        {
            m_save.loop_done_no_job();
        }
        else
        {
            m_no_save.loop_done_no_job();
        }
    }

    boost::asynchronous::no_cpu_load_saving m_no_save;
    boost::asynchronous::default_save_cpu_load<Loops,MinDurationUs,SleepTimeUs> m_save;
};

}} // boost::asynchronous::scheduler
#endif // BOOST_ASYNCHRONOUS_SCHEDULER_CPU_LOAD_POLICIES_HPP
