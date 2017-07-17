// Boost.Asynchronous library
// Copyright (C) Christophe Henry 2017
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_SCHEDULER_EXECUTE_IN_ALL_THREADS_HPP
#define BOOST_ASYNCHRONOUS_SCHEDULER_EXECUTE_IN_ALL_THREADS_HPP

#include <future>
#include <memory>
#include <boost/asynchronous/callable_any.hpp>

namespace boost { namespace asynchronous { namespace detail
{
// execute task in a thread, set promise when done
struct execute_in_all_threads_task
{
    execute_in_all_threads_task(boost::asynchronous::any_callable c, std::promise<void> p)
        : to_call_(std::move(c))
        , done_promise_(std::make_shared<std::promise<void>>(std::move(p)))
    {}
    void operator()()
    {
        to_call_();
        done_promise_->set_value();
    }
    boost::asynchronous::any_callable to_call_;
    std::shared_ptr<std::promise<void>> done_promise_;
};

}}}

#endif // BOOST_ASYNCHRONOUS_SCHEDULER_EXECUTE_IN_ALL_THREADS_HPP

