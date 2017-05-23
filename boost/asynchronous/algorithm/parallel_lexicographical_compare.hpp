// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_LEXICOGRAPHICAL_COMPARE_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_LEXICOGRAPHICAL_COMPARE_HPP

#include <algorithm>
#include <utility>
#include <type_traits>
#include <iterator>

#include <boost/thread/future.hpp>
#include <boost/serialization/vector.hpp>

#include <boost/asynchronous/detail/any_interruptible.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/scheduler/serializable_task.hpp>
#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/algorithm/detail/parallel_all_of_helper.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>

namespace boost { namespace asynchronous
{
namespace detail
{
// version for iterators
template <class Iterator1, class Iterator2, class Func,class Job>
struct parallel_lexicographical_compare_helper: public boost::asynchronous::continuation_task<bool>
{
    parallel_lexicographical_compare_helper(Iterator1 beg1, Iterator1 end1,Iterator2 beg2,Iterator2 end2, Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<bool>(task_name)
        , beg1_(beg1),end1_(end1), beg2_(beg2),end2_(end2)
        ,func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<bool> task_res = this->this_task_result();
        try
        {
            // advance up to cutoff
            Iterator1 it1 = boost::asynchronous::detail::find_cutoff(beg1_,cutoff_,end1_);
            Iterator2 it2 = boost::asynchronous::detail::find_cutoff(beg2_,cutoff_,end2_);
            // if not at end, recurse, otherwise execute here
            if ((it1 == end1_) || (it2 == end2_))
            {
                task_res.set_value(std::lexicographical_compare(beg1_,end1_,beg2_,end2_,func_));
            }
            else
            {
                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [task_res](std::tuple<boost::asynchronous::expected<bool>,boost::asynchronous::expected<bool>> res) mutable
                            {
                                try
                                {
                                    bool r1 = std::get<0>(res).get();
                                    bool r2 = std::get<1>(res).get();
                                    if (r1)
                                        task_res.set_value(true);
                                    else
                                        task_res.set_value(r2);
                                }
                                catch(std::exception& e)
                                {
                                    task_res.set_exception(std::make_exception_ptr(e));
                                }
                            },
                            // recursive tasks
                            parallel_lexicographical_compare_helper<Iterator1,Iterator2,Func,Job>
                                    (beg1_,it1,beg2_,it2,func_,cutoff_,this->get_name(),prio_),
                            parallel_lexicographical_compare_helper<Iterator1,Iterator2,Func,Job>
                                    (it1,end1_,it2,end2_,func_,cutoff_,this->get_name(),prio_)
                );
            }
        }
        catch(std::exception& e)
        {
            task_res.set_exception(std::make_exception_ptr(e));
        }
    }
    Iterator1 beg1_;
    Iterator1 end1_;
    Iterator2 beg2_;
    Iterator2 end2_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
template <class Iterator1, class Iterator2,class Job>
struct parallel_lexicographical_compare_helper2: public boost::asynchronous::continuation_task<bool>
{
    parallel_lexicographical_compare_helper2(Iterator1 beg1, Iterator1 end1,Iterator2 beg2,Iterator2 end2,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<bool>(task_name)
        , beg1_(beg1),end1_(end1), beg2_(beg2),end2_(end2)
        , cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<bool> task_res = this->this_task_result();
        try
        {
            // advance up to cutoff
            Iterator1 it1 = boost::asynchronous::detail::find_cutoff(beg1_,cutoff_,end1_);
            Iterator2 it2 = boost::asynchronous::detail::find_cutoff(beg2_,cutoff_,end2_);
            // if not at end, recurse, otherwise execute here
            if ((it1 == end1_) || (it2 == end2_))
            {
                task_res.set_value(std::lexicographical_compare(beg1_,end1_,beg2_,end2_));
            }
            else
            {
                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [task_res](std::tuple<boost::asynchronous::expected<bool>,boost::asynchronous::expected<bool>> res) mutable
                            {
                                try
                                {
                                    bool r1 = std::get<0>(res).get();
                                    bool r2 = std::get<1>(res).get();
                                    if (r1)
                                        task_res.set_value(true);
                                    else
                                        task_res.set_value(r2);
                                }
                                catch(std::exception& e)
                                {
                                    task_res.set_exception(std::make_exception_ptr(e));
                                }
                            },
                            // recursive tasks
                            parallel_lexicographical_compare_helper2<Iterator1,Iterator2,Job>
                                    (beg1_,it1,beg2_,it2,cutoff_,this->get_name(),prio_),
                            parallel_lexicographical_compare_helper2<Iterator1,Iterator2,Job>
                                    (it1,end1_,it2,end2_,cutoff_,this->get_name(),prio_)
                );
            }
        }
        catch(std::exception& e)
        {
            task_res.set_exception(std::make_exception_ptr(e));
        }
    }
    Iterator1 beg1_;
    Iterator1 end1_;
    Iterator2 beg2_;
    Iterator2 end2_;
    long cutoff_;
    std::size_t prio_;
};
}

// versions for iterators
template <class Iterator1,class Iterator2, class Func,
          class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<bool,Job>
parallel_lexicographical_compare(Iterator1 beg1, Iterator1 end1,Iterator2 beg2,Iterator2 end2,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio=0)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<bool,Job>
            (boost::asynchronous::detail::parallel_lexicographical_compare_helper<Iterator1,Iterator2,Func,Job>
                (beg1,end1,beg2,end2,func,cutoff,task_name,prio));
}
template <class Iterator1,class Iterator2,
          class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<bool,Job>
parallel_lexicographical_compare(Iterator1 beg1, Iterator1 end1,Iterator2 beg2,Iterator2 end2,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio=0)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<bool,Job>
            (boost::asynchronous::detail::parallel_lexicographical_compare_helper2<Iterator1,Iterator2,Job>
                (beg1,end1,beg2,end2,cutoff,task_name,prio));
}
}}

#endif // BOOST_ASYNCHRONOUS_PARALLEL_LEXICOGRAPHICAL_COMPARE_HPP

