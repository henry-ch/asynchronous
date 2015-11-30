#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <vector>
#include <memory>
//#include <stdlib.h>

#include <boost/lexical_cast.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/smart_ptr/shared_array.hpp>

#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/any_queue_container.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_partition_copy.hpp>

#include <boost/asynchronous/helpers/lazy_irange.hpp>
#include <boost/asynchronous/algorithm/parallel_copy.hpp>
#include <boost/asynchronous/algorithm/parallel_fill.hpp>
#include <boost/asynchronous/algorithm/parallel_generate.hpp>

#include <boost/asynchronous/helpers/random_provider.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

using namespace std;


#define LOOP 1

#define NELEM 200000000
#define SORTED_TYPE uint32_t

//#define NELEM 100000000
//#define SORTED_TYPE std::string

typename boost::chrono::high_resolution_clock::time_point servant_time;
double servant_intern=0.0;
long tpsize = 12;
long tasks = 500;

boost::asynchronous::any_shared_scheduler_proxy<> pool;

namespace boost {
template<>
inline std::string lexical_cast(const uint32_t& arg)
{
    return std::to_string(arg);
}
}

template <class T, class U>
typename boost::disable_if<boost::is_same<T,U>,U >::type
test_cast(T const& t)
{
    return boost::lexical_cast<U>(t);
}
template <class T, class U>
typename boost::enable_if<boost::is_same<T,U>,U >::type
test_cast(T const& t)
{
    return t;
}

SORTED_TYPE compare_with = test_cast<uint32_t,SORTED_TYPE>(NELEM/2);

struct Servant : boost::asynchronous::trackable_servant<>
{
    //typedef int simple_ctor;
    //typedef int requires_weak_scheduler;
    typedef std::vector<SORTED_TYPE>::iterator Iterator;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler, pool)
        , m_promise(new boost::promise<void>)
        , m_tpsize(tpsize)
    {
    }
    ~Servant(){}

    void on_callback_vec(std::pair<Iterator,Iterator>)
    {
        servant_intern += (boost::chrono::nanoseconds(boost::chrono::high_resolution_clock::now() - servant_time).count() / 1000000);
        m_promise->set_value();
    }
    boost::shared_future<void> do_partition_vec(std::vector<SORTED_TYPE> a)
    {
        boost::shared_future<void> fu = m_promise->get_future();
        long tasksize = NELEM / tasks;
        std::shared_ptr<std::vector<SORTED_TYPE>> vec = std::make_shared<std::vector<SORTED_TYPE>>(std::move(a));
        std::shared_ptr<std::vector<SORTED_TYPE>> vec_true = std::make_shared<std::vector<SORTED_TYPE>>(NELEM);
        std::shared_ptr<std::vector<SORTED_TYPE>> vec_false = std::make_shared<std::vector<SORTED_TYPE>>(NELEM);

        servant_time = boost::chrono::high_resolution_clock::now();
        post_callback(
               [vec,vec_true,vec_false,tasksize](){
                        return boost::asynchronous::parallel_partition_copy(
                                   (*vec).begin(),(*vec).end(),
                                   (*vec_true).begin(),(*vec_false).begin(),
                                   [](SORTED_TYPE const& i)
                                   {
                                      return i < compare_with;
                                   },tasksize,"",0);
                      },// work
               // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
               [this,vec,vec_true,vec_false](boost::asynchronous::expected<std::pair<Iterator,Iterator>> res)
               {
                    auto res_pair = std::move(res.get());
                    this->on_callback_vec(res_pair);
               }// callback functor.
               ,"",0,0
        );
        return fu;
    }
private:
// for testing
std::shared_ptr<boost::promise<void> > m_promise;
size_t m_tpsize;
};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(do_partition_vec,0)
};

