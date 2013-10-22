// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef  BOOST_ASYNCHRONOUS_SCHEDULER_SERIALIZABLE_TASK_HPP
#define BOOST_ASYNCHRONOUS_SCHEDULER_SERIALIZABLE_TASK_HPP
#include <string>

namespace boost { namespace asynchronous
{
struct serializable_task
{
    serializable_task(std::string const& task_name):m_task_name(task_name){}
    typedef int serializable_type;
    std::string get_task_name()const
    {
        return m_task_name;
    }
    std::string m_task_name;
};
}}

#endif // BOOST_ASYNCHRONOUS_SCHEDULER_SERIALIZABLE_TASK_HPP
