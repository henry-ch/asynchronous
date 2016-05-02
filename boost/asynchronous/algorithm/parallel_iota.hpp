// Boost.Asynchronous library
//  Copyright (C) Tobias Holl 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRON_PARALLEL_IOTA_HPP
#define BOOST_ASYNCHRON_PARALLEL_IOTA_HPP

#include <algorithm>
#include <numeric>

#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/has_range_iterator.hpp>

namespace boost { namespace asynchronous {

namespace detail {

template <typename Iterator>
struct iterator_value_type
{
    using type = typename std::remove_reference<typename std::remove_cv<decltype(*std::declval<Iterator>())>::type>::type;
};

template <typename Range>
struct range_value_type
{
    using type = typename std::remove_reference<typename std::remove_cv<decltype(*boost::begin(std::declval<Range>()))>::type>::type;
};

template <typename Iterator, typename Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
struct parallel_iota_inplace_helper : public boost::asynchronous::continuation_task<void>
{
    using value_type = typename boost::asynchronous::detail::iterator_value_type<Iterator>::type;

    parallel_iota_inplace_helper(Iterator beg, Iterator end, value_type start, long cutoff,
                                 std::string const& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<void>(task_name)
        , beg_(beg), end_(end), start_(start), cutoff_(cutoff), prio_(prio)
    {}

    void operator()()
    {
        boost::asynchronous::continuation_result<void> task_res = this_task_result();
        
        // Advance iterator up to the cutoff
        Iterator it = boost::asynchronous::detail::find_cutoff(beg_, cutoff_, end_);
        // If the end is reached, use this thread to execute the subtask, otherwise recurse
        if (it == end_)
        {
            std::iota(beg_, it, start_);
            task_res.set_value();
        }
        else
        {
            auto distance = std::distance(beg_, it);
            boost::asynchronous::create_callback_continuation_job<Job>(
                        // Callback
                        [task_res]
                        (std::tuple<boost::asynchronous::expected<void>, boost::asynchronous::expected<void> > res) mutable
                        {
                            try
                            {
                                // Call get() to check for exceptions
                                std::get<0>(res).get();
                                std::get<1>(res).get();
                                task_res.set_value();
                            }
                            catch (std::exception& e)
                            {
                                task_res.set_exception(boost::copy_exception(e));
                            }
                        },
                        // recursive tasks
                        parallel_iota_inplace_helper<Iterator, Job>(beg_, it, start_, cutoff_, this->get_name(), prio_),
                        parallel_iota_inplace_helper<Iterator, Job>(it, end_, start_ + distance, cutoff_, this->get_name(), prio_)
               );
        }
    }

    Iterator beg_;
    Iterator end_;
    value_type start_;
    long cutoff_;
    std::size_t prio_;
};

}

template <typename Iterator, typename T, typename Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void, Job>
parallel_iota(Iterator begin, Iterator end, T const& value, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void, Job>(boost::asynchronous::detail::parallel_iota_inplace_helper<Iterator, Job>(begin, end, value, cutoff, task_name, prio));
}

namespace detail {

template <typename Range, typename Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
struct parallel_iota_generate_helper : public boost::asynchronous::continuation_task<Range>
{
    using value_type = typename boost::asynchronous::detail::range_value_type<Range>::type;

    parallel_iota_generate_helper(boost::shared_ptr<Range> range, value_type start, long cutoff,
                                 std::string const& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<Range>(task_name)
        , range_(range), start_(start), cutoff_(cutoff), prio_(prio)
    {}

    void operator()()
    {
        boost::shared_ptr<Range> range = std::move(range_);
        boost::asynchronous::continuation_result<Range> task_res = this->this_task_result();
        
        // Get iterators
        auto beg = boost::begin(*range);
        auto end = boost::end(*range);

        // Advance iterator up to the cutoff
        auto it = boost::asynchronous::detail::find_cutoff(beg, cutoff_, end);
        // If the end is reached, use this thread to execute the subtask, otherwise recurse
        if (it == end)
        {
            std::iota(beg, it, start_);
            task_res.set_value(std::move(*range));
        }
        else
        {
            auto distance = std::distance(beg, it);
            boost::asynchronous::create_callback_continuation_job<Job>(
                        // Callback
                        [task_res, range]
                        (std::tuple<boost::asynchronous::expected<void>, boost::asynchronous::expected<void> > res) mutable
                        {
                            try
                            {
                                // Call get() to check for exceptions
                                std::get<0>(res).get();
                                std::get<1>(res).get();
                                task_res.set_value(std::move(*range));
                            }
                            catch (std::exception& e)
                            {
                                task_res.set_exception(boost::copy_exception(e));
                            }
                        },
                        // recursive tasks
                        parallel_iota_inplace_helper<decltype(it), Job>(beg, it, start_, cutoff_, this->get_name(), prio_),
                        parallel_iota_inplace_helper<decltype(it), Job>(it, end, start_ + distance, cutoff_, this->get_name(), prio_)
               );
        }
    }

    boost::shared_ptr<Range> range_;
    value_type start_;
    long cutoff_;
    std::size_t prio_;
};

}

template <typename Range, typename T, typename Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::enable_if<boost::has_range_iterator<Range>, boost::asynchronous::detail::callback_continuation<Range, Job>>::type
parallel_iota(Range && range, T const& value, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto range_ptr = boost::make_shared<Range>(std::forward<Range>(range));
    return boost::asynchronous::top_level_callback_continuation_job<Range, Job>(boost::asynchronous::detail::parallel_iota_generate_helper<Range, Job>(range_ptr, value, cutoff, task_name, prio));
}

}}

#endif // BOOST_ASYNCHRON_PARALLEL_IOTA_HPP
