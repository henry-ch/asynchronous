// Boost.Asynchronous library
//  Copyright (C) Christophe Henry, Tobias Holl 2014
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE
#endif
#include <future>
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/asynchronous/detail/move_bind.hpp>
#include "test_common.hpp"

#include <boost/test/unit_test.hpp>


using namespace boost::asynchronous::test;

BOOST_AUTO_TEST_CASE( test_move_bind )
{
    int index = 5;
    std::string tralala("tralala");
    auto f = boost::asynchronous::move_bind([](int i1, std::string s,int i2) {BOOST_CHECK_MESSAGE(s=="tralala","string is not as expected");return i1+i2;},
                                   index,tralala);
    int res = f(3);
    BOOST_CHECK_MESSAGE(res==8,"res should be 8.");
}

BOOST_AUTO_TEST_CASE( test_move_bind_moveable_data )
{
    std::promise<int> p1;
    std::future<int> fu1 = p1.get_future();
    p1.set_value(17);
    std::promise<int> p2;
    std::future<int> fu2 = p2.get_future();
    p2.set_value(18);
    auto f2 = boost::asynchronous::move_bind([](std::future<int> fu1,std::future<int> fu2)
                                             {return fu1.get()+fu2.get();},std::move(fu1));
    int res = f2(std::move(fu2));
    BOOST_CHECK_MESSAGE(res==35,"res should be 35.");
}

BOOST_AUTO_TEST_CASE( test_move_bind_no_bind_argument )
{
    auto f3 = boost::asynchronous::move_bind([](int i1, std::string s,int i2) {BOOST_CHECK_MESSAGE(s=="tralala","string is not as expected");return i1+i2;});
    int res = f3(5,"tralala",3);

    BOOST_CHECK_MESSAGE(res==8,"res should be 8.");
}

