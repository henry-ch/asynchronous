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
#include <future>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_sort.hpp>
#include <boost/asynchronous/algorithm/parallel_for.hpp>
#include <boost/asynchronous/algorithm/parallel_sort_inplace.hpp>

#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

namespace
{
// main thread id
boost::thread::id main_thread_id;
bool servant_dtor=false;

struct my_exception : public boost::asynchronous::asynchronous_exception
{
    virtual const char* what() const throw()
    {
        return "my_exception";
    }
};
std::atomic<int> counter_;
struct BadType
{
    // this one is called during sort and must throw
    BadType(int data=0):data_(data)
    {
    }
    int data_=0;
};

inline bool operator< (const BadType& /*lhs*/, const BadType& /*rhs*/){ ASYNCHRONOUS_THROW( my_exception());}

std::atomic<int> ctor_count(0);
std::atomic<int> dtor_count(0);
struct some_type
{
    some_type(int d=0)
        :data(d)
    {
        ++ctor_count;
    }
    some_type(some_type const& rhs)
        :data(rhs.data)
    {
        ++ctor_count;
    }
    some_type(some_type&& rhs)
        :data(rhs.data)
    {
        ++ctor_count;
    }
    some_type& operator=(some_type const&)=default;
    some_type& operator=(some_type&&)=default;

    ~some_type()
    {
        ++dtor_count;
    }

