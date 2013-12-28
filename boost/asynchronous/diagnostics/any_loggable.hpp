// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_ANY_LOGGABLE_HPP
#define BOOST_ASYNCHRONOUS_ANY_LOGGABLE_HPP

#include <string>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/detail/concept_members.hpp>
#include <boost/asynchronous/diagnostics/diagnostic_item.hpp>

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/member.hpp>
#include <boost/chrono/chrono.hpp>
//TODO find better
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_set_posted_time), set_posted_time, 0);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_set_started_time), set_started_time, 0);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_set_finished_time), set_finished_time, 0);

namespace boost { namespace asynchronous
{
template <class Clock>
struct any_loggable_concept :
 ::boost::mpl::vector<
    boost::asynchronous::any_callable_concept,
    boost::asynchronous::has_set_name<void(std::string const&)>,
    boost::asynchronous::has_get_name<std::string(void), const boost::type_erasure::_self>,
    boost::asynchronous::has_set_posted_time<void()>,
    boost::asynchronous::has_set_started_time<void()>,
    boost::asynchronous::has_set_finished_time<void()>,
    boost::asynchronous::has_get_diagnostic_item<boost::asynchronous::diagnostic_item<Clock>(), const boost::type_erasure::_self>
> {};

template <class Clock = boost::chrono::high_resolution_clock>
struct any_loggable: boost::type_erasure::any<any_loggable_concept<Clock> >
{
    typedef Clock clock_type;
    template <class U>
    any_loggable(U const& u): boost::type_erasure::any< boost::asynchronous::any_loggable_concept<Clock> > (u){}
    any_loggable(): boost::type_erasure::any< boost::asynchronous::any_loggable_concept<Clock> > (){}
    // dummies
    typedef boost::archive::text_oarchive oarchive;
    typedef boost::archive::text_iarchive iarchive;
};

}} // boost::async

#endif // BOOST_ASYNCHRONOUS_ANY_LOGGABLE_HPP
