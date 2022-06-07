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
    #if defined(__ANDROID__)
        // Android doesn't have pthread_setaffinity_np, we need to use the non-pthread interface.
        // Some sources claim that this affects the whole process, but that is incorrect.
        // A PID of 0 in the system call will always use the kernel's `current` task, i.e. the
        // currently running thread. In general, the `pid` argument is actually what userspace
        // sometimes calls a TID (= PID in the Linux kernel, see gettid()), not what userspace
        // claims to be the PID (= TGID in the kernel, the PID of the thread group leader).
        ::sched_setaffinity(0 /* current thread */, sizeof(cpuset), &cpuset);
    #else
        ::pthread_setaffinity_np( ::pthread_self(), sizeof( cpuset), & cpuset);
    #endif
#endif
# if defined(WINVER)
        ::SetThreadAffinityMask( ::GetCurrentThread(), ( DWORD_PTR)1 << proc_);
#endif
    }

    unsigned int proc_;
};

}}}

#endif // BOOST_ASYNCHRONOUS_SCHEDULER_PROCESSOR_BIND_HPP

