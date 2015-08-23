// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_IS_PARTITIONED_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_IS_PARTITIONED_HPP

#include <algorithm>

#include <boost/utility/enable_if.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>

namespace boost { namespace asynchronous
{
namespace detail
{
enum class parallel_is_partitioned_state
{
    true_,false_,error_
};
using parallel_is_partitioned_state_pair = std::pair<boost::asynchronous::detail::parallel_is_partitioned_state,
                                                     boost::asynchronous::detail::parallel_is_partitioned_state>;

template <class Iterator, class Func, class Job>
struct parallel_is_partitioned_helper2:
        public boost::asynchronous::continuation_task<boost::asynchronous::detail::parallel_is_partitioned_state_pair>
{
    parallel_is_partitioned_helper2(Iterator beg, Iterator end, Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<boost::asynchronous::detail::parallel_is_partitioned_state_pair>(task_name),
          beg_(beg),end_(end),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<boost::asynchronous::detail::parallel_is_partitioned_state_pair> task_res
                = this_task_result();
        // advance up to cutoff
        Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
        if (it == end_)
        {
            bool at_begin = func_(*beg_);
            bool current = at_begin;
            for (auto it2 = beg_ ; it2 != end_ ; ++it2)
            {
                bool val = func_(*it2);
                if (!current && val)
                {
                    // false => true. We are done, not partitioned
                    task_res.set_value(std::make_pair(boost::asynchronous::detail::parallel_is_partitioned_state::error_,
                                                      boost::asynchronous::detail::parallel_is_partitioned_state::error_));
                    return;
                }
                current = val;
            }
            task_res.set_value(std::make_pair(at_begin ? boost::asynchronous::detail::parallel_is_partitioned_state::true_ :
                                                         boost::asynchronous::detail::parallel_is_partitioned_state::false_,
                                              current ? boost::asynchronous::detail::parallel_is_partitioned_state::true_ :
                                                        boost::asynchronous::detail::parallel_is_partitioned_state::false_));
        }
        else
        {
            boost::asynchronous::create_callback_continuation_job<Job>(
                        // called when subtasks are done, set our result
                        [task_res]
                        (std::tuple<boost::asynchronous::expected<boost::asynchronous::detail::parallel_is_partitioned_state_pair>,
                                    boost::asynchronous::expected<boost::asynchronous::detail::parallel_is_partitioned_state_pair> > res) mutable
                        {
                            try
                            {
                                auto res1 = std::get<0>(res).get();
                                auto res2 = std::get<1>(res).get();
                                if (res1.first == boost::asynchronous::detail::parallel_is_partitioned_state::error_ ||
                                    res2.first == boost::asynchronous::detail::parallel_is_partitioned_state::error_ )
                                {
                                    task_res.set_value(
                                                std::make_pair(boost::asynchronous::detail::parallel_is_partitioned_state::error_,
                                                               boost::asynchronous::detail::parallel_is_partitioned_state::error_));
                                }
                                else if (res1.second == boost::asynchronous::detail::parallel_is_partitioned_state::false_ &&
                                         res2.first == boost::asynchronous::detail::parallel_is_partitioned_state::true_ )
                                {
                                    task_res.set_value(
                                                std::make_pair(boost::asynchronous::detail::parallel_is_partitioned_state::error_,
                                                               boost::asynchronous::detail::parallel_is_partitioned_state::error_));
                                }
                                else
                                {
                                    task_res.set_value(
                                                std::make_pair(res1.first,
                                                               res2.second));
                                }
                            }
                            catch(std::exception& e)
                            {
                                task_res.set_exception(boost::copy_exception(e));
                            }
                        },
                        // recursive tasks
                        parallel_is_partitioned_helper2<Iterator,Func,Job>
                            (beg_,it,func_,cutoff_,this->get_name(),prio_),
                        parallel_is_partitioned_helper2<Iterator,Func,Job>
                            (it,end_,func_,cutoff_,this->get_name(),prio_)
            );
        }
    }
    Iterator beg_;
    Iterator end_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};

template <class Iterator, class Func, class Job>
struct parallel_is_partitioned_helper: public boost::asynchronous::continuation_task<bool>
{
    parallel_is_partitioned_helper(Iterator beg, Iterator end, Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<bool>(task_name),
          beg_(beg),end_(end),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<bool> task_res = this_task_result();

        boost::asynchronous::create_callback_continuation_job<Job>(
                    // called when subtasks are done, set our result
                    [task_res]
                    (std::tuple<boost::asynchronous::expected<boost::asynchronous::detail::parallel_is_partitioned_state_pair>> res) mutable
                    {
                        try
                        {
                            // get to check that no exception
                            auto res1 = std::get<0>(res).get();
                            task_res.set_value((res1.first != boost::asynchronous::detail::parallel_is_partitioned_state::error_)&&
                                               (res1.second != boost::asynchronous::detail::parallel_is_partitioned_state::error_));
                        }
                        catch(std::exception& e)
                        {
                            task_res.set_exception(boost::copy_exception(e));
                        }
                    },
                    // recursive tasks
                    parallel_is_partitioned_helper2<Iterator,Func,Job>
                        (beg_,end_,func_,cutoff_,this->get_name(),prio_)
        );
    }
    Iterator beg_;
    Iterator end_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
}

template <class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<bool,Job>
parallel_is_partitioned(Iterator beg, Iterator end, Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<bool,Job>
            (boost::asynchronous::detail::parallel_is_partitioned_helper<Iterator,Func,Job>
                (beg,end,std::move(func),cutoff,task_name,prio));

}

}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_IS_PARTITIONED_HPP

