// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_SCHEDULER_TCP_SERVER_RESPONSE_HPP
#define BOOST_ASYNCHRONOUS_SCHEDULER_TCP_SERVER_RESPONSE_HPP

#include <string>
#include <boost/serialization/string.hpp>

namespace boost { namespace asynchronous { namespace tcp {

struct server_reponse
{
    server_reponse(long task_id, std::string const& task,std::string const& task_name )
        : m_task_id(task_id),m_task(task),m_task_name(task_name){}
    template <typename Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar & m_task_id;
      ar & m_task;
      ar & m_task_name;
    }
    long m_task_id;
    std::string m_task;
    std::string m_task_name;
};
}}}

#endif // BOOST_ASYNCHRONOUS_SCHEDULER_TCP_SERVER_RESPONSE_HPP
