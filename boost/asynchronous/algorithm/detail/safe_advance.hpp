// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_SAFE_ADVANCE_HPP
#define BOOST_ASYNCHRONOUS_SAFE_ADVANCE_HPP

#include <iterator>

namespace boost { namespace asynchronous
{
namespace detail
{
template<typename Iterator, typename Distance>
void safe_advance_helper(Iterator& it, Distance n, Iterator end,std::random_access_iterator_tag)
{
    if (it + n < end)
    {
        std::advance(it,n);
    }
    else
    {
        // only until end
        it = end;
    }
}
template<typename Iterator, typename Distance>
void safe_advance_helper(Iterator& it, Distance n, Iterator end,std::input_iterator_tag)
{
    while ((it != end) && (--n >= 0))
    {
        std::advance(it,1);
    }
}

template<typename Iterator, typename Distance>
void safe_advance(Iterator& it, Distance n, Iterator end)
{
    safe_advance_helper(it,n,end,typename std::iterator_traits<Iterator>::iterator_category());
}
}}}
#endif // BOOST_ASYNCHRONOUS_SAFE_ADVANCE_HPP
