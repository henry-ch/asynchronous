// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_FOR_EACH_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_FOR_EACH_HPP

#include <algorithm>
#include <vector>
#include <iterator> // for std::iterator_traits

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

// version for iterators => will return nothing
namespace detail
{
template <class Iterator, class Func, class Job>
struct parallel_for_each_helper: public boost::asynchronous::continuation_task<Func>
{
    parallel_for_each_helper(Iterator beg, Iterator end,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<Func>(task_name)
        , beg_(beg),end_(end),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()const
    {
        boost::asynchronous::continuation_result<Func> task_res = this->this_task_result();
        // advance up to cutoff
        Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
        // if not at end, recurse, otherwise execute here
        if (it == end_)
        {
            task_res.set_value(std::move(std::for_each(beg_,it,std::move(func_))));
        }
        else
        {
            boost::asynchronous::create_callback_continuation_job<Job>(
                        // called when subtasks are done, set our result
                        [task_res](std::tuple<boost::asynchronous::expected<Func>,boost::asynchronous::expected<Func> > res)
                        {
                            try
                            {
                                // get to check that no exception
                                Func f1 (std::move(std::get<0>(res).get()));
                                Func f2 (std::move(std::get<1>(res).get()));
                                f1.merge(f2);
                                task_res.set_value(std::move(f1));
                            }
                            catch(std::exception& e)
                            {
                                task_res.set_exception(boost::copy_exception(e));
                            }
                        },
                        // recursive tasks
                        parallel_for_each_helper<Iterator,Func,Job>(beg_,it,func_,cutoff_,this->get_name(),prio_),
                        parallel_for_each_helper<Iterator,Func,Job>(it,end_,func_,cutoff_,this->get_name(),prio_)
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

template <class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<Func,Job>
parallel_for_each(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<Func,Job>
            (boost::asynchronous::detail::parallel_for_each_helper<Iterator,Func,Job>(beg,end,std::move(func),cutoff,task_name,prio));
}
}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_FOR_EACH_HPP