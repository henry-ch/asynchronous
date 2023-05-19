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

// define one of them before using
#ifdef BOOST_ASYNCHRONOUS_USE_PORTABLE_BINARY_ARCHIVE
#include <portable_binary_oarchive.hpp>
#include <portable_binary_iarchive.hpp>
#elif defined BOOST_ASYNCHRONOUS_USE_BINARY_ARCHIVE
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#elif BOOST_ASYNCHRONOUS_USE_DEFAULT_ARCHIVE
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#endif

#include <boost/asynchronous/callable_any.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/split_member.hpp>

BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_load), serialize, 2);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_save), serialize, 2);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_get_task_name), get_task_name, 0);

// concept for a job type being callable (the minimum requirement, default)
// and also serializable (in sense of boost::archive
// supports text and portable binary archives
// BOOST_ASYNCHRONOUS_USE_PORTABLE_BINARY_ARCHIVE allows choosing the binary type

namespace boost { namespace asynchronous
{
typedef boost::mpl::vector<
    boost::asynchronous::any_callable_concept,
#ifdef BOOST_ASYNCHRONOUS_USE_PORTABLE_BINARY_ARCHIVE
    boost::asynchronous::has_save<void(portable_binary_oarchive&,const unsigned int)>,
    boost::asynchronous::has_load<void(portable_binary_iarchive&,const unsigned int)>,
#elif defined BOOST_ASYNCHRONOUS_USE_BINARY_ARCHIVE
    boost::asynchronous::has_save<void(boost::archive::binary_oarchive&,const unsigned int)>,
    boost::asynchronous::has_load<void(boost::archive::binary_iarchive&,const unsigned int)>,
#elif BOOST_ASYNCHRONOUS_USE_DEFAULT_ARCHIVE
    boost::asynchronous::has_save<void(boost::archive::text_oarchive&,const unsigned int)>,
    boost::asynchronous::has_load<void(boost::archive::text_iarchive&,const unsigned int)>,
#endif
    boost::asynchronous::has_get_task_name<std::string()>
> any_serializable_concept;

typedef
boost::type_erasure::any<
    boost::asynchronous::any_serializable_concept,
    boost::type_erasure::_self
> any_serializable_helper;

struct any_serializable: public boost::asynchronous::any_serializable_helper
{
    any_serializable(){}
    template <class T>
    any_serializable(T t):any_serializable_helper(t){}
#ifdef BOOST_ASYNCHRONOUS_USE_PORTABLE_BINARY_ARCHIVE
    typedef portable_binary_oarchive oarchive;
    typedef portable_binary_iarchive iarchive;
#elif defined BOOST_ASYNCHRONOUS_USE_BINARY_ARCHIVE
    typedef boost::archive::binary_oarchive oarchive;
    typedef boost::archive::binary_iarchive iarchive;
#elif BOOST_ASYNCHRONOUS_USE_DEFAULT_ARCHIVE
    typedef boost::archive::text_oarchive oarchive;
    typedef boost::archive::text_iarchive iarchive;
#endif
};
}}
#endif // BOOST_ASYNCHRONOUS_ANY_SERIALIZABLE_HPP
