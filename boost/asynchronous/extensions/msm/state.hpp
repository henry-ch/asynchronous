// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2025
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_EXTENSIONS_MSM_STATE_HPP
#define BOOST_ASYNCHRONOUS_EXTENSIONS_MSM_STATE_HPP


#include <boost/msm/back/common_types.hpp>
#include <boost/msm/front/states.hpp>

#include <boost/asynchronous/trackable_servant.hpp>

namespace boost { namespace asynchronous { namespace msm
{
    template <class StateType, class JOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB, class WJOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB>
    struct state : public StateType
                 , public boost::asynchronous::trackable_servant<JOB, WJOB>
    {

    };

    template <class StateType, class JOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB, class WJOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB>
    struct state_machine : public StateType
        //, public boost::asynchronous::trackable_servant<JOB, WJOB>
    {
        template<class Event>
        ::boost::msm::back::HandledEnum  process_event_internal(Event const& evt)
        {
            return static_cast<StateType*>(this)->process_event_internal(evt);
        }
    };

}}}
#endif // BOOST_ASYNCHRONOUS_EXTENSIONS_MSM_STATE_HPP
