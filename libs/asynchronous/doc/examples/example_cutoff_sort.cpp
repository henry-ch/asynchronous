#include <iostream>
#include <random>
#include <numeric>
#include <boost/enable_shared_from_this.hpp>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_sort.hpp>

using namespace std;

namespace
{
void generate(std::vector<int>& test_data)
{
    test_data = std::vector<int>(10000000,1);
    std::mt19937 mt(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<> dis(0, 1000000);
    std::generate(test_data.begin(), test_data.end(), std::bind(dis, std::ref(mt)));
}

// calls f(cutoff) on the given scheduler and measures elapsed time
// returns elapsed time in us
template <class Func, class Scheduler>
std::size_t measure_cutoff(Scheduler s, Func f,std::size_t cutoff)
{
    typename boost::chrono::high_resolution_clock::time_point start;
    typename boost::chrono::high_resolution_clock::time_point stop;
    start = boost::chrono::high_resolution_clock::now();
    auto fu = boost::asynchronous::post_future(s,[cutoff,&f]()mutable{return f(cutoff);});
    fu.get();
    stop = boost::chrono::high_resolution_clock::now();
    return (boost::chrono::nanoseconds(stop - start).count() / 1000);
}
template <class Func, class Scheduler>
std::tuple<std::size_t,std::vector<std::size_t>> find_best_cutoff(Scheduler s, Func f,
                                                     std::size_t cutoff_min,
                                                     std::size_t cutoff_max,
                                                     std::size_t steps,
                                                     std::size_t retries)
{
    std::map<std::size_t, std::vector<std::size_t>> map_cutoff_to_elapsed;
    for (std::size_t i = 0; i< steps; ++i)
    {
        map_cutoff_to_elapsed[cutoff_min+((cutoff_max-cutoff_min)/steps)].reserve(retries);
        for (std::size_t j = 0; j< retries; ++j)
        {
            std::size_t cutoff = cutoff_min+((cutoff_max-cutoff_min)/steps)*i;
            std::size_t one_step = measure_cutoff(s, f, cutoff);
            map_cutoff_to_elapsed[cutoff].push_back(one_step);
            std::cout << "example_cutoff_sort took for cutoff "<< cutoff << " in us:" << one_step << std::endl;
        }
    }
    // look for best
    std::size_t best_cutoff=0;
    std::size_t best = std::numeric_limits<std::size_t>::max();
    for (auto it = map_cutoff_to_elapsed.begin(); it != map_cutoff_to_elapsed.end();++it)
    {
        std::size_t acc = std::accumulate((*it).second.begin(),(*it).second.end(),0,[](std::size_t a, std::size_t b){return a+b;});
        if (acc < best)
        {
            best_cutoff = (*it).first;
            best = acc;
        }
    }
    return std::make_tuple(best_cutoff,std::move(map_cutoff_to_elapsed[best_cutoff]));
}

}

void example_cutoff_sort(std::size_t cutoff_min,
                         std::size_t cutoff_max,
                         std::size_t steps,
                         std::size_t retries)
{
    std::cout << "example_cutoff_sort" << std::endl;
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::multiqueue_threadpool_scheduler<
                                        boost::asynchronous::lockfree_queue<>,
                                        boost::asynchronous::default_find_position< >,
                                        boost::asynchronous::no_cpu_load_saving
                                    >
                        (boost::thread::hardware_concurrency()));
        std::vector<int> test_data;
        generate(test_data);
        std::tuple<std::size_t,std::vector<std::size_t>> best_cutoff=
                find_best_cutoff(scheduler,
                    [test_data](std::size_t cutoff)mutable
                    {return boost::asynchronous::parallel_sort_move(std::move(test_data),std::less<int>(),cutoff);},
                    cutoff_min,cutoff_max,steps,retries
        );
        std::size_t acc = std::accumulate(std::get<1>(best_cutoff).begin(),std::get<1>(best_cutoff).end(),0,
                                          [](std::size_t a, std::size_t b){return a+b;});
        std::cout << "example_cutoff_sort has best cutoff: "<< std::get<0>(best_cutoff)
                  << " , took us (average):" << acc/10 << std::endl;

    }
    std::cout << "end example_sort \n" << std::endl;
}



