// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_ANY_QUEUE_CONTAINER_HPP
#define BOOST_ASYNC_ANY_QUEUE_CONTAINER_HPP

#include <vector>
#include <cstddef>
#include <utility>
#include <numeric>

#include <boost/shared_ptr.hpp>
#include <boost/asynchronous/queue/any_queue.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/queue/queue_base.hpp>
#include <boost/asynchronous/queue/find_queue_position.hpp>

namespace boost { namespace asynchronous
{
//used for easier construction
template <class Queue>
struct any_queue_container_config
{
    typedef Queue queue_type;
    typedef std::vector<boost::shared_ptr<Queue> > queue_sequence;
    template <typename... Args>
    any_queue_container_config(std::size_t number,Args... args):m_number(number)
    {
        for (std::size_t i = 0; i< number; ++i)
        {
            m_queues.push_back(boost::make_shared<Queue>(std::move(args)...));
        }
    }
    std::pair<typename queue_sequence::const_iterator,typename queue_sequence::const_iterator> queues()const
    {
        return std::make_pair(m_queues.begin(),m_queues.end());
    }

private:
    std::size_t m_number;
    queue_sequence m_queues;
};

// this class manages a sequence of (possibly different) queues
template <class JOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB,
          class PushPolicy = boost::asynchronous::default_find_position< > >
class any_queue_container: 
#ifndef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
                           public boost::asynchronous::any_queue_concept<JOB>,
#endif          
                           public boost::asynchronous::queue_base<JOB>,
                           public PushPolicy,
                           private boost::noncopyable
{
    typedef std::vector<boost::asynchronous::any_queue_ptr<JOB> > queues_type;
public:
    typedef JOB job_type;

    template <typename... Args>
    any_queue_container(Args... args)
    {
        //TODO
        //m_queues.reserve(sizeof...(args));
        ctor_helper(m_queues,args...);
    }
    std::vector<std::size_t> get_queue_size()const
    {
        std::vector<std::size_t> res;
        for (typename queues_type::const_iterator it = m_queues.begin(); it != m_queues.end();++it)
        {
            auto one_queue_vec = (*(*it)).get_queue_size();
            res.push_back(std::accumulate(one_queue_vec.begin(),one_queue_vec.end(),0,
                                          [](std::size_t rhs,std::size_t lhs){return rhs + lhs;}));
        }
        return res;
    }
    std::vector<std::size_t> get_max_queue_size() const
    {
        std::vector<std::size_t> res;
        for (typename queues_type::const_iterator it = m_queues.begin(); it != m_queues.end();++it)
        {
            auto one_queue_vec = (*(*it)).get_max_queue_size();
            res.push_back(std::accumulate(one_queue_vec.begin(),one_queue_vec.end(),0,
                                          [](std::size_t rhs,std::size_t lhs){return std::max(rhs,lhs);}));
        }
        return res;
    }
#ifndef BOOST_NO_RVALUE_REFERENCES
    void push(JOB&& j, std::size_t pos)
    {
        if (pos == std::numeric_limits<std::size_t>::max())
        {
            // this HAS to be the lowest prio job
            (*(m_queues.at(m_queues.size()-1))).push(std::forward<JOB>(j),pos);
        }
        else
        {
            // use the desired position
            (*(m_queues.at(this->find_position(pos,m_queues.size())))).push(std::forward<JOB>(j),pos);
        }
    }
    void push(JOB&& j)
    {
        push(std::forward<JOB>(j),0);
    }
#endif
    void push(JOB const& j, std::size_t pos=0)
    {
        if (pos == std::numeric_limits<std::size_t>::max())
        {
            // this HAS to be the lowest prio job
            (*(m_queues.at(m_queues.size()-1))).push(j,pos);
        }
        else
        {
            // use the desired position
            (*(m_queues.at(this->find_position(pos,m_queues.size())))).push(j,pos);
        }
    }
    JOB pop()
    {
        while(true)
        {
            // we iterate through our queues in order index 0 -> max to respect our priority
            for (typename queues_type::iterator it = m_queues.begin(); it != m_queues.end();++it)
            {
                JOB j;
                if((*(*it)).try_pop(j))
                {
                    return j;
                }
            }
            boost::this_thread::yield();
        }
    }
    bool try_pop(JOB& j)
    {
        // we iterate through our queues in order index 0 -> max to respect our priority
        for (typename queues_type::iterator it = m_queues.begin(); it != m_queues.end();++it)
        {
            if((*(*it)).try_pop(j))
            {
                return true;
            }
        }
        return false;
    }
    bool try_steal(JOB& j)
    {
        // we iterate through our queues in order index 0 -> max to respect our priority
        for (typename queues_type::iterator it = m_queues.begin(); it != m_queues.end();++it)
        {
            if((*(*it)).try_steal(j))
            {
                return true;
            }
        }
        return false;
    }
    template <typename T,typename Last>
    void ctor_helper(T& t, Last const& l)
    {
        std::pair<typename Last::queue_sequence::const_iterator,
                typename Last::queue_sequence::const_iterator> p = l.queues();
        for (typename Last::queue_sequence::const_iterator it = p.first; it != p.second; ++it)
        {
            boost::asynchronous::any_queue_ptr<JOB> q(*it);
            t.push_back(q);
        }
    }

    template <typename T,typename... Tail, typename Front>
    void ctor_helper(T& t,Front const& front,Tail const&... tail)
    {
        std::pair<typename Front::queue_sequence::const_iterator,
                  typename Front::queue_sequence::const_iterator> p = front.queues();
        for (typename Front::queue_sequence::const_iterator it = p.first; it != p.second; ++it)
        {
            boost::asynchronous::any_queue_ptr<JOB> q(*it);
            t.push_back(q);
        }
        ctor_helper(t,tail...);
    }


private:
    std::vector<boost::asynchronous::any_queue_ptr<JOB> > m_queues;
};

}}
#endif // BOOST_ASYNC_ANY_QUEUE_CONTAINER_HPP
