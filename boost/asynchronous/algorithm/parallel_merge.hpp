// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2014
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_MERGE_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_MERGE_HPP

#include <algorithm>

#include <boost/utility/enable_if.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/scheduler/serializable_task.hpp>
#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>

namespace boost { namespace asynchronous
{
namespace detail
{
template <class Iterator1, class Iterator2, class OutIterator, class Func, class Job>
struct parallel_merge_helper: public boost::asynchronous::continuation_task<void>
{
    parallel_merge_helper(Iterator1 beg1, Iterator1 end1, Iterator2 beg2, Iterator2 end2, OutIterator out, Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<void>(task_name),
          beg1_(beg1),end1_(end1),beg2_(beg2),end2_(end2),out_(out),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()const
    {
        boost::asynchronous::continuation_result<void> task_res = this_task_result();
        auto length1 = std::distance(beg1_,end1_);
        auto length2 = std::distance(beg2_,end2_);
        // if not at end, recurse, otherwise execute here
        if ((length1+length2) <= cutoff_)
        {
            std::merge(beg1_,end1_,beg2_,end2_,out_,func_);
            task_res.set_value();
        }
        else
        {
            if (length1 >= length2)
            {
                if (length1 == 0)
                {
                    task_res.set_value();
                    return;
                }
                helper(beg1_,end1_,beg2_,end2_,out_,std::move(task_res));
            }
            else
            {
                if (length2 == 0)
                {
                    task_res.set_value();
                    return;
                }
                helper(beg2_,end2_,beg1_,end1_,out_,std::move(task_res));
            }
        }
    }
    template <class It1, class It2, class OutIt>
    void helper(It1 beg1, It1 end1, It2 beg2, It2 end2, OutIt out,boost::asynchronous::continuation_result<void> task_res)const
    {
        auto it1 = (beg1+ (end1 - beg1)/2);
        auto it2 = std::lower_bound(beg2,end2, *it1);
        auto it3 = out + ((it1 - beg1) + (it2 - beg2));
        // TODO move?
        *it3 = *it1;
        boost::asynchronous::create_callback_continuation_job<Job>(
                    // called when subtasks are done, set our result
                    [task_res](std::tuple<boost::asynchronous::expected<void>,boost::asynchronous::expected<void> > res)
                    {
                        try
                        {
                            // get to check that no exception
                            std::get<0>(res).get();
                            std::get<1>(res).get();
                            task_res.set_value();
                        }
                        catch(std::exception& e)
                        {
                            task_res.set_exception(boost::copy_exception(e));
                        }
                    },
                    // recursive tasks
                    parallel_merge_helper<Iterator1,Iterator2,OutIterator,Func,Job>
                        (beg1,it1,beg2,it2,out,func_,cutoff_,this->get_name(),prio_),
                    parallel_merge_helper<Iterator1,Iterator2,OutIterator,Func,Job>
                        (it1+1,end1,it2,end2,it3+1,func_,cutoff_,this->get_name(),prio_)
        );
    }

    Iterator1 beg1_;
    Iterator1 end1_;
    Iterator2 beg2_;
    Iterator2 end2_;
    OutIterator out_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
}

template <class Iterator1, class Iterator2, class OutIterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
// TODO out iterator instead of void
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_merge(Iterator1 beg1, Iterator1 end1, Iterator2 beg2, Iterator2 end2, OutIterator out, Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_merge_helper<Iterator1,Iterator2,OutIterator,Func,Job>
                (beg1,end1,beg2,end2,out,std::move(func),cutoff,task_name,prio));

}

}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_MERGE_HPP
