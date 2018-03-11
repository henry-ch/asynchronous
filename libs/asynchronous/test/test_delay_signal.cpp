//#define BOOST_TEST_MODULE parallel_for_each

#ifdef INCLUDE_UNIT_TEST_HEADERS
#include <boost/test/included/unit_test.hpp>
#else
#include <boost/test/unit_test.hpp>
#endif

#define BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS 1

#include <algorithm>
#include <iterator>
#include <thread>
#include <type_traits>
#include <boost/enable_shared_from_this.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/random/mersenne_twister.hpp>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_for_each.hpp>
#include <boost/asynchronous/diagnostics/any_loggable.hpp>
#include <boost/asynchronous/diagnostics/diagnostic_item.hpp>

#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <boost/mpl/vector.hpp>

template < typename Job >
auto& make_scheduler ()
{
  static auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
    boost::asynchronous::single_thread_scheduler<
      boost::asynchronous::lockfree_queue<Job>,boost::asynchronous::no_cpu_load_saving>>();
  return scheduler;
}

template < typename Job >
auto& make_threadpool ()
{
  using queue = boost::asynchronous::lockfree_queue<Job>;
  using find_position = boost::asynchronous::default_find_position<>;
  using cpu_load = boost::asynchronous::no_cpu_load_saving;
  // using cpu_load = boost::asynchronous::default_save_cpu_load<>;
  using threadpool_type
    = boost::asynchronous::multiqueue_threadpool_scheduler
    <queue,find_position,cpu_load>;

  using boost::asynchronous::make_shared_scheduler_proxy;

  auto threads = std::thread::hardware_concurrency()-1;
  auto queue_size = 1024;
  static auto threadpool = make_shared_scheduler_proxy<threadpool_type>(threads,queue_size);
  return threadpool;
}

template < typename T >
class parallel_for_each
{
  using diagnostic_item = boost::asynchronous::diagnostic_item;
  using diagnostics_type = std::map<std::string,std::list<boost::asynchronous::diagnostic_item>>;

  template < typename SlotIterator
         , typename Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB
         , typename WJob = BOOST_ASYNCHRONOUS_DEFAULT_JOB
         >
  struct Servant : boost::asynchronous::trackable_servant<Job,WJob>
  {
    using base = boost::asynchronous::trackable_servant<Job,WJob>;

    using value_type = T;

    class slot_combiner_type
    {
      using self = slot_combiner_type;
      using slot_type = std::function<T()>;

    public:
      auto value () const { return value_; }
      void merge (self const& other) { value_ = value_ ^ other.value_; }
      void operator () (slot_type const& slot) { value_ = value_ ^ slot(); }

    private:
      value_type value_ = 0;
    };

    using promise_type = boost::promise<value_type>;
    using future_type = boost::shared_future<value_type>;
    using future_future_type = boost::shared_future<future_type>;
    using expected_type = boost::asynchronous::expected<slot_combiner_type>;

    Servant (boost::asynchronous::any_weak_scheduler<Job> scheduler,
         boost::asynchronous::any_shared_scheduler_proxy<WJob> threadpool,
         unsigned cutoff)
      : boost::asynchronous::trackable_servant<Job,WJob>(scheduler,threadpool)
      , cutoff_(cutoff)
    {}

    auto start_async_work (SlotIterator begin, SlotIterator end)
    {
      future_type future = promise_->get_future();
      this->post_callback
    ([begin,end,cutoff=cutoff_]()
     {
       return boost::asynchronous::parallel_for_each
         (begin,end,slot_combiner_type(),cutoff,"worker task");
     },
     [this](expected_type expected)
     {
       this->promise_->set_value(expected.get().value());
     },
     "post_callback",
     0,0);
      return future;
    }

    diagnostics_type get_diagnostics() const
    {
      return this->get_worker().get_diagnostics().totals();
    }

  private:
    static auto initialize_promise () { return std::make_shared<promise_type>(); }

  private:
    unsigned cutoff_;
    std::shared_ptr<promise_type> promise_ = initialize_promise();
  };

