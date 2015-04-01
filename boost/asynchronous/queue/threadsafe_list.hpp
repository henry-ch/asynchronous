// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//  This is a slightly modified version of the code presented in
//  C++ Concurrency In Action, Anthony Williams.
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_QUEUE_THREADSAFE_LIST_HPP
#define BOOST_ASYNC_QUEUE_THREADSAFE_LIST_HPP

#include <memory>
#include <mutex>

#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/condition.hpp>

#include <boost/asynchronous/queue/queue_base.hpp>
#include <boost/asynchronous/queue/any_queue.hpp>

namespace boost { namespace asynchronous
{

template <class JOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB >
class threadsafe_list: 
#ifndef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
        public boost::asynchronous::any_queue_concept<JOB>,
#endif            
        public boost::asynchronous::queue_base<JOB>, private boost::noncopyable
{
private:
    struct node
    {
        JOB data;
        std::unique_ptr<node> next;
    };

    boost::mutex m_head_mutex;
    std::unique_ptr<node> m_head;
    boost::mutex m_tail_mutex;
    node* m_tail;
    boost::condition_variable m_data_cond;

public:
    typedef threadsafe_list<JOB> this_type;
    typedef JOB job_type;
    //typedef boost::mutex  mutex_type;
    //typedef boost::unique_lock<mutex_type> lock_type;


    threadsafe_list():m_head(new node),m_tail(m_head.get()) {}
    threadsafe_list(const threadsafe_list&) = delete;
    threadsafe_list& operator=(const threadsafe_list&) = delete;

    std::vector<std::size_t> get_queue_size() const
    {
        // not supported
        return std::vector<std::size_t>();
    }
    void push(JOB && j, std::size_t)
    {
        std::unique_ptr<node> p (new node);
        {
            boost::lock_guard<boost::mutex> tail_lock(m_tail_mutex);
            m_tail->data = std::forward<JOB>(j);
            node* const new_tail = p.get();
            m_tail->next = std::move(p);
            m_tail = new_tail;
        }
        m_data_cond.notify_one();
    }
    void push(JOB && j)
    {
        std::unique_ptr<node> p (new node);
        {
            boost::lock_guard<boost::mutex> tail_lock(m_tail_mutex);
            m_tail->data = std::forward<JOB>(j);
            node* const new_tail = p.get();
            m_tail->next = std::move(p);
            m_tail = new_tail;
        }
        m_data_cond.notify_one();
    }
    // todo remove when type_erasure supports rvalue ref
    void push(JOB const& j, std::size_t=0)
    {
        std::unique_ptr<node> p (new node);
        {
            boost::lock_guard<boost::mutex> tail_lock(m_tail_mutex);
            m_tail->data = j;
            node* const new_tail = p.get();
            m_tail->next = std::move(p);
            m_tail = new_tail;
        }
        m_data_cond.notify_one();
    }

    JOB pop()
    {
        std::unique_ptr<node> const old_head = wait_pop_head();
        return std::move(old_head->data);
    }
    bool try_pop(JOB& j)
    {
        std::unique_ptr<node> const old_head = try_pop_head(j);
        return !!old_head;
    }
    //TODO at other end
    bool try_steal(JOB& j)
    {
        std::unique_ptr<node> const old_head = try_pop_head(j);
        return !!old_head;
    }

private:
    node* get_tail()
    {
        boost::lock_guard<boost::mutex> tail_lock(m_tail_mutex);
        return m_tail;
    }
    std::unique_ptr<node> try_pop_head(JOB& value)
    {
        boost::lock_guard<boost::mutex> head_lock(m_head_mutex);
        if(m_head.get()==get_tail())
        {
            return std::unique_ptr<node>();
        }
        value=std::move(m_head->data);
        return pop_head();
    }

    std::unique_ptr<node> pop_head()
    {
        std::unique_ptr<node> old_head = std::move(m_head);
        m_head=std::move(old_head->next);
        return old_head;
    }

    boost::unique_lock<boost::mutex> wait_for_data()
    {
        boost::unique_lock<boost::mutex> head_lock(m_head_mutex);
        //TODO correct timing
//        while( !m_data_cond.wait_for(head_lock,std::chrono::milliseconds(10),[&]{return (this->m_head.get()!= this->get_tail());}))
//        {
//        }
        m_data_cond.wait(head_lock,[&]{return (this->m_head.get()!= this->get_tail());});
        return std::move(head_lock);
    }

    std::unique_ptr<node> wait_pop_head()
    {
        boost::unique_lock<boost::mutex> head_lock(wait_for_data());
        return pop_head();
    }
};

}} // boost::async::queue

#endif // BOOST_ASYNC_QUEUE_THREADSAFE_LIST_HPP
