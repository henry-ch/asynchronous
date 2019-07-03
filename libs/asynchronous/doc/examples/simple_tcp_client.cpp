#include <iostream>
#include <boost/program_options.hpp>

#include <boost/asynchronous/scheduler/tcp/simple_tcp_client.hpp>
#include <boost/asynchronous/extensions/asio/asio_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/queue/guarded_deque.hpp>

#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>

#include <boost/asynchronous/nn/cnn.hpp>
// our app-specific functors
#include <libs/asynchronous/doc/examples/dummy_tcp_task.hpp>
#include <libs/asynchronous/doc/examples/serializable_fib_task.hpp>
#include <libs/asynchronous/doc/examples/dummy_parallel_for_task.hpp>
#include <libs/asynchronous/doc/examples/dummy_parallel_reduce_task.hpp>
#include <libs/asynchronous/doc/examples/dummy_parallel_find_all_task.hpp>
#include <libs/asynchronous/doc/examples/dummy_parallel_count_task.hpp>
#include <libs/asynchronous/doc/examples/dummy_parallel_sort_task.hpp>

using namespace std;
namespace po = boost::program_options;

template <class Queue>
boost::asynchronous::any_shared_scheduler_proxy<> create_pool(std::size_t numaStride,
                                                              std::size_t partThreadPoolSize,
                                                              std::size_t numaNodes,
                                                              std::size_t threads,
                                                              std::size_t numaBegin)
{
    boost::asynchronous::any_shared_scheduler_proxy<> pool;
    std::size_t tasksSize=64;

    // if many threads, binding is better
    if (threads > 20 && partThreadPoolSize == 0)
    {
        // create threadpool
        pool = boost::asynchronous::create_shared_scheduler_proxy(
                          new boost::asynchronous::multiqueue_threadpool_scheduler<
                            boost::asynchronous::lockfree_queue<>,
                            boost::asynchronous::default_find_position<>,
                            //boost::asynchronous::no_cpu_load_saving
                            boost::asynchronous::default_save_cpu_load<10, 80000, 1000>
                            >(threads, tasksSize));
        //pool.processor_bind({{0,threads}});
        std::vector<std::tuple<unsigned int,unsigned int>> v;
        v.push_back(std::make_tuple(0,threads));
        pool.processor_bind(std::move(v));
        std::cout << "created bound pool with threads= " << threads << std::endl;
    }
    else if (partThreadPoolSize > 1)
    {
        std::cout << "use NUMA" << std::endl;
        for(std::size_t i = 0; i < numaNodes ; ++i)
        {
            pool = boost::asynchronous::create_shared_scheduler_proxy(
                                new boost::asynchronous::multiqueue_threadpool_scheduler<
                                boost::asynchronous::lockfree_queue<>,
                                boost::asynchronous::default_find_position<>,
                                //boost::asynchronous::no_cpu_load_saving
                                boost::asynchronous::default_save_cpu_load<10, 80000, 1000>
                                >(threads / numaNodes, tasksSize));
            std::cout << "created pool with threads= " << threads / numaNodes
                      << std::endl;
            // bind to cores
            std::vector<std::tuple<unsigned int,unsigned int>> v;
            std::size_t usedCores = 0;
            std::size_t nodeStartCore = (i+numaBegin) * partThreadPoolSize;
            std::cout << "nodeStartCore = " << nodeStartCore << " partThreadPoolSize= " << partThreadPoolSize
                      << " numaBegin= " << numaBegin << std::endl;

            std::size_t currentStride = 0;

            while(usedCores < threads / numaNodes)
            {
              std::cout << " adding for node: " << i << " pool of: " << partThreadPoolSize
                        << " threads starting at core: " << nodeStartCore+currentStride << std::endl;
              v.push_back(std::make_tuple(nodeStartCore+currentStride,partThreadPoolSize));
              usedCores += partThreadPoolSize;
              currentStride += numaStride;
            }
            pool.processor_bind(std::move(v));
        }
    }
    else
    {
        pool = boost::asynchronous::create_shared_scheduler_proxy(
                          new boost::asynchronous::multiqueue_threadpool_scheduler<
                            boost::asynchronous::lockfree_queue<>,
                            boost::asynchronous::default_find_position<>,
                            //boost::asynchronous::no_cpu_load_saving
                            boost::asynchronous::default_save_cpu_load<10, 80000, 1000>
                            >(threads, tasksSize));
        std::cout << "created pool with threads= " << threads << std::endl;
    }
    return pool;
}
int main(int argc, char* argv[])
{
    std::size_t numaStride = 0;
    std::size_t partThreadPoolSize = 0; // use if numaStride > 0
    std::size_t numaNodes = 1;
    std::size_t threads = std::thread::hardware_concurrency()/2;
    std::size_t numaBegin = 0;
    // 0 => default, try getting a job at regular time intervals
    // 1..n => check at regular time intervals if the queue is under the given size
    int job_getting_policy=0;
    std::string server_address = "localhost";
    std::string server_port = "12346";

    po::options_description desc("Allowed Options");
    desc.add_options()
      ("help", "show allowed options")
      ("address,a", po::value<std::string>(&server_address)->default_value("localhost"), "address of job server")
      ("port,p", po::value<std::string>(&server_port)->default_value("12346"), "port of job server")
      ("job,j", po::value<int>(&job_getting_policy)->default_value(0), "0 => default, try getting a job at regular time intervals. 1..n => check at regular time intervals if the queue is under the given size")
      ("numaStride,S", po::value<std::size_t>(&numaStride)->default_value(0), "jump between 2 sets of core within a numa node")
      ("partThreadPoolSize,P", po::value<std::size_t>(&partThreadPoolSize)->default_value(0), "number of cores in a numa stride")
      ("threads,T", po::value<std::size_t>(&threads), "size of threadpool")
      ("numaBegin,B", po::value<std::size_t>(&numaBegin)->default_value(0), "first used numa node")
      ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    server_address = vm["address"].as<std::string>();
    server_port = vm["port"].as<std::string>();

    numaStride = vm["numaStride"].as<std::size_t>();
    numaBegin = vm["numaBegin"].as<std::size_t>();
    if (vm.count("threads"))
    {
        threads = vm["threads"].as<std::size_t>();
    }
    partThreadPoolSize = vm["partThreadPoolSize"].as<std::size_t>();

//    std::string server_address = (argc>1) ? argv[1]:"localhost";
//    std::string server_port = (argc>2) ? argv[2]:"12346";
//    int threads = (argc>3) ? strtol(argv[3],0,0) : 4;
//    // 0 => default, try getting a job at regular time intervals
//    // 1..n => check at regular time intervals if the queue is under the given size
//    int job_getting_policy = (argc>4) ? strtol(argv[4],0,0):0;
    cout << "Starting connecting to " << server_address << " port " << server_port << " with " << threads << " threads" << endl;

    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                            boost::asynchronous::asio_scheduler<>>();
    {
        std::function<void(std::string const&,boost::asynchronous::tcp::server_reponse,std::function<void(boost::asynchronous::tcp::client_request const&)>)> executor=
        [](std::string const& task_name,boost::asynchronous::tcp::server_reponse resp,
           std::function<void(boost::asynchronous::tcp::client_request const&)> when_done)
        {
            std::cout << "got task: " << task_name
                      << " m_task_id: " << resp.m_task_id
                      << std::endl;
            if (task_name=="serializable_train_task")
            {
                boost::asynchronous::cnn<>::serializable_train_task t;
                boost::asynchronous::tcp::deserialize_and_call_top_level_callback_continuation_task<
                                    boost::asynchronous::cnn<>::serializable_train_task,
                                    boost::asynchronous::any_serializable>
                        (t,resp,when_done);
            }
            else if (task_name=="dummy_tcp_task")
            {
                dummy_tcp_task t(0);
                boost::asynchronous::tcp::deserialize_and_call_task(t,resp,when_done);
            }
            else if (task_name=="serializable_fib_task")
            {
                tcp_example::serializable_fib_task fib(0,0);
                boost::asynchronous::tcp::deserialize_and_call_top_level_callback_continuation_task(fib,resp,when_done);
            }
            else if (task_name=="serializable_sub_fib_task")
            {
                tcp_example::fib_task fib(0,0);
                boost::asynchronous::tcp::deserialize_and_call_callback_continuation_task(fib,resp,when_done);
            }
            else if (task_name=="dummy_parallel_for_task")
            {
                dummy_parallel_for_task t;
                boost::asynchronous::tcp::deserialize_and_call_top_level_callback_continuation_task(t,resp,when_done);
            }
            else if (task_name=="dummy_parallel_for_subtask")
            {
                boost::asynchronous::parallel_for_range_move_helper<vector<int>,
                                                                    dummy_parallel_for_subtask,
                                                                    boost::asynchronous::any_serializable> t;
                boost::asynchronous::tcp::deserialize_and_call_callback_continuation_task(t,resp,when_done);
            }
            else if (task_name=="dummy_parallel_reduce_task")
            {
                dummy_parallel_reduce_task t;
                boost::asynchronous::tcp::deserialize_and_call_top_level_callback_continuation_task(t,resp,when_done);
            }
            else if (task_name=="dummy_parallel_reduce_subtask")
            {
                boost::asynchronous::parallel_reduce_range_move_helper<vector<long>,
                                                                       dummy_parallel_reduce_subtask,
                                                                       dummy_parallel_reduce_subtask,
                                                                       long,
                                                                       boost::asynchronous::any_serializable> t;
                boost::asynchronous::tcp::deserialize_and_call_callback_continuation_task(t,resp,when_done);
            }
            else if (task_name=="dummy_parallel_find_all_task")
            {
                dummy_parallel_find_all_task t;
                boost::asynchronous::tcp::deserialize_and_call_top_level_callback_continuation_task(t,resp,when_done);
            }
            else if (task_name=="dummy_parallel_find_all_subtask")
            {
                boost::asynchronous::parallel_find_all_range_move_helper<vector<int>,
                                                                       dummy_parallel_find_all_subtask,
                                                                       vector<int>,
                                                                       boost::asynchronous::any_serializable> t;
                boost::asynchronous::tcp::deserialize_and_call_callback_continuation_task(t,resp,when_done);
            }
            else if (task_name=="dummy_parallel_count_task")
            {
                dummy_parallel_count_task t;
                boost::asynchronous::tcp::deserialize_and_call_top_level_callback_continuation_task(t,resp,when_done);
            }
            else if (task_name=="dummy_parallel_count_subtask")
            {
                boost::asynchronous::parallel_count_range_move_helper<vector<int>,
                                                                       dummy_parallel_count_subtask,
                                                                       boost::asynchronous::any_serializable> t;
                boost::asynchronous::tcp::deserialize_and_call_callback_continuation_task(t,resp,when_done);
            }
            else if (task_name=="dummy_parallel_sort_task")
            {
                dummy_parallel_sort_task t;
                boost::asynchronous::tcp::deserialize_and_call_top_level_callback_continuation_task(t,resp,when_done);
            }
            else if (task_name=="dummy_parallel_sort_subtask")
            {
                boost::asynchronous::parallel_sort_range_move_helper_serializable<vector<int>,
                                                                       dummy_parallel_sort_subtask,
                                                                       boost::asynchronous::any_serializable,
                                                                       boost::asynchronous::std_sort> t;
                boost::asynchronous::tcp::deserialize_and_call_callback_continuation_task(t,resp,when_done);
            }
            // else whatever functor we support
            else
            {
                std::cout << "unknown task! Sorry, don't know: " << task_name << std::endl;
                throw boost::asynchronous::tcp::transport_exception("unknown task");
            }
        };

        if (job_getting_policy == 0)
        {
            /*auto pool = boost::asynchronous::make_shared_scheduler_proxy<
                        boost::asynchronous::multiqueue_threadpool_scheduler<
                            boost::asynchronous::lockfree_queue<>>>(threads);*/
            auto pool = create_pool<boost::asynchronous::lockfree_queue<>>(numaStride,partThreadPoolSize,numaNodes,
                                                                           threads,numaBegin);
            boost::asynchronous::tcp::simple_tcp_client_proxy_ext<> proxy(scheduler,pool,server_address,server_port,executor,
                                                                    10/*ms between calls to server*/);

            // run forever
            std::future<std::future<void> > fu = std::move(proxy.run());
            std::future<void> fu_end = std::move(fu.get());
            fu_end.get();
        }
        else
        {
            // guarded_deque supports queue size
            /*auto pool = boost::asynchronous::make_shared_scheduler_proxy<
                          boost::asynchronous::threadpool_scheduler<
                            boost::asynchronous::guarded_deque<>>>(threads);*/

            auto pool = create_pool<boost::asynchronous::guarded_deque<>>(numaStride,partThreadPoolSize,numaNodes,
                                                                           threads,numaBegin);

            boost::asynchronous::tcp::simple_tcp_client_proxy_ext<
                    boost::asynchronous::tcp::queue_size_check_policy<>,
                    boost::asynchronous::any_serializable,
                    boost::asynchronous::any_callable>
                                proxy( scheduler,pool,server_address,server_port,executor,
                                       10/*ms between calls to server*/,
                                       job_getting_policy /* number of jobs we try to keep in queue */);
            // run forever
            std::future<std::future<void> > fu = std::move(proxy.run());
            std::future<void> fu_end = std::move(fu.get());
            fu_end.get();
        }
    }

    return 0;
}

