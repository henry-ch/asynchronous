// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_ADJACENT_FIND_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_ADJACENT_FIND_HPP

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

template<class Iterator, class Func, class Job>
struct parallel_adjacent_find_helper : public boost::asynchronous::continuation_task<Iterator>
{
    parallel_adjacent_find_helper(Iterator begin, Iterator end, Func func,
                                  long cutoff, std::string const & task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<Iterator>(task_name)
        , begin_(begin)
        , end_(end)
        , func_(std::move(func))
        , cutoff_(cutoff)
        , task_name_(std::move(task_name))
        , prio_(prio)
    {}

    void operator()() const
    {
        boost::asynchronous::continuation_result<Iterator> task_res = this->this_task_result();
        try
        {
            // advance up to cutoff
            // remember previous as we have to handle forward iterators
            std::pair<Iterator,Iterator> itpair = boost::asynchronous::detail::find_cutoff_and_prev(begin_, cutoff_, end_);
            auto it = itpair.second;
            auto itprev = itpair.first;
            // if not at end, recurse, otherwise execute here
            if (it == end_)
            {
                task_res.set_value(std::adjacent_find(begin_, end_, func_));
            }
            else
            {
                auto func = func_;
                boost::asynchronous::create_callback_continuation_job<Job>(
                    // called when subtasks are done, set our result
                    [task_res,func,itprev,it]
                    (std::tuple<boost::asynchronous::expected<Iterator>, boost::asynchronous::expected<Iterator> > res) mutable
                    {
                        try
                        {
                            // get to check that no exception
                            Iterator it1 = std::get<0>(res).get();
                            if (it1 != it)
                            {
                                task_res.set_value(it1);
                            }
                            else if (func(*(itprev),*it1))
                            {
                                task_res.set_value(itprev);
                            }
                            else
                            {
                                task_res.set_value(std::get<1>(res).get());
                            }
                        }
                        catch (std::exception const & e)
                        {
                            task_res.set_exception(std::make_exception_ptr(e));
                        }
                    },
                    // recursive tasks
                    parallel_adjacent_find_helper<Iterator, Func, Job>(begin_, it, func_, cutoff_, task_name_, prio_),
                    parallel_adjacent_find_helper<Iterator, Func, Job>(it, end_, func_, cutoff_, task_name_, prio_)
                );
            }
        }
        catch(std::exception& e)
        {
            task_res.set_exception(std::make_exception_ptr(e));
        }
    }

    Iterator begin_;
    Iterator end_;
    Func func_;
    long cutoff_;
    std::string task_name_;
    std::size_t prio_;
};
}

// versions for iterators
template <class Iterator, class Func,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<Iterator,Job>
parallel_adjacent_find(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio=0)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<Iterator,Job>
            (boost::asynchronous::detail::parallel_adjacent_find_helper<Iterator,Func,Job>
                (beg,end,func,cutoff,task_name,prio));
}

template <class Iterator,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<Iterator,Job>
parallel_adjacent_find(Iterator beg, Iterator end,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio=0)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto l = [](const typename std::iterator_traits<Iterator>::value_type& i,
                const typename std::iterator_traits<Iterator>::value_type& j)
    {
        return i == j;
    };
    return boost::asynchronous::parallel_adjacent_find<Iterator,decltype(l),Job>
                (beg,end,std::move(l),cutoff,task_name,prio);
}

}}

#endif // BOOST_ASYNCHRONOUS_PARALLEL_ADJACENT_FIND_HPP

