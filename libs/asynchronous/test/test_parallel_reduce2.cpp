//#define BOOST_TEST_MODULE parallel_reduce_policy

#ifdef INCLUDE_UNIT_TEST_HEADERS
#include <boost/test/included/unit_test.hpp>
#else
#include <boost/test/unit_test.hpp>
#endif

#include <iterator>
#include <boost/enable_shared_from_this.hpp>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_reduce.hpp>

#include <functional>
#include <iostream>
#include <vector>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/mpl/vector.hpp>

#ifndef XXX_USE_THREADPOOL
#define XXX_USE_THREADPOOL 1
#endif	// XXX_USE_THREADPOOL

using test_types = boost::mpl::vector<int,long,float,double,long double>;

class parallel_reduce_policy_base
{
protected:
  static auto& scheduler ()
  {
    static auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
      boost::asynchronous::single_thread_scheduler<
    boost::asynchronous::lockfree_queue<>>>();
    return scheduler;
  }

  static auto& threadpool ()
  {
#if XXX_USE_THREADPOOL
    static auto threadpool = boost::asynchronous::make_shared_scheduler_proxy<
      boost::asynchronous::threadpool_scheduler<
    boost::asynchronous::lockfree_queue<>>>(4);
#else  // XXX_USE_THREADPOOL
    static auto threadpool = scheduler();
#endif // XXX_USE_THREADPOOL
    return threadpool;
  }
};

template < typename T >
class parallel_reduce_policy
  : parallel_reduce_policy_base
{
  template < typename Iterator
         , typename Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB
         , typename WJob = BOOST_ASYNCHRONOUS_DEFAULT_JOB
         >
  struct Servant : boost::asynchronous::trackable_servant<Job,WJob>
  {
    using base = boost::asynchronous::trackable_servant<Job,WJob>;

    using value_type = T;
    using promise_type = boost::promise<value_type>;
    using future_type = boost::shared_future<value_type>;
    using future_future_type = boost::shared_future<future_type>;
    using expected_type = boost::asynchronous::expected<value_type>;

    Servant (boost::asynchronous::any_weak_scheduler<Job> scheduler,
         boost::asynchronous::any_shared_scheduler_proxy<WJob> threadpool)
      : base(scheduler,threadpool)
      , m_promise_(new promise_type)
    {}

    Servant (boost::asynchronous::any_weak_scheduler<Job> scheduler,
         boost::asynchronous::any_shared_scheduler_proxy<WJob> threadpool,
         unsigned cutoff)
      : boost::asynchronous::trackable_servant<Job,WJob>(scheduler,threadpool)
      , m_promise_(new promise_type)
      , cutoff_(cutoff)
    {}

    future_type start_async_work(Iterator begin, Iterator end)
    {
      future_type future = m_promise_->get_future();
      auto cutoff = cutoff_;
      this->post_callback
    ([begin,end,cutoff]()
     {
       return boost::asynchronous::parallel_reduce
         (begin,end,
          [](T const& a, T const& b){ return a+b; },
          cutoff);
     },
     [this](expected_type expected)
     {
       this->m_promise_->set_value(expected.get());
     });
      return future;
    }

  private:
    boost::shared_ptr<promise_type> m_promise_;
    unsigned cutoff_ = 1024;
  };

  template < typename Iterator
         , typename Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB
         , typename WJob = BOOST_ASYNCHRONOUS_DEFAULT_JOB
         >
  class ServantProxy
    : public boost::asynchronous::servant_proxy< ServantProxy<Iterator,Job,WJob>
                         , Servant<Iterator,Job,WJob>
                         , Job
                         >
  {
    using self = ServantProxy;
    using base = boost::asynchronous::servant_proxy<self,Servant<Iterator,Job,WJob>,Job>;
    using servant_type = typename base::servant_type;

  public:
    using future_future_type = typename servant_type::future_future_type;

  public:
    template < typename Scheduler, typename ThreadPool >
    ServantProxy(Scheduler scheduler, ThreadPool threadpool)
      : base(scheduler,threadpool)
    {}
    template < typename Scheduler, typename ThreadPool >
    ServantProxy(Scheduler scheduler, ThreadPool threadpool, unsigned cutoff)
      : base(scheduler,threadpool,cutoff)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work) // caller will get a future
  };

public:
  template < typename Iterator >
  static auto apply (Iterator begin, Iterator end)
  {
    auto cutoff = 1;
    ServantProxy<Iterator> proxy (scheduler(),threadpool(),cutoff);
    typename decltype(proxy)::future_future_type future_future = proxy.start_async_work(begin,end);
    auto future = future_future.get();
    auto result = future.get();
    return result;
  }
};

template < typename T >
struct F
{
  auto operator () () const { return T(1); }
};

struct transform
{
  template < typename F >
  auto operator () (F const& f) const { return f(); }
};

BOOST_AUTO_TEST_CASE_TEMPLATE(test_parallel_reduce2,T,test_types)
{
  using function_type = std::function<T()>;
  using functions_type = std::vector<function_type>;

  using boost::make_transform_iterator;

  int n = 3000;			// XXX - completes correctly for n <= 137
  std::cout << "===> test_parallel_reduce: " << n << std::endl;
  functions_type functions;
  for (int i = 0; i < n; ++i)
    functions.push_back(F<T>());

  BOOST_CHECK_EQUAL
    (parallel_reduce_policy<T>::apply
     (make_transform_iterator<transform>(functions.begin()),
      make_transform_iterator<transform>(functions.end())),
     n);
}

