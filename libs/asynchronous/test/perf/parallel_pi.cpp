#include <iostream>
#include <vector>
#include <limits>

#include <boost/range/irange.hpp>
#include <boost/asynchronous/callable_any.hpp>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/queue/any_queue_container.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_reduce.hpp>
#include <boost/asynchronous/algorithm/parallel_for.hpp>
#include <boost/asynchronous/algorithm/invoke.hpp>
#include <boost/asynchronous/helpers/lazy_irange.hpp>

#define COUNT 1000000000L
//#define THREAD_COUNT 12
#define TASK_PER_THREAD 200
//#define STEP_SIZE COUNT/THREAD_COUNT/TASK_PER_THREAD


struct pi {
    double operator()(long n) {
        return ((double) ((((int) n) % 2 == 0) ? 1 : -1)) / ((double) (2 * n + 1));
    }
};


struct Servant : boost::asynchronous::trackable_servant<>
{
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler,int threads,int cutoff)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::multiqueue_threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<>,
                                                           boost::asynchronous::default_find_position< >,
                                                           //boost::asynchronous::default_save_cpu_load<10,80000,1000>
                                                           boost::asynchronous::no_cpu_load_saving
                                                           >>(threads))
        , task_number_(0)
        , total_(0.0)
        , cutoff_(cutoff)
    {}

    boost::shared_future<double> calc_pi()
    {
        std::cout << "start calculating PI" << std::endl;
        // we need a promise to inform caller when we're done
        boost::shared_ptr<boost::promise<double> > aPromise(new boost::promise<double>);
        boost::shared_future<double> fu = aPromise->get_future();
        start_ = boost::chrono::high_resolution_clock::now();
        // start long tasks
        auto  cutoff = cutoff_;
        boost::asynchronous::post_callback(
                   get_worker(),
                   [cutoff]()
                   {
                        auto sum = [](double a, double b) { return a + b; };
                        return boost::asynchronous::parallel_reduce(boost::asynchronous::lazy_irange(0L, COUNT, pi()), sum, cutoff,"",0);
                   },// work
                   get_scheduler(),
                   [aPromise,this](boost::asynchronous::expected<double> res){
                        double p = res.get();
                        stop_ = boost::chrono::high_resolution_clock::now();
                        std::cout << "parallel_pi_inner took in us:"
                                  <<  (boost::chrono::nanoseconds(stop_ - start_).count() / 1000) <<"\n" <<std::endl;
                        aPromise->set_value(p*4.0);
                   }// callback functor.
                   ,"",0,0
        );
        return fu;
    }

private:
    typename boost::chrono::high_resolution_clock::time_point start_;
    typename boost::chrono::high_resolution_clock::time_point stop_;
    unsigned task_number_;
    double total_;
    int cutoff_;
};


class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant> {
public:
    template <class Scheduler>
    ServantProxy(Scheduler s,int threads,int cutoff):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s,threads,cutoff)
    {}
    // caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER(calc_pi,0)
};

int main(int argc, char* argv[])
{
    int threads = (argc>1) ? strtol(argv[1],0,0):boost::thread::hardware_concurrency();
    int cutoff = (argc>2) ? strtol(argv[2],0,0) : COUNT/threads/TASK_PER_THREAD;
    std::cout << "cutoff=" << cutoff << std::endl;
    std::cout << "threads=" << threads << std::endl;
    
    std::cout.precision(std::numeric_limits< double >::digits10 +2);
    typename boost::chrono::high_resolution_clock::time_point start;
    typename boost::chrono::high_resolution_clock::time_point stop;
    start = boost::chrono::high_resolution_clock::now();
    double res = 0.0;

    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                            boost::asynchronous::single_thread_scheduler<
                                    boost::asynchronous::lockfree_queue<>,
                                    boost::asynchronous::default_save_cpu_load<10,80000,1000>>>();
    {
        ServantProxy proxy(scheduler,threads,cutoff);
        start = boost::chrono::high_resolution_clock::now();
        boost::shared_future<boost::shared_future<double> > fu = proxy.calc_pi();
        boost::shared_future<double> resfu = fu.get();
        res = resfu.get();
        stop = boost::chrono::high_resolution_clock::now();
        std::cout << "PI = " << res << std::endl;
        std::cout << "parallel_pi took in us:"
                  <<  (boost::chrono::nanoseconds(stop - start).count() / 1000) <<"\n" <<std::endl;
    }
    return 0;
}
