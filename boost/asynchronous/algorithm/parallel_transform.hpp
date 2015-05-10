// Boost.Asynchronous library
//  Copyright (C) Franz Alt, Christophe Henry 2014
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_TRANSFORM_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_TRANSFORM_HPP

#include <algorithm>
#include <iterator>
#include <string>
#include <tuple>
#include <type_traits>

#include <boost/asynchronous/detail/any_interruptible.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/scheduler/serializable_task.hpp>
#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>

#include <boost/mpl/has_xxx.hpp>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/iterator_range.hpp>

#include <boost/utility/enable_if.hpp>

BOOST_MPL_HAS_XXX_TRAIT_DEF(iterator_category)

namespace boost { namespace asynchronous
{

struct std_transform
{
    template<class Iterator, class ResultIterator, class Func>
    ResultIterator operator()(Iterator beg, Iterator end, ResultIterator result, Func & f)
    {
        return std::transform(beg, end, result, f);
    }

    template<class Iterator1, class Iterator2, class ResultIterator, class Func>
    ResultIterator operator()(Iterator1 beg1, Iterator1 end1, Iterator2 beg2, ResultIterator result, Func & f)
    {
        return std::transform(beg1, end1, beg2, result, f);
    }
};

// version for iterators => will return nothing
namespace detail
{

template<class Iterator, class ResultIterator, class Func, class Job, class Transform>
struct parallel_transform_helper : public boost::asynchronous::continuation_task<ResultIterator>
{
    parallel_transform_helper(Iterator begin, Iterator end, ResultIterator result, Func func, long cutoff, std::string const & task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<ResultIterator>(task_name)
        , begin_(begin)
        , end_(end)
        , result_(result)
        , func_(std::move(func))
        , cutoff_(cutoff)
        , task_name_(std::move(task_name))
        , prio_(prio)
    {}

    void operator()() const
    {
        boost::asynchronous::continuation_result<ResultIterator> task_res = this->this_task_result();

        // advance up to cutoff
        Iterator it = boost::asynchronous::detail::find_cutoff(begin_, cutoff_, end_);

        // distance between begin and it
        std::size_t dist = std::distance(begin_, it);

        // if not at end, recurse, otherwise execute here
        if (it == end_)
        {
            task_res.set_value(Transform()(begin_, it, result_, func_));
        }
        else
        {
            ResultIterator result = result_;
            std::advance(result, dist);
            boost::asynchronous::create_callback_continuation_job<Job>(
                // called when subtasks are done, set our result
                [task_res](std::tuple<boost::asynchronous::expected<ResultIterator>, boost::asynchronous::expected<ResultIterator> > res)
                {
                    try
                    {
                        // get to check that no exception
                        std::get<0>(res).get();
                        task_res.set_value(std::get<1>(res).get());
                    }
                    catch (std::exception const & e)
                    {
                        task_res.set_exception(boost::copy_exception(e));
                    }
                },
                // recursive tasks
                parallel_transform_helper<Iterator, ResultIterator, Func, Job, Transform>(begin_, it, result_, func_, cutoff_, task_name_, prio_),
                parallel_transform_helper<Iterator, ResultIterator, Func, Job, Transform>(it, end_, result, func_, cutoff_, task_name_, prio_)
            );
        }
    }

    Iterator begin_;
    Iterator end_;
    ResultIterator result_;
    Func func_;
    long cutoff_;
    std::string task_name_;
    std::size_t prio_;
};

}

// version for iterators => will return nothing
template<class Iterator, class ResultIterator, class Func, class Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::enable_if<has_iterator_category<std::iterator_traits<Iterator> >,
                          boost::asynchronous::detail::callback_continuation<ResultIterator, Job> >::type
parallel_transform(Iterator begin, Iterator end, ResultIterator result, Func func, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio)
#else
                     const std::string& task_name = "", std::size_t prio = 0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<ResultIterator, Job>
               (boost::asynchronous::detail::parallel_transform_helper<Iterator, ResultIterator, Func, Job, boost::asynchronous::std_transform>(begin, end, result, func, cutoff, task_name, prio));
}

// version for two iterators => will return nothing
namespace detail
{

template<class Iterator1, class Iterator2, class ResultIterator, class Func, class Job, class Transform>
struct parallel_transform2_helper : public boost::asynchronous::continuation_task<ResultIterator>
{
    parallel_transform2_helper(Iterator1 begin1, Iterator1 end1, Iterator2 begin2, ResultIterator result, Func func, long cutoff, std::string const & task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<ResultIterator>(task_name)
        , begin1_(begin1)
        , end1_(end1)
        , begin2_(begin2)
        , result_(result)
        , func_(std::move(func))
        , cutoff_(cutoff)
        , task_name_(std::move(task_name))
        , prio_(prio)
    {}

    void operator()() const
    {
        boost::asynchronous::continuation_result<ResultIterator> task_res = this->this_task_result();

        // advance first up to cutoff
        Iterator1 it1 = boost::asynchronous::detail::find_cutoff(begin1_, cutoff_, end1_);

        // distance between begin and it
        std::size_t dist = std::distance(begin1_, it1);

        // advance second up to first cutoff
        Iterator2 begin2 = begin2_;
        Iterator2 it2 = begin2 + dist;

        // if not at end, recurse, otherwise execute here
        if (it1 == end1_)
        {
            task_res.set_value(Transform()(begin1_, it1, begin2_, result_, func_));
        }
        else
        {
            boost::asynchronous::create_callback_continuation_job<Job>(
                // called when subtasks are done, set our result
                [task_res](std::tuple<boost::asynchronous::expected<ResultIterator>, boost::asynchronous::expected<ResultIterator> > res)
                {
                    try
                    {
                        // get to check that no exception
                        std::get<0>(res).get();
                        task_res.set_value(std::get<1>(res).get());
                    }
                    catch (std::exception const & e)
                    {
                        task_res.set_exception(boost::copy_exception(e));
                    }
                },
                // recursive tasks
                parallel_transform2_helper<Iterator1, Iterator2, ResultIterator, Func, Job, Transform>(begin1_, it1, begin2_, result_, func_, cutoff_, task_name_, prio_),
                parallel_transform2_helper<Iterator1, Iterator2, ResultIterator, Func, Job, Transform>(it1, end1_, it2, result_ + dist, func_, cutoff_, task_name_, prio_)
            );
        }
    }

    Iterator1 begin1_;
    Iterator1 end1_;
    Iterator2 begin2_;
    ResultIterator result_;
    Func func_;
    long cutoff_;
    std::string task_name_;
    std::size_t prio_;
};

}

// version for two iterators => will return nothing
template<class Iterator1, class Iterator2, class ResultIterator, class Func, class Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::enable_if<has_iterator_category<std::iterator_traits<Iterator1> >,
                          boost::asynchronous::detail::callback_continuation<ResultIterator, Job> >::type
parallel_transform(Iterator1 begin1, Iterator1 end1, Iterator2 begin2, ResultIterator result, Func func, long cutoff, std::string const & task_name = "", std::size_t prio = 0)
{
    return boost::asynchronous::top_level_callback_continuation_job<ResultIterator, Job>
               (boost::asynchronous::detail::parallel_transform2_helper<Iterator1, Iterator2, ResultIterator, Func, Job, boost::asynchronous::std_transform>(begin1, end1, begin2, result, func, cutoff, task_name, prio));
}

// version for ranges held only by reference => will return nothing (void)
namespace detail
{

template<class Range, class ResultIterator, class Func, class Job, class Transform>
struct parallel_transform_range_helper : public boost::asynchronous::continuation_task<ResultIterator>
{
    parallel_transform_range_helper(Range & range, ResultIterator result, Func func, long cutoff, std::string const & task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<ResultIterator>(task_name)
        , range_(range)
        , result_(result)
        , func_(std::move(func))
        , cutoff_(cutoff)
        , task_name_(std::move(task_name))
        , prio_(prio)
    {}

    void operator()() const
    {
        boost::asynchronous::continuation_result<ResultIterator> task_res = this->this_task_result();

        // advance up to cutoff
        auto it = boost::asynchronous::detail::find_cutoff(boost::begin(range_), cutoff_, boost::end(range_));

        // distance between begin and it
        std::size_t dist = std::distance(boost::begin(range_), it);

        // if not at end, recurse, otherwise execute here
        if (it == boost::end(range_))
        {
            task_res.set_value(Transform()(boost::begin(range_), it, result_, func_));
        }
        else
        {
            boost::asynchronous::create_callback_continuation_job<Job>(
                // called when subtasks are done, set our result
                [task_res](std::tuple<boost::asynchronous::expected<ResultIterator>, boost::asynchronous::expected<ResultIterator> > res)
                {
                    try
                    {
                        // get to check that no exception
                        std::get<0>(res).get();
                        task_res.set_value(std::get<1>(res).get());
                    }
                    catch (std::exception const & e)
                    {
                        task_res.set_exception(boost::copy_exception(e));
                    }
                },
                // recursive tasks
                parallel_transform_helper<decltype(boost::begin(range_)), ResultIterator, Func, Job, Transform>(boost::begin(range_), it, result_, func_, cutoff_, task_name_, prio_),
                parallel_transform_helper<decltype(boost::begin(range_)), ResultIterator, Func, Job, Transform>(it, boost::end(range_), result_ + dist, func_, cutoff_, task_name_, prio_)
            );
        }
    }

    Range & range_;
    ResultIterator result_;
    Func func_;
    long cutoff_;
    std::string task_name_;
    std::size_t prio_;
};

}

// version for ranges held only by reference => will return nothing (void)
template<class Range, class ResultIterator, class Func, class Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::disable_if<has_iterator_category<std::iterator_traits<Range> >,
                           boost::asynchronous::detail::callback_continuation<ResultIterator, Job> >::type
parallel_transform(Range & range, ResultIterator result, Func func, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio)
#else
                     const std::string& task_name = "", std::size_t prio = 0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<ResultIterator, Job>
               (boost::asynchronous::detail::parallel_transform_range_helper<Range, ResultIterator, Func, Job, boost::asynchronous::std_transform>(range, result, func, cutoff, task_name, prio));
}

// version for two ranges held only by reference => will return nothing (void)
namespace detail
{

template<class Range1, class Range2, class ResultIterator, class Func, class Job, class Transform>
struct parallel_transform2_range_helper : public boost::asynchronous::continuation_task<ResultIterator>
{
    parallel_transform2_range_helper(Range1 & range1, Range2 & range2, ResultIterator result, Func func, long cutoff, std::string const & task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<ResultIterator>(task_name)
        , range1_(range1)
        , range2_(range2)
        , result_(result)
        , func_(std::move(func))
        , cutoff_(cutoff)
        , task_name_(std::move(task_name))
        , prio_(prio)
    {}

    void operator()() const
    {
        boost::asynchronous::continuation_result<ResultIterator> task_res = this->this_task_result();

        // advance first up to cutoff
        auto it1 = boost::asynchronous::detail::find_cutoff(boost::begin(range1_), cutoff_, boost::end(range1_));

        // distance between begin and it
        std::size_t dist = std::distance(boost::begin(range1_), it1);

        // advance seconf up to first cutoff
        auto it2 = boost::begin(range1_) + dist;

        // if not at end, recurse, otherwise execute here
        if (it1 == boost::end(range1_))
        {
            task_res.set_value(Transform()(boost::begin(range1_), it1, boost::begin(range2_), result_, func_));
        }
        else
        {
            boost::asynchronous::create_callback_continuation_job<Job>(
                // called when subtasks are done, set our result
                [task_res](std::tuple<boost::asynchronous::expected<ResultIterator>, boost::asynchronous::expected<ResultIterator> > res)
                {
                    try
                    {
                        // get to check that no exception
                        std::get<0>(res).get();
                        task_res.set_value(std::get<1>(res).get());
                    }
                    catch (std::exception const & e)
                    {
                        task_res.set_exception(boost::copy_exception(e));
                    }
                },
                // recursive tasks
                parallel_transform2_helper<decltype(boost::begin(range1_)), decltype(boost::begin(range2_)), ResultIterator, Func, Job, Transform>(boost::begin(range1_), it1, boost::begin(range2_), result_, func_, cutoff_, task_name_, prio_),
                parallel_transform2_helper<decltype(boost::begin(range1_)), decltype(boost::begin(range2_)), ResultIterator, Func, Job, Transform>(it1, boost::end(range1_), it2, result_ + dist, func_, cutoff_, task_name_, prio_)
            );
        }
    }

    Range1 & range1_;
    Range2 & range2_;
    ResultIterator result_;
    Func func_;
    long cutoff_;
    std::string task_name_;
    std::size_t prio_;
};

}

// version for two ranges held only by reference => will return nothing (void)
template<class Range1, class Range2, class ResultIterator, class Func, class Job = BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::disable_if<has_iterator_category<std::iterator_traits<Range1> >,
                           boost::asynchronous::detail::callback_continuation<ResultIterator, Job> >::type
parallel_transform(Range1 & range1, Range2 & range2, ResultIterator result, Func func, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio)
#else
                     const std::string& task_name = "", std::size_t prio = 0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<ResultIterator, Job>
               (boost::asynchronous::detail::parallel_transform2_range_helper<Range1, Range2, ResultIterator, Func, Job, boost::asynchronous::std_transform>(range1, range2, result, func, cutoff, task_name, prio));
}

}}

#endif // BOOST_ASYNCHRONOUS_PARALLEL_TRANSFORM_HPP
