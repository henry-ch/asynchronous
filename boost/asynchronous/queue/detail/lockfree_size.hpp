// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_QUEUE_LOCKFREE_SIZE_HPP
#define BOOST_ASYNCHRONOUS_QUEUE_LOCKFREE_SIZE_HPP


#include <atomic>


namespace boost { namespace asynchronous
{

struct no_lockfree_size
{
    void increase(){}
    void decrease(){}
    std::size_t size() const {return 0;}
};

struct lockfree_size
{
    lockfree_size() noexcept
        : m_size(0)
    {}
    void increase(){++m_size;}
    void decrease(){--m_size;}
    std::size_t size() const {return m_size;}
private:
    std::atomic<std::size_t> m_size;
};

}} // boost::asynchronous
#endif // BOOST_ASYNCHRONOUS_QUEUE_LOCKFREE_SIZE_HPP
