// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <vector>
#include <set>
#include <functional>
#include <random>
#include <boost/lexical_cast.hpp>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_partition_copy.hpp>
#include <boost/asynchronous/algorithm/parallel_for.hpp>

#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

namespace
{
// main thread id
boost::thread::id main_thread_id;
bool servant_dtor=false;
typedef std::vector<int>::iterator Iterator;
typedef std::vector<int>::const_iterator ConstIterator;

struct my_exception : virtual boost::exception, virtual std::exception
{
    virtual const char* what() const throw()
    {
        return "my_exception";
    }
};
void generate(std::vector<int>& data, unsigned elements, unsigned dist)
{
    data = std::vector<int>(elements,1);
    std::random_device rd;
    std::mt19937 mt(rd());
    //std::mt19937 mt(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<> dis(0, dist);
    std::generate(data.begin(), data.end(), std::bind(dis, std::ref(mt)));
}

struct Servant : boost::asynchronous::trackable_servant<>
{
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               boost::asynchronous::create_shared_scheduler_proxy(
                                                   new boost::asynchronous::threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<> >(boost::thread::hardware_concurrency())))
    {
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
        servant_dtor = true;
    }

    boost::shared_future<void> test_parallel_partition_copy_iterators()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        generate(m_data1,10000,7000);
        m_data_true = std::vector<int>(10000,0);
        m_data_false = std::vector<int>(10000,0);
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
                    return boost::asynchronous::parallel_partition_copy(m_data1.begin(),m_data1.end(),
                                                                        m_data_true.begin(),m_data_false.begin(),
                                                                        [](int i){return i < 300;},
                                                                        100);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<std::pair<Iterator,Iterator>> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");

                        std::vector<int> data_copy_true(10000,0);
                        std::vector<int> data_copy_false(10000,0);
                        auto it = std::partition_copy(m_data1.begin(),m_data1.end(),
                                                      data_copy_true.begin(),data_copy_false.begin(),
                                                      [](int i){return i < 300;});
                        std::size_t dist2_true = it.first - data_copy_true.begin();
                        std::size_t dist2_false = it.second - data_copy_false.begin();

                        auto it2 = res.get();
                        // check if the iterator is at the same position
                        std::size_t dist1_true = it2.first - m_data_true.begin();
                        std::size_t dist1_false = it2.second - m_data_false.begin();
                        BOOST_CHECK_MESSAGE(dist1_true == dist2_true,"parallel_partition_copy gave the wrong true iterator.");
                        BOOST_CHECK_MESSAGE(dist1_false == dist2_false,"parallel_partition_copy gave the wrong false iterator.");

                        bool ok = true;
                        for (auto it3 = m_data_true.begin() ; it3 != it2.first ; ++it3)
                        {
                            if (*it3 >= 300 || *it3 == -1)
                                ok = false;
                        }
                        BOOST_CHECK_MESSAGE(ok,"parallel_partition_copy has true elements at wrong place.");
                        ok = true;
                        for (auto it3 = m_data_false.begin() ; it3 != it2.second ; ++it3)
                        {
                           if (*it3 < 300)
                               ok = false;
                        }
                        BOOST_CHECK_MESSAGE(ok,"parallel_partition_copy has false elements at wrong place.");

                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }

private:
    std::vector<int> m_data1;
    std::vector<int> m_data_true;
    std::vector<int> m_data_false;
};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(test_parallel_partition_copy_iterators)
};

}

BOOST_AUTO_TEST_CASE( test_parallel_partition_copy_iterators )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<> >);

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::shared_future<boost::shared_future<void> > fuv = proxy.test_parallel_partition_copy_iterators();
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

