// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_QUEUE_LOCKFREE_QUEUE_HPP
#define BOOST_ASYNC_QUEUE_LOCKFREE_QUEUE_HPP


#include <boost/thread/thread.hpp>
#include <boost/smart_ptr/scoped_ptr.hpp>

#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/queue/queue_base.hpp>
#include <boost/asynchronous/queue/any_queue.hpp>
#include <boost/asynchronous/queue/detail/lockfree_size.hpp>

#ifdef BOOST_ASYNCHRONOUS_NO_LOCKFREE
#include <boost/asynchronous/queue/guarded_deque.hpp>
namespace boost { namespace asynchronous
{
template <class Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB, class Size = boost::asynchronous::no_lockfree_size>
using lockfree_queue = boost::asynchronous::guarded_deque<Job,Size>;
}
}
#else
#include <boost/lockfree/queue.hpp>
namespace boost { namespace asynchronous
{
template <class JOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB, class Size = boost::asynchronous::no_lockfree_size >
class lockfree_queue: 
#ifndef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
        public boost::asynchronous::any_queue_concept<JOB>,
#endif          
        public boost::asynchronous::queue_base<JOB>, Size, private boost::noncopyable
{
public:
    typedef lockfree_queue<JOB> this_type;
    typedef JOB job_type;

    std::vector<std::size_t> get_queue_size() const
    {
        std::vector<std::size_t> res;
        res.push_back(Size::size());
        return res;
    }
    std::vector<std::size_t> get_max_queue_size() const
    {
        std::vector<std::size_t> res;
        res.push_back(Size::max_size());
        return res;
    }
#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES
    template<typename... Args>
    lockfree_queue(Args... args):m_queue(std::move(args)...){}
#ifndef _MSC_VER
    lockfree_queue():m_queue(16){}
#endif
#else
    lockfree_queue(std::size_t size=16):m_queue(size){}
#endif
#ifndef BOOST_NO_RVALUE_REFERENCES

    void push(JOB && j, std::size_t)
    {
        JOB* task = new JOB(std::forward<JOB>(j));
        while (!m_queue.push(task))
        {
            boost::this_thread::yield();
        }
        Size::increase();
    }
    void push(JOB && j)
    {
        JOB* task = new JOB(std::forward<JOB>(j));
        while (!m_queue.push(task))
        {
            boost::this_thread::yield();
        }
        Size::increase();
    }
#endif
    void push(JOB const& j, std::size_t=0)
    {
        JOB* task = new JOB(j);
        while (!m_queue.push(task))
        {
            boost::this_thread::yield();
        }
        Size::increase();
    }

    JOB pop()
    {
        JOB* resp;
        while (!m_queue.pop(resp))
        {
            boost::this_thread::yield();
        }
        Size::decrease();
        boost::scoped_ptr<JOB> for_cleanup(resp);
#ifndef BOOST_NO_RVALUE_REFERENCES
        JOB res(std::move(*resp));
        return std::move(res);
#else
        JOB res(*resp);
        return res;
#endif
    }
    bool try_pop(JOB& job)
    {
        JOB* jptr;
        bool res = m_queue.pop(jptr);
        if (res)
        {
            Size::decrease();
            boost::scoped_ptr<JOB> for_cleanup(jptr);
#ifndef BOOST_NO_RVALUE_REFERENCES
            job = std::move(*jptr);
#else
            job = *jptr;
#endif
            return true;
        }
        return false;
    }
    bool try_steal(JOB& job)
    {
        JOB* jptr;
        bool res = m_queue.pop(jptr);
        if (res)
        {
            Size::decrease();
            boost::scoped_ptr<JOB> for_cleanup(jptr);
#ifndef BOOST_NO_RVALUE_REFERENCES
            job = std::move(*jptr);
#else
            job = *jptr;
#endif
            return true;
        }
        return false;
    }

private:
    boost::lockfree::queue<JOB*> m_queue;
};
}} // boost::async::queue
#endif
#endif // BOOST_ASYNC_QUEUE_LOCKFREE_QUEUE_HPP
