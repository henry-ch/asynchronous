// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <iostream>
#include <future>

#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/extensions/asio/asio_scheduler.hpp>
#include "test_common.hpp"
#include <boost/thread/future.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::asynchronous::test;

namespace
{
struct DummyJob
{
    DummyJob(std::shared_ptr<std::promise<boost::thread::id> > p):m_done(p){}
    void operator()()const
    {
        //std::cout << "DummyJob called in thread:" << boost::this_thread::get_id() << std::endl;
        boost::this_thread::sleep(boost::posix_time::milliseconds(50));
        m_done->set_value(boost::this_thread::get_id());
    }
    // to check we ran in a correct thread
    std::shared_ptr<std::promise<boost::thread::id> > m_done;
};

}  
    
BOOST_AUTO_TEST_CASE( create_asio_scheduler )
{
    {
        boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::asio_scheduler<>>(3);
    }
}

BOOST_AUTO_TEST_CASE( default_post_asio_scheduler )
{
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::asio_scheduler<>>(3);
        
        std::vector<boost::thread::id> sids = scheduler.thread_ids();
        
        BOOST_CHECK_MESSAGE(number_of_threads(sids.begin(),sids.end())==3,"scheduler has wrong number of threads");
        std::vector<std::future<boost::thread::id> > fus;
        for (int i = 1 ; i< 4 ; ++i)
        {
            std::shared_ptr<std::promise<boost::thread::id> > p = std::make_shared<std::promise<boost::thread::id> >();
            fus.push_back(p->get_future());
            scheduler.post(boost::asynchronous::any_callable(DummyJob(p)),i);
        }
        boost::wait_for_all(fus.begin(), fus.end());
        std::set<boost::thread::id> ids;
        for (std::vector<std::future<boost::thread::id> >::iterator it = fus.begin(); it != fus.end() ; ++it)
        {
            boost::thread::id tid = (*it).get();
            BOOST_CHECK_MESSAGE(contains_id(sids.begin(),sids.end(),tid),"task executed in the wrong thread");
            ids.insert(tid);
        }
        BOOST_CHECK_MESSAGE(ids.size() ==3,"too many threads used in scheduler");
    }
}
