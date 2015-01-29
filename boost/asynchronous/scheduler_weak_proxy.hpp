// Copyright 2012 Christophe Henry
// christophe DOT j DOT henry AT googlemail DOT com
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ASYNC_SCHEDULER_WEAK_PROXY_HPP
#define BOOST_ASYNC_SCHEDULER_WEAK_PROXY_HPP

#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/async/any_shared_scheduler.hpp>

namespace boost { namespace asynchronous
{

// scheduler proxy for use in the "inside" of the active object world
template<class S>
class scheduler_weak_proxy
{
public:
    typedef S scheduler_type;
    typedef typename S::job_type job_type;

    explicit scheduler_weak_proxy(boost::shared_ptr<scheduler_type> scheduler): m_scheduler(std::move(scheduler)){}
    any_shared_scheduler<job_type> lock()
    {

    }

private:
    boost::weak_ptr<scheduler_type> m_scheduler;
};

}} // boost::asynchronous


#endif /* BOOST_ASYNC_SCHEDULER_WEAK_PROXY_HPP */
