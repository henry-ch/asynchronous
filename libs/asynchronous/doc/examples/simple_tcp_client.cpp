#include <iostream>
#include <boost/asynchronous/scheduler/tcp/simple_tcp_client.hpp>
#include <boost/asynchronous/extensions/asio/asio_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>

// our app-specific functors
#include <libs/asynchronous/doc/examples/dummy_tcp_task.hpp>

using namespace std;

int main()
{
    cout << "Starting" << endl;
    auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                new boost::asynchronous::asio_scheduler<>(4));
    {
        std::function<void(std::string const&,boost::archive::text_iarchive&, boost::archive::text_oarchive&)> executor=
        [](std::string const& task_name,boost::archive::text_iarchive& in, boost::archive::text_oarchive& out)
        {
            if (task_name=="dummy_tcp_task")
            {
                dummy_tcp_task t(0);
                in >> t;
                // call task
                auto task_res = t();
                // serialize result
                out << task_res;
            }
            // else whatever functor we support
            else
            {
                std::cout << "unknown task! Sorry, can't do" << std::endl;
                throw boost::asynchronous::tcp::transport_exception("unknown task");
            }
        };

        boost::asynchronous::tcp::simple_tcp_client_proxy proxy(scheduler,"localhost","12345",100/*ms between calls to server*/,executor);
        boost::future<boost::future<void> > fu = proxy.run();
        boost::future<void> fu_end = fu.get();
        fu_end.get();
    }

    return 0;
}

