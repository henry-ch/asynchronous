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

#include <boost/utility/enable_if.hpp>
#include <boost/serialization/vector.hpp>

#include <boost/asynchronous/detail/any_interruptible.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/scheduler/serializable_task.hpp>
#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>

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
    
}

// version for iterators
namespace detail
{
template <class Iterator, class Func, class ReturnType, class Job>
struct parallel_reduce_helper: public boost::asynchronous::continuation_task<ReturnType>
{
    parallel_reduce_helper(Iterator beg, Iterator end,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<ReturnType>(task_name)
        , beg_(beg),end_(end),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()const
    {
        boost::asynchronous::continuation_result<ReturnType> task_res = this->this_task_result();
        // advance up to cutoff
        Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
        // if not at end, recurse, otherwise execute here
        if (it == end_)
        {
            task_res.set_value(std::move(reduce<Iterator, Func, ReturnType>(beg_,it,std::move(func_))));
        }
        else
        {
            auto func = func_;
            boost::asynchronous::create_callback_continuation_job<Job>(
                // called when subtasks are done, set our result
                [task_res, func](std::tuple<boost::asynchronous::expected<ReturnType>,boost::asynchronous::expected<ReturnType>> res)
                {
                    try
                    {
                        ReturnType rt = std::get<0>(res).get();
                        rt = std::move(func(rt, std::get<1>(res).get()));
                        task_res.set_value(std::move(rt));
                    }
                    catch(std::exception& e)
                    {
                        task_res.set_exception(boost::copy_exception(e));
                    }
                },
                // recursive tasks
                parallel_reduce_helper<Iterator,Func,ReturnType,Job>(beg_,it,func_,cutoff_,this->get_name(),prio_),
                parallel_reduce_helper<Iterator,Func,ReturnType,Job>(it,end_,func_,cutoff_,this->get_name(),prio_)
            );
        }
    }
    Iterator beg_;
    Iterator end_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
}
template <class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
auto parallel_reduce(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio)
#else
                     const std::string& task_name="", std::size_t prio=0)
#endif
-> boost::asynchronous::detail::callback_continuation<decltype(func(std::declval<typename Iterator::value_type>(), std::declval<typename Iterator::value_type>())),Job>
{
    typedef decltype(func(std::declval<typename Iterator::value_type>(), std::declval<typename Iterator::value_type>())) ReturnType;
    return boost::asynchronous::top_level_callback_continuation_job<ReturnType,Job>
            (boost::asynchronous::detail::parallel_reduce_helper<Iterator,Func,ReturnType,Job>(beg,end,func,cutoff,task_name,prio));
}

// version for moved ranges
template <class Range, class Func, class ReturnType, class Job,class Enable=void>
struct parallel_reduce_range_move_helper: public boost::asynchronous::continuation_task<ReturnType>
{
    template <class Iterator>
    parallel_reduce_range_move_helper(boost::shared_ptr<Range> range,Iterator, Iterator,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<ReturnType>(task_name)
        ,range_(range),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()const
    {
        boost::shared_ptr<Range> range = std::move(range_);
        boost::asynchronous::continuation_result<ReturnType> task_res = this->this_task_result();
        // advance up to cutoff
        auto it = boost::asynchronous::detail::find_cutoff(boost::begin(*range),cutoff_,boost::end(*range));
        // if not at end, recurse, otherwise execute here
        if (it == boost::end(*range))
        {
            task_res.set_value(std::move(boost::asynchronous::detail::reduce<decltype(it), Func, ReturnType>
                                                                        (boost::begin(*range),it,std::move(func_))));
        }
        else
        {
            auto func = func_;
            boost::asynchronous::create_callback_continuation_job<Job>(
                // called when subtasks are done, set our result
                [task_res, func,range](std::tuple<boost::asynchronous::expected<ReturnType>,boost::asynchronous::expected<ReturnType>> res)
                {
                    try
                    {
                        ReturnType rt = std::get<0>(res).get();
                        rt = std::move(func(rt, std::get<1>(res).get()));
                        task_res.set_value(std::move(rt));
                    }
                    catch(std::exception& e)
                    {
                        task_res.set_exception(boost::copy_exception(e));
                    }
                },
                // recursive tasks
                boost::asynchronous::detail::parallel_reduce_helper<decltype(boost::begin(*range_)),Func,ReturnType,Job>(
                    boost::begin(*range),it,func_,cutoff_,this->get_name(),prio_),
                boost::asynchronous::detail::parallel_reduce_helper<decltype(boost::begin(*range_)),Func,ReturnType,Job>(
                    it,boost::end(*range),func_,cutoff_,this->get_name(),prio_)
            );
        }
    }
    boost::shared_ptr<Range> range_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};

template <class Range, class Func, class ReturnType, class Job>
struct parallel_reduce_range_move_helper<Range,Func,ReturnType,Job,typename ::boost::enable_if<boost::asynchronous::detail::is_serializable<Func> >::type>
        : public boost::asynchronous::continuation_task<ReturnType>
        , public boost::asynchronous::serializable_task
{
    //default ctor only when deserialized immediately after
    parallel_reduce_range_move_helper():boost::asynchronous::serializable_task("parallel_reduce_range_move_helper")
    {
    }
    template <class Iterator>
    parallel_reduce_range_move_helper(boost::shared_ptr<Range> range,Iterator beg, Iterator end,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<ReturnType>(task_name)
        , boost::asynchronous::serializable_task(func.get_task_name())
        , range_(range),func_(std::move(func)),cutoff_(cutoff),task_name_(task_name),prio_(prio), begin_(beg), end_(end)
    {}
    void operator()()const
    {
        boost::asynchronous::continuation_result<ReturnType> task_res = this->this_task_result();
        // advance up to cutoff
        auto it = boost::asynchronous::detail::find_cutoff(begin_,cutoff_,end_);
        // if not at end, recurse, otherwise execute here
        if (it == end_)
        {
            task_res.set_value(std::move(boost::asynchronous::detail::reduce<decltype(it), Func, ReturnType>
                                                                            (begin_,it,std::move(func_))));
        }
        else
        {
            auto func = func_;
            boost::asynchronous::create_callback_continuation_job<Job>(
                        // called when subtasks are done, set our result
                        [task_res, func](std::tuple<boost::asynchronous::expected<ReturnType>,boost::asynchronous::expected<ReturnType> > res)
                        {
                            try
                            {
                                ReturnType rt = std::get<0>(res).get();
                                rt = std::move(func(rt, std::get<1>(res).get()));
                                task_res.set_value(std::move(rt));
                            }
                            catch(std::exception& e)
                            {
                                task_res.set_exception(boost::copy_exception(e));
                            }
                        },
                        // recursive tasks
                        parallel_reduce_range_move_helper<Range,Func,ReturnType,Job>(
                                    range_,begin_,it,
                                    func_,cutoff_,task_name_,prio_),
                        parallel_reduce_range_move_helper<Range,Func,ReturnType,Job>(
                                    range_,it,end_,
                                    func_,cutoff_,task_name_,prio_)
            );
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
        ar & cutoff_;
        ar & task_name_;
        ar & prio_;
    }
    template <class Archive>
    void load(Archive & ar, const unsigned int /*version*/)
    {
        range_ = boost::make_shared<Range>();
        ar & (*range_);
        ar & func_;
        ar & cutoff_;
        ar & task_name_;
        ar & prio_;
        begin_ = boost::begin(*range_);
        end_ = boost::end(*range_);
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
    boost::shared_ptr<Range> range_;
    Func func_;
    long cutoff_;
    std::string task_name_;
    std::size_t prio_;
    decltype(boost::begin(*range_)) begin_;
    decltype(boost::end(*range_)) end_;
};

template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
auto parallel_reduce(Range&& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio)
#else
                     const std::string& task_name="", std::size_t prio=0)
#endif
-> typename boost::disable_if<has_is_continuation_task<Range>,boost::asynchronous::detail::callback_continuation<decltype(func(*(range.begin()), *(range.end()))),Job> >::type
{
    typedef decltype(func(*(range.begin()), *(range.end()))) ReturnType;
    auto r = boost::make_shared<Range>(std::forward<Range>(range));
    auto beg = boost::begin(*r);
    auto end = boost::end(*r);
    return boost::asynchronous::top_level_callback_continuation_job<ReturnType,Job>
            (boost::asynchronous::parallel_reduce_range_move_helper<Range,Func,ReturnType,Job>(
                 r,beg,end,func,cutoff,task_name,prio));
}

// version for ranges held only by reference
namespace detail
{
template <class Range, class Func, class ReturnType, class Job>
struct parallel_reduce_range_helper: public boost::asynchronous::continuation_task<ReturnType>
{
    parallel_reduce_range_helper(Range const& range,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<ReturnType>(task_name)
        ,range_(range),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()const
    {
        boost::asynchronous::continuation_result<ReturnType> task_res = this->this_task_result();
        // advance up to cutoff
        auto it = boost::asynchronous::detail::find_cutoff(boost::begin(range_),cutoff_,boost::end(range_));
        // if not at end, recurse, otherwise execute here
        if (it == boost::end(range_))
        {
            task_res.set_value(std::move(reduce<decltype(it), Func, ReturnType>(boost::begin(range_),it,std::move(func_))));
        }
        else
        {
            auto func = func_;
            boost::asynchronous::create_callback_continuation_job<Job>(
                        // called when subtasks are done, set our result
                        [task_res, func](std::tuple<boost::asynchronous::expected<ReturnType>,boost::asynchronous::expected<ReturnType>> res)
                        {
                            try
                            {
                                ReturnType rt = std::get<0>(res).get();
                                rt = std::move(func(rt, std::get<1>(res).get()));
                                task_res.set_value(std::move(rt));
                            }
                            catch(std::exception& e)
                            {
                                task_res.set_exception(boost::copy_exception(e));
                            }
                        },
                        // recursive tasks
                        parallel_reduce_helper<decltype(boost::begin(range_)),Func,ReturnType,Job>(boost::begin(range_),it,func_,cutoff_,this->get_name(),prio_),
                        parallel_reduce_helper<decltype(boost::begin(range_)),Func,ReturnType,Job>(it,boost::end(range_),func_,cutoff_,this->get_name(),prio_)
            );
        }
    }
    Range const& range_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
}
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
auto parallel_reduce(Range const& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio)
#else
                     const std::string& task_name="", std::size_t prio=0)
#endif
-> typename boost::disable_if<has_is_continuation_task<Range>,boost::asynchronous::detail::callback_continuation<decltype(func(*(range.begin()), *(range.end()))),Job> >::type
{
    typedef decltype(func(*(range.begin()), *(range.end()))) ReturnType;
    return boost::asynchronous::top_level_callback_continuation_job<ReturnType,Job>
            (boost::asynchronous::detail::parallel_reduce_range_helper<Range,Func,ReturnType,Job>(range,func,cutoff,task_name,prio));
}

// version for ranges given as continuation
namespace detail
{
// adapter to non-callback continuations
template <class Continuation, class Func, class ReturnType, class Job,class Enable=void>
struct parallel_reduce_continuation_range_helper: public boost::asynchronous::continuation_task<ReturnType>
{
    parallel_reduce_continuation_range_helper(Continuation const& c,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<ReturnType>(task_name)
        ,cont_(c),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<ReturnType> task_res = this->this_task_result();
        auto func(std::move(func_));
        auto cutoff = cutoff_;
        auto task_name = this->get_name();
        auto prio = prio_;
        cont_.on_done([task_res,func,cutoff,task_name,prio](std::tuple<boost::future<typename Continuation::return_type> >&& continuation_res)
        {
            try
            {
                auto new_continuation = boost::asynchronous::parallel_reduce<typename Continuation::return_type, Func, Job>(std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<ReturnType> >&& new_continuation_res)
                {
                    task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                });
            }
            catch(std::exception& e)
            {
                task_res.set_exception(boost::copy_exception(e));
            }
        }
        );
        boost::asynchronous::any_continuation ac(std::move(cont_));
        boost::asynchronous::get_continuations().emplace_front(std::move(ac));
    }
    Continuation cont_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
// Continuation is a callback continuation
template <class Continuation, class Func, class ReturnType, class Job>
struct parallel_reduce_continuation_range_helper<Continuation,Func,ReturnType,Job,typename ::boost::enable_if< has_is_callback_continuation_task<Continuation> >::type>
        : public boost::asynchronous::continuation_task<ReturnType>
{
  parallel_reduce_continuation_range_helper(Continuation const& c,Func func,long cutoff,
                      const std::string& task_name, std::size_t prio)
      :boost::asynchronous::continuation_task<ReturnType>(task_name)
      ,cont_(c),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
  {}
  void operator()()
  {
      boost::asynchronous::continuation_result<ReturnType> task_res = this->this_task_result();
      auto func(std::move(func_));
      auto cutoff = cutoff_;
      auto task_name = this->get_name();
      auto prio = prio_;
      cont_.on_done([task_res,func,cutoff,task_name,prio](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& continuation_res)
      {
          try
          {
              auto new_continuation = boost::asynchronous::parallel_reduce<typename Continuation::return_type, Func, Job>(std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
              new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<ReturnType> >&& new_continuation_res)
              {
                  task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
              });
          }
          catch(std::exception& e)
          {
              task_res.set_exception(boost::copy_exception(e));
          }
      }
      );
  }
  Continuation cont_;
  Func func_;
  long cutoff_;
  std::size_t prio_;
};
}

#define _VALUE_TYPE typename Range::return_type::value_type
#define _VALUE std::declval<_VALUE_TYPE>()
#define _FUNC_RETURN_TYPE decltype(func(_VALUE, _VALUE))

template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
auto parallel_reduce(Range range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio)
#else
                     const std::string& task_name="", std::size_t prio=0)
#endif
  -> typename boost::enable_if<has_is_continuation_task<Range>, boost::asynchronous::detail::callback_continuation<_FUNC_RETURN_TYPE, Job>>::type
{
    typedef _FUNC_RETURN_TYPE ReturnType;
    return boost::asynchronous::top_level_callback_continuation_job<ReturnType,Job>
            (boost::asynchronous::detail::parallel_reduce_continuation_range_helper<Range,Func,ReturnType,Job>(range,func,cutoff,task_name,prio));
}

}}
#endif // BOOST_ASYNCHRON_parallel_reduce_HPP
