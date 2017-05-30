// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2014
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_ALL_OF_HELPER_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_ALL_OF_HELPER_HPP

#include <algorithm>
#include <utility>
#include <type_traits>
#include <iterator>
#include <exception>

#include <boost/serialization/vector.hpp>

#include <boost/asynchronous/detail/any_interruptible.hpp>
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

// version for iterators
template <class Iterator, class Func, class Op,class Job>
struct parallel_all_of_helper: public boost::asynchronous::continuation_task<bool>
{
    parallel_all_of_helper(Iterator beg, Iterator end,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<bool>(task_name)
        , beg_(beg),end_(end),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()const
    {
        boost::asynchronous::continuation_result<bool> task_res = this->this_task_result();
        // advance up to cutoff
        Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
        // if not at end, recurse, otherwise execute here
        if (it == end_)
        {
            task_res.set_value(Op().algorithm(beg_,end_,func_));
        }
        else
        {
            boost::asynchronous::create_callback_continuation_job<Job>(
                        // called when subtasks are done, set our result
                        [task_res](std::tuple<boost::asynchronous::expected<bool>,boost::asynchronous::expected<bool>> res)
                        {
                            try
                            {
                                bool rt = std::get<0>(res).get();
                                bool rt2 = std::get<1>(res).get();
                                task_res.set_value( Op().merge(rt,rt2));
                            }
                            catch(...)
                            {
                                task_res.set_exception(std::current_exception());
                            }
                        },
                        // recursive tasks
                        parallel_all_of_helper<Iterator,Func,Op,Job>(beg_,it,func_,cutoff_,this->get_name(),prio_),
                        parallel_all_of_helper<Iterator,Func,Op,Job>(it,end_,func_,cutoff_,this->get_name(),prio_)
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
}}

#endif // BOOST_ASYNCHRONOUS_PARALLEL_ALL_OF_HELPER_HPP

