// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRON_PARALLEL_FOR_HPP
#define BOOST_ASYNCHRON_PARALLEL_FOR_HPP

#include <algorithm>
#include <vector>
#include <iterator> // for std::iterator_traits

#include <boost/thread/future.hpp>
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
#include <boost/range/algorithm_ext/push_back.hpp>

namespace boost { namespace asynchronous
{


// version for moved ranges => will return the range as continuation
namespace detail
{
template <class Range, class Func, class Job,class Enable=void>
struct parallel_for_range_move_helper: public boost::asynchronous::continuation_task<Range>
{
    parallel_for_range_move_helper(Range&& range,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :range_(boost::make_shared<Range>(std::forward<Range>(range))),func_(std::move(func)),cutoff_(cutoff),task_name_(std::move(task_name)),prio_(prio)
    {}
    parallel_for_range_move_helper(parallel_for_range_move_helper&&)=default;
    parallel_for_range_move_helper& operator=(parallel_for_range_move_helper&&)=default;
    parallel_for_range_move_helper(parallel_for_range_move_helper const&)=delete;
    parallel_for_range_move_helper& operator=(parallel_for_range_move_helper const&)=delete;

    void operator()()const
    {
        std::vector<boost::future<void> > fus;
        boost::asynchronous::any_weak_scheduler<Job> weak_scheduler = boost::asynchronous::get_thread_scheduler<Job>();
        boost::asynchronous::any_shared_scheduler<Job> locked_scheduler = weak_scheduler.lock();
        //TODO return what?
    //    if (!locked_scheduler.is_valid())
    //        // give up
    //        return;
        boost::shared_ptr<Range> range = std::move(range_);
        for (auto it= boost::begin(*range); it != boost::end(*range) ; )
        {
            auto itp = it;
            boost::asynchronous::detail::safe_advance(it,cutoff_,boost::end(*range));
            auto func = func_;
            boost::future<void> fu = boost::asynchronous::post_future(locked_scheduler,
                                                                      [it,itp,func]()
                                                                      {
                                                                        std::for_each(itp,it,func);
                                                                      },
                                                                      task_name_,prio_);
            fus.emplace_back(std::move(fu));
        }
        boost::asynchronous::continuation_result<Range> task_res = this->this_task_result();
        boost::asynchronous::create_continuation_log<Job>(
                    // called when subtasks are done, set our result
                    [task_res,range](std::vector<boost::future<void>> res)
                    {
                        try
                        {
                            for (typename std::vector<boost::future<void>>::iterator itr = res.begin();itr != res.end();++itr)
                            {
                                // get to check that no exception
                                (*itr).get();
                            }
                            task_res.emplace_value(std::move(*range));
                        }
                        catch(std::exception& e)
                        {
                            task_res.set_exception(boost::copy_exception(e));
                        }
                    },
                    // future results of recursive tasks
                    std::move(fus));
    }
    boost::shared_ptr<Range> range_;
    Func func_;
    long cutoff_;
    std::string task_name_;
    std::size_t prio_;
};
}
template <class Func, class Range>
struct serializable_for_each : public boost::asynchronous::serializable_task
{
    serializable_for_each(): boost::asynchronous::serializable_task(""){}
    serializable_for_each(Func&& f, Range&& r)
        : boost::asynchronous::serializable_task(f.get_task_name())
        , func_(std::forward<Func>(f))
        , range_(std::forward<Range>(r))
    {}
    template <class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/)
    {
        ar & func_;
        ar & range_;
    }
    Range operator()()
    {
        std::for_each(range_.begin(),range_.end(),func_);
        return std::move(range_);
    }

