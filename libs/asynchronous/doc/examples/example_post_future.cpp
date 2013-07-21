#include <iostream>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/queue/threadsafe_list.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/post.hpp>
using namespace std;

namespace{
    struct void_task
    {
        void operator()()const
        {
            std::cout << "void_task called" << std::endl;
        }
    };
    struct int_task
    {
        int operator()()const
        {
            std::cout << "int_task called" << std::endl;
            return 42;
        }
    };
}
void example_post_future()
{
    {
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                            new boost::asynchronous::threadpool_scheduler<
                                    boost::asynchronous::threadsafe_list<> >(3));

        boost::shared_future<void> fuv = boost::asynchronous::post_future(scheduler, void_task());
        fuv.get();

        boost::shared_future<int> fui = boost::asynchronous::post_future(scheduler, int_task());
        int res = fui.get();
        std::cout << "future set with res:" << res << std::endl;
    }
}

void example_post_future_lambda()
{
    {
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                            new boost::asynchronous::threadpool_scheduler<
                                    boost::asynchronous::threadsafe_list<> >(3));

        boost::shared_future<void> fuv =
                boost::asynchronous::post_future(scheduler, [](){std::cout << "void lambda" << std::endl;});
        fuv.get();

        boost::shared_future<int> fui =
                boost::asynchronous::post_future(scheduler, [](){std::cout << "int lambda" << std::endl;return 42;});
        int res = fui.get();
        std::cout << "future set with res:" << res << std::endl;
    }
}
