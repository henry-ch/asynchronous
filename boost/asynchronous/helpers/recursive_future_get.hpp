// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2024
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_RECURSIVE_FUTURE_GET_HPP
#define BOOST_ASYNCHRONOUS_RECURSIVE_FUTURE_GET_HPP

#include <future>
#include <type_traits>

#include <boost/asynchronous/helpers/type_traits.hpp>

namespace boost { namespace asynchronous
{
    template <class F>
    auto recursive_future_get(F&& fu)
    {
        if constexpr (boost::asynchronous::is_specialization_of_v<F, std::future>)
        {
            if constexpr (std::is_same_v<void, std::remove_cv_t<decltype(fu.get())>>)
            {
                fu.get();
            }
            else
            {
                return recursive_future_get(std::move(fu.get()));
            }
        }
        else
        {
            return fu;
        }
    }

}}
#endif // BOOST_ASYNCHRONOUS_RECURSIVE_FUTURE_GET_HPP
