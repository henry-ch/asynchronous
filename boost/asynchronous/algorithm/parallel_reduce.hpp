// Boost.Asynchronous library
//  Copyright (C) Christophe Henry, Tobias Holl 2014
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRON_PARALLEL_REDUCE_HPP
#define BOOST_ASYNCHRON_PARALLEL_REDUCE_HPP

#include <algorithm>
#include <utility>

#include <type_traits>
#include <boost/serialization/vector.hpp>

#include <boost/asynchronous/detail/any_interruptible.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/scheduler/serializable_task.hpp>
#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>
#include <boost/asynchronous/detail/function_traits.hpp>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/iterator_range.hpp>

namespace boost { namespace asynchronous
{

namespace detail {
    
template <class Iterator, class Func, class ReturnType>
ReturnType reduce(Iterator begin, Iterator end, Func fn) {
    // if range is empty, return a default-constructed element
    if (begin == end)
        return ReturnType();
    ReturnType t = *(begin++);
    for (; begin != end; ++begin) {
        t = fn(t, *begin);
    }
    return t;
}

template <class Iterator, class Func, class ReturnType, class Enable=void>
struct reduce_helper
{
    ReturnType operator()(Iterator beg, Iterator end, Func&& func)
    {
        return func(beg,end);
    }
};
template <class Iterator, class Func, class ReturnType>
struct reduce_helper<Iterator,Func,ReturnType,
                     typename std::enable_if<
                        std::is_same<
                           typename std::remove_cv<
                               typename std::remove_reference<
                                    typename boost::asynchronous::function_traits<Func>::template arg_<0>::type>::type>::type,
                           ReturnType>::value
                     >::type>
{
    ReturnType operator()(Iterator beg, Iterator end, Func&& func)
    {
        return boost::asynchronous::detail::reduce<Iterator, Func, ReturnType>(beg,end,std::move(func));
    }
};
    
}

// version for iterators
namespace detail
{
template <class Iterator, class Func, class Func2, class ReturnType, class Job>
struct parallel_reduce_helper: public boost::asynchronous::continuation_task<ReturnType>
{
    parallel_reduce_helper(Iterator beg, Iterator end,Func func,Func2 func2,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<ReturnType>(task_name)
        , beg_(beg),end_(end),func_(std::move(func)),func2_(std::move(func2)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<ReturnType> task_res = this->this_task_result();
        try
        {
            // advance up to cutoff
            Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
            // if not at end, recurse, otherwise execute here
            if (it == end_)
            {
                task_res.set_value(std::move(boost::asynchronous::detail::reduce_helper<Iterator, Func, ReturnType>()(beg_,it,std::move(func_))));
            }
            else
            {
                auto func = func2_;
                boost::asynchronous::create_callback_continuation_job<Job>(
                    // called when subtasks are done, set our result
                    [task_res, func](std::tuple<boost::asynchronous::expected<ReturnType>,boost::asynchronous::expected<ReturnType>> res) mutable
                    {
                        try
                        {
                            //ReturnType r [2] = {std::move(std::get<0>(res).get()),std::move(std::get<1>(res).get())};
                            //std::vector<ReturnType> r = {std::move(std::get<0>(res).get()),std::move(std::get<1>(res).get())};
                            //task_res.set_value(std::move(boost::asynchronous::detail::reduce_helper<Iterator, Func, ReturnType>()(r.begin(),r.end(),std::move(func))));
                            ReturnType rt = std::get<0>(res).get();
                            rt = std::move(func(rt, std::get<1>(res).get()));
                            task_res.set_value(std::move(rt));
                        }
                        catch(...)
                        {
                            task_res.set_exception(std::current_exception());
                        }
                    },
                    // recursive tasks
                    parallel_reduce_helper<Iterator,Func,Func2,ReturnType,Job>(beg_,it,func_,func2_,cutoff_,this->get_name(),prio_),
                    parallel_reduce_helper<Iterator,Func,Func2,ReturnType,Job>(it,end_,func_,func2_,cutoff_,this->get_name(),prio_)
                );
            }
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
        }
    }
    Iterator beg_;
    Iterator end_;
    Func func_;
    Func2 func2_;
    long cutoff_;
    std::size_t prio_;
};
}
template <class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
auto parallel_reduce(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio=0)
#else
                     const std::string& task_name="", std::size_t prio=0)
#endif
-> boost::asynchronous::detail::callback_continuation<decltype(func(std::declval<typename boost::asynchronous::function_traits<Func>::template arg_<0>::type>(),
                                                                    std::declval<typename boost::asynchronous::function_traits<Func>::template arg_<1>::type>())),Job>
{
    typedef decltype(func(std::declval<typename boost::asynchronous::function_traits<Func>::template arg_<0>::type>(),
                          std::declval<typename boost::asynchronous::function_traits<Func>::template arg_<1>::type>())) ReturnType;
    return boost::asynchronous::top_level_callback_continuation_job<ReturnType,Job>
            (boost::asynchronous::detail::parallel_reduce_helper<Iterator,Func,Func,ReturnType,Job>(beg,end,func,func,cutoff,task_name,prio));
}
// reduce with 2 functors
template <class Iterator, class Func, class Func2, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
auto parallel_reduce(Iterator beg, Iterator end,Func func,Func2 func2,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio=0)
#else
                     const std::string& task_name="", std::size_t prio=0)
#endif
-> boost::asynchronous::detail::callback_continuation<decltype(func(std::declval<typename boost::asynchronous::function_traits<Func>::template arg_<0>::type>(),
                                                                    std::declval<typename boost::asynchronous::function_traits<Func>::template arg_<1>::type>())),Job>
{
    typedef decltype(func(std::declval<typename boost::asynchronous::function_traits<Func>::template arg_<0>::type>(),
                          std::declval<typename boost::asynchronous::function_traits<Func>::template arg_<1>::type>())) ReturnType;
    return boost::asynchronous::top_level_callback_continuation_job<ReturnType,Job>
            (boost::asynchronous::detail::parallel_reduce_helper<Iterator,Func,Func2,ReturnType,Job>(beg,end,func,func2,cutoff,task_name,prio));
}
// version for moved ranges
template <class Range, class Func, class Func2,class ReturnType, class Job,class Enable=void>
struct parallel_reduce_range_move_helper: public boost::asynchronous::continuation_task<ReturnType>
{
    template <class Iterator>
    parallel_reduce_range_move_helper(std::shared_ptr<Range> range,Iterator, Iterator,Func func,Func2 func2,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<ReturnType>(task_name)
        ,range_(range),func_(std::move(func)),func2_(std::move(func2)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<ReturnType> task_res = this->this_task_result();
        try
        {
            std::shared_ptr<Range> range = std::move(range_);
            // advance up to cutoff
            auto it = boost::asynchronous::detail::find_cutoff(boost::begin(*range),cutoff_,boost::end(*range));
            // if not at end, recurse, otherwise execute here
            if (it == boost::end(*range))
            {
                task_res.set_value(std::move(boost::asynchronous::detail::reduce_helper<decltype(it), Func, ReturnType>()(boost::begin(*range),it,std::move(func_))));
            }
            else
            {
                auto func = func2_;
                boost::asynchronous::create_callback_continuation_job<Job>(
                    // called when subtasks are done, set our result
                    [task_res, func,range](std::tuple<boost::asynchronous::expected<ReturnType>,boost::asynchronous::expected<ReturnType>> res) mutable
                    {
                        try
                        {
                            ReturnType rt = std::get<0>(res).get();
                            rt = std::move(func(rt, std::get<1>(res).get()));
                            task_res.set_value(std::move(rt));
                        }
                        catch(...)
                        {
                            task_res.set_exception(std::current_exception());
                        }
                    },
                    // recursive tasks
                    boost::asynchronous::detail::parallel_reduce_helper<decltype(boost::begin(*range_)),Func,Func2,ReturnType,Job>(
                        boost::begin(*range),it,func_,func2_,cutoff_,this->get_name(),prio_),
                    boost::asynchronous::detail::parallel_reduce_helper<decltype(boost::begin(*range_)),Func,Func2,ReturnType,Job>(
                        it,boost::end(*range),func_,func2_,cutoff_,this->get_name(),prio_)
                );
            }
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
        }
    }
    std::shared_ptr<Range> range_;
    Func func_;
    Func2 func2_;
    long cutoff_;
    std::size_t prio_;
};

template <class Range, class Func, class Func2, class ReturnType, class Job>
struct parallel_reduce_range_move_helper<Range,Func,Func2,ReturnType,Job,typename std::enable_if<boost::asynchronous::detail::is_serializable<Func>::value >::type>
        : public boost::asynchronous::continuation_task<ReturnType>
        , public boost::asynchronous::serializable_task
{
    //default ctor only when deserialized immediately after
    parallel_reduce_range_move_helper():boost::asynchronous::serializable_task("parallel_reduce_range_move_helper")
    {
    }
    template <class Iterator>
    parallel_reduce_range_move_helper(std::shared_ptr<Range> range,Iterator beg, Iterator end,Func func,Func2 func2,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<ReturnType>(task_name)
        , boost::asynchronous::serializable_task(func.get_task_name())
        , range_(range),func_(std::move(func)),func2_(std::move(func2)),cutoff_(cutoff),task_name_(task_name),prio_(prio), begin_(beg), end_(end)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<ReturnType> task_res = this->this_task_result();
        try
        {
            // advance up to cutoff
            auto it = boost::asynchronous::detail::find_cutoff(begin_,cutoff_,end_);
            // if not at end, recurse, otherwise execute here
            if (it == end_)
            {
                task_res.set_value(std::move(boost::asynchronous::detail::reduce_helper<decltype(it), Func, ReturnType>()(begin_,it,std::move(func_))));
            }
            else
            {
                auto func = func2_;
                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [task_res, func]
                            (std::tuple<boost::asynchronous::expected<ReturnType>,boost::asynchronous::expected<ReturnType> > res) mutable
                            {
                                try
                                {
                                    ReturnType rt = std::get<0>(res).get();
                                    rt = std::move(func(rt, std::get<1>(res).get()));
                                    task_res.set_value(std::move(rt));
                                }
                                catch(...)
                                {
                                    task_res.set_exception(std::current_exception());
                                }
                            },
                            // recursive tasks
                            parallel_reduce_range_move_helper<Range,Func,Func2,ReturnType,Job>(
                                        range_,begin_,it,
                                        func_,func2_,cutoff_,task_name_,prio_),
                            parallel_reduce_range_move_helper<Range,Func,Func2,ReturnType,Job>(
                                        range_,it,end_,
                                        func_,func2_,cutoff_,task_name_,prio_)
                );
            }
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
        }
    }
    template <class Archive>
    void save(Archive & ar, const unsigned int /*version*/)const
    {
        // only part range
        // TODO avoid copying
        auto r = std::move(boost::copy_range< Range>(boost::make_iterator_range(begin_,end_)));
        ar & r;
        ar & func_;
        ar & func2_;
        ar & cutoff_;
        ar & task_name_;
        ar & prio_;
    }
    template <class Archive>
    void load(Archive & ar, const unsigned int /*version*/)
    {
        range_ = std::make_shared<Range>();
        ar & (*range_);
        ar & func_;
        ar & func2_;
        ar & cutoff_;
        ar & task_name_;
        ar & prio_;
        begin_ = boost::begin(*range_);
        end_ = boost::end(*range_);
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
    std::shared_ptr<Range> range_;
    Func func_;
    Func2 func2_;
    long cutoff_;
    std::string task_name_;
    std::size_t prio_;
    decltype(boost::begin(*range_)) begin_;
    decltype(boost::end(*range_)) end_;
};

template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
auto parallel_reduce(Range&& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio=0)
#else
                     const std::string& task_name="", std::size_t prio=0)
#endif
-> typename std::enable_if<!boost::asynchronous::detail::has_is_continuation_task<Range>::value,boost::asynchronous::detail::callback_continuation<decltype(func(*(range.begin()), *(range.end()))),Job> >::type
{
    typedef decltype(func(*(range.begin()), *(range.end()))) ReturnType;
    auto r = std::make_shared<Range>(std::forward<Range>(range));
    auto beg = boost::begin(*r);
    auto end = boost::end(*r);
    return boost::asynchronous::top_level_callback_continuation_job<ReturnType,Job>
            (boost::asynchronous::parallel_reduce_range_move_helper<Range,Func,Func,ReturnType,Job>(
                 r,beg,end,func,func,cutoff,task_name,prio));
}

// reduce with 2 functors
template <class Range, class Func, class Func2, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
auto parallel_reduce(Range&& range,Func func,Func2 func2,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio=0)
#else
                     const std::string& task_name="", std::size_t prio=0)
