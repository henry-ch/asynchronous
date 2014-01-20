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

#include <boost/thread/future.hpp>
#include <boost/asynchronous/detail/any_interruptible.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>

#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

namespace boost { namespace asynchronous
{

namespace detail {
    
template <class Iterator, class Func, class ReturnType>
ReturnType reduce(Iterator begin, Iterator end, Func fn) {
    ReturnType t = *(begin++);
    for (; begin != end; ++begin) {
        t = fn(t, *begin);
    }
    return t;
}
    
}

// version for moved ranges
namespace detail
{
template <class Range, class Func, class ReturnType, class Job>
struct parallel_reduce_range_move_helper: public boost::asynchronous::continuation_task<ReturnType>
{
    parallel_reduce_range_move_helper(Range&& range,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :range_(std::forward<Range>(range)),func_(std::move(func)),cutoff_(cutoff),task_name_(std::move(task_name)),prio_(prio)
    {}
    void operator()()const
    {
        std::vector<boost::future<ReturnType> > fus;
        boost::asynchronous::any_weak_scheduler<Job> weak_scheduler = boost::asynchronous::get_thread_scheduler<Job>();
        boost::asynchronous::any_shared_scheduler<Job> locked_scheduler = weak_scheduler.lock();
        //TODO return what?
    //    if (!locked_scheduler.is_valid())
    //        // give up
    //        return;
        boost::shared_ptr<Range> range = boost::make_shared<Range>(std::move(range_));
        for (auto it= boost::begin(*range); it != boost::end(*range) ; )
        {
            auto itp = it;
            boost::asynchronous::detail::safe_advance(it,cutoff_,boost::end(*range));
            auto func = func_;
            boost::future<ReturnType> fu = boost::asynchronous::post_future(locked_scheduler,
                                                                      [it,itp,func]()
                                                                      {
                                                                        return reduce<decltype(itp), Func, ReturnType>(itp,it,func);
                                                                      },
                                                                      task_name_,prio_);
            fus.emplace_back(std::move(fu));
        }
        boost::asynchronous::continuation_result<ReturnType> task_res = this->this_task_result();
        auto func = func_;
        boost::asynchronous::create_continuation_log<Job>(
                    // called when subtasks are done, set our result
                    [task_res, func,range](std::vector<boost::future<ReturnType>> res)
                    {
                        try
                        {
                            ReturnType rt;
                            bool set = false;
                            for (typename std::vector<boost::future<ReturnType>>::iterator itr = res.begin();itr != res.end();++itr)
                            {
                                // get values, check that no exception exists
                                if (set)
                                    rt = func(rt, (*itr).get());
                                else {
                                    rt = (*itr).get();
                                    set = true;
                                }
                            }
                            task_res.emplace_value(std::move(rt));
                        }
                        catch(std::exception& e)
                        {
                            task_res.set_exception(boost::copy_exception(e));
                        }
                    },
                    // future results of recursive tasks
                    std::move(fus));
    }
    Range range_;
    Func func_;
    long cutoff_;
    std::string task_name_;
    std::size_t prio_;
};
}
template <class Range, class Func, class Job=boost::asynchronous::any_callable>
auto parallel_reduce(Range&& range,Func func,long cutoff,
             const std::string& task_name="", std::size_t prio=0) -> typename boost::disable_if<has_is_continuation_task<Range>,boost::asynchronous::detail::continuation<decltype(func(*(range.begin()), *(range.end()))),Job> >::type
{
    typedef decltype(func(*(range.begin()), *(range.end()))) ReturnType;
    return boost::asynchronous::top_level_continuation_log<ReturnType,Job>
            (boost::asynchronous::detail::parallel_reduce_range_move_helper<Range,Func,ReturnType,Job>(std::forward<Range>(range),func,cutoff,task_name,prio));
}

// version for ranges held only by reference
namespace detail
{
template <class Range, class Func, class ReturnType, class Job>
struct parallel_reduce_range_helper: public boost::asynchronous::continuation_task<ReturnType>
{
    parallel_reduce_range_helper(Range const& range,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :range_(range),func_(std::move(func)),cutoff_(cutoff),task_name_(std::move(task_name)),prio_(prio)
    {}
    void operator()()const
    {
        std::vector<boost::future<ReturnType> > fus;
        boost::asynchronous::any_weak_scheduler<Job> weak_scheduler = boost::asynchronous::get_thread_scheduler<Job>();
        boost::asynchronous::any_shared_scheduler<Job> locked_scheduler = weak_scheduler.lock();
        //TODO return what?
    //    if (!locked_scheduler.is_valid())
    //        // give up
    //        return;
        for (auto it= boost::begin(range_); it != boost::end(range_) ; )
        {
            auto itp = it;
            boost::asynchronous::detail::safe_advance(it,cutoff_,boost::end(range_));
            auto func = func_;
            boost::future<ReturnType> fu = boost::asynchronous::post_future(locked_scheduler,
                                                                      [it,itp,func]()
                                                                      {
                                                                        return reduce<decltype(itp), Func, ReturnType>(itp,it,func);
                                                                      },
                                                                      task_name_,prio_);
            fus.emplace_back(std::move(fu));
        }
        boost::asynchronous::continuation_result<ReturnType> task_res = this->this_task_result();
        auto func = func_;
        boost::asynchronous::create_continuation_log<Job>(
                    // called when subtasks are done, set our result
                    [task_res, func](std::vector<boost::future<ReturnType>> res)
                    {
                        try
                        {
                            ReturnType rt;
                            bool set = false;
                            for (typename std::vector<boost::future<ReturnType>>::iterator itr = res.begin();itr != res.end();++itr)
                            {
                                // get values, check that no exception exists
                                if (set)
                                    rt = func(rt, (*itr).get());
                                else {
                                    rt = (*itr).get();
                                    set = true;
                                }
                            }
                            task_res.emplace_value(std::move(rt));
                        }
                        catch(std::exception& e)
                        {
                            task_res.set_exception(boost::copy_exception(e));
                        }
                    },
                    // future results of recursive tasks
                    std::move(fus));
    }
    Range const& range_;
    Func func_;
    long cutoff_;
    std::string task_name_;
    std::size_t prio_;
};
}
template <class Range, class Func, class Job=boost::asynchronous::any_callable>
auto parallel_reduce(Range const& range,Func func,long cutoff,
             const std::string& task_name="", std::size_t prio=0) -> typename boost::disable_if<has_is_continuation_task<Range>,boost::asynchronous::detail::continuation<decltype(func(*(range.begin()), *(range.end()))),Job> >::type
{
    typedef decltype(func(*(range.begin()), *(range.end()))) ReturnType;
    return boost::asynchronous::top_level_continuation_log<ReturnType,Job>
            (boost::asynchronous::detail::parallel_reduce_range_helper<Range,Func,ReturnType,Job>(range,func,cutoff,task_name,prio));
}

// version for ranges given as continuation
namespace detail
{
template <class Continuation, class Func, class ReturnType, class Job>
struct parallel_reduce_continuation_range_helper: public boost::asynchronous::continuation_task<ReturnType>
{
    parallel_reduce_continuation_range_helper(Continuation const& c,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :cont_(c),func_(std::move(func)),cutoff_(cutoff),task_name_(std::move(task_name)),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<ReturnType> task_res = this->this_task_result();
        auto func(std::move(func_));
        auto cutoff = cutoff_;
        auto task_name = task_name_;
        auto prio = prio_;
        cont_.on_done([task_res,func,cutoff,task_name,prio](std::tuple<boost::future<typename Continuation::return_type> >&& continuation_res)
        {
            try
            {
                auto new_continuation = boost::asynchronous::parallel_reduce<typename Continuation::return_type, Func, Job>(std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                new_continuation.on_done([task_res](std::tuple<boost::future<ReturnType> >&& new_continuation_res)
                {
                    task_res.emplace_value(std::move(std::get<0>(new_continuation_res).get()));
                });
                boost::asynchronous::any_continuation nac(new_continuation);
                boost::asynchronous::get_continuations().push_front(nac);
            }
            catch(std::exception& e)
            {
                task_res.set_exception(boost::copy_exception(e));
            }
        }
        );
        boost::asynchronous::any_continuation ac(cont_);
        boost::asynchronous::get_continuations().push_front(ac);
    }
    Continuation cont_;
    Func func_;
    long cutoff_;
    std::string task_name_;
    std::size_t prio_;
};
}

#define _VALUE_TYPE typename Range::return_type::value_type
#define _VALUE std::declval<_VALUE_TYPE>()
#define _FUNC_RETURN_TYPE decltype(func(_VALUE, _VALUE))

template <class Range, class Func, class Job=boost::asynchronous::any_callable>
auto parallel_reduce(Range range,Func func,long cutoff,
             const std::string& task_name="", std::size_t prio=0)
  -> typename boost::enable_if<has_is_continuation_task<Range>, boost::asynchronous::detail::continuation<_FUNC_RETURN_TYPE, Job>>::type
{
    typedef _FUNC_RETURN_TYPE ReturnType;
    return boost::asynchronous::top_level_continuation_log<ReturnType,Job>
            (boost::asynchronous::detail::parallel_reduce_continuation_range_helper<Range,Func,ReturnType,Job>(range,func,cutoff,task_name,prio));
}

// version for iterators
namespace detail
{
template <class Iterator, class Func, class ReturnType, class Job>
struct parallel_reduce_helper: public boost::asynchronous::continuation_task<ReturnType>
{
    parallel_reduce_helper(Iterator beg, Iterator end,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : beg_(beg),end_(end),func_(std::move(func)),cutoff_(cutoff),task_name_(std::move(task_name)),prio_(prio)
    {}
    void operator()()const
    {
        std::vector<boost::future<ReturnType> > fus;
        boost::asynchronous::any_weak_scheduler<Job> weak_scheduler = boost::asynchronous::get_thread_scheduler<Job>();
        boost::asynchronous::any_shared_scheduler<Job> locked_scheduler = weak_scheduler.lock();
        //TODO return what?
    //    if (!locked_scheduler.is_valid())
    //        // give up
    //        return;
        for (Iterator it=beg_; it != end_ ; )
        {
            Iterator itp = it;
            boost::asynchronous::detail::safe_advance(it,cutoff_,end_);
            auto func = func_;
            boost::future<ReturnType> fu = boost::asynchronous::post_future(locked_scheduler,
                                                                      [it,itp,func]()
                                                                      {
                                                                        return reduce<decltype(itp), Func, ReturnType>(itp,it,func);
                                                                      },
                                                                      task_name_,prio_);
            fus.emplace_back(std::move(fu));
        }
        boost::asynchronous::continuation_result<ReturnType> task_res = this->this_task_result();
        auto func = func_;
        boost::asynchronous::create_continuation_log<Job>(
                    // called when subtasks are done, set our result
                    [task_res, func](std::vector<boost::future<ReturnType>> res)
                    {
                        try
                        {
                            ReturnType rt;
                            bool set = false;
                            for (typename std::vector<boost::future<ReturnType>>::iterator itr = res.begin();itr != res.end();++itr)
                            {
                                // get values, check that no exception exists
                                if (set)
                                    rt = func(rt, (*itr).get());
                                else {
                                    rt = (*itr).get();
                                    set = true;
                                }
                            }
                            task_res.emplace_value(std::move(rt));
                        }
                        catch(std::exception& e)
                        {
                            task_res.set_exception(boost::copy_exception(e));
                        }
                    },
                    // future results of recursive tasks
                    std::move(fus));
    }
    Iterator beg_;
    Iterator end_;
    Func func_;
    long cutoff_;
    std::string task_name_;
    std::size_t prio_;
};
}
template <class Iterator, class Func, class Job=boost::asynchronous::any_callable>
auto parallel_reduce(Iterator beg, Iterator end,Func func,long cutoff,
             const std::string& task_name="", std::size_t prio=0) -> boost::asynchronous::detail::continuation<decltype(func(std::declval<typename Iterator::value_type>(), std::declval<typename Iterator::value_type>())),Job>
{
    typedef decltype(func(std::declval<typename Iterator::value_type>(), std::declval<typename Iterator::value_type>())) ReturnType;
    return boost::asynchronous::top_level_continuation_log<ReturnType,Job>
            (boost::asynchronous::detail::parallel_reduce_helper<Iterator,Func,ReturnType,Job>(beg,end,func,cutoff,task_name,prio));
}

}}
#endif // BOOST_ASYNCHRON_parallel_reduce_HPP
