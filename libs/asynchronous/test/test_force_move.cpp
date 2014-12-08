// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2014
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <functional>
#include <boost/asynchronous/helpers.hpp>
#include <boost/thread/future.hpp>

#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_CASE( test_force_move )
{
    bool called=false;
    std::function<void(boost::future<int>)> f =
            [&](boost::future<int> fui)
            {
                called=true;
                BOOST_CHECK_MESSAGE(fui.get()==42,"future should be 42.");
            };
    boost::future<int> fu (boost::make_ready_future(42));
    f(boost::asynchronous::force_move(std::move(fu)));
    BOOST_CHECK_MESSAGE(called,"function not called.");
}