    int data;
};
bool operator== (some_type const& lhs, some_type const& rhs)
{
    return rhs.data == lhs.data;
}
bool operator< (some_type const& lhs, some_type const& rhs)
{
    return lhs.data < rhs.data;
}

void generate(std::vector<int>& data)
{
    data = std::vector<int>(10000,1);
    // avoid mingw bug by not using random_device
    //std::random_device rd;
    //std::mt19937 mt(rd());
    std::mt19937 mt(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<> dis(0, 1000);
    std::generate(data.begin(), data.end(), std::bind(dis, std::ref(mt)));
}
void generate(std::vector<BadType>& data)
{
    data = std::vector<BadType>(10000,1);
    std::mt19937 mt(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<> dis(0, 1000);
    std::generate(data.begin(), data.end(), std::bind(dis, std::ref(mt)));
}
void generate(std::vector<some_type>& data)
{
    data = std::vector<some_type>(10000,1);
    std::mt19937 mt(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<> dis(0, 1000);
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
        generate(m_data);
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
        servant_dtor = true;
    }
    std::future<void> start_parallel_sort()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        auto data_copy = m_data;
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_sort(this->m_data.begin(),this->m_data.end(),std::less<int>(),1500);
                    },// work
           [aPromise,ids,data_copy,this](boost::asynchronous::expected<void> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        std::sort(data_copy.begin(),data_copy.end(),std::less<int>());
                        BOOST_CHECK_MESSAGE(data_copy == this->m_data,"parallel_sort gave a wrong value.");
                        // reset
                        generate(this->m_data);
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    std::future<void> start_parallel_stable_sort()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        auto data_copy = m_data;
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_stable_sort(this->m_data.begin(),this->m_data.end(),std::less<int>(),1500);
                    },// work
           [aPromise,ids,data_copy,this](boost::asynchronous::expected<void> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        std::stable_sort(data_copy.begin(),data_copy.end(),std::less<int>());
                        BOOST_CHECK_MESSAGE(data_copy == this->m_data,"parallel_stable_sort gave a wrong value.");
                        // reset
                        generate(this->m_data);
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    std::future<void> start_parallel_sort_moved_range()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        auto data_copy = m_data;
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");
                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_sort(std::move(this->m_data),std::less<int>(),1500);
                    },// work
           [aPromise,ids,data_copy,this](boost::asynchronous::expected<std::vector<int>> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        std::sort(data_copy.begin(),data_copy.end(),std::less<int>());
                        auto modified_vec = res.get();
                        BOOST_CHECK_MESSAGE(data_copy == modified_vec,"parallel_sort gave a wrong value.");
                        // reset
                        generate(this->m_data);
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    std::future<void> start_parallel_sort_continuation()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        auto data_copy = m_data;
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_sort(
                                boost::asynchronous::parallel_for(std::move(this->m_data),
                                                                  [](int const& i)
                                                                  {
                                                                    const_cast<int&>(i) += 2;
                                                                  },1500),
                                std::less<int>(),1500);
                    },// work
           [aPromise,ids,data_copy,this](boost::asynchronous::expected<std::vector<int>> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        std::for_each(data_copy.begin(),data_copy.end(),[](int const& i){const_cast<int&>(i) += 2;});
                        std::sort(data_copy.begin(),data_copy.end(),std::less<int>());
                        auto modified_vec = res.get();
                        BOOST_CHECK_MESSAGE(data_copy == modified_vec,"parallel_sort gave a wrong value.");
                        // reset
                        generate(this->m_data);
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }

    // inplace versions
    std::future<void> start_parallel_sort_inplace()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        auto data_copy = m_data;
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_sort_inplace(this->m_data.begin(),this->m_data.end(),std::less<int>(),1500);
                    },// work
           [aPromise,ids,data_copy,this](boost::asynchronous::expected<void> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        std::sort(data_copy.begin(),data_copy.end(),std::less<int>());
                        BOOST_CHECK_MESSAGE(data_copy == this->m_data,"parallel_sort gave a wrong value.");
                        // reset
                        generate(this->m_data);
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    std::future<void> start_parallel_stable_sort_inplace()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        auto data_copy = m_data;
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_stable_sort_inplace(this->m_data.begin(),this->m_data.end(),std::less<int>(),1500);
                    },// work
           [aPromise,ids,data_copy,this](boost::asynchronous::expected<void> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        std::stable_sort(data_copy.begin(),data_copy.end(),std::less<int>());
                        BOOST_CHECK_MESSAGE(data_copy == this->m_data,"parallel_stable_sort gave a wrong value.");
                        // reset
                        generate(this->m_data);
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    std::future<void> start_parallel_sort_range_inplace()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        auto data_copy = m_data;
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_sort_inplace(this->m_data,std::less<int>(),1500);
                    },// work
           [aPromise,ids,data_copy,this](boost::asynchronous::expected<void> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        std::sort(data_copy.begin(),data_copy.end(),std::less<int>());
                        BOOST_CHECK_MESSAGE(data_copy == this->m_data,"parallel_sort gave a wrong value.");
                        // reset
                        generate(this->m_data);
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    std::future<void> start_parallel_stable_sort_range_inplace()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        auto data_copy = m_data;
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_stable_sort_inplace(this->m_data,std::less<int>(),1500);
                    },// work
           [aPromise,ids,data_copy,this](boost::asynchronous::expected<void> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        std::stable_sort(data_copy.begin(),data_copy.end(),std::less<int>());
                        BOOST_CHECK_MESSAGE(data_copy == this->m_data,"parallel_stable_sort gave a wrong value.");
                        // reset
                        generate(this->m_data);
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    std::future<void> start_parallel_sort_moved_range_inplace()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        auto data_copy = m_data;
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_sort_move_inplace(std::move(this->m_data),std::less<int>(),1500);
                    },// work
           [aPromise,ids,data_copy,this](boost::asynchronous::expected<std::vector<int>> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        std::sort(data_copy.begin(),data_copy.end(),std::less<int>());
                        auto modified_vec = res.get();
                        BOOST_CHECK_MESSAGE(data_copy == modified_vec,"parallel_sort gave a wrong value.");
                        // reset
                        generate(this->m_data);
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    std::future<void> start_parallel_sort_continuation_inplace()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        auto data_copy = m_data;
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_sort_inplace(
                                boost::asynchronous::parallel_for(std::move(this->m_data),
                                                                  [](int const& i)
                                                                  {
                                                                    const_cast<int&>(i) += 2;
                                                                  },1500),
                                std::less<int>(),1500);
                    },// work
           [aPromise,ids,data_copy,this](boost::asynchronous::expected<std::vector<int>> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        std::for_each(data_copy.begin(),data_copy.end(),[](int const& i){const_cast<int&>(i) += 2;});
                        std::sort(data_copy.begin(),data_copy.end(),std::less<int>());
                        auto modified_vec = res.get();
                        BOOST_CHECK_MESSAGE(data_copy == modified_vec,"parallel_sort gave a wrong value.");
                        // reset
                        generate(this->m_data);
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    std::future<void> start_parallel_reverse_sorted()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        std::sort(m_data.begin(),m_data.end(),std::greater<int>());
        auto data_copy = m_data;
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_sort(this->m_data.begin(),this->m_data.end(),std::less<int>(),1500);
                    },// work
           [aPromise,ids,data_copy,this](boost::asynchronous::expected<void> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        std::sort(data_copy.begin(),data_copy.end(),std::less<int>());
                        BOOST_CHECK_MESSAGE(data_copy == this->m_data,"parallel_sort gave a wrong value.");
                        // reset
                        generate(this->m_data);
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    std::future<void> start_parallel_already_sorted()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        std::sort(m_data.begin(),m_data.end(),std::less<int>());
        auto data_copy = m_data;
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_sort(this->m_data.begin(),this->m_data.end(),std::less<int>(),1500);
                    },// work
           [aPromise,ids,data_copy,this](boost::asynchronous::expected<void> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        std::sort(data_copy.begin(),data_copy.end(),std::less<int>());
                        BOOST_CHECK_MESSAGE(data_copy == this->m_data,"parallel_sort gave a wrong value.");
                        // reset
                        generate(this->m_data);
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    std::future<void> start_parallel_sort_exception()
    {
        counter_=0;
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        generate(this->m_data2);
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_sort(this->m_data2.begin(),this->m_data2.end(),std::less<BadType>(),1500);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<void> res) mutable{
                        BOOST_CHECK_MESSAGE(res.has_exception(),"servant work should throw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        try{res.get();aPromise->set_value();}
                        catch(my_exception&){/*std::cout << "ok" << std::endl;*/aPromise->set_exception(std::current_exception());}
                        catch(...){/*std::cout << "nok" << std::endl;*/aPromise->set_exception(std::current_exception());}
           }// callback functor.
        );
        return fu;
    }
    std::future<void> parallel_sort_check_dtors()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        generate(m_data3);
        auto data_copy = m_data3;
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_sort(this->m_data3.begin(),this->m_data3.end(),std::less<some_type>(),1500);
                    },// work
           [aPromise,ids,data_copy,this](boost::asynchronous::expected<void> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        std::sort(data_copy.begin(),data_copy.end(),std::less<some_type>());
                        BOOST_CHECK_MESSAGE(data_copy == this->m_data3,"parallel_sort gave a wrong value.");
                        // reset
                        data_copy.clear();
                        this->m_data3.clear();
                        BOOST_CHECK_MESSAGE(ctor_count.load()==dtor_count.load(),"wrong number of ctors/dtors called.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    std::future<void> parallel_sort_check_dtors_no_parallel()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        generate(m_data3);
        auto data_copy = m_data3;
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_sort(this->m_data3.begin(),this->m_data3.end(),std::less<some_type>(),11000);
                    },// work
           [aPromise,ids,data_copy,this](boost::asynchronous::expected<void> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        std::sort(data_copy.begin(),data_copy.end(),std::less<some_type>());
                        BOOST_CHECK_MESSAGE(data_copy == this->m_data3,"parallel_sort gave a wrong value.");
                        // reset
                        data_copy.clear();
                        this->m_data3.clear();
                        BOOST_CHECK_MESSAGE(ctor_count.load()==dtor_count.load(),"wrong number of ctors/dtors called.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
private:
    std::vector<int> m_data;
    std::vector<BadType> m_data2;
    std::vector<some_type> m_data3;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
#ifndef _MSC_VER
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_sort)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_stable_sort)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_sort_moved_range)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_sort_continuation)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_sort_inplace)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_stable_sort_inplace)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_sort_range_inplace)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_stable_sort_range_inplace)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_sort_moved_range_inplace)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_sort_continuation_inplace)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_reverse_sorted)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_already_sorted)
    BOOST_ASYNC_FUTURE_MEMBER(start_parallel_sort_exception)
    BOOST_ASYNC_FUTURE_MEMBER(parallel_sort_check_dtors)
    BOOST_ASYNC_FUTURE_MEMBER(parallel_sort_check_dtors_no_parallel)
