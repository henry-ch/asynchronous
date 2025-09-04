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


#include <boost/msm/front/states.hpp>

#include <boost/asynchronous/trackable_servant.hpp>

namespace boost { namespace asynchronous { namespace msm
{
    template <class JOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB, class WJOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB>
    struct state : public boost::msm::front::state<>
                 , public boost::asynchronous::trackable_servant<JOB, WJOB>
    {

    };
}}}
#endif // BOOST_ASYNCHRONOUS_EXTENSIONS_MSM_STATE_HPP
