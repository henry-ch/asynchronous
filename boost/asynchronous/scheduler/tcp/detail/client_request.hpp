// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_SCHEDULER_TCP_CLIENT_REQUEST_HPP
#define BOOST_ASYNCHRONOUS_SCHEDULER_TCP_CLIENT_REQUEST_HPP

#include <string>

// supported commands:
// 1 => get job
// 2 => return result
#define BOOST_ASYNCHRONOUS_TCP_CLIENT_GET_JOB 1
#define BOOST_ASYNCHRONOUS_TCP_CLIENT_JOB_RESULT 2


namespace boost { namespace asynchronous { namespace tcp {


struct client_request
{
    client_request(char cmd_id=0, std::string const& data=""):m_cmd_id(cmd_id),m_data(data){}
    template <typename Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar & m_cmd_id;
      ar & m_data;
      ar & m_task_id;
    }
    char m_cmd_id;
    // used to return result if BOOST_ASYNCHRONOUS_TCP_CLIENT_JOB_RESULT
    std::string m_data;
    // id of executed task if BOOST_ASYNCHRONOUS_TCP_CLIENT_JOB_RESULT
    long m_task_id;
};

}}}
#endif // BOOST_ASYNCHRONOUS_SCHEDULER_TCP_CLIENT_REQUEST_HPP
