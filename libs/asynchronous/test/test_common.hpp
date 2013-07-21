// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_TEST_COMMON_HPP
#define BOOST_ASYNC_TEST_COMMON_HPP

#include <algorithm> 

namespace boost { namespace asynchronous { namespace test
{

//helpers
template <class It>
bool contains_id(It begin, It end,boost::thread::id tid)
{
    // todo lower_bound?
    return (std::find(begin,end,tid) != end);
}
template <class It>
std::size_t number_of_threads(It begin, It end)
{
    return std::distance(begin,end);
}

}}}
#endif //  BOOST_ASYNC_TEST_COMMON_HPP