#endif
-> typename std::enable_if<!boost::asynchronous::detail::has_is_continuation_task<Range>::value,boost::asynchronous::detail::callback_continuation<decltype(func2(*(range.begin()), *(range.end()))),Job> >::type
{
    typedef decltype(func2(*(range.begin()), *(range.end()))) ReturnType;
    auto r = std::make_shared<Range>(std::forward<Range>(range));
    auto beg = boost::begin(*r);
    auto end = boost::end(*r);
    return boost::asynchronous::top_level_callback_continuation_job<ReturnType,Job>
            (boost::asynchronous::parallel_reduce_range_move_helper<Range,Func,Func2,ReturnType,Job>(
                 r,beg,end,func,func2,cutoff,task_name,prio));
}

// version for ranges held only by reference
namespace detail
{
template <class Range, class Func, class Func2, class ReturnType, class Job>
struct parallel_reduce_range_helper: public boost::asynchronous::continuation_task<ReturnType>
{
    parallel_reduce_range_helper(Range const& range,Func func,Func2 func2,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<ReturnType>(task_name)
        ,range_(range),func_(std::move(func)),func2_(std::move(func2)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<ReturnType> task_res = this->this_task_result();
        try
        {
            // advance up to cutoff
            auto it = boost::asynchronous::detail::find_cutoff(boost::begin(range_),cutoff_,boost::end(range_));
            // if not at end, recurse, otherwise execute here
            if (it == boost::end(range_))
            {
                task_res.set_value(std::move(boost::asynchronous::detail::reduce_helper<decltype(it), Func, ReturnType>()(boost::begin(range_),it,std::move(func_))));
            }
            else
            {
                auto func = func2_;
                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [task_res, func]
                            (std::tuple<boost::asynchronous::expected<ReturnType>,boost::asynchronous::expected<ReturnType>> res) mutable
                            {
                                try
                                {
                                    ReturnType rt = std::get<0>(res).get();
                                    rt = std::move(func(rt, std::get<1>(res).get()));
                                    task_res.set_value(std::move(rt));
                                }
                                catch(...)
                                {
                                    task_res.set_exception(std::current_exception());
                                }
                            },
                            // recursive tasks
                            parallel_reduce_helper<decltype(boost::begin(range_)),Func,Func2,ReturnType,Job>
                                (boost::begin(range_),it,func_,func2_,cutoff_,this->get_name(),prio_),
                            parallel_reduce_helper<decltype(boost::begin(range_)),Func,Func2,ReturnType,Job>
                                (it,boost::end(range_),func_,func2_,cutoff_,this->get_name(),prio_)
                );
            }
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
        }
    }
    Range const& range_;
    Func func_;
    Func2 func2_;
    long cutoff_;
    std::size_t prio_;
};
}
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
auto parallel_reduce(Range const& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio=0)
#else
                     const std::string& task_name="", std::size_t prio=0)
