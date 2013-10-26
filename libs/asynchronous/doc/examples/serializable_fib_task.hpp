// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org
#ifndef SERIALIZABLE_FIB_TASK_HPP
#define SERIALIZABLE_FIB_TASK_HPP

#include <iostream>
#include <boost/asynchronous/scheduler/serializable_task.hpp>
#include <boost/asynchronous/continuation_task.hpp>

namespace tcp_example
{
// a simple, single-threaded fibonacci function used for cutoff
long serial_fib( long n ) {
    if( n<2 )
        return n;
    else
        return serial_fib(n-1)+serial_fib(n-2);
}

// our recursive fibonacci tasks. Needs to inherit continuation_task<value type returned by this task>
struct fib_task : public boost::asynchronous::continuation_task<long>
{
    fib_task(long n,long cutoff):n_(n),cutoff_(cutoff){}
    void operator()()const
    {
        // the result of this task, will be either set directly if < cutoff, otherwise when taks is ready
        boost::asynchronous::continuation_result<long> task_res = this_task_result();
        if (n_<cutoff_)
        {
            // n < cutoff => execute ourselves
            task_res.set_value(serial_fib(n_));
        }
        else
        {
            // n> cutoff, create 2 new tasks and when both are done, set our result (res(task1) + res(task2))
            boost::asynchronous::create_continuation<long>(
                        // called when subtasks are done, set our result
                        [task_res](std::tuple<boost::future<long>,boost::future<long> >&& res)
                        {
                            long r = std::get<0>(res).get() + std::get<1>(res).get();
                            task_res.set_value(r);
                        },
                        // recursive tasks
                        fib_task(n_-1,cutoff_),
                        fib_task(n_-2,cutoff_));
        }
    }
    long n_;
    long cutoff_;
};

struct serializable_fib_task : public boost::asynchronous::serializable_task
{
    serializable_fib_task(long n,long cutoff):boost::asynchronous::serializable_task("serializable_fib_task"),n_(n),cutoff_(cutoff){}
    template <class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/)
    {
        ar & n_;
        ar & cutoff_;
    }
    auto operator()()const -> decltype(boost::asynchronous::top_level_continuation<long>(tcp_example::fib_task(long(0),long(0))))
    {
        std::cout << "serializable_fib_task operator(): " << n_ << "," << cutoff_ << std::endl;
        return boost::asynchronous::top_level_continuation<long>(tcp_example::fib_task(n_,cutoff_));

    }
    long n_;
    long cutoff_;
};

}

#endif // SERIALIZABLE_FIB_TASK_HPP
