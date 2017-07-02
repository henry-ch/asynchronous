#include <iostream>
#include <vector>

#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/container/vector.hpp>

using namespace std;

typename std::chrono::high_resolution_clock::time_point servant_time;
double duration1=0.0;
double duration2=0.0;

long tpsize = 0;
long tasks = 0;
std::size_t vec_size=0;
std::size_t long_size=10;
boost::asynchronous::any_shared_scheduler_proxy<> pool;

//#define COMPLICATED_CONSTRUCTION

struct LongOne
{
    LongOne(int n = 0) noexcept
        :data(long_size,n)
    {
#ifdef COMPLICATED_CONSTRUCTION
        for (std::size_t i = 0 ; i<long_size; ++i)
        {
            data[i]= rand();
        }
#endif
    }
    LongOne& operator= (LongOne const& rhs)noexcept
    {
        data = rhs.data;
        return *this;
    }
    LongOne& operator= (LongOne&& rhs)noexcept
    {
        data = std::move(rhs.data);
        return *this;
    }
    LongOne(LongOne const& rhs)noexcept
        : data(rhs.data)
    {}
    LongOne(LongOne&&)noexcept = default;

    std::vector<int> data;
};
bool operator== (LongOne const& lhs, LongOne const& rhs)
{
    return rhs.data == lhs.data;
}
bool operator< (LongOne const& lhs, LongOne const& rhs)
{
    return lhs.data < rhs.data;
}

// creation
std::vector<LongOne> test_vector_create(std::size_t s)
{
    return std::vector<LongOne> (s,LongOne());
}
boost::asynchronous::vector<LongOne> test_async_vector_create(std::size_t s)
{
    return boost::asynchronous::vector<LongOne> (pool,s/tasks,s,LongOne());
}

int main( int argc, const char *argv[] )
{
    tpsize = (argc>1) ? strtol(argv[1],0,0) : boost::thread::hardware_concurrency();
    tasks = (argc>2) ? strtol(argv[2],0,0) : 64;
    vec_size = (argc>3) ? strtol(argv[3],0,0) : 10000000;
    long_size = (argc>4) ? strtol(argv[4],0,0) : 10;
    std::cout << "tpsize=" << tpsize << std::endl;
    std::cout << "tasks=" << tasks << std::endl;
    std::cout << "vec_size=" << vec_size << std::endl;
    std::cout << "long_size=" << long_size << std::endl;
    std::cout << std::endl;

    // creation std
    duration1=0.0;
    auto start = std::chrono::high_resolution_clock::now();
    auto stdv = test_vector_create(vec_size);
    duration1 = (std::chrono::nanoseconds(std::chrono::high_resolution_clock::now() - start).count() / 1000000);
    std::cout << "Construction of std::vector<LongOne>(" << stdv.size() << ") took in ms: " << duration1 << std::endl;

    pool = boost::asynchronous::make_shared_scheduler_proxy<
                    boost::asynchronous::multiqueue_threadpool_scheduler<
                            boost::asynchronous::lockfree_queue<>/*,
                            boost::asynchronous::default_find_position< boost::asynchronous::sequential_push_policy>,
                            boost::asynchronous::no_cpu_load_saving*/
                        >>(tpsize,tasks);
    // set processor affinity to improve cache usage. We start at core 0, until tpsize-1
    pool.processor_bind({{0,tpsize}});
    // creation asynchronous
    duration2=0.0;
    start = std::chrono::high_resolution_clock::now();
    auto asyncv = test_async_vector_create(vec_size);
    duration2 = (std::chrono::nanoseconds(std::chrono::high_resolution_clock::now() - start).count() / 1000000);
    std::cout << "Construction of boost::asynchronous::vector<LongOne>(" << asyncv.size() << ") took in ms: " << duration2 << std::endl;
    std::cout << "speedup asynchronous: " << duration1 / duration2 << std::endl << std::endl;

    // copy std
    duration1=0.0;
    start = std::chrono::high_resolution_clock::now();
    auto stdv2 = stdv;
    duration1 = (std::chrono::nanoseconds(std::chrono::high_resolution_clock::now() - start).count() / 1000000);
    std::cout << "Copy of std::vector<LongOne>(" << stdv.size() << ") took in ms: " << duration1 << std::endl;

    // copy asynchronous
    duration2=0.0;
    start = std::chrono::high_resolution_clock::now();
    auto asyncv2 = asyncv;
    duration2 = (std::chrono::nanoseconds(std::chrono::high_resolution_clock::now() - start).count() / 1000000);
    std::cout << "Copy of boost::asynchronous::vector<LongOne>(" << asyncv.size() << ") took in ms: " << duration2 << std::endl;
    std::cout << "speedup asynchronous: " << duration1 / duration2 << std::endl << std::endl;

    // compare std
    duration1=0.0;
    start = std::chrono::high_resolution_clock::now();
    bool equal = (stdv2 == stdv);
    duration1 = (std::chrono::nanoseconds(std::chrono::high_resolution_clock::now() - start).count() / 1000000);
    std::cout << "Compare of std::vector<LongOne>(" << stdv.size() << ") took in ms: " << duration1 << ". Res:" << equal << std::endl;

    // compare asynchronous
    duration2=0.0;
    start = std::chrono::high_resolution_clock::now();
    equal = (asyncv2 == asyncv);
    duration2 = (std::chrono::nanoseconds(std::chrono::high_resolution_clock::now() - start).count() / 1000000);
    std::cout << "Compare of boost::asynchronous::vector<LongOne>(" << asyncv.size() << ") took in ms: " << duration2 << ". Res:" << equal<< std::endl;
    std::cout << "speedup asynchronous: " << duration1 / duration2 << std::endl << std::endl;

    // clear std
    duration1=0.0;
    start = std::chrono::high_resolution_clock::now();
    stdv2.clear();
    duration1 = (std::chrono::nanoseconds(std::chrono::high_resolution_clock::now() - start).count() / 1000000);
    std::cout << "Clear of std::vector<LongOne>(" << stdv.size() << ") took in ms: " << duration1 << std::endl;

    // clear asynchronous
    duration2=0.0;
    start = std::chrono::high_resolution_clock::now();
    asyncv2.clear();
    duration2 = (std::chrono::nanoseconds(std::chrono::high_resolution_clock::now() - start).count() / 1000000);
    std::cout << "Clear of boost::asynchronous::vector<LongOne>(" << asyncv.size() << ") took in ms: " << duration2 << std::endl;
    std::cout << "speedup asynchronous: " << duration1 / duration2 << std::endl << std::endl;

    // resize std
    duration1=0.0;
    start = std::chrono::high_resolution_clock::now();
    stdv.resize(vec_size * 2);
    duration1 = (std::chrono::nanoseconds(std::chrono::high_resolution_clock::now() - start).count() / 1000000);
    std::cout << "Resize of std::vector<LongOne>(" << stdv.size() << ") took in ms: " << duration1 << std::endl;

    // resize asynchronous
    duration2=0.0;
    start = std::chrono::high_resolution_clock::now();
    asyncv.resize(vec_size * 2);;
    duration2 = (std::chrono::nanoseconds(std::chrono::high_resolution_clock::now() - start).count() / 1000000);
    std::cout << "Resize of boost::asynchronous::vector<LongOne>(" << asyncv.size() << ") took in ms: " << duration2 << std::endl;
    std::cout << "speedup asynchronous: " << duration1 / duration2 << std::endl << std::endl;

    return 0;
}
