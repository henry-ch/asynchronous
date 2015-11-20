// Boost.Asynchronous library
//  Copyright (C) Tobias Holl, Franz Alt, Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_MOVE_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_MOVE_HPP

#include <algorithm>
#include <iterator> // for std::iterator_traits
#include <type_traits>

#include <boost/utility/enable_if.hpp>

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

#include <boost/mpl/has_xxx.hpp>

namespace boost { namespace asynchronous {

// version for iterators => will return nothing


namespace detail
{

template<class Iterator, class ResultIterator, class Job>
struct parallel_move_helper : public boost::asynchronous::continuation_task<void>
{
    parallel_move_helper(Iterator begin, Iterator end, ResultIterator result, long cutoff, std::string const & task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<void>(task_name)
        , begin_(begin)
        , end_(end)
        , result_(result)
        , cutoff_(cutoff)
        , task_name_(std::move(task_name))
        , prio_(prio)
    {}

    void operator()()
    {
        boost::asynchronous::continuation_result<void> task_res = this_task_result();
        try
        {
            // advance up to cutoff
            Iterator it = boost::asynchronous::detail::find_cutoff(begin_, cutoff_, end_);

            // distance between begin and it
            std::size_t dist = std::distance(begin_, it);

            // if not at end, recurse, otherwise execute here
            if (it == end_)
            {
                std::move(begin_, it, result_);
                task_res.set_value();
            }
            else
            {
                boost::asynchronous::create_callback_continuation_job<Job>(
                    // called when subtasks are done, set our result
                    [task_res](std::tuple<boost::asynchronous::expected<void>, boost::asynchronous::expected<void> > res) mutable
                    {
                        try
                        {
                            // get to check that no exception
                            std::get<0>(res).get();
                            std::get<1>(res).get();

                            task_res.set_value();
                        }
                        catch (std::exception const & e)
                        {
                            task_res.set_exception(boost::copy_exception(e));
                        }
                    },
                    // recursive tasks
                    parallel_move_helper<Iterator, ResultIterator, Job>(begin_, it, result_, cutoff_, task_name_, prio_),
                    parallel_move_helper<Iterator, ResultIterator, Job>(it, end_, result_ + dist, cutoff_, task_name_, prio_)
                );
            }
        }
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }

    Iterator begin_;
    Iterator end_;
    ResultIterator result_;
    long cutoff_;
    std::string task_name_;
    std::size_t prio_;
};

}

template<class Iterator, class ResultIterator, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::enable_if<boost::asynchronous::detail::has_iterator_category<std::iterator_traits<Iterator>>, boost::asynchronous::detail::callback_continuation<void, Job>>::type
parallel_move(Iterator begin, Iterator end, ResultIterator result, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio)
#else
                     const std::string& task_name = "", std::size_t prio = 0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void, Job>
               (boost::asynchronous::detail::parallel_move_helper<Iterator, ResultIterator, Job>(begin, end, result, cutoff, task_name, prio));
}


// version for moved ranges => will return the range as continuation

namespace detail {

template <class Range, class ResultIterator, class Job,class Enable=void>
struct parallel_move_range_move_helper: public boost::asynchronous::continuation_task<Range>
{
    parallel_move_range_move_helper(boost::shared_ptr<Range> range, ResultIterator out, long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<Range>(task_name)
        ,range_(range),out_(out),cutoff_(cutoff),prio_(prio)
    {
    }
    parallel_move_range_move_helper(parallel_move_range_move_helper&&)=default;
    parallel_move_range_move_helper& operator=(parallel_move_range_move_helper&&)=default;
    parallel_move_range_move_helper(parallel_move_range_move_helper const&)=delete;
    parallel_move_range_move_helper& operator=(parallel_move_range_move_helper const&)=delete;

    void operator()()
    {
        boost::asynchronous::continuation_result<Range> task_res = this->this_task_result();
        try
        {
            boost::shared_ptr<Range> range = std::move(range_);
            // advance up to cutoff
            auto it = boost::asynchronous::detail::find_cutoff(boost::begin(*range),cutoff_,boost::end(*range));
            std::size_t dist = std::distance(boost::begin(*range), it);
            // if not at end, recurse, otherwise execute here
            if (it == boost::end(*range))
            {
                std::move(boost::begin(*range),it,out_);
                task_res.set_value(std::move(*range));
            }
            else
            {
                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [task_res,range]
                            (std::tuple<boost::asynchronous::expected<void>,boost::asynchronous::expected<void> > res) mutable
                            {
                                try
                                {
                                    // get to check that no exception
                                    std::get<0>(res).get();
                                    std::get<1>(res).get();
                                    task_res.set_value(std::move(*range));
                                }
                                catch(std::exception& e)
                                {
                                    task_res.set_exception(boost::copy_exception(e));
                                }
                            },
                            // recursive tasks (via iterators)
                            boost::asynchronous::detail::parallel_move_helper<decltype(boost::begin(*range_)),ResultIterator,Job>
                                (boost::begin(*range),it,out_,cutoff_,this->get_name(),prio_),
                            boost::asynchronous::detail::parallel_move_helper<decltype(boost::begin(*range_)),ResultIterator,Job>
                                (it,boost::end(*range),out_ + dist,cutoff_,this->get_name(),prio_)
                );
            }
        }
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }
    boost::shared_ptr<Range> range_;
    ResultIterator out_;
    long cutoff_;
    std::size_t prio_;
};

}

template <class Range, class ResultIterator, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::disable_if<has_is_continuation_task<Range>,boost::asynchronous::detail::callback_continuation<Range,Job> >::type
parallel_move(Range&& range, ResultIterator out, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto r = boost::make_shared<Range>(std::forward<Range>(range));
    return boost::asynchronous::top_level_callback_continuation_job<Range, Job>
            (boost::asynchronous::detail::parallel_move_range_move_helper<Range, ResultIterator, Job>
                (r,out,cutoff,task_name,prio));
}


// version for ranges given as continuation => will return the original range as continuation


namespace detail
{

// adapter to non-callback continuations
template <class Continuation, class ResultIterator, class Job, class Enable=void>
struct parallel_move_continuation_range_helper: public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_move_continuation_range_helper(Continuation const& c, ResultIterator out, long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<typename Continuation::return_type>(task_name)
        ,cont_(c),out_(out),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<typename Continuation::return_type> task_res = this->this_task_result();
        try
        {
            auto out = out_;
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            cont_.on_done([task_res,out,cutoff,task_name,prio](std::tuple<boost::future<typename Continuation::return_type> >&& continuation_res) mutable
            {
                try
                {
                    auto new_continuation = boost::asynchronous::parallel_move<typename Continuation::return_type, ResultIterator, Job>(std::move(std::get<0>(continuation_res).get()),out,cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res) mutable
                    {
                        task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                    });
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(boost::copy_exception(e));
                }
            }
            );
            boost::asynchronous::any_continuation ac(std::move(cont_));
            boost::asynchronous::get_continuations().emplace_front(std::move(ac));
        }
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }
    Continuation cont_;
    ResultIterator out_;
    long cutoff_;
    std::size_t prio_;
};

// Continuation is a callback continuation
template <class Continuation, class ResultIterator, class Job>
struct parallel_move_continuation_range_helper<Continuation, ResultIterator, Job, typename ::boost::enable_if< has_is_callback_continuation_task<Continuation> >::type>
        : public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_move_continuation_range_helper(Continuation const& c, ResultIterator out,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<typename Continuation::return_type>(task_name)
        ,cont_(c),out_(out),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<typename Continuation::return_type> task_res = this->this_task_result();
        try
        {
            auto out = out_;
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            cont_.on_done([task_res,out,cutoff,task_name,prio](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& continuation_res) mutable
            {
                try
                {
                    auto new_continuation = boost::asynchronous::parallel_move<typename Continuation::return_type, ResultIterator, Job>(std::move(std::get<0>(continuation_res).get()),out,cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res)mutable
                    {
                        task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                    });
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(boost::copy_exception(e));
                }
            }
            );
        }
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }
    Continuation cont_;
    ResultIterator out_;
    long cutoff_;
    std::size_t prio_;
};
}

template <class Range, class ResultIterator, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::enable_if<has_is_continuation_task<Range>,boost::asynchronous::detail::callback_continuation<typename Range::return_type,Job> >::type
parallel_move(Range range,ResultIterator out,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<typename Range::return_type,Job>
            (boost::asynchronous::detail::parallel_move_continuation_range_helper<Range,ResultIterator,Job>(range,out,cutoff,task_name,prio));
}

}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_MOVE_HPP