#endif
-> typename std::enable_if<!boost::asynchronous::detail::has_is_continuation_task<Range>::value,boost::asynchronous::detail::callback_continuation<decltype(func(*(range.begin()), *(range.end()))),Job> >::type
{
    typedef decltype(func(*(range.begin()), *(range.end()))) ReturnType;
    return boost::asynchronous::top_level_callback_continuation_job<ReturnType,Job>
            (boost::asynchronous::detail::parallel_reduce_range_helper<Range,Func,Func,ReturnType,Job>(range,func,func,cutoff,task_name,prio));
}

// version with 2 functors
template <class Range, class Func, class Func2, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
auto parallel_reduce(Range const& range,Func func,Func2 func2,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio=0)
#else
                     const std::string& task_name="", std::size_t prio=0)
#endif
-> typename std::enable_if<!boost::asynchronous::detail::has_is_continuation_task<Range>::value,boost::asynchronous::detail::callback_continuation<decltype(func2(*(range.begin()), *(range.end()))),Job> >::type
{
    typedef decltype(func2(*(range.begin()), *(range.end()))) ReturnType;
    return boost::asynchronous::top_level_callback_continuation_job<ReturnType,Job>
            (boost::asynchronous::detail::parallel_reduce_range_helper<Range,Func,Func2,ReturnType,Job>(range,func,func2,cutoff,task_name,prio));
}

