// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2014
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_REVERSE_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_REVERSE_HPP

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
template <class Iterator,class Job>
struct parallel_reverse_helper: public boost::asynchronous::continuation_task<void>
{
    parallel_reverse_helper(Iterator beg, Iterator end,Iterator beg_reverse,Iterator end_reverse,
                            long cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<void>(task_name),
          beg_(beg),end_(end),beg_reverse_(beg_reverse),end_reverse_(end_reverse),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<void> task_res = this_task_result();
        // advance up to cutoff
        Iterator it = boost::asynchronous::detail::find_cutoff(beg_reverse_,cutoff_,end_reverse_);
        if (it == end_reverse_)
        {
            auto end_swap = end_;
            std::advance(end_swap,-(std::distance(beg_,beg_reverse_)+1));
            for (auto it2 = beg_reverse_; it2 != it; ++it2)
            {
                std::iter_swap(it2,end_swap);
                --end_swap;
            }
            task_res.set_value();
        }
        else
        {
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
                        parallel_reverse_helper<Iterator,Job>
                            (beg_,end_,beg_reverse_,it,cutoff_,this->get_name(),prio_),
                        parallel_reverse_helper<Iterator,Job>
                            (beg_,end_,it,end_reverse_,cutoff_,this->get_name(),prio_)
            );
        }
    }
    Iterator beg_;
    Iterator end_;
    Iterator beg_reverse_;
    Iterator end_reverse_;
    long cutoff_;
    std::size_t prio_;
};
}

template <class Iterator, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_reverse(Iterator beg, Iterator end, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto number_elements = std::distance(beg,end);
    auto end_reverse = beg;
    std::advance(end_reverse,number_elements/2);
    return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_reverse_helper<Iterator,Job>
                (beg,end,beg,end_reverse,cutoff,task_name,prio));

}
}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_REVERSE_HPP
