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
#include <boost/asynchronous/algorithm/parallel_scan.hpp>
#include <boost/asynchronous/algorithm/parallel_for.hpp>

#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

namespace
{
// main thread id
boost::thread::id main_thread_id;
bool servant_dtor=false;
using container = std::vector<int>;
using Iterator = std::vector<int>::iterator;
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
template <class Iterator,class OutIterator,class T,class Combine>
OutIterator inclusive_scan(Iterator beg, Iterator end, OutIterator out, T init, Combine c)
{
    for (;beg != end; ++beg)
    {
        init = c(init,*beg);
        *out++ = init;
    }
    return out;
}
template <class Iterator,class OutIterator,class T,class Combine>
OutIterator exclusive_scan(Iterator beg, Iterator end, OutIterator out, T init, Combine c)
{
    for (;beg != end-1; ++beg)
    {
        *out++ = init;
        init = c(init,*beg);
    }
    *out++ = init;
    return out;
//    for (;beg != end; ++beg)
//    {
//        *out++ = init;
//        init = c(init,*beg);
//    }
//    return out;
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

    boost::future<void> test_scan_inclusive_scan()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        generate(m_data1,1000,700);
        m_data2 = std::vector<int>(1000,0);
        auto data_copy = m_data1;
        auto data_copy2 = m_data2;
        // we need a promise to inform caller when we're done
        std::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");
                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_scan(m_data1.begin(),m_data1.end(),m_data2.begin(),0,
                                                              [](Iterator beg, Iterator end)
                                                              {
                                                                int r=0;
                                                                for (;beg != end; ++beg)
                                                                {
                                                                    r = r + *beg;
                                                                }
                                                                return r;
                                                              },
                                                              std::plus<int>(),
                                                              [](Iterator beg, Iterator end, Iterator out, int init) mutable
                                                              {
                                                                for (;beg != end; ++beg)
                                                                {
                                                                    init = *beg + init;
                                                                    *out++ = init;
                                                                };
                                                              },
                                                              100);
                    },// work
           [aPromise,ids,data_copy,data_copy2,this](boost::asynchronous::expected<int> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        inclusive_scan(data_copy.begin(),data_copy.end(),data_copy2.begin(),0,std::plus<int>());
                        BOOST_CHECK_MESSAGE(m_data2 == data_copy2,"parallel_scan gave a wrong value.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    boost::future<void> test_scan_inclusive_scan_init()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        generate(m_data1,1000,700);
        m_data2 = std::vector<int>(1000,0);
        auto data_copy = m_data1;
        auto data_copy2 = m_data2;
        // we need a promise to inform caller when we're done
        std::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");
                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_scan(m_data1.begin(),m_data1.end(),m_data2.begin(),2,
                                                              [](Iterator beg, Iterator end)
                                                              {
                                                                int r=0;
                                                                for (;beg != end; ++beg)
                                                                {
                                                                    r = r + *beg;
                                                                }
                                                                return r;
                                                              },
                                                              std::plus<int>(),
                                                              [](Iterator beg, Iterator end, Iterator out, int init) mutable
                                                              {
                                                                for (;beg != end; ++beg)
                                                                {
                                                                    init = *beg + init;
                                                                    *out++ = init;
                                                                };
                                                              },
                                                              100);
                    },// work
           [aPromise,ids,data_copy,data_copy2,this](boost::asynchronous::expected<int> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        inclusive_scan(data_copy.begin(),data_copy.end(),data_copy2.begin(),2,std::plus<int>());
                        BOOST_CHECK_MESSAGE(m_data2 == data_copy2,"parallel_scan gave a wrong value.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }

    boost::future<void> test_scan_exclusive_scan()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        generate(m_data1,1000,700);
        m_data2 = std::vector<int>(1000,0);
        auto data_copy = m_data1;
        auto data_copy2 = m_data2;
        // we need a promise to inform caller when we're done
        std::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");
                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_scan(m_data1.begin(),m_data1.end(),m_data2.begin(),0,
                                                              [](Iterator beg, Iterator end)
                                                              {
                                                                int r=0;
                                                                for (;beg != end; ++beg)
                                                                {
                                                                    r = r + *beg;
                                                                }
                                                                return r;
                                                              },
                                                              std::plus<int>(),
                                                              [](Iterator beg, Iterator end, Iterator out, int init) mutable
                                                              {
                                                                for (;beg != end; ++beg)
                                                                {
                                                                    *out++ = init;
                                                                    init = *beg + init;
                                                                };
                                                              },
                                                              100);
                    },// work
           [aPromise,ids,data_copy,data_copy2,this](boost::asynchronous::expected<int> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        exclusive_scan(data_copy.begin(),data_copy.end(),data_copy2.begin(),0,std::plus<int>());
                        BOOST_CHECK_MESSAGE(m_data2 == data_copy2,"parallel_scan gave a wrong value.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    boost::future<void> test_scan_moved_ranges()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        generate(m_data1,1000,700);
        m_data2 = std::vector<int>(1000,0);
        auto data_copy = m_data1;
        auto data_copy2 = m_data2;
        // we need a promise to inform caller when we're done
        std::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");
                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_scan(std::move(m_data1),std::move(m_data2),0,
                                                              [](Iterator beg, Iterator end)
                                                              {
                                                                int r=0;
                                                                for (;beg != end; ++beg)
                                                                {
                                                                    r = r + *beg;
                                                                }
                                                                return r;
                                                              },
                                                              std::plus<int>(),
                                                              [](Iterator beg, Iterator end, Iterator out, int init) mutable
                                                              {
                                                                for (;beg != end; ++beg)
                                                                {
                                                                    init = *beg + init;
                                                                    *out++ = init;
                                                                };
                                                              },
                                                              100);
                    },// work
           [aPromise,ids,data_copy,data_copy2,this](boost::asynchronous::expected<std::pair<container,container>> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        auto r = res.get();
                        auto data2 = std::move(r.second);
                        m_data1 = std::move(r.first);
                        inclusive_scan(data_copy.begin(),data_copy.end(),data_copy2.begin(),0,std::plus<int>());
                        BOOST_CHECK_MESSAGE(data2 == data_copy2,"parallel_scan gave a wrong value.");
                        BOOST_CHECK_MESSAGE(m_data1 == data_copy,"parallel_scan should not have modified its input.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    boost::future<void> test_scan_moved_range()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        generate(m_data1,1000,700);
        m_data2 = std::vector<int>(1000,0);
        auto data_copy = m_data1;
        auto data_copy2 = m_data2;
        // we need a promise to inform caller when we're done
        std::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");
                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_scan(std::move(m_data1),0,
                                                              [](Iterator beg, Iterator end)
                                                              {
                                                                int r=0;
                                                                for (;beg != end; ++beg)
                                                                {
                                                                    r = r + *beg;
                                                                }
                                                                return r;
                                                              },
                                                              std::plus<int>(),
                                                              [](Iterator beg, Iterator end, Iterator out, int init) mutable
                                                              {
                                                                for (;beg != end; ++beg)
                                                                {
                                                                    init = *beg + init;
                                                                    *out++ = init;
                                                                };
                                                              },
                                                              100);
                    },// work
           [aPromise,ids,data_copy,data_copy2,this](boost::asynchronous::expected<container> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        m_data1 = std::move(res.get());
                        inclusive_scan(data_copy.begin(),data_copy.end(),data_copy2.begin(),0,std::plus<int>());
                        BOOST_CHECK_MESSAGE(m_data1 == data_copy2,"parallel_scan gave a wrong value.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }

    boost::future<void> test_scan_continuation()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");
        generate(m_data1,1000,700);
        m_data2 = std::vector<int>(1000,0);
        auto data_copy = m_data1;
        std::for_each(data_copy.begin(),data_copy.end(),[](int const& i)
                                                        {
                                                          const_cast<int&>(i) += 2;
                                                        });
        auto data_copy2 = m_data2;
        // we need a promise to inform caller when we're done
        std::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");
                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_scan(boost::asynchronous::parallel_for(std::move(m_data1),
                                                                                                [](int& i)
                                                                                                {
                                                                                                  i += 2;
                                                                                                },100),
                                                              0, // scan's init
                                                              [](Iterator beg, Iterator end)
                                                              {
                                                                int r=0;
                                                                for (;beg != end; ++beg)
                                                                {
                                                                    r = r + *beg;
                                                                }
                                                                return r;
                                                              },
                                                              std::plus<int>(),
                                                              [](Iterator beg, Iterator end, Iterator out, int init) mutable
                                                              {
                                                                for (;beg != end; ++beg)
                                                                {
                                                                    init = *beg + init;
                                                                    *out++ = init;
                                                                };
                                                              },
                                                              100);
                    },// work
           [aPromise,ids,data_copy,data_copy2,this](boost::asynchronous::expected<container> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        m_data1 = std::move(res.get());
                        inclusive_scan(data_copy.begin(),data_copy.end(),data_copy2.begin(),0,std::plus<int>());
                        BOOST_CHECK_MESSAGE(m_data1 == data_copy2,"parallel_scan gave a wrong value.");
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    // we are going to implement a partition into x classes, parallel and inplace
    // in this case, x is 3. Elements not being one of the 3 will be discarded, because it's just what we happen to want
    boost::future<void> test_classify()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant async work not posted.");

        generate_struct(m_data,100,4);

        m_data_res = std::vector<ToClassify>(100,-1);
        // we need a promise to inform caller when we're done
        std::shared_ptr<boost::promise<void> > aPromise(new boost::promise<void>);
        boost::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        auto res_begin = m_data_res.begin();
        // start long tasks
        post_callback(
           [ids,res_begin,this](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");
                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    std::map<int,int> empty;
                    empty[0]=0;
                    empty[1]=0;
                    empty[2]=0;
                    return boost::asynchronous::parallel_scan(m_data.begin(),m_data.end(),m_data_res.begin(),
                                                              empty,
                                                              [](std::vector<ToClassify>::iterator beg, std::vector<ToClassify>::iterator end)
                                                              {
                                                                // reduce part. Build a map of partition index to number of elements in this partition
                                                                std::map<int,int> res;
                                                                for (;beg != end; ++beg)
                                                                {
                                                                    ++res[(*beg).m_id];
                                                                }
                                                                return res;
                                                              },
                                                              [](std::map<int,int> m1, std::map<int,int> m2)
                                                              {
                                                                 // combine 2 maps: add their element counts
                                                                 for (auto it = m2.begin(); it != m2.end();++it)
                                                                 {
                                                                     m1[(*it).first] += (*it).second;
                                                                 }
                                                                 return m1;
                                                              },
                                                              [res_begin]
                                                              (std::vector<ToClassify>::iterator beg, std::vector<ToClassify>::iterator end,
                                                               std::vector<ToClassify>::iterator /* out */,
                                                               std::map<int,int> init,
                                                               std::map<int,int> total) mutable
                                                              {
                                                                // scan: write elements in their correct place in destination
                                                                // Note that we ignore the out iterator because we write in a different place
                                                                // init will contain the result one element before us. For the partition != 0 we need
                                                                // the count of total elements in each partition
                                                                int begin0 = init[0];
                                                                int begin1 = init[1]+total[0];
                                                                int begin2 = init[2]+total[0]+total[1];
                                                                for (auto it = beg; it != end; ++it)
                                                                {
                                                                    if ((*it).m_id == 0)
                                                                    {
                                                                        *(res_begin+begin0)= *it;
                                                                        ++begin0;
                                                                    }
                                                                    else if ((*it).m_id == 1)
                                                                    {
                                                                        *(res_begin+begin1)= *it;
                                                                        ++begin1;
                                                                    }
                                                                    else if ((*it).m_id == 2)
                                                                    {
                                                                        *(res_begin+begin2)= *it;
                                                                        ++begin2;
                                                                    }
                                                                    // we choose not to copy elements with index > 2
                                                                }
                                                              },
                                                              10);
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<std::map<int,int>> res) mutable{
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");

                        auto total = res.get();
                        BOOST_CHECK_MESSAGE(std::is_sorted(m_data_res.begin(),m_data_res.begin()+total[0]+total[1]+total[2],
                                                           [](ToClassify const& lhs,ToClassify const& rhs)
                                                           {
                                                                return lhs.m_id < rhs.m_id;
                                                           }),"scan should have partititoned.");

                        // res will contain the accumulated map so that we can build iterators to each partition begin
                        // reactivate to see results
                        /*std::cout << "total:";
                        for (auto t: total)
                        {
                            std::cout << t.first << ":" << t.second << " , ";
                        }
                        std::cout << std::endl;
                        std::cout << "scan result:" << std::endl;
                        for (auto it = m_data_res.begin(); it != m_data_res.end();++it)
                        {
                            std::cout << (*it).m_id << " , ";
                        }
                        std::cout << std::endl;*/
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }

private:
    std::vector<int> m_data1;
    std::vector<int> m_data2;
    // a scan on a structure
    struct ToClassify
    {
        ToClassify(int id=-1):m_id(id){}
        int m_id;
    };
    void generate_struct(std::vector<ToClassify>& data, unsigned elements, unsigned dist)
    {
        data = std::vector<ToClassify>(elements,1);
        std::random_device rd;
        std::mt19937 mt(rd());
        //std::mt19937 mt(static_cast<unsigned int>(std::time(nullptr)));
        std::uniform_int_distribution<> dis(0, dist);
        std::generate(data.begin(), data.end(), std::bind(dis, std::ref(mt)));
    }
    std::vector<ToClassify> m_data;
    std::vector<ToClassify> m_data_res;
};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(test_scan_inclusive_scan)
    BOOST_ASYNC_FUTURE_MEMBER(test_scan_inclusive_scan_init)
    BOOST_ASYNC_FUTURE_MEMBER(test_scan_exclusive_scan)
    BOOST_ASYNC_FUTURE_MEMBER(test_scan_moved_ranges)
    BOOST_ASYNC_FUTURE_MEMBER(test_scan_moved_range)
    BOOST_ASYNC_FUTURE_MEMBER(test_scan_continuation)
    BOOST_ASYNC_FUTURE_MEMBER(test_classify)
};
}

BOOST_AUTO_TEST_CASE( test_scan_inclusive_scan )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<> >);

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<void> > fuv = proxy.test_scan_inclusive_scan();
        try
        {
            boost::future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_scan_inclusive_scan_init )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<> >);

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<void> > fuv = proxy.test_scan_inclusive_scan_init();
        try
        {
            boost::future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_scan_exclusive_scan )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<> >);

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<void> > fuv = proxy.test_scan_exclusive_scan();
        try
        {
            boost::future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_scan_moved_ranges )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<> >);

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<void> > fuv = proxy.test_scan_moved_ranges();
        try
        {
            boost::future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_scan_moved_range )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<> >);

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<void> > fuv = proxy.test_scan_moved_range();
        try
        {
            boost::future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_scan_continuation )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<> >);

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<void> > fuv = proxy.test_scan_continuation();
        try
        {
            boost::future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_scan_classify )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<> >);

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        boost::future<boost::future<void> > fuv = proxy.test_classify();
        try
        {
            boost::future<void> resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}
