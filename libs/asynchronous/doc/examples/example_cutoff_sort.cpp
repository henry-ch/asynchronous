#include <iostream>
#include <random>
#include <numeric>


#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_sort.hpp>
// we want to see intermediate results
#define BOOST_ASYNCHRONOUS_USE_COUT
#include <boost/asynchronous/helpers.hpp>

using namespace std;

namespace
{
void generate(std::vector<int>& test_data)
{
    test_data = std::vector<int>(10000000,1);
    std::mt19937 mt(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<> dis(0, 100000);
    std::generate(test_data.begin(), test_data.end(), std::bind(dis, std::ref(mt)));
}
}

void example_cutoff_sort(std::size_t cutoff_begin,
                         std::size_t cutoff_end,
                         std::size_t steps,
                         std::size_t retries)
{
    std::cout << "example_cutoff_sort" << std::endl;
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                                boost::asynchronous::multiqueue_threadpool_scheduler<
                                        boost::asynchronous::lockfree_queue<>,
                                        boost::asynchronous::default_find_position< >,
                                        boost::asynchronous::no_cpu_load_saving
                                    >>
                        (boost::thread::hardware_concurrency());
        std::vector<int> test_data;
        generate(test_data);
        std::tuple<std::size_t,std::vector<std::size_t>> best_cutoff=
                boost::asynchronous::find_best_cutoff(scheduler,
                    [test_data](std::size_t cutoff)mutable
                    {return boost::asynchronous::parallel_sort(std::move(test_data),std::less<int>(),cutoff);},
                    cutoff_begin,cutoff_end,steps,retries
        );
        std::size_t acc = std::accumulate(std::get<1>(best_cutoff).begin(),std::get<1>(best_cutoff).end(),0,
                                          [](std::size_t a, std::size_t b){return a+b;});
        std::cout << "example_cutoff_sort has best cutoff: "<< std::get<0>(best_cutoff)
                  << " , took us (average):" << acc/std::get<1>(best_cutoff).size() << std::endl;

    }
    std::cout << "end example_sort \n" << std::endl;
}



