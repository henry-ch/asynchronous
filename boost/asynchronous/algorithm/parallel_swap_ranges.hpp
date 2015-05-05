// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2014
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_SWAP_RANGES_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_SWAP_RANGES_HPP

#include <algorithm>

#include <boost/utility/enable_if.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>

namespace boost { namespace asynchronous
{
namespace detail
{
template <class Iterator1,class Iterator2,class Job>
struct parallel_swap_ranges_helper: public boost::asynchronous::continuation_task<Iterator2>
{
    parallel_swap_ranges_helper(Iterator1 beg1, Iterator1 end1,Iterator2 beg2,
                            long cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<Iterator2>(task_name),
          beg1_(beg1),end1_(end1),beg2_(beg2),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<Iterator2> task_res = this->this_task_result();
        // advance up to cutoff
        Iterator1 it1 = boost::asynchronous::detail::find_cutoff(beg1_,cutoff_,end1_);
        if (it1 == end1_)
        {
            task_res.set_value(std::swap_ranges(beg1_,end1_,beg2_));
        }
        else
        {
            auto it2 = beg2_;
            std::advance(it2,std::distance(beg1_,it1));
            boost::asynchronous::create_callback_continuation_job<Job>(
                    // called when subtasks are done, set our result
                    [task_res](std::tuple<boost::asynchronous::expected<Iterator2>,boost::asynchronous::expected<Iterator2> > res)
                    {
                        try
                        {
                            // get to check that no exception
                            std::get<0>(res).get();
                            task_res.set_value(std::get<1>(res).get());
                        }
                        catch(std::exception& e)
                        {
                            task_res.set_exception(boost::copy_exception(e));
                        }
                    },
                    // recursive tasks
                    parallel_swap_ranges_helper<Iterator1,Iterator2,Job>
                        (beg1_,it1,beg2_,cutoff_,this->get_name(),prio_),
                    parallel_swap_ranges_helper<Iterator1,Iterator2,Job>
                        (it1,end1_,it2,cutoff_,this->get_name(),prio_)
            );
        }
    }
    Iterator1 beg1_;
    Iterator1 end1_;
    Iterator2 beg2_;
    long cutoff_;
    std::size_t prio_;
};
}

template <class Iterator1,class Iterator2, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<Iterator2,Job>
parallel_swap_ranges(Iterator1 beg1, Iterator1 end1,Iterator2 beg2, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<Iterator2,Job>
            (boost::asynchronous::detail::parallel_swap_ranges_helper<Iterator1,Iterator2,Job>
                (beg1,end1,beg2,cutoff,task_name,prio));

}

}}

#endif // BOOST_ASYNCHRONOUS_PARALLEL_SWAP_RANGES_HPP

