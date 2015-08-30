// Boost.Asynchronous library
// Copyright (C) Christophe Henry 2015
// Copied from Boost.Context, Copyright Oliver Kowalke 2009.
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_SCHEDULER_PROCESSOR_BIND_HPP
#define BOOST_ASYNCHRONOUS_SCHEDULER_PROCESSOR_BIND_HPP

# if defined(LINUX)
extern "C"
{
#include <pthread.h>
#include <sched.h>
}
#endif

# if defined(WINVER)
extern "C"
{
#include <winsock2.h>
#include <windows.h>
}
#endif

#include <boost/thread/thread.hpp>

namespace boost { namespace asynchronous { namespace detail
{

struct processor_bind_task
{
    processor_bind_task(unsigned int p):proc_(p % boost::thread::hardware_concurrency()){}
    void operator()()const
    {
#if defined(linux) || defined(__linux) || defined(__linux__)
        cpu_set_t cpuset;
        CPU_ZERO( & cpuset);
        CPU_SET( proc_, & cpuset);
        ::pthread_setaffinity_np( ::pthread_self(), sizeof( cpuset), & cpuset);
#endif
# if defined(WINVER)
        ::SetThreadAffinityMask( ::GetCurrentThread(), ( DWORD_PTR)1 << proc_);
#endif
    }

    unsigned int proc_;
};

}}}

#endif // BOOST_ASYNCHRONOUS_SCHEDULER_PROCESSOR_BIND_HPP

