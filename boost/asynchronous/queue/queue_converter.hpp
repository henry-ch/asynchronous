// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_QUEUE_CONVERTER_HPP
#define BOOST_ASYNCHRONOUS_QUEUE_CONVERTER_HPP

#include <boost/asynchronous/queue/any_queue.hpp>

// converts job stealing from a queue to another provided jobs are (type-erasure) up-castable
// Implements only what is needed for stealing
namespace boost { namespace asynchronous
{
template <class To, class From>
class queue_converter:
        #ifdef BOOST_ASYNCHRONOUS_NO_TYPE_ERASURE
                public boost::asynchronous::any_queue_concept<To>,
        #endif
                public boost::asynchronous::queue_base<To>, private boost::noncopyable
{
public:
    queue_converter(boost::asynchronous::any_queue_ptr<From> const& f):m_from_queue(f){}

    virtual ~queue_converter<To,From>(){}
    virtual void push(To&&,std::size_t)
    {
        // not implemented
    }

    virtual void push(To&&)
    {
        // not implemented
    }
    virtual To pop()
    {
        // not implemented
        return To();
    }

    virtual bool try_pop(To&)
    {
        // not implemented
        return false;
    }
    virtual bool try_steal(To& to)
    {
        From job;
        bool success = m_from_queue->try_steal(job);
        if (success)
        {
            to = job;
            return true;
        }
        //else
        return false;
    }
    virtual std::size_t get_queue_size()const
    {
        return m_from_queue->get_queue_size();
    }

private:
    boost::asynchronous::any_queue_ptr<From> m_from_queue;
};
}}
#endif // BOOST_ASYNCHRONOUS_QUEUE_CONVERTER_HPP
