// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_ANY_SERIALIZABLE_HPP
#define BOOST_ASYNCHRONOUS_ANY_SERIALIZABLE_HPP

#include <string>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/split_member.hpp>

BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_load), serialize, 2);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_save), serialize, 2);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_get_task_name), get_task_name, 0);

namespace boost { namespace asynchronous
{
typedef
boost::type_erasure::any<
    boost::mpl::vector<
        boost::asynchronous::any_callable_concept,
        boost::asynchronous::has_save<void(boost::archive::text_oarchive&,const unsigned int)/*,const boost::type_erasure::_self*/>,
        boost::asynchronous::has_load<void(boost::archive::text_iarchive&,const unsigned int)>,
        boost::asynchronous::has_get_task_name<std::string()>
    >,
    boost::type_erasure::_self
> any_serializable_helper;

struct any_serializable: public boost::asynchronous::any_serializable_helper
{
    any_serializable(){}
    template <class T>
    any_serializable(T t):any_serializable_helper(t){}
    typedef boost::archive::text_oarchive oarchive;
    typedef boost::archive::text_iarchive iarchive;
};
}}
#endif // BOOST_ASYNCHRONOUS_ANY_SERIALIZABLE_HPP