void ParallelAsyncPostCb(std::vector<SORTED_TYPE>& vec)
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                            boost::asynchronous::single_thread_scheduler<boost::asynchronous::lockfree_queue<>,
                                                                         boost::asynchronous::default_save_cpu_load<10,80000,1000>>>(tpsize);
    {
        ServantProxy proxy(scheduler);
        boost::shared_future<boost::shared_future<void> > fu = proxy.do_partition_vec(vec);
        boost::shared_future<void> resfu = fu.get();
        resfu.get();
    }
    {
        auto seq_start = boost::chrono::high_resolution_clock::now();
        std::vector<SORTED_TYPE> vec_true(NELEM);
        std::vector<SORTED_TYPE> vec_false(NELEM);
        std::partition_copy(vec.begin(),vec.end(),
                            vec_true.begin(),vec_false.begin(),
                            [](SORTED_TYPE const& i)
                            {
                                return i < compare_with;
                            });
        auto seq_stop = boost::chrono::high_resolution_clock::now();
        double seq_time = (boost::chrono::nanoseconds(seq_stop - seq_start).count() / 1000000);
        printf ("\n%50s: time = %.1f msec\n","sequential", seq_time);
    }
}
void test_sorted_elements(void(*pf)(std::vector<SORTED_TYPE>& ))
{
    std::vector<SORTED_TYPE> a(NELEM);
    auto lazy = boost::asynchronous::lazy_irange(
                  0, NELEM,
                  [](uint32_t index) {
                      return test_cast<decltype(index), SORTED_TYPE>(index + NELEM);\
                  });
    auto fu = boost::asynchronous::post_future(
                pool,
                [&]{
                    return boost::asynchronous::parallel_copy(
                                lazy.begin(), lazy.end(),
                                a.begin(),
                                NELEM / tasks);
                });
    fu.get();
    /* serial version:
    for ( uint32_t i = 0 ; i < NELEM ; ++i)
    {
        a[i] = test_cast<uint32_t,SORTED_TYPE>( i+NELEM) ;
    }
     */
    (*pf)(a);
}
void test_random_elements_many_repeated(void(*pf)(std::vector<SORTED_TYPE>& ))
{
    std::vector<SORTED_TYPE> a(NELEM);
    auto fu = boost::asynchronous::post_future(
                pool,
                [&]{
                    return boost::asynchronous::parallel_generate(
                                a.begin(), a.end(),
                                []{
                                    boost::random::uniform_int_distribution<> distribution(0, 9999); // 9999 is inclusive, rand() % 10000 was exclusive
                                    uint32_t gen = boost::asynchronous::random_provider<boost::random::mt19937>::generate(distribution);
                                    return test_cast<uint32_t, SORTED_TYPE>(gen);
                                }, NELEM / tasks);
                });
    fu.get();
    (*pf)(a);
}
void test_random_elements_few_repeated(void(*pf)(std::vector<SORTED_TYPE>& ))
{
    std::vector<SORTED_TYPE> a(NELEM);
    auto fu = boost::asynchronous::post_future(
                pool,
                [&]{
                    return boost::asynchronous::parallel_generate(
                                a.begin(), a.end(),
                                []{
                                    uint32_t gen = boost::asynchronous::random_provider<boost::random::mt19937>::generate();
                                    return test_cast<uint32_t, SORTED_TYPE>(gen);
                                }, NELEM / tasks);
                });
    fu.get();
    (*pf)(a);
}
void test_random_elements_quite_repeated(void(*pf)(std::vector<SORTED_TYPE>& ))
{
    std::vector<SORTED_TYPE> a(NELEM);
    auto fu = boost::asynchronous::post_future(
                pool,
                [&]{
                    return boost::asynchronous::parallel_generate(
                                a.begin(), a.end(),
                                []{
                                    boost::random::uniform_int_distribution<> distribution(0, NELEM / 2 - 1);
                                    uint32_t gen = boost::asynchronous::random_provider<boost::random::mt19937>::generate(distribution);
                                    return test_cast<uint32_t, SORTED_TYPE>(gen);
                                }, NELEM / tasks);
                });
    fu.get();
    (*pf)(a);
}
void test_reversed_sorted_elements(void(*pf)(std::vector<SORTED_TYPE>& ))
{
    std::vector<SORTED_TYPE> a(NELEM);
    auto lazy = boost::asynchronous::lazy_irange(
                  0, NELEM,
                  [](uint32_t index) {
                      return test_cast<decltype(index), SORTED_TYPE>((NELEM << 1) - index);\
                  });
    auto fu = boost::asynchronous::post_future(
                pool,
                [&]{
                    return boost::asynchronous::parallel_copy(
                                lazy.begin(), lazy.end(),
                                a.begin(),
                                NELEM / tasks);
                });
    fu.get();
    /* serial version:
    for ( uint32_t i = 0 ; i < NELEM ; ++i)
    {
        a[i] = test_cast<uint32_t,SORTED_TYPE>((NELEM<<1) -i) ;
    }
     */
    (*pf)(a);
}
void test_equal_elements(void(*pf)(std::vector<SORTED_TYPE>& ))
{
    std::vector<SORTED_TYPE> a(NELEM);
    auto fu = boost::asynchronous::post_future(
                pool,
                [&]{
                    return boost::asynchronous::parallel_fill(a.begin(), a.end(), test_cast<uint32_t, SORTED_TYPE>(NELEM), NELEM / tasks);
                });
    fu.get();
    (*pf)(a);
}
int main( int argc, const char *argv[] ) 
{           
    tpsize = (argc>1) ? strtol(argv[1],0,0) : boost::thread::hardware_concurrency();
    tasks = (argc>2) ? strtol(argv[2],0,0) : 500;
    std::cout << "tpsize=" << tpsize << std::endl;
    std::cout << "tasks=" << tasks << std::endl;

    pool = boost::asynchronous::make_shared_scheduler_proxy<
           boost::asynchronous::multiqueue_threadpool_scheduler<
                   boost::asynchronous::lockfree_queue<>,
                   boost::asynchronous::default_find_position< boost::asynchronous::sequential_push_policy>,
               //boost::asynchronous::default_save_cpu_load<10,80000,1000>
               boost::asynchronous::no_cpu_load_saving
               >>(tpsize);

    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {
        test_random_elements_many_repeated(ParallelAsyncPostCb);
    }
    printf ("%50s: time = %.1f msec\n","test_random_elements_many_repeated", servant_intern);
    
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {
        test_random_elements_few_repeated(ParallelAsyncPostCb);
    }
    printf ("%50s: time = %.1f msec\n","test_random_elements_few_repeated", servant_intern);
    
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {     
        test_random_elements_quite_repeated(ParallelAsyncPostCb);
    }
    printf ("%50s: time = %.1f msec\n","test_random_elements_quite_repeated", servant_intern);
    
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {     
        test_sorted_elements(ParallelAsyncPostCb);
    }
    printf ("%50s: time = %.1f msec\n","test_sorted_elements", servant_intern);
    
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {     
        test_reversed_sorted_elements(ParallelAsyncPostCb);
    }
    printf ("%50s: time = %.1f msec\n","test_reversed_sorted_elements", servant_intern);
    
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {     
        test_equal_elements(ParallelAsyncPostCb);
    }
    printf ("%50s: time = %.1f msec\n","test_equal_elements", servant_intern);
    
    std::cout << std::endl;
    
    return 0;
}