    Func func_;
    Range range_;
};
namespace detail
{
template <class Range, class Func, class Job>
struct parallel_for_range_move_helper<Range,Func,Job,typename ::boost::enable_if<boost::asynchronous::detail::is_serializable<Func> >::type>
        : public boost::asynchronous::continuation_task<Range>
        , public boost::asynchronous::serializable_task
{
    parallel_for_range_move_helper(Range&& range,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<Range>()
        , boost::asynchronous::serializable_task(func.get_task_name())
        , range_(std::forward<Range>(range)),func_(std::move(func)),cutoff_(cutoff),task_name_(std::move(task_name)),prio_(prio)
    {}
    void operator()()const
    {
        typedef std::vector<typename std::iterator_traits<decltype(boost::begin(range_))>::value_type> sub_range;
        std::vector<boost::future<sub_range> > fus;
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
            auto part_vec = boost::copy_range< sub_range>(boost::make_iterator_range(itp,it));
            auto func = func_;
            boost::future<sub_range> fu = boost::asynchronous::post_future(locked_scheduler,
                                                                      boost::asynchronous::serializable_for_each<Func,sub_range>
                                                                        (std::move(func),std::move(part_vec)),
                                                                      task_name_,prio_);
            fus.emplace_back(std::move(fu));
        }
        boost::asynchronous::continuation_result<Range> task_res = this->this_task_result();
        boost::asynchronous::create_continuation_log<Job>(
                    // called when subtasks are done, set our result
                    [task_res](std::vector<boost::future<sub_range> > res)
                    {
                        Range range;
                        try
                        {
                            for (typename std::vector<boost::future<sub_range>>::iterator itr = res.begin();itr != res.end();++itr)
                            {
                                range = boost::push_back(range,(*itr).get());

                            }
                            task_res.emplace_value(std::move(range));
                        }
                        catch(std::exception& e)
                        {
                            task_res.set_exception(boost::copy_exception(e));
                        }
                    },
                    // future results of recursive tasks
                    std::move(fus));
    }
    template <class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/)
    {
        ar & range_;
        ar & func_;
        ar & cutoff_;
        ar & task_name_;
        ar & prio_;
    }
    Range range_;
    Func func_;
    long cutoff_;
    std::string task_name_;
    std::size_t prio_;
};
}
template <class Range, class Func, class Job=boost::asynchronous::any_callable>
typename boost::disable_if<has_is_continuation_task<Range>,boost::asynchronous::detail::continuation<Range,Job> >::type
parallel_for(Range&& range,Func func,long cutoff,
             const std::string& task_name="", std::size_t prio=0)
{
    return boost::asynchronous::top_level_continuation_log<Range,Job>
            (boost::asynchronous::detail::parallel_for_range_move_helper<Range,Func,Job>(std::forward<Range>(range),func,cutoff,task_name,prio));
}

// version for ranges held only by reference => will return nothing (void)
namespace detail
{
template <class Range, class Func, class Job>
struct parallel_for_range_helper: public boost::asynchronous::continuation_task<void>
{
    parallel_for_range_helper(Range const& range,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :range_(range),func_(std::move(func)),cutoff_(cutoff),task_name_(std::move(task_name)),prio_(prio)
    {}
    void operator()()const
    {
        std::vector<boost::future<void> > fus;
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
            boost::future<void> fu = boost::asynchronous::post_future(locked_scheduler,
                                                                      [it,itp,func]()
                                                                      {
                                                                        std::for_each(itp,it,func);
                                                                      },
                                                                      task_name_,prio_);
            fus.emplace_back(std::move(fu));
        }
        boost::asynchronous::continuation_result<void> task_res = this->this_task_result();
        boost::asynchronous::create_continuation_log<Job>(
                    // called when subtasks are done, set our result
                    [task_res](std::vector<boost::future<void>> res)
                    {
                        try
                        {
                            for (typename std::vector<boost::future<void>>::iterator itr = res.begin();itr != res.end();++itr)
                            {
                                // get to check that no exception
                                (*itr).get();
                            }
                            task_res.set_value();
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
typename boost::disable_if<has_is_continuation_task<Range>,boost::asynchronous::detail::continuation<void,Job> >::type
parallel_for(Range const& range,Func func,long cutoff,
             const std::string& task_name="", std::size_t prio=0)
{
   return boost::asynchronous::top_level_continuation_log<void,Job>
            (boost::asynchronous::detail::parallel_for_range_helper<Range,Func,Job>(range,func,cutoff,task_name,prio));
}

// version for ranges given as continuation => will return the range as continuation
namespace detail
{
template <class Continuation, class Func, class Job>
struct parallel_for_continuation_range_helper: public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_for_continuation_range_helper(Continuation const& c,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :cont_(c),func_(std::move(func)),cutoff_(cutoff),task_name_(std::move(task_name)),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<typename Continuation::return_type> task_res = this->this_task_result();
        auto func(std::move(func_));
        auto cutoff = cutoff_;
        auto task_name = task_name_;
        auto prio = prio_;
        cont_.on_done([task_res,func,cutoff,task_name,prio](std::tuple<boost::future<typename Continuation::return_type> >&& continuation_res)
        {
            try
            {
                auto new_continuation = boost::asynchronous::parallel_for(std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                new_continuation.on_done([task_res](std::tuple<boost::future<typename Continuation::return_type> >&& new_continuation_res)
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
template <class Range, class Func, class Job=boost::asynchronous::any_callable>
typename boost::enable_if<has_is_continuation_task<Range>,boost::asynchronous::detail::continuation<typename Range::return_type,Job> >::type
parallel_for(Range range,Func func,long cutoff,
             const std::string& task_name="", std::size_t prio=0)
{
    return boost::asynchronous::top_level_continuation_log<typename Range::return_type,Job>
            (boost::asynchronous::detail::parallel_for_continuation_range_helper<Range,Func,Job>(range,func,cutoff,task_name,prio));
}

// version for iterators => will return nothing
namespace detail
{
template <class Iterator, class Func, class Job>
struct parallel_for_helper: public boost::asynchronous::continuation_task<void>
{
    parallel_for_helper(Iterator beg, Iterator end,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : beg_(beg),end_(end),func_(std::move(func)),cutoff_(cutoff),task_name_(std::move(task_name)),prio_(prio)
    {}
    void operator()()const
    {
        std::vector<boost::future<void> > fus;
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
            boost::future<void> fu = boost::asynchronous::post_future(locked_scheduler,
                                                                      [it,itp,func]()
                                                                      {
                                                                        std::for_each(itp,it,func);
                                                                      },
                                                                      task_name_,prio_);
            fus.emplace_back(std::move(fu));
        }
        boost::asynchronous::continuation_result<void> task_res = this_task_result();
        boost::asynchronous::create_continuation_log<Job>(
                    // called when subtasks are done, set our result
                    [task_res](std::vector<boost::future<void>> res)
                    {
                        try
                        {
                            for (typename std::vector<boost::future<void>>::iterator itr = res.begin();itr != res.end();++itr)
                            {
                                // get to check that no exception
                                (*itr).get();
                            }
                            task_res.set_value();
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
boost::asynchronous::detail::continuation<void,Job>
parallel_for(Iterator beg, Iterator end,Func func,long cutoff,
             const std::string& task_name="", std::size_t prio=0)
{
    return boost::asynchronous::top_level_continuation_log<void,Job>
            (boost::asynchronous::detail::parallel_for_helper<Iterator,Func,Job>(beg,end,func,cutoff,task_name,prio));
}

}}
#endif // BOOST_ASYNCHRON_PARALLEL_FOR_HPP
