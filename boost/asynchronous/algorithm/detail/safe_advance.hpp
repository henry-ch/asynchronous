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

// finds the best position to cut a range in 2: in the middle if random access iterators, given cutoff otherwise
template <class Iterator, class Distance>
Iterator find_cutoff_helper(Iterator it, Distance n, Iterator end,std::random_access_iterator_tag)
{
    if (end-it <= n)
        return end;
    return it + (end-it)/2;
}
template <class Iterator, class Distance>
Iterator find_cutoff_helper(Iterator it, Distance n, Iterator end,std::input_iterator_tag)
{
    // advance up to cutoff or end
    boost::asynchronous::detail::safe_advance(it,n,end);
    return it;
}
template <class Iterator, class Distance>
Iterator find_cutoff(Iterator it, Distance n, Iterator end)
{
    return find_cutoff_helper(it,n,end,typename std::iterator_traits<Iterator>::iterator_category());
}

template <class It, class Distance>
std::pair<It,It> find_cutoff_and_prev_helper(It it, Distance n, It end,std::random_access_iterator_tag)
{
    if (end-it <= n)
        return std::make_pair(end,end);
    return std::make_pair(it -1 + (end-it)/2,it + (end-it)/2);
}
template <class It, class Distance>
std::pair<It,It> find_cutoff_and_prev_helper(It it, Distance n, It end,std::input_iterator_tag)
{
    // advance up to cutoff or end
    It it2 = it;
    boost::asynchronous::detail::safe_advance(it2,n-1,end);
    it = it2;
    boost::asynchronous::detail::safe_advance(it,1,end);
    return std::make_pair(it2,it);
}
template <class It, class Distance>
std::pair<It,It> find_cutoff_and_prev(It it, Distance n, It end)
{
    return find_cutoff_and_prev_helper(it,n,end,typename std::iterator_traits<It>::iterator_category());
}

}}}
#endif // BOOST_ASYNCHRONOUS_SAFE_ADVANCE_HPP
