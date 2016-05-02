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
#include <random>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_remove_copy.hpp>

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
    std::uniform_int_distribution<> dis(0, 10);
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
        , m_dest(10000,1)
    {
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
        servant_dtor = true;
    }

    boost::shared_future<void> parallel_remove_copy_if()
    {
        generate(m_data);

        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant parallel_remove_copy_if not posted.");
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
                    return boost::asynchronous::parallel_remove_copy_if(
                                  this->m_data.begin(),this->m_data.end(),
                                  this->m_dest.begin(),
                                  [](int i){return i >= 2000;},
                                  1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<Iterator> res)mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");

                        std::vector<int> dest(10000,1);
                        std::vector<int>::iterator it = std::remove_copy_if(this->m_data.begin(),this->m_data.end(),dest.begin(),
                                               [](int i){return i >= 2000;});

                        auto itres = res.get();
                        BOOST_CHECK_MESSAGE((std::distance(dest.begin(),it) == std::distance(m_dest.begin(),itres)),
                                            "parallel_remove_copy_if is wrong");

                        m_dest.erase(itres,m_dest.end());
                        dest.erase(it,dest.end());
                        BOOST_CHECK_MESSAGE(dest == m_dest,"parallel_remove_copy_if went wrong");


                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    boost::shared_future<void> parallel_remove_copy()
    {
        generate(m_data);

        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant parallel_remove_copy not posted.");
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
                    return boost::asynchronous::parallel_remove_copy(
                                  this->m_data.begin(),this->m_data.end(),
                                  this->m_dest.begin(),
                                  5,
                                  1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<Iterator> res)mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");

                        std::vector<int> dest(10000,1);
                        std::vector<int>::iterator it = std::remove_copy(this->m_data.begin(),this->m_data.end(),dest.begin(),5);

                        auto itres = res.get();
                        BOOST_CHECK_MESSAGE((std::distance(dest.begin(),it) == std::distance(m_dest.begin(),itres)),
                                            "parallel_remove_copy is wrong");

                        m_dest.erase(itres,m_dest.end());
                        dest.erase(it,dest.end());
                        BOOST_CHECK_MESSAGE(dest == m_dest,"parallel_remove_copy went wrong");


                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
private:
    std::vector<int> m_data;
    std::vector<int> m_dest;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(parallel_remove_copy_if)
    BOOST_ASYNC_FUTURE_MEMBER(parallel_remove_copy)
};

}

BOOST_AUTO_TEST_CASE( test_parallel_remove_copy_if )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);

        boost::shared_future<boost::shared_future<void> > fuv = proxy.parallel_remove_copy_if();
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
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_parallel_remove_copy )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);

        boost::shared_future<boost::shared_future<void> > fuv = proxy.parallel_remove_copy();
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
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}
