// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef DUMMY_TCP_TASK_HPP
#define DUMMY_TCP_TASK_HPP

#include <iostream>
#include <boost/thread/thread.hpp>
#include <boost/asynchronous/scheduler/serializable_task.hpp>

// for first tests
struct dummy_tcp_task : public boost::asynchronous::serializable_task
{
    dummy_tcp_task(int d):boost::asynchronous::serializable_task("dummy_tcp_task"),m_data(d){}
    template <class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/)
    {
        ar & m_data;
    }
    int operator()()const
    {
        std::cout << "dummy_tcp_task operator(): " << m_data << std::endl;
        boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
        std::cout << "dummy_tcp_task operator() finished" << std::endl;
        return m_data;
    }

    int m_data;
};

#endif // DUMMY_TCP_TASK_HPP
