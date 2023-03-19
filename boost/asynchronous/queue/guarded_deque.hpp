// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_QUEUE_GUARDED_DEQUE_HPP
#define BOOST_ASYNC_QUEUE_GUARDED_DEQUE_HPP

#include <functional>
#include <mutex>
#include <chrono>
#include <deque>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/queue/queue_base.hpp>
#include <boost/asynchronous/queue/any_queue.hpp>

namespace boost { namespace asynchronous
{

template <class JOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB, int WaitTime=5 >
class guarded_deque: 
#ifndef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
        public boost::asynchronous::any_queue_concept<JOB>,
#endif           
        public boost::asynchronous::queue_base<JOB>
{
public:
    typedef guarded_deque<JOB,WaitTime> this_type;
    typedef std::mutex  mutex_type;
    typedef std::unique_lock<mutex_type> lock_type;
    template<typename... Args>
    guarded_deque(Args... ){}
    guarded_deque(const guarded_deque&) = delete;
    guarded_deque& operator=(const guarded_deque&) = delete;

    std::vector<std::size_t> get_queue_size() const
    {
        std::vector<std::size_t> res;
        lock_type lock(const_cast<this_type&>(*this).m_mutex);
        res.push_back(m_jobs.size());
        return res;
    }
    std::vector<std::size_t> get_max_queue_size() const
    {
        std::vector<std::size_t> res;
        res.reserve(1);
        lock_type lock(const_cast<this_type&>(*this).m_mutex);
        res.push_back(m_max_size);
        return res;
    }
    void reset_max_queue_size()
    {
        lock_type lock(m_mutex);
        m_max_size = 0;
    }

    bool is_not_empty() const
    {
        return !m_jobs.empty();
    }
#ifndef BOOST_NO_RVALUE_REFERENCES
    void push(JOB && j, std::size_t)
    {
        lock_type lock(m_mutex);
        m_max_size = std::max(m_max_size,m_jobs.size());
        m_jobs.emplace_front(std::forward<JOB>(j));
        lock.unlock();
        m_not_empty.notify_one();
    }
    void push(JOB && j)
    {
        lock_type lock(m_mutex);
        m_max_size = std::max(m_max_size,m_jobs.size());
        m_jobs.emplace_front(std::forward<JOB>(j));
        lock.unlock();
        m_not_empty.notify_one();
    }
#endif
    void push(JOB const& j, std::size_t=0)
    {
        lock_type lock(m_mutex);
        m_max_size = std::max(m_max_size,m_jobs.size());
        m_jobs.push_front(j);
        lock.unlock();
        m_not_empty.notify_one();
    }

    //todo move?
    JOB pop()
    {
        lock_type lock(m_mutex);
        m_not_empty.wait(lock, std::bind(&this_type::is_not_empty, this));
        JOB res = m_jobs.back();
        m_jobs.pop_back();
        lock.unlock();
        return res;
    }
    bool try_pop(JOB& job)
    {
        lock_type lock(m_mutex);
        if (is_not_empty())
        {
            job = m_jobs.back();
            m_jobs.pop_back();
            return true;
        }
        else
        {
            // lock with short waiting time
            m_not_empty.wait_for(lock, std::chrono::milliseconds(WaitTime), std::bind(&this_type::is_not_empty, this));
            if (is_not_empty())
            {
                job = m_jobs.back();
                m_jobs.pop_back();
                return true;
            }
        }
        return false;
    }
    //TODO at other end
    bool try_steal(JOB& job)
    {
        lock_type lock(m_mutex);
        if (is_not_empty())
        {
            job = m_jobs.back();
            m_jobs.pop_back();
            return true;
        }
        return false;
    }

private:
    std::deque<JOB> m_jobs;
    std::size_t m_max_size=0;
    std::condition_variable m_not_empty;
    mutex_type m_mutex;
};

}} // boost::async::queue

#endif /* BOOST_ASYNC_QUEUE_GUARDED_DEQUE_HPP */
