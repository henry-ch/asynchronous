// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRON_PARALLEL_FOR_HPP
#define BOOST_ASYNCHRON_PARALLEL_FOR_HPP

#include <boost/asynchronous/detail/any_interruptible.hpp>

namespace boost { namespace asynchronous
{

template <class Iterator, class S1, class S2, class Func, class Callback>
boost::asynchronous::any_interruptible
parallel_for(Iterator begin, Iterator end, S1 const& scheduler,Func && func, S2 const& weak_cb_scheduler, Callback&& c,
             const std::string& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0)
{

}

}}
#endif // BOOST_ASYNCHRON_PARALLEL_FOR_HPP
