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
    std::size_t max_size() const {return 0;}
    void reset_max_size(){}
};

struct lockfree_size
{
    lockfree_size() noexcept
        : m_size(0)
    {}
    void increase(){++m_size;}
    void decrease(){--m_size;}
    std::size_t size() const {return m_size;}
    std::size_t max_size() const {return 0;}
    void reset_max_size(){}
private:
    std::atomic<std::size_t> m_size;
};

struct lockfree_size_max_size
{
    lockfree_size_max_size() noexcept
        : m_size(0),m_max_size(0)
    {}
    void increase()
    {
        ++m_size;
        // actualize the max size this queue ever had, useful for diagnostics
        bool done = false;
        do
        {
            auto old = m_max_size.load();
            done = m_max_size.compare_exchange_strong(old,std::max(m_size.load(),old));
        }
        while(!done);
    }
    void decrease(){--m_size;}
    std::size_t size() const {return m_size;}
    std::size_t max_size() const {return m_max_size;}
    void reset_max_size()
    {
        m_max_size = 0;
    }

private:
    std::atomic<std::size_t> m_size;
    std::atomic<std::size_t> m_max_size;
};

}} // boost::asynchronous
#endif // BOOST_ASYNCHRONOUS_QUEUE_LOCKFREE_SIZE_HPP
