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
#include <boost/asynchronous/scheduler/tcp/detail/transport_exception.hpp>

// supported commands:
// 1 => get job
// 2 => return result
#define BOOST_ASYNCHRONOUS_TCP_CLIENT_GET_JOB 1
#define BOOST_ASYNCHRONOUS_TCP_CLIENT_JOB_RESULT 2


namespace boost { namespace asynchronous { namespace tcp {


struct client_request
{
    // what contains the payload of the message: result, exception
    struct message_payload
    {
        message_payload(std::string const& data=""):m_data(data),m_has_exception(false){}
        template <typename Archive>
        void serialize(Archive& ar, const unsigned int /*version*/)
        {
          ar & m_data;
          ar & m_has_exception;
          ar & m_exception;
        }
        // used to return result if BOOST_ASYNCHRONOUS_TCP_CLIENT_JOB_RESULT
        std::string m_data;
        // exception?
        bool m_has_exception;
        boost::asynchronous::tcp::transport_exception m_exception;
    };

    client_request(char cmd_id=0, std::string const& data="")
        :m_cmd_id(cmd_id)
        ,m_task_id(0)
        ,m_load(data)
    {}
    template <typename Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar & m_cmd_id;
      ar & m_task_id;
      ar & m_load;
    }
    char m_cmd_id;
    // id of executed task if BOOST_ASYNCHRONOUS_TCP_CLIENT_JOB_RESULT
    long m_task_id;
    message_payload m_load;
};

}}}
#endif // BOOST_ASYNCHRONOUS_SCHEDULER_TCP_CLIENT_REQUEST_HPP
