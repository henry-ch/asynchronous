#include <algorithm>
#include <fstream>
#include <iostream>
#include <type_traits>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_for.hpp>
#include <boost/asynchronous/algorithm/parallel_reduce.hpp>

#include <boost/asynchronous/diagnostics/formatter.hpp>
#include <boost/asynchronous/diagnostics/html_formatter.hpp>

#define MOD 1000000009
#define EXPONENT 42

// 2 ** 24 items in the example servant
#define DATA_EXPONENT 24

typedef boost::asynchronous::any_loggable job;

class Servant : public boost::asynchronous::trackable_servant<job, job>
{
public:
    Servant(boost::asynchronous::any_weak_scheduler<job> scheduler, boost::asynchronous::any_shared_scheduler_proxy<job> pool)
        : boost::asynchronous::trackable_servant<job, job>(scheduler, pool)
        , m_promise(new boost::promise<void>)
    {
        m_data = std::vector<long long>(1 << DATA_EXPONENT);
        std::iota(m_data.begin(), m_data.end(), 0);
    }

    void on_callback(long long result)
    {
        m_promise->set_value();
    }

    boost::future<void> start_async_work()
    {
        boost::future<void> fu = m_promise->get_future();

        auto fn = [](long long const& i)
        {
           long long& j = const_cast<long long&>(i);
           long long k = j;
           for (int m = 0; m < EXPONENT; ++m) {
               j = (j * k) % MOD;
           }
        };
        auto reduction = [](long long const& a, long long const& b) {
            return (a + b) % MOD;
        };

        post_callback(
               [this, fn, reduction]() mutable {
                   // Capturing 'this' is evil, but we'll do it anyways, because this is just a test.
                   // Also, if everything works, the servant is guaranteed to stay alive until the callback finishes.
                   // With C++14, we would instead capture [data=std::move(this->m_data), fn=std::move(fn), reduction=std::move(reduction)]
                   auto continuation = boost::asynchronous::parallel_for<decltype(this->m_data),
                                                                         decltype(fn),
                                                                         job>(
                                           std::move(this->m_data),
                                           std::move(fn),
                                           8192,
                                           "parallel_for",
                                           0);
                   return boost::asynchronous::parallel_reduce<decltype(continuation),
                                                               decltype(reduction),
                                                               job>(
                              std::move(continuation),
                              std::move(reduction),
                              8192,
                              "parallel_reduce",
                              0);
               },
               [this](boost::asynchronous::expected<long long> res){
                   this->on_callback(res.get());
               },
               "post_callback",
               0,
               0
        );
        return fu;
    }
private:
    boost::shared_ptr<boost::promise<void> > m_promise;
    std::vector<long long> m_data;
};


class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy, Servant, job, 20000>
{
public:
    template <class Scheduler, class Pool>
    ServantProxy(Scheduler s, Pool p):
        boost::asynchronous::servant_proxy<ServantProxy, Servant, job, 20000>(s, p)
    {}

    BOOST_ASYNC_FUTURE_MEMBER_LOG(start_async_work, "start_async_work", 0);
    BOOST_ASYNC_SERVANT_POST_CTOR_LOG("ctor", 0);
};


void test_html_diagnostics(int argc, char *argv[])
{

    if (argc != 4) {
        std::cerr << (argc - 1) << " arguments given, should be 3." << std::endl;
        std::cerr << "Usage:" << std::endl << "    " << argv[0] << " <final.html> <in_progress.html> <wait ms>" << std::endl;
        return;
    }

    // Create schedulers
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                                boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::lockfree_queue<job>>>(std::string("<Scheduler>"));
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<
                                boost::asynchronous::threadpool_scheduler<
                                     boost::asynchronous::lockfree_queue<job>>>(6, std::string("Threadpool"), 12);

    auto formatter_scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                                boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::lockfree_queue<job>>>(std::string("Formatter scheduler"));


    typedef boost::asynchronous::html_formatter::formatter<> html_formatter_t;

    boost::asynchronous::formatter_proxy<html_formatter_t> formatter(formatter_scheduler,
                                                                     pool,
                                                                     boost::asynchronous::make_scheduler_interfaces(scheduler, pool, formatter_scheduler));

    {
        // Create proxy
        ServantProxy proxy(scheduler, pool);
        boost::future<boost::future<void> > fu = proxy.start_async_work();

        // Sleep
        boost::this_thread::sleep_for(boost::chrono::milliseconds(std::atoi(argv[3])));

        // Output intermediate statistics

        std::ofstream(argv[2]) << formatter.format().get() << std::endl;

        // Clear schedulers
        formatter.clear_schedulers();

        // Wait for the task to finish
        boost::future<void> resfu = fu.get();
        resfu.get();
    }

    // Output final statistics

    std::ofstream(argv[1]) << formatter.format().get() << std::endl;}
