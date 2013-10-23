// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_SCHEDULER_TCP_TRANSPORT_EXCEPTION_HPP
#define BOOST_ASYNCHRONOUS_SCHEDULER_TCP_TRANSPORT_EXCEPTION_HPP

#include <string>
#include <exception>

namespace boost { namespace asynchronous { namespace tcp {

struct transport_exception : public std::exception
{
    transport_exception(std::string const& text = "") noexcept:m_text(text){}
    virtual ~transport_exception() noexcept{}
    virtual const char* what() const noexcept
    {
        return m_text.c_str();
    }
    template <typename Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar & m_text;
    }
    std::string m_text;
};


}}}
#endif // BOOST_ASYNCHRONOUS_SCHEDULER_TCP_TRANSPORT_EXCEPTION_HPP
