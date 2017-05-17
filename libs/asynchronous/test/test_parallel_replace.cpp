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
#include <future>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_replace.hpp>

#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

namespace
{
// main thread id
boost::thread::id main_thread_id;
bool servant_dtor=false;

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

    std::future<void> parallel_replace_if_iterators()
    {
        m_data = std::vector<int>(10000,1);
        std::vector<int> more(5000,2);
        m_data.insert(m_data.end(),more.begin(),more.end());
        auto copy_vec = m_data;

        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant parallel_replace not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_replace_if(
                                  this->m_data.begin(),this->m_data.end(),
                                  [](int const& i)
                                  {
                                    return i == 2;
                                  },
                                  3, //new value if i==2
                                  1500);
                    },// work
           [aPromise,ids,copy_vec,this](boost::asynchronous::expected<void> res)mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");

                        std::replace(copy_vec.begin(),copy_vec.end(),2,3);

                        BOOST_CHECK_MESSAGE((copy_vec == this->m_data),"parallel_replace_if is wrong");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }

    std::future<void> parallel_replace_iterators()
    {
        m_data = std::vector<int>(10000,1);
        std::vector<int> more(5000,2);
        m_data.insert(m_data.end(),more.begin(),more.end());
        auto copy_vec = m_data;

        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant parallel_replace not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_replace(
                                  this->m_data.begin(),this->m_data.end(),
                                  2, //old value
                                  3, //new value if i==2
                                  1500);
                    },// work
           [aPromise,ids,copy_vec,this](boost::asynchronous::expected<void> res)mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");

                        std::replace(copy_vec.begin(),copy_vec.end(),2,3);

                        BOOST_CHECK_MESSAGE((copy_vec == this->m_data),"parallel_replace_if is wrong");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }

    std::future<void> parallel_replace_if_moved_range()
    {
        m_data = std::vector<int>(10000,1);
        std::vector<int> more(5000,2);
        m_data.insert(m_data.end(),more.begin(),more.end());
        auto copy_vec = m_data;

        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant parallel_replace not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_replace_if(
                                  std::move(this->m_data),
                                  [](int const& i)
                                  {
                                    return i == 2;
                                  },
                                  3, //new value if i==2
                                  1500);
                    },// work
           [aPromise,ids,copy_vec,this](boost::asynchronous::expected<std::vector<int>> res)mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");

                        std::replace(copy_vec.begin(),copy_vec.end(),2,3);

                        BOOST_CHECK_MESSAGE((copy_vec == res.get()),"parallel_replace_if moved range is wrong");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }

    std::future<void> parallel_replace_moved_range()
    {
        m_data = std::vector<int>(10000,1);
        std::vector<int> more(5000,2);
        m_data.insert(m_data.end(),more.begin(),more.end());
        auto copy_vec = m_data;

        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant parallel_replace not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_replace(
                                  std::move(this->m_data),
                                  2, //old value
                                  3, //new value if i==2
                                  1500);
                    },// work
           [aPromise,ids,copy_vec,this](boost::asynchronous::expected<std::vector<int>> res)mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");

                        std::replace(copy_vec.begin(),copy_vec.end(),2,3);

                        BOOST_CHECK_MESSAGE((copy_vec == res.get()),"parallel_replace_if moved range is wrong");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }

    std::future<void> parallel_replace_if_continuation()
    {
        m_data = std::vector<int>(10000,1);
        std::vector<int> more(5000,2);
        m_data.insert(m_data.end(),more.begin(),more.end());
        auto copy_vec = m_data;
        std::transform(copy_vec.begin(),copy_vec.end(),copy_vec.begin(),[](int i){return i+2;});

        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant parallel_replace not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_replace_if(
                                  boost::asynchronous::parallel_for(
                                         std::move(this->m_data),
                                         [](int const& i)
                                         {
                                            const_cast<int&>(i) += 2;
                                         },1500),
                                  [](int const& i)
                                  {
                                    return i == 4;
                                  },
                                  5, //new value if i==2
                                  1500);
                    },// work
           [aPromise,ids,copy_vec,this](boost::asynchronous::expected<std::vector<int>> res)mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");

                        std::replace(copy_vec.begin(),copy_vec.end(),4,5);

                        BOOST_CHECK_MESSAGE((copy_vec == res.get()),"parallel_replace_if moved range is wrong");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }

    std::future<void> parallel_replace_continuation()
    {
        m_data = std::vector<int>(10000,1);
        std::vector<int> more(5000,2);
        m_data.insert(m_data.end(),more.begin(),more.end());
        auto copy_vec = m_data;
        std::transform(copy_vec.begin(),copy_vec.end(),copy_vec.begin(),[](int i){return i+2;});

        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant parallel_replace not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_replace(
                                  boost::asynchronous::parallel_for(
                                         std::move(this->m_data),
                                         [](int const& i)
                                         {
                                            const_cast<int&>(i) += 2;
                                         },1500),
                                  4, //old value
                                  5, //new value if i==2
                                  1500);
                    },// work
           [aPromise,ids,copy_vec,this](boost::asynchronous::expected<std::vector<int>> res)mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");

                        std::replace(copy_vec.begin(),copy_vec.end(),4,5);

                        BOOST_CHECK_MESSAGE((copy_vec == res.get()),"parallel_replace_if moved range is wrong");
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
    BOOST_ASYNC_FUTURE_MEMBER(parallel_replace_if_iterators)
    BOOST_ASYNC_FUTURE_MEMBER(parallel_replace_iterators)
    BOOST_ASYNC_FUTURE_MEMBER(parallel_replace_if_moved_range)
    BOOST_ASYNC_FUTURE_MEMBER(parallel_replace_moved_range)
    BOOST_ASYNC_FUTURE_MEMBER(parallel_replace_if_continuation)
    BOOST_ASYNC_FUTURE_MEMBER(parallel_replace_continuation)
};

}

BOOST_AUTO_TEST_CASE( test_parallel_replace_if )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.parallel_replace_if_iterators();
        try
        {
            auto resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_parallel_replace )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.parallel_replace_iterators();
        try
        {
            auto resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_parallel_replace_if_moved_range )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.parallel_replace_if_moved_range();
        try
        {
            auto resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_parallel_replace_moved_range )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.parallel_replace_moved_range();
        try
        {
            auto resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_parallel_replace_if_continuation )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.parallel_replace_if_continuation();
        try
        {
            auto resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_parallel_replace_continuation )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();
        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.parallel_replace_continuation();
        try
        {
            auto resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}