  template < typename SlotIterator
         , typename Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB
         , typename WJob = BOOST_ASYNCHRONOUS_DEFAULT_JOB
         >
  class ServantProxy
    : public boost::asynchronous::servant_proxy< ServantProxy<SlotIterator,Job,WJob>
                         , Servant<SlotIterator,Job,WJob>
                         , Job
                         >
  {
    using self = ServantProxy;
    using base = boost::asynchronous::servant_proxy<self,Servant<SlotIterator,Job,WJob>,Job>;
    using servant_type = typename base::servant_type;

  public:
    using future_future_type = typename servant_type::future_future_type;

  public:
    template < typename Scheduler, typename ThreadPool >
    ServantProxy (Scheduler scheduler, ThreadPool threadpool, unsigned cutoff)
      : base(scheduler,threadpool,cutoff)
    {}

    BOOST_ASYNC_SERVANT_POST_CTOR_LOG("servant ctor",0);
    BOOST_ASYNC_SERVANT_POST_DTOR_LOG("servant dtor",0);
    BOOST_ASYNC_FUTURE_MEMBER_LOG(start_async_work,"servant start",0) // caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER_LOG(get_diagnostics,"get_diagnostics",0) // caller will get a future
  };

public:
  template < typename SlotIterator >
  static auto apply (SlotIterator begin, SlotIterator end)
  {
    using servant_job = boost::asynchronous::any_loggable;
    using servant_proxy_type = ServantProxy<SlotIterator,servant_job,servant_job>;

    using future_future_type = typename servant_proxy_type::future_future_type;

    auto& scheduler = make_scheduler<servant_job>();
    auto& threadpool = make_threadpool<servant_job>();
    int cutoff = 1024;
    servant_proxy_type proxy (scheduler,threadpool,cutoff);

    future_future_type future_future = proxy.start_async_work(begin,end);
    auto future = future_future.get();
    auto result = future.get();

    boost::filesystem::ofstream ostream ("diagnostics.dat");
    ostream << "name\ttype\tposted\tstarted\tfinished\twaited\tlasted" << std::endl;
    write_diagnostics_dataframe(ostream,"scheduler",scheduler.get_diagnostics().totals());
    write_diagnostics_dataframe(ostream,"threadpool",threadpool.get_diagnostics().totals());

    return result;
  }

private:
  template < typename Diagnostics >
  static void write_diagnostics_dataframe (std::ostream& os, std::string name, Diagnostics diagnostics)
  {
    constexpr char tab = '\t';
    for (auto mit = diagnostics.begin(); mit != diagnostics.end() ; ++mit)
      {
    for (auto jit = mit->second.begin(); jit != mit->second.end(); ++jit)
      {
        os
          << name << tab
          << mit->first << tab
          << boost::chrono::nanoseconds(jit->get_posted_time().time_since_epoch()).count() / 1000 << tab
          << boost::chrono::nanoseconds(jit->get_started_time().time_since_epoch()).count() / 1000 << tab
          << boost::chrono::nanoseconds(jit->get_finished_time().time_since_epoch()).count() / 1000 << tab
          << boost::chrono::nanoseconds(jit->get_started_time() - jit->get_posted_time()).count() / 1000 << tab
          << boost::chrono::nanoseconds(jit->get_finished_time() - jit->get_started_time()).count() / 1000
          << std::endl;
      }
      }
  }
};

template < typename T >
struct Slot
{
  T operator () () const
  {
    using mt19937 = boost::random::mt19937;
    using result_type = typename mt19937::result_type;

    constexpr long n = 1;
    mt19937 generator;
    result_type result = 0;
    for (long i = 0; i < n; ++i)
      result = result ^ generator();
    return result;
  }
};

using test_types = boost::mpl::vector<unsigned long>;

BOOST_AUTO_TEST_CASE_TEMPLATE(test_parallel_for_each,T,test_types)
{
  using slot_type = std::function<T()>;
  using signal_type = std::vector<slot_type>;

  // XXX - register slot functions with a signal
  int n = 8000;
  signal_type signal;
  for (int i = 0; i < n; ++i)
    signal.push_back(Slot<T>());

  int t = 2;
  int interval = 1;
  for (int i = 0; i <= t; ++i)
    {
      if (i % interval == 0)
    std::cout << "===> iteration " << i << std::endl;
      BOOST_CHECK_EQUAL(parallel_for_each<T>::apply(signal.begin(),signal.end()),0);
    }
  std::cout << std::endl;
}