#else
    BOOST_ASYNC_FUTURE_MEMBER_1(start_parallel_sort)
    BOOST_ASYNC_FUTURE_MEMBER_1(start_parallel_stable_sort)
    BOOST_ASYNC_FUTURE_MEMBER_1(start_parallel_sort_moved_range)
    BOOST_ASYNC_FUTURE_MEMBER_1(start_parallel_sort_continuation)
    BOOST_ASYNC_FUTURE_MEMBER_1(start_parallel_sort_inplace)
    BOOST_ASYNC_FUTURE_MEMBER_1(start_parallel_stable_sort_inplace)
    BOOST_ASYNC_FUTURE_MEMBER_1(start_parallel_sort_range_inplace)
    BOOST_ASYNC_FUTURE_MEMBER_1(start_parallel_stable_sort_range_inplace)
    BOOST_ASYNC_FUTURE_MEMBER_1(start_parallel_sort_moved_range_inplace)
    BOOST_ASYNC_FUTURE_MEMBER_1(start_parallel_sort_continuation_inplace)
    BOOST_ASYNC_FUTURE_MEMBER_1(start_parallel_reverse_sorted)
    BOOST_ASYNC_FUTURE_MEMBER_1(start_parallel_already_sorted)
    BOOST_ASYNC_FUTURE_MEMBER_1(start_parallel_sort_exception)
    BOOST_ASYNC_FUTURE_MEMBER_1(parallel_sort_check_dtors)
    BOOST_ASYNC_FUTURE_MEMBER_1(parallel_sort_check_dtors_no_parallel)
