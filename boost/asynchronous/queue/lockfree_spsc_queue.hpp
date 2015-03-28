// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_QUEUE_LOCKFREE_SPSC_QUEUE_HPP
#define BOOST_ASYNC_QUEUE_LOCKFREE_SPSC_QUEUE_HPP

#include <boost/lockfree/spsc_queue.hpp>
#include <boost/thread/thread.hpp>

#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/queue/queue_base.hpp>
#include <boost/asynchronous/queue/any_queue.hpp>
#include <boost/asynchronous/queue/detail/lockfree_size.hpp>

namespace boost { namespace asynchronous
{
template <class JOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB, class Size = boost::asynchronous::no_lockfree_size>
class lockfree_spsc_queue: 
#ifndef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
        public boost::asynchronous::any_queue_concept<JOB>,
#endif           
        public boost::asynchronous::queue_base<JOB>, private boost::noncopyable
{
public:
    typedef lockfree_spsc_queue<JOB> this_type;
    typedef JOB job_type;

    std::size_t get_queue_size() const
    {
        return Size::size();
    }
#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES
    template<typename... Args>
    lockfree_spsc_queue(Args... args):m_queue(std::move(args)...){}
    lockfree_spsc_queue():m_queue(16){}
#else
    lockfree_spsc_queue(std::size_t size=16):m_queue(size){}
#endif
#ifndef BOOST_NO_RVALUE_REFERENCES
    void push(JOB && j, std::size_t)
    {
        while (!m_queue.push(std::forward<JOB>(j)))
        {
            boost::this_thread::yield();
        }
        Size::increase();
    }
    void push(JOB && j)
    {
        while (!m_queue.push(std::forward<JOB>(j)))
        {
            boost::this_thread::yield();
        }
        Size::increase();
    }
#endif
    void push(JOB const& j, std::size_t=0)
    {
        while (!m_queue.push(j))
        {
            boost::this_thread::yield();
        }
        Size::increase();
    }

    JOB pop()
    {
        JOB res;
        while (!m_queue.pop(res))
        {
            boost::this_thread::yield();
        }
        Size::decrease();
#ifndef BOOST_NO_RVALUE_REFERENCES
        return std::move(res);
#else
        return res;
#endif
    }
    bool try_pop(JOB& job)
    {
        bool res = m_queue.pop(job);
        if (res)
        {
            Size::decrease();
        }
        return res;
    }
    // not supported
    bool try_steal(JOB&)
    {
        return false;
    }

private:
    boost::lockfree::spsc_queue<JOB> m_queue;
};

}} // boost::async::queue

#endif // BOOST_ASYNC_QUEUE_LOCKFREE_SPSC_QUEUE_HPP
