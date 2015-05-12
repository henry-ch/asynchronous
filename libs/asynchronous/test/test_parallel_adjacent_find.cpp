
// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <vector>
#include <set>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_adjacent_find.hpp>

#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

namespace
{
// main thread id
boost::thread::id main_thread_id;
bool servant_dtor=false;
typedef std::vector<int>::iterator Iterator;

void generate(std::vector<int>& data)
{
    data = std::vector<int>(10000,1);
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<> dis(0, 3000);
    std::generate(data.begin(), data.end(), std::bind(dis, std::ref(mt)));
}

struct Servant : boost::asynchronous::trackable_servant<>
{
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<>>>(6))
    {
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
        servant_dtor = true;
    }

    boost::shared_future<void> parallel_adjacent_find()
    {
        generate(m_data);

        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant parallel_replace not posted.");
        // we need a promise to inform caller when we're done
        boost::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::shared_future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_adjacent_find(
                                  this->m_data.begin(),this->m_data.end(),
                                  1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<Iterator> res)mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");

                        auto it = std::adjacent_find(this->m_data.begin(),this->m_data.end());
                        auto itres = res.get();
                        BOOST_CHECK_MESSAGE((it == itres),"parallel_adjacent_find is wrong");
                        if (it != this->m_data.end())
                            BOOST_CHECK_MESSAGE((*itres == *(itres+1)),"parallel_adjacent_find found 2 non-consecutive elements");

                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    boost::shared_future<void> parallel_adjacent_find_if()
    {
        generate(m_data);

        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant parallel_replace not posted.");
        // we need a promise to inform caller when we're done
        boost::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::shared_future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_adjacent_find(
                                  this->m_data.begin(),this->m_data.end(),
                                  [](int i,int j)
                                  {
                                    return i == j;
                                  },
                                  1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<Iterator> res)mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");

                        auto it = std::adjacent_find(this->m_data.begin(),this->m_data.end(),
                                                     [](int i,int j)
                                                     {
                                                       return i == j;
                                                     });
                        auto itres = res.get();
                        BOOST_CHECK_MESSAGE((it == itres),"parallel_adjacent_find is wrong");
                        if (it != this->m_data.end())
                            BOOST_CHECK_MESSAGE((*itres == *(itres+1)),"parallel_adjacent_find found 2 non-consecutive elements");

                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
private:
    std::vector<int> m_data;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(parallel_adjacent_find)
    BOOST_ASYNC_FUTURE_MEMBER(parallel_adjacent_find_if)
};

}

BOOST_AUTO_TEST_CASE( test_parallel_adjacent_find )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        for (int i = 0; i < 30 ; ++i)
        {
            boost::shared_future<boost::shared_future<void> > fuv = proxy.parallel_adjacent_find();
            try
            {
                boost::shared_future<void> resfuv = fuv.get();
                resfuv.get();
            }
            catch(...)
            {
                BOOST_FAIL( "unexpected exception" );
            }
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_parallel_adjacent_find_if )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        for (int i = 0; i < 30 ; ++i)
        {
            boost::shared_future<boost::shared_future<void> > fuv = proxy.parallel_adjacent_find_if();
            try
            {
                boost::shared_future<void> resfuv = fuv.get();
                resfuv.get();
            }
            catch(...)
            {
                BOOST_FAIL( "unexpected exception" );
            }
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}
