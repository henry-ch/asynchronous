// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_PLACEMENT_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_PLACEMENT_HPP


#include <boost/utility/enable_if.hpp>

#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>

namespace boost { namespace asynchronous
{
namespace detail
{
template <class T, class Job>
struct parallel_placement_helper: public boost::asynchronous::continuation_task<void>
{
    parallel_placement_helper(std::size_t beg, std::size_t end, char* data,long cutoff,const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<void>(task_name),
          beg_(beg),end_(end),data_(data),cutoff_(cutoff),prio_(prio)
    {
    }
    void operator()()
    {
        boost::asynchronous::continuation_result<void> task_res = this_task_result();
        try
        {
            // advance up to cutoff
            auto it =(end_-beg_ <= (std::size_t)cutoff_)? end_: beg_ + (end_-beg_)/2;
            // if not at end, recurse, otherwise execute here
            if (it == end_)
            {
                for (std::size_t i = 0; i < (end_ - beg_); ++i)
                {
                    new (((T*)data_)+ (i+beg_)) T;
                }
                task_res.set_value();
            }
            else
            {
                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [task_res](std::tuple<boost::asynchronous::expected<void>,boost::asynchronous::expected<void> > res) mutable
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
                            boost::asynchronous::detail::parallel_placement_helper<T,Job>
                                    (beg_,it,data_,cutoff_,this->get_name(),prio_),
                            boost::asynchronous::detail::parallel_placement_helper<T,Job>
                                    (it,end_,data_,cutoff_,this->get_name(),prio_)
                 );
            }
        }
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }
    std::size_t beg_;
    std::size_t end_;
    char* data_;
    long cutoff_;
    std::size_t prio_;
};
}

template <class T, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_placement(std::size_t beg, std::size_t end, char* data,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
   return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_placement_helper<T,Job>(beg,end,data,cutoff,task_name,prio));
}

}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_PLACEMENT_HPP

