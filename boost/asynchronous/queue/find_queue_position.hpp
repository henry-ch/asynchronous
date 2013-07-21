// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_FIND_QUEUE_POSITION_HPP
#define BOOST_ASYNC_FIND_QUEUE_POSITION_HPP

#include <atomic>
#include <random>
#include <cstddef>

namespace boost { namespace asynchronous
{
struct sequential_push_policy
{
    sequential_push_policy():m_next(0){}
    std::size_t find_any_position()const
    {
        ++m_next;
        return m_next.load();
    }
    mutable std::atomic<std::size_t> m_next;
};

template <std::size_t  max_val=999>
struct default_random_push_policy
{
    default_random_push_policy():m_generator(),m_distribution(0,max_val){}
    std::size_t find_any_position()const
    {
        return m_distribution(m_generator);
    }
    mutable std::default_random_engine m_generator;
    mutable std::uniform_int_distribution<std::size_t> m_distribution;
};

template <class RandomPositionPolicy = boost::asynchronous::default_random_push_policy<> >
struct default_find_position : public RandomPositionPolicy
{
  std::size_t find_position(std::size_t user_pos,std::size_t queue_size)const
  {
    if (user_pos == 0)
    {
        // user does not care which queue we use
        return (this->find_any_position()%queue_size);
    }
    else
    {
        // use the desired position, limiting at the lowest prio
        return std::min(user_pos-1,queue_size-1);
    }
  }
};

}}

#endif // BOOST_ASYNC_FIND_QUEUE_POSITION_HPP