// version for ranges given as continuation
namespace detail
{
// adapter to non-callback continuations
template <class Continuation, class Func, class Func2, class ReturnType, class Job,class Enable=void>
struct parallel_reduce_continuation_range_helper: public boost::asynchronous::continuation_task<ReturnType>
{
    parallel_reduce_continuation_range_helper(Continuation const& c,Func func,Func2 func2,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<ReturnType>(task_name)
        ,cont_(c),func_(std::move(func)),func2_(std::move(func2)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<ReturnType> task_res = this->this_task_result();
        try
        {
            auto func(std::move(func_));
            auto func2(std::move(func2_));
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            cont_.on_done([task_res,func,func2,cutoff,task_name,prio]
                          (std::tuple<std::future<typename Continuation::return_type> >&& continuation_res)
            {
                try
                {
                    auto new_continuation = boost::asynchronous::parallel_reduce<typename Continuation::return_type, Func, Func2,Job>
                            (std::move(std::get<0>(continuation_res).get()),func,func2,cutoff,task_name,prio);
                    new_continuation.on_done([task_res]
                                             (std::tuple<boost::asynchronous::expected<ReturnType> >&& new_continuation_res) mutable
                    {
                        task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                    });
                }
                catch(...)
                {
                    task_res.set_exception(std::current_exception());
                }
            }
            );
            boost::asynchronous::any_continuation ac(std::move(cont_));
            boost::asynchronous::get_continuations().emplace_front(std::move(ac));
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
        }
    }
    Continuation cont_;
    Func func_;
    Func2 func2_;
    long cutoff_;
    std::size_t prio_;
};
// Continuation is a callback continuation
template <class Continuation, class Func, class Func2, class ReturnType, class Job>
struct parallel_reduce_continuation_range_helper<Continuation,Func,Func2,ReturnType,Job,
                                                 typename std::enable_if< boost::asynchronous::detail::has_is_callback_continuation_task<Continuation>::value >::type>
        : public boost::asynchronous::continuation_task<ReturnType>
{
  parallel_reduce_continuation_range_helper(Continuation const& c,Func func,Func2 func2,long cutoff,
                      const std::string& task_name, std::size_t prio)
      :boost::asynchronous::continuation_task<ReturnType>(task_name)
      ,cont_(c),func_(std::move(func)),func2_(std::move(func2)),cutoff_(cutoff),prio_(prio)
  {}
  void operator()()
  {
      boost::asynchronous::continuation_result<ReturnType> task_res = this->this_task_result();
      try
      {
          auto func(std::move(func_));
          auto func2(std::move(func2_));
          auto cutoff = cutoff_;
          auto task_name = this->get_name();
          auto prio = prio_;
          cont_.on_done([task_res,func,func2,cutoff,task_name,prio]
                        (std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& continuation_res) mutable
          {
              try
              {
                  auto new_continuation = boost::asynchronous::parallel_reduce<typename Continuation::return_type, Func, Func2, Job>
                          (std::move(std::get<0>(continuation_res).get()),func,func2,cutoff,task_name,prio);
                  new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<ReturnType> >&& new_continuation_res)
                  {
                      task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                  });
              }
              catch(...)
              {
                  task_res.set_exception(std::current_exception());
              }
          }
          );
      }
      catch(...)
      {
          task_res.set_exception(std::current_exception());
      }
  }
  Continuation cont_;
  Func func_;
  Func2 func2_;
  long cutoff_;
  std::size_t prio_;
};
}

