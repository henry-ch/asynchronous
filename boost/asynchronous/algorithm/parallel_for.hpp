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
#include <boost/thread/future.hpp>
#include <boost/asynchronous/detail/any_interruptible.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>

namespace boost { namespace asynchronous
{
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
            std::advance(it,cutoff_);
            auto func = func_;
            boost::future<void> fu = boost::asynchronous::post_future(locked_scheduler,
                                                                      [it,func]()
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
                            for (std::vector<boost::future<void>>::iterator itr = res.begin();itr != res.end();++itr)
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
//template <class Iterator, class S1, class S2, class Func, class Callback>
//boost::asynchronous::any_interruptible
//parallel_for(Iterator begin, Iterator end, S1 const& scheduler,Func && func, S2 const& weak_cb_scheduler, Callback&& c,
//             const std::string& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0)
//{

//}

//template <class Range, class Func, class Job=boost::asynchronous::any_callable>
//boost::asynchronous::detail::continuation<Range,Job>
//parallel_for(Range& range,Func func,long cutoff,
// TODO ranges
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
