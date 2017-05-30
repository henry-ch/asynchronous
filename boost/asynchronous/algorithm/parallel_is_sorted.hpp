// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2014
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_IS_SORTED_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_IS_SORTED_HPP

#include <algorithm>

#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>

namespace boost { namespace asynchronous
{
namespace detail
{
template <class Iterator, class Func, class Job>
struct parallel_is_sorted_helper: public boost::asynchronous::continuation_task<bool>
{
    parallel_is_sorted_helper(Iterator beg, Iterator end, Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<bool>(task_name),
          beg_(beg),end_(end),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<bool> task_res = this_task_result();
        try
        {
            // advance up to cutoff
            Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
            if (it == end_)
            {
                task_res.set_value(std::is_sorted(beg_,end_,func_));
            }
            else
            {
                // optimize for not sorted range
                auto it2 = it;
                std::advance(it2,-1);
                // TODO safe_advance must handle negative distance
                if (func_(*it,*it2))
                {
                    task_res.set_value(false);
                    return;
                }
                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [task_res](std::tuple<boost::asynchronous::expected<bool>,boost::asynchronous::expected<bool> > res) mutable
                            {
                                try
                                {
                                    // get to check that no exception
                                    bool res1 = std::get<0>(res).get();
                                    bool res2 = std::get<1>(res).get();
                                    task_res.set_value(res1 && res2);
                                }
                                catch(...)
                                {
                                    task_res.set_exception(std::current_exception());
                                }
                            },
                            // recursive tasks
                            parallel_is_sorted_helper<Iterator,Func,Job>
                                (beg_,it,func_,cutoff_,this->get_name(),prio_),
                            parallel_is_sorted_helper<Iterator,Func,Job>
                                (it,end_,func_,cutoff_,this->get_name(),prio_)
                );
            }
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
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
boost::asynchronous::detail::callback_continuation<bool,Job>
parallel_is_sorted(Iterator beg, Iterator end, Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio=0)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<bool,Job>
            (boost::asynchronous::detail::parallel_is_sorted_helper<Iterator,Func,Job>
                (beg,end,std::move(func),cutoff,task_name,prio));

}

namespace detail
{
    template <class Func>
    struct reverse_sorted
    {
        reverse_sorted(Func f):func_(std::move(f)){}
        template <class T>
        bool operator ()(T const& lhs, T const& rhs)const
        {
            return func_(rhs,lhs);
        }
        Func func_;
    };
}

template <class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<bool,Job>
parallel_is_reverse_sorted(Iterator beg, Iterator end, Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio=0)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<bool,Job>
            (boost::asynchronous::detail::parallel_is_sorted_helper<Iterator,boost::asynchronous::detail::reverse_sorted<Func>,Job>
                (beg,end,boost::asynchronous::detail::reverse_sorted<Func>(std::move(func)),cutoff,task_name,prio));

}

}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_IS_SORTED_HPP
