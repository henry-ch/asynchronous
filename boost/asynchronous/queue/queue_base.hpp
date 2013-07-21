// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_QUEUE_QUEUE_BASE_HPP
#define BOOST_ASYNC_QUEUE_QUEUE_BASE_HPP

#include <cstddef>
#include <boost/asynchronous/job_traits.hpp>

namespace boost { namespace asynchronous
{

template <class JOB>
struct queue_base
{
    typedef JOB job_type;
    typedef typename boost::asynchronous::job_traits<JOB>::diagnostic_type diagnostic_type;
};

}} // boost::asynchronous::queue

#endif // BOOST_ASYNC_QUEUE_QUEUE_BASE_HPP
