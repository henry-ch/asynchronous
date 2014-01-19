#include <iostream>
#include <boost/asynchronous/scheduler/tcp/simple_tcp_client.hpp>
#include <boost/asynchronous/extensions/asio/asio_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/queue/guarded_deque.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

// our app-specific functors
#include <libs/asynchronous/doc/examples/dummy_tcp_task.hpp>
#include <libs/asynchronous/doc/examples/serializable_fib_task.hpp>
#include <libs/asynchronous/doc/examples/dummy_parallel_for_task.hpp>

using namespace std;

int main(int argc, char* argv[])
{
    std::string server_address = (argc>1) ? argv[1]:"localhost";
    std::string server_port = (argc>2) ? argv[2]:"12346";
    int threads = (argc>3) ? strtol(argv[3],0,0) : 4;
    // 0 => default, try getting a job at regular time intervals
    // 1..n => check at regular time intervals if the queue is under the given size
    int job_getting_policy = (argc>4) ? strtol(argv[4],0,0):0;
    cout << "Starting connecting to " << server_address << " port " << server_port << " with " << threads << " threads" << endl;

    auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                new boost::asynchronous::asio_scheduler<>);
    {
        std::function<void(std::string const&,boost::asynchronous::tcp::server_reponse,std::function<void(boost::asynchronous::tcp::client_request const&)>)> executor=
        [](std::string const& task_name,boost::asynchronous::tcp::server_reponse resp,
           std::function<void(boost::asynchronous::tcp::client_request const&)> when_done)
        {
            std::cout << "got task: " << task_name
//                      << " task: " << resp.m_task
                      << " m_task_id: " << resp.m_task_id
                      << std::endl;
            if (task_name=="dummy_tcp_task")
            {
                dummy_tcp_task t(0);
                boost::asynchronous::tcp::deserialize_and_call_task(t,resp,when_done);
            }
            else if (task_name=="serializable_fib_task")
            {
                tcp_example::serializable_fib_task fib(0,0);
                boost::asynchronous::tcp::deserialize_and_call_top_level_continuation_task(fib,resp,when_done);
            }
            else if (task_name=="serializable_sub_fib_task")
            {
                tcp_example::fib_task fib(0,0);
                boost::asynchronous::tcp::deserialize_and_call_continuation_task(fib,resp,when_done);
            }
            else if (task_name=="dummy_parallel_for_task")
            {
                dummy_parallel_for_task t;
                boost::asynchronous::tcp::deserialize_and_call_top_level_continuation_task(t,resp,when_done);
            }
            else if (task_name=="dummy_parallel_for_subtask")
            {
                boost::asynchronous::serializable_for_each<dummy_parallel_for_subtask,vector<int>> t;
                boost::asynchronous::tcp::deserialize_and_call_task(t,resp,when_done);
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
            auto pool = boost::asynchronous::create_shared_scheduler_proxy(
                        new boost::asynchronous::threadpool_scheduler<
                            boost::asynchronous::lockfree_queue<boost::asynchronous::any_serializable> >(threads));
            boost::asynchronous::tcp::simple_tcp_client_proxy proxy(scheduler,pool,server_address,server_port,executor,
                                                                    0/*ms between calls to server*/);
            boost::future<boost::future<void> > fu = proxy.run();
            boost::future<void> fu_end = fu.get();
            fu_end.get();
        }
        else
        {
            // guarded_deque supports queue size
            auto pool = boost::asynchronous::create_shared_scheduler_proxy(
                        new boost::asynchronous::threadpool_scheduler<
                            boost::asynchronous::guarded_deque<boost::asynchronous::any_serializable> >(threads));
            // more advanced policy
            // or simple_tcp_client_proxy<boost::asynchronous::tcp::queue_size_check_policy<>> if your compiler can (clang)
            typename boost::asynchronous::tcp::get_correct_simple_tcp_client_proxy<boost::asynchronous::tcp::queue_size_check_policy<>>::type proxy(
                        scheduler,pool,server_address,server_port,executor,
                        0/*ms between calls to server*/,
                        job_getting_policy /* number of jobs we try to keep in queue */);
            boost::future<boost::future<void> > fu = proxy.run();
            boost::future<void> fu_end = fu.get();
            fu_end.get();
        }
    }

    return 0;
}

