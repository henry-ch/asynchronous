#include <iostream>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_stack.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/continuation_task.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>

using namespace std;

namespace
{
// a simple, single-threaded fibonacci function used for cutoff
long serial_fib( long n ) {
    if( n<2 )
        return n;
    else
        return serial_fib(n-1)+serial_fib(n-2);
}

// our recursive fibonacci tasks. Needs to inherit continuation_task<value type returned by this task>
struct fib_task : public boost::asynchronous::continuation_task<long>
{
    fib_task(long n,long cutoff):n_(n),cutoff_(cutoff){}
    void operator()()const
    {
        // the result of this task, will be either set directly if < cutoff, otherwise when taks is ready
        boost::asynchronous::continuation_result<long> task_res = this_task_result();
        if (n_<cutoff_)
        {
            // n < cutoff => execute ourselves
            task_res.set_value(serial_fib(n_));
        }
        else
        {
            // n> cutoff, create 2 new tasks and when both are done, set our result (res(task1) + res(task2))
            boost::asynchronous::create_callback_continuation(
                        // called when subtasks are done, set our result
                        [task_res](std::tuple<boost::asynchronous::expected<long>,boost::asynchronous::expected<long> > res)
                        {
                            long r = std::get<0>(res).get() + std::get<1>(res).get();
                            task_res.set_value(r);
                        },
                        // recursive tasks
                        fib_task(n_-1,cutoff_),
                        fib_task(n_-2,cutoff_));
        }
    }
    long n_;
    long cutoff_;
};

struct Servant : boost::asynchronous::trackable_servant<>
{
    // optional, ctor is simple enough not to be posted
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler, int threads)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               // threadpool and a simple lockfree_queue
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::multiqueue_threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<>,
                                                           boost::asynchronous::default_find_position< >,
                                                           //boost::asynchronous::default_save_cpu_load<10,80000,5000>
                                                           //boost::asynchronous::default_save_cpu_load<10,80000,1000>
                                                           boost::asynchronous::no_cpu_load_saving
                                                       >>(threads))
        // for testing purpose
        , m_promise(new boost::promise<long>)
    {
    }
    // called when task done, in our thread
    void on_callback(long res)
    {
        // inform test caller
        m_promise->set_value(res);
    }
    // call to this is posted and executes in our (safe) single-thread scheduler
    boost::shared_future<long> calc_fibonacci(long n,long cutoff)
    {
        // for testing purpose
        boost::shared_future<long> fu = m_promise->get_future();
        // start long tasks in threadpool (first lambda) and callback in our thread
        post_callback(
                [n,cutoff]()
                {
                     // a top-level continuation is the first one in a recursive serie.
                     // Its result will be passed to callback
                     return boost::asynchronous::top_level_callback_continuation<long>(fib_task(n,cutoff));
                 }// work
               ,
               // callback with fibonacci result.
               [this](boost::asynchronous::expected<long> res){
                            this->on_callback(res.get());
               }// callback functor.
        ,"",0,0
        );
        return fu;
    }
private:
// for testing
std::shared_ptr<boost::promise<long> > m_promise;
};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s, int threads):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s,threads)
    {}
    // caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER(calc_fibonacci,0)
};

}

void example_fibonacci(long fibo_val,long cutoff, int threads)
{
    typename boost::chrono::high_resolution_clock::time_point start;
    typename boost::chrono::high_resolution_clock::time_point stop;

    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                                boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::lockfree_queue<>,
                                     boost::asynchronous::default_save_cpu_load<10,80000,1000>>>();
        {
            ServantProxy proxy(scheduler,threads);
            start = boost::chrono::high_resolution_clock::now();
            boost::shared_future<boost::shared_future<long> > fu = proxy.calc_fibonacci(fibo_val,cutoff);
            boost::shared_future<long> resfu = fu.get();
            long res = resfu.get();
            stop = boost::chrono::high_resolution_clock::now();
            std::cout << "res= " << res << std::endl;
            std::cout << "example_fibonacci parallel took in us:"
                      <<  (boost::chrono::nanoseconds(stop - start).count() / 1000) <<"\n" <<std::endl;
        }
    }
    std::cout << "end example_fibonacci \n" << std::endl;
}
int main(int argc, char* argv[])
{
  long fib = (argc>2) ? strtol(argv[1],0,0) : 48;
  long cutOff = (argc>2) ? strtol(argv[2],0,0) : 30;
  int threads = (argc>3) ? strtol(argv[3],0,0) : 12;
  std::cout << "fib=" << fib << std::endl;
  std::cout << "cutoff=" << cutOff << std::endl;
  std::cout << "threads=" << threads << std::endl;
  example_fibonacci(fib,cutOff,threads);
  return 0;
}
