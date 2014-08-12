#include <iostream>
#include <boost/enable_shared_from_this.hpp>

#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/queue/guarded_deque.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/tcp/tcp_server_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/any_serializable.hpp>
#include <boost/asynchronous/scheduler/composite_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/tcp/simple_tcp_client.hpp>
#include <boost/asynchronous/extensions/asio/asio_scheduler.hpp>
#include <boost/asynchronous/scheduler/serializable_task.hpp>
#include <boost/asynchronous/any_serializable.hpp>

using namespace std;
namespace tcp_example
{
// a simple, single-threaded fibonacci function used for cutoff
template <class T>
long serial_fib( T n ) {
    if( n<2 )
        return n;
    else
        return serial_fib(n-1)+serial_fib(n-2);
}

// our recursive fibonacci tasks. Needs to inherit continuation_task<value type returned by this task>
struct fib_task : public boost::asynchronous::continuation_task<long>
                , public boost::asynchronous::serializable_task
{
    fib_task(long n,long cutoff)
        :  boost::asynchronous::continuation_task<long>()
        , boost::asynchronous::serializable_task("serializable_sub_fib_task")
        ,n_(n),cutoff_(cutoff)
    {
    }
    template <class Archive>
    void save(Archive & ar, const unsigned int /*version*/)const
    {
        ar & n_;
        ar & cutoff_;
    }
    template <class Archive>
    void load(Archive & ar, const unsigned int /*version*/)
    {
        ar & n_;
        ar & cutoff_;
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
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
            boost::asynchronous::create_callback_continuation_job<boost::asynchronous::any_serializable>(
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

struct serializable_fib_task : public boost::asynchronous::serializable_task
{
    serializable_fib_task(long n,long cutoff):boost::asynchronous::serializable_task("serializable_fib_task"),n_(n),cutoff_(cutoff){}
    template <class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/)
    {
        ar & n_;
        ar & cutoff_;
    }
    auto operator()()const
        -> decltype(boost::asynchronous::top_level_callback_continuation_job<long,boost::asynchronous::any_serializable>
                    (tcp_example::fib_task(long(0),long(0))))
    {
        auto cont =  boost::asynchronous::top_level_callback_continuation_job<long,boost::asynchronous::any_serializable>
                (tcp_example::fib_task(n_,cutoff_));
        return cont;

    }
    long n_;
    long cutoff_;
};
}
namespace
{
typename boost::chrono::high_resolution_clock::time_point start;
typename boost::chrono::high_resolution_clock::time_point stop;
struct Servant : boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,boost::asynchronous::any_serializable>
{
    // optional, ctor is simple enough not to be posted
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler,
            boost::asynchronous::any_shared_scheduler_proxy<boost::asynchronous::any_serializable> worker)
        : boost::asynchronous::trackable_servant<boost::asynchronous::any_callable,boost::asynchronous::any_serializable>
          (scheduler,worker)
        // for testing purpose
        , m_promise(new boost::promise<long>)
    {
    }
    // called when task done, in our thread
    void on_callback(long res)
    {
        stop = boost::chrono::high_resolution_clock::now();
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
                    tcp_example::serializable_fib_task(n,cutoff)
               ,
               // callback with fibonacci result.
               [this](boost::asynchronous::expected<long> res){
                            this->on_callback(res.get());
               }// callback functor.
        );
        return fu;
    }
private:
// for testing
boost::shared_ptr<boost::promise<long> > m_promise;
};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s, boost::asynchronous::any_shared_scheduler_proxy<boost::asynchronous::any_serializable> worker):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s,worker)
    {}
    // caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER(calc_fibonacci)
};

}
int main(int argc, char* argv[])
{
  long fibo_val = (argc>2) ? strtol(argv[1],0,0) : 48;
  long cutoff = (argc>2) ? strtol(argv[2],0,0) : 30;
  int threads = (argc>3) ? strtol(argv[3],0,0) : 12;
  std::cout << "fib=" << fibo_val << std::endl;
  std::cout << "cutoff=" << cutoff << std::endl;
  std::cout << "threads=" << threads << std::endl;
  // Compiler problem? these 2 lines && composite add 25% speed. Why???
  std::string const server_address="localhost";
  std::string const server_port="12346";

  std::string const own_server_address="localhost";
  long own_server_port=12345;
  {
      std::cout << "fibonacci parallel TCP 2" << std::endl;
      // create pools
      // we need a pool where the tasks execute
      auto pool = boost::asynchronous::create_shared_scheduler_proxy(
                  new boost::asynchronous::multiqueue_threadpool_scheduler<
                          boost::asynchronous::lockfree_queue<boost::asynchronous::any_serializable> >(threads));
      // we need a server
      // we use a tcp pool using 1 worker
      auto server_pool = boost::asynchronous::create_shared_scheduler_proxy(
                  new boost::asynchronous::threadpool_scheduler<
                          boost::asynchronous::lockfree_queue<> >(1));
      auto tcp_server= boost::asynchronous::create_shared_scheduler_proxy(
                  new boost::asynchronous::tcp_server_scheduler<
                          boost::asynchronous::lockfree_queue<boost::asynchronous::any_serializable>,
                          boost::asynchronous::any_callable,true>
                              (server_pool,own_server_address,(unsigned int)own_server_port));
      // we need a composite for stealing
      auto composite = boost::asynchronous::create_shared_scheduler_proxy
              (new boost::asynchronous::composite_threadpool_scheduler<boost::asynchronous::any_serializable>
                        (pool,tcp_server));

      // a single-threaded world, where Servant will live.
      auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                              new boost::asynchronous::single_thread_scheduler<
                                   boost::asynchronous::lockfree_queue<> >);
      {
          ServantProxy proxy(scheduler,pool);
          start = boost::chrono::high_resolution_clock::now();
          // result of BOOST_ASYNC_FUTURE_MEMBER is a shared_future,
          // so we have a shared_future of a shared_future(result of start_async_work)
          boost::shared_future<boost::shared_future<long> > fu = proxy.calc_fibonacci(fibo_val,cutoff);
          boost::shared_future<long> resfu = fu.get();
          long res = resfu.get();
          std::cout << "res= " << res << std::endl;
          std::cout << "fibonacci parallel TCP 2 took in us:"
                    <<  (boost::chrono::nanoseconds(stop - start).count() / 1000) <<"\n" <<std::endl;
      }
  }
  std::cout << "end example_post_tcp_fib2 \n" << std::endl;
  return 0;
}