#define _VALUE_TYPE typename Range::return_type::value_type
#define _VALUE std::declval<_VALUE_TYPE>()
#define _FUNC_RETURN_TYPE decltype(func(_VALUE, _VALUE))
#define _FUNC_RETURN_TYPE2 decltype(func2(_VALUE, _VALUE))

template <class Range, class Func, class Job=typename Range::job_type>
auto parallel_reduce(Range range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio=0)
#else
                     const std::string& task_name="", std::size_t prio=0)
#endif
  -> typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<Range>::value, boost::asynchronous::detail::callback_continuation<_FUNC_RETURN_TYPE, Job>>::type
{
    typedef _FUNC_RETURN_TYPE ReturnType;
    return boost::asynchronous::top_level_callback_continuation_job<ReturnType,Job>
            (boost::asynchronous::detail::parallel_reduce_continuation_range_helper<Range,Func,Func,ReturnType,Job>(range,func,func,cutoff,task_name,prio));
}

// version with 2 functors
template <class Range, class Func, class Func2, class Job=typename Range::job_type>
auto parallel_reduce(Range range,Func func,Func2 func2,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio=0)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
 -> typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<Range>::value, boost::asynchronous::detail::callback_continuation<_FUNC_RETURN_TYPE2, Job>>::type
{
   typedef _FUNC_RETURN_TYPE2 ReturnType;
   return boost::asynchronous::top_level_callback_continuation_job<ReturnType,Job>
           (boost::asynchronous::detail::parallel_reduce_continuation_range_helper<Range,Func,Func2,ReturnType,Job>(range,func,func2,cutoff,task_name,prio));
}
}}
#endif // BOOST_ASYNCHRON_parallel_reduce_HPP
