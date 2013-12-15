// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_QUEUE_CIRCULAR_QUEUE_HPP
#define BOOST_ASYNC_QUEUE_CIRCULAR_QUEUE_HPP

// circular queue
#include <boost/circular_buffer.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/condition.hpp>
#include <boost/bind.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/queue/queue_base.hpp>
#include <boost/asynchronous/queue/any_queue.hpp>

namespace boost { namespace asynchronous
{

template <class JOB = boost::asynchronous::any_callable >
class circular_queue: 
#ifdef BOOST_ASYNCHRONOUS_NO_TYPE_ERASURE
        public boost::asynchronous::any_queue_concept<JOB>,
#endif         
        public boost::asynchronous::queue_base<JOB>, private boost::noncopyable
{
public:
    typedef circular_queue<JOB> this_type;
    typedef JOB job_type;
    typedef boost::mutex  mutex_type;
    typedef boost::unique_lock<mutex_type> lock_type;

    circular_queue(std::size_t capacity=100): m_size(0),m_jobs(capacity)
    {

    }

    std::size_t get_queue_size() const
    {
        return m_size;
    }
    bool is_not_empty() const
    {
        return m_size > 0;
    }
    bool containerFull(void) const
    {
      return m_size == m_jobs.capacity();
    }
    std::size_t computeNewCapacity(std::size_t currentCapacity)
    {
      return currentCapacity + currentCapacity / 2 + 1;
    }
    void increaseCapacity()
    {
      std::size_t newCapacity = computeNewCapacity(m_jobs.capacity());
      m_jobs.set_capacity(newCapacity);
    }
#ifndef BOOST_NO_RVALUE_REFERENCES
    void push(JOB && j, std::size_t)
    {
        lock_type lock(m_mutex);
        if(containerFull())
        {
          increaseCapacity();
        }
        m_jobs.push_front(std::forward<JOB>(j));
        ++m_size;
        lock.unlock();
        m_not_empty.notify_one();
    }
    void push(JOB && j)
    {
        lock_type lock(m_mutex);
        if(containerFull())
        {
          increaseCapacity();
        }
        m_jobs.push_front(std::forward<JOB>(j));
        ++m_size;
        lock.unlock();
        m_not_empty.notify_one();
    }
#endif
    void push(JOB const& j, std::size_t=0)
    {
        lock_type lock(m_mutex);
        if(containerFull())
        {
          increaseCapacity();
        }
        m_jobs.push_front(j);
        ++m_size;
        lock.unlock();
        m_not_empty.notify_one();
    }

    //todo move?
    JOB pop()
    {
        lock_type lock(m_mutex);
        m_not_empty.wait(lock, boost::bind(&this_type::is_not_empty, this));
        JOB res = m_jobs[--m_size];
        m_jobs[m_size]=JOB();
        lock.unlock();
        return res;
    }
    bool try_pop(JOB& job)
    {
        lock_type lock(m_mutex);
        if (is_not_empty())
        {
            job = m_jobs[--m_size];
            m_jobs[m_size]=JOB();
            return true;
        }
        return false;
    }
    //TODO at other end
    bool try_steal(JOB& job)
    {
        lock_type lock(m_mutex);
        if (is_not_empty())
        {
            job = m_jobs[--m_size];
            m_jobs[m_size]=JOB();
            return true;
        }
        return false;
    }
private:
    std::size_t m_size;
    boost::circular_buffer<JOB> m_jobs;
    boost::condition m_not_empty;
    mutex_type m_mutex;
};

}} // boost::async::queue

#endif // BOOST_ASYNC_QUEUE_CIRCULAR_QUEUE_HPP
