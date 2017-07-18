
// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org
#include <memory>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/post.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/thread/future.hpp>

namespace
{
thread_local std::thread::id tid;
}
BOOST_AUTO_TEST_CASE( test_execute_in_all_threads )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>(4);

    std::vector<std::future<void>> fus = scheduler.execute_in_all_threads([]()
        {
            //std::cout << "id: " << std::this_thread::get_id() << std::endl;
            tid = std::this_thread::get_id();
        });
    boost::wait_for_all(fus.begin(), fus.end());
    BOOST_CHECK_MESSAGE(fus.size()==4,"should have as many values as threads.");
    // check if we got 4 different ids
    /*std::future<std::thread::id> id_fu1 = boost::asynchronous::post_future(scheduler,[](){return std::this_thread::get_id();},"",1);
    std::future<std::thread::id> id_fu2 = boost::asynchronous::post_future(scheduler,[](){return std::this_thread::get_id();},"",2);
    std::future<std::thread::id> id_fu3 = boost::asynchronous::post_future(scheduler,[](){return std::this_thread::get_id();},"",3);
    std::future<std::thread::id> id_fu4 = boost::asynchronous::post_future(scheduler,[](){return std::this_thread::get_id();},"",4);
    std::cout << std::endl;
    std::cout << "id: " << id_fu1.get() << std::endl;
    std::cout << "id: " << id_fu2.get() << std::endl;
    std::cout << "id: " << id_fu3.get() << std::endl;
    std::cout << "id: " << id_fu4.get() << std::endl;*/

}

