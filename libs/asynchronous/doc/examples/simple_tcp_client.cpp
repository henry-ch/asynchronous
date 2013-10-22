#include <iostream>
#include <boost/asynchronous/scheduler/tcp/simple_tcp_client.hpp>
#include <boost/asynchronous/extensions/asio/asio_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>

using namespace std;

int main()
{
    cout << "Starting" << endl;
    auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                            new boost::asynchronous::asio_scheduler<>);
    {
        boost::asynchronous::tcp::simple_tcp_client_proxy proxy(scheduler,"localhost","12345",100/*ms between calls to server*/);
        boost::future<boost::future<void> > fu = proxy.run();
        boost::future<void> fu_end = fu.get();
        fu_end.get();
    }

    return 0;
}

