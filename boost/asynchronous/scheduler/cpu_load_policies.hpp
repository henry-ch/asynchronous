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

#include <boost/chrono/chrono.hpp>
#include <boost/thread/thread.hpp>

namespace boost { namespace asynchronous
{

// no sleep, fastest reaction time, highest cpu load
struct no_cpu_load_saving
{
public:
    // called each time a job is popped and executed
    void popped_job()
    {
        // nothing to do
    }
    void loop_done_no_job()
    {
        // nothing to do
    }
};

template <unsigned Loops=10, unsigned MinDurationUs=80000, unsigned SleepTimeUs=1000>
struct default_save_cpu_load
{
public:
    default_save_cpu_load():m_cpt_nojob(0),m_start( boost::chrono::high_resolution_clock::now()){}
    // called each time a job is popped and executed
    void popped_job()
    {
        // reset counter and timer
        m_cpt_nojob=0;
        m_start = boost::chrono::high_resolution_clock::now();
    }
    // called each time a loop on all queues is done and no job was found
    void loop_done_no_job()
    {
        ++m_cpt_nojob;
        if (m_cpt_nojob >= Loops)
        {
            m_cpt_nojob = 0;
            auto elapsed = boost::chrono::high_resolution_clock::now() - m_start;
            if(boost::chrono::duration_cast<boost::chrono::microseconds>(elapsed).count() <= MinDurationUs)
            {
                boost::this_thread::sleep_for( boost::chrono::microseconds(SleepTimeUs) );
                m_start = boost::chrono::high_resolution_clock::now();
            }
        }
    }
private:
    unsigned int m_cpt_nojob;
    boost::chrono::high_resolution_clock::time_point m_start;
};

}} // boost::asynchronous::scheduler
#endif // BOOST_ASYNCHRONOUS_SCHEDULER_CPU_LOAD_POLICIES_HPP
