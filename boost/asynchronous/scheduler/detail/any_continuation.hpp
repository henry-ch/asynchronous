// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_SCHEDULER_DETAIL_ANY_CONTINUATION_HPP
#define BOOST_ASYNCHRONOUS_SCHEDULER_DETAIL_ANY_CONTINUATION_HPP

#include <boost/asynchronous/callable_any.hpp>

BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_is_ready), is_ready, 0);

namespace boost { namespace asynchronous
{
struct any_continuation_concept :
 ::boost::mpl::vector<
    boost::asynchronous::any_callable_concept,
    boost::asynchronous::has_is_ready<bool(), boost::type_erasure::_self>
> {};
typedef boost::type_erasure::any<any_continuation_concept> any_continuation;


}} // boost::asynchronous

#endif // BOOST_ASYNCHRONOUS_SCHEDULER_DETAIL_ANY_CONTINUATION_HPP