#endif
};

}
BOOST_AUTO_TEST_CASE( test_parallel_sort_check_dtors )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.parallel_sort_check_dtors();
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

BOOST_AUTO_TEST_CASE( test_parallel_sort_check_dtors_no_parallel )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.parallel_sort_check_dtors_no_parallel();
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

BOOST_AUTO_TEST_CASE( test_parallel_reverse_sorted )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_parallel_reverse_sorted();
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
BOOST_AUTO_TEST_CASE( test_parallel_already_sorted )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_parallel_already_sorted();
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
BOOST_AUTO_TEST_CASE( test_parallel_sort )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_parallel_sort();
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
BOOST_AUTO_TEST_CASE( test_parallel_stable_sort )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_parallel_stable_sort();
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
BOOST_AUTO_TEST_CASE( test_parallel_stable_sort_moved_range )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_parallel_sort_moved_range();
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
BOOST_AUTO_TEST_CASE( test_parallel_stable_sort_continuation )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_parallel_sort_continuation();
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
BOOST_AUTO_TEST_CASE( test_parallel_sort_post_future )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
                                                                                boost::asynchronous::lockfree_queue<>>>(6);
    std::vector<int> data;
    generate(data);
    // make a copy and execute in pool
    std::future<std::vector<int>> fu = boost::asynchronous::post_future(
                scheduler,
                [data]() mutable {return boost::asynchronous::parallel_sort(std::move(data),std::less<int>(),1500);});
    try
    {
        std::vector<int> res = fu.get();
        std::sort(data.begin(),data.end(),std::less<int>());
        BOOST_CHECK_MESSAGE(std::is_sorted(res.begin(),res.end(),std::less<int>()),"parallel_sort did not sort.");
        BOOST_CHECK_MESSAGE(res == data,"parallel_sort gave a wrong value.");
    }
    catch(...)
    {
        BOOST_FAIL( "unexpected exception" );
    }
}
BOOST_AUTO_TEST_CASE( test_parallel_sort_inplace )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_parallel_sort_inplace();
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
BOOST_AUTO_TEST_CASE( test_parallel_stable_sort_inplace )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_parallel_stable_sort_inplace();
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
BOOST_AUTO_TEST_CASE( test_parallel_sort_range_inplace )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_parallel_sort_range_inplace();
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
BOOST_AUTO_TEST_CASE( test_parallel_stable_sort_range_inplace )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_parallel_stable_sort_range_inplace();
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
BOOST_AUTO_TEST_CASE( test_parallel_stable_sort_moved_range_inplace )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_parallel_sort_moved_range_inplace();
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
BOOST_AUTO_TEST_CASE( test_parallel_stable_sort_continuation_inplace )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_parallel_sort_continuation_inplace();
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
BOOST_AUTO_TEST_CASE( test_parallel_sort_post_future_inplace )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
                                                                                boost::asynchronous::lockfree_queue<>>>(6);
    std::vector<int> data;
    generate(data);
    // make a copy and execute in pool
    std::future<std::vector<int>> fu = boost::asynchronous::post_future(
                scheduler,
                [data]() mutable {return boost::asynchronous::parallel_sort_move_inplace(std::move(data),std::less<int>(),1500);});
    try
    {
        std::vector<int> res = fu.get();
        std::sort(data.begin(),data.end(),std::less<int>());
        BOOST_CHECK_MESSAGE(std::is_sorted(res.begin(),res.end(),std::less<int>()),"parallel_sort did not sort.");
        BOOST_CHECK_MESSAGE(res == data,"parallel_sort gave a wrong value.");
    }
    catch(...)
    {
        BOOST_FAIL( "unexpected exception" );
    }
}

BOOST_AUTO_TEST_CASE( test_parallel_sort_exception )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_parallel_sort_exception();
        bool got_exception=false;
        try
        {
            auto resfuv = fuv.get();
            resfuv.get();
        }
        catch ( my_exception& e)
        {
            got_exception=true;
            BOOST_CHECK_MESSAGE(std::string(e.what_) == "my_exception","no what data");
            BOOST_CHECK_MESSAGE(!std::string(e.file_).empty(),"no file data");
            BOOST_CHECK_MESSAGE(e.line_ != -1,"no line data");
        }
        //TODO
        catch ( std::exception& e)
        {
            got_exception=true;
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
        BOOST_CHECK_MESSAGE(got_exception,"servant didn't send an expected exception.");
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

