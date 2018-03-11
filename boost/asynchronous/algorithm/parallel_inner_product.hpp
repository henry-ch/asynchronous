// Boost.Asynchronous library
//  Copyright (C) Tobias Holl 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_INNER_PRODUCT_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_INNER_PRODUCT_HPP

#include <type_traits>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/has_range_iterator.hpp>
#include <boost/range/iterator_range.hpp>

#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/algorithm/invoke.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>

#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/scheduler/serializable_task.hpp>

namespace boost { namespace asynchronous
{

namespace detail
{

// Calculates inner_product serially.
// Assumes that beg1 and beg2 are valid.
template <class Iterator1, class Iterator2, class T, class BinaryOperation, class Reduce>
T inner_product_helper(Iterator1 beg1, Iterator1 end1, Iterator2 beg2, BinaryOperation op, Reduce red)
{
    T store = op(*beg1++, *beg2++);
    while (beg1 != end1) {
        store = red(store, op(*beg1++, *beg2++));
    }
    return store;
}

// Continuation task returning the original value immediately.
//Used if the ranges are empty and if begin and end iterators are equal
template <class T>
struct default_value_helper : public boost::asynchronous::continuation_task<T>
{
    default_value_helper(T const& t, const std::string& task_name)
        : boost::asynchronous::continuation_task<T>(task_name)
        , t_(t)
    {}

    default_value_helper(const std::string& task_name)
        : boost::asynchronous::continuation_task<T>(task_name)
        , t_()
    {}

    void operator()()
    {
        boost::asynchronous::continuation_result<T> task_res = this->this_task_result();
        try {
            task_res.set_value(std::move(t_));
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
        }
    }
    T t_;
};

struct automatic_return_type {};

}

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ *
 * +++++++++++++++++++++++++++++++++++++++++++++ ITERATORS +++++++++++++++++++++++++++++++++++++++++++++ *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

namespace detail
{

// Continuation task for parallel_inner_product with iterators
template <class Iterator1, class Iterator2, class T, class BinaryOperation, class Reduce, class Job>
struct parallel_inner_product_helper: public boost::asynchronous::continuation_task<T>
{
    parallel_inner_product_helper(Iterator1 beg1, Iterator1 end1, Iterator2 beg2, BinaryOperation op, Reduce red, long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<T>(task_name)
        , beg1_(beg1)
        , end1_(end1)
        , beg2_(beg2)
        , op_(op)
        , red_(red)
        , cutoff_(cutoff)
        , prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<T> task_res = this->this_task_result();
        try
        {
            // Advance the first iterator based on the cutoff, but not past the end
            Iterator1 it1 = boost::asynchronous::detail::find_cutoff(beg1_, cutoff_, end1_);
            // If the cutoff reaches the end, calculate the result here, otherwise, recurse
            if (it1 == end1_)
            {
                task_res.set_value(std::move(boost::asynchronous::detail::inner_product_helper<Iterator1, Iterator2, T, BinaryOperation, Reduce>(beg1_, it1, beg2_, std::move(op_), std::move(red_))));
            }
            else
            {
                // Advance second iterator
                Iterator2 it2 = beg2_;
                std::advance(it2, std::distance(beg1_, it1));
                // Recursion
                auto red = red_;
                boost::asynchronous::create_callback_continuation_job<Job>(
                    // Combine the subtask results and set the result
                    [task_res, red](std::tuple<boost::asynchronous::expected<T>, boost::asynchronous::expected<T>> res) mutable
                    {
                        try
                        {
                            task_res.set_value(std::move(red(std::get<0>(res).get(), std::get<1>(res).get())));
                        }
                        catch(...)
                        {
                            task_res.set_exception(std::current_exception());
                        }
                    },
                    // Subtasks
                    parallel_inner_product_helper<Iterator1, Iterator2, T, BinaryOperation, Reduce, Job>(beg1_, it1, beg2_, op_, red_, cutoff_, this->get_name(), prio_),
                    parallel_inner_product_helper<Iterator1, Iterator2, T, BinaryOperation, Reduce, Job>(it1, end1_, it2, op_, red_, cutoff_, this->get_name(), prio_)
                );
            }
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
        }
    }
    Iterator1 beg1_;
    Iterator1 end1_;
    Iterator2 beg2_;
    BinaryOperation op_;
    Reduce red_;
    long cutoff_;
    std::size_t prio_;
};

}

// Iterators (with starting value)
template <class Iterator1,
          class Iterator2,
          class BinaryOperation,
          class Reduce,
          class Value,
          class Enable = typename std::enable_if<!boost::has_range_iterator<Iterator1>::value>::type,
          class T = decltype( // Automatically deduce return type
                        std::declval<Reduce>()(
                            std::declval<BinaryOperation>()(
                                *std::declval<Iterator1>(),
                                *std::declval<Iterator2>()
                            ),
                            std::declval<BinaryOperation>()(
                                *std::declval<Iterator1>(),
                                *std::declval<Iterator2>()
                            )
                        )
                    ),
          class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<T, Job> parallel_inner_product(Iterator1 beg1, Iterator1 end1, Iterator2 beg2, BinaryOperation op, Reduce red, const Value& value, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                       const std::string& task_name, std::size_t prio=0)
#else
                       const std::string& task_name="", std::size_t prio=0)
#endif
{
    if (beg1 == end1) {
        // There is no data, return the default value
        return boost::asynchronous::top_level_callback_continuation_job<T, Job>(boost::asynchronous::detail::default_value_helper<T>(value, task_name));
    }
    return boost::asynchronous::invoke(
        boost::asynchronous::top_level_callback_continuation_job<T, Job>(
            boost::asynchronous::detail::parallel_inner_product_helper<Iterator1, Iterator2, T, BinaryOperation, Reduce, Job>(beg1, end1, beg2, op, red, cutoff, task_name, prio)
        ),
        [value, red](T const& t) {
            return red(value, t);
        }
    );
}

// Iterators (without starting value)
template <class Iterator1,
          class Iterator2,
          class BinaryOperation,
          class Reduce,
          class Enable = typename std::enable_if<!boost::has_range_iterator<Iterator1>::value>::type,
          class T = decltype( // Automatically deduce return type
                        std::declval<Reduce>()(
                            std::declval<BinaryOperation>()(
                                *std::declval<Iterator1>(),
                                *std::declval<Iterator2>()
                            ),
                            std::declval<BinaryOperation>()(
                                *std::declval<Iterator1>(),
                                *std::declval<Iterator2>()
                            )
                        )
                    ),
          class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<T, Job> parallel_inner_product(Iterator1 beg1, Iterator1 end1, Iterator2 beg2, BinaryOperation op, Reduce red, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                       const std::string& task_name, std::size_t prio=0)
#else
                       const std::string& task_name="", std::size_t prio=0)
#endif
{
    if (beg1 == end1) {
        // There is no data, return the default value
        return boost::asynchronous::top_level_callback_continuation_job<T, Job>(boost::asynchronous::detail::default_value_helper<T>(task_name));
    }
    return boost::asynchronous::top_level_callback_continuation_job<T, Job>(
        boost::asynchronous::detail::parallel_inner_product_helper<Iterator1, Iterator2, T, BinaryOperation, Reduce, Job>(beg1, end1, beg2, op, red, cutoff, task_name, prio)
    );
}

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ *
 * +++++++++++++++++++++++++++++++++++++++++++++ MOVED RANGES +++++++++++++++++++++++++++++++++++++++++++++ *
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

namespace detail
{

// Continuation task for parallel_inner_product with moved ranges
template <class Range1, class Range2, class T, class BinaryOperation, class Reduce, class Job, class Enable1=void, class Enable2=void>
struct parallel_inner_product_range_move_helper: public boost::asynchronous::continuation_task<T>
{
    parallel_inner_product_range_move_helper(std::shared_ptr<Range1> range1, std::shared_ptr<Range2> range2, BinaryOperation op, Reduce red,
                                             long cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<T>(task_name)
        , range1_(range1)
        , range2_(range2)
        , op_(op)
        , red_(red)
        , cutoff_(cutoff)
        , prio_(prio)
    {}

    void operator()()
    {
        boost::asynchronous::continuation_result<T> task_res = this->this_task_result();
        try
        {
            std::shared_ptr<Range1> range1 = std::move(range1_);
            std::shared_ptr<Range2> range2 = std::move(range2_);
            // Get iterators for range 1
            auto beg1 = boost::begin(*range1);
            auto end1 = boost::end(*range1);
            // Get iterator for range 2
            auto beg2 = boost::begin(*range2);
            // Advance iterator for range 1 based on the cutoff, but not past the end.
            auto it1 = boost::asynchronous::detail::find_cutoff(beg1, cutoff_, end1);
            // If the cutoff reaches the end, calculate the result here, otherwise, recurse
            if (it1 == end1)
            {
                task_res.set_value(std::move(boost::asynchronous::detail::inner_product_helper<decltype(beg1), decltype(beg2), T, BinaryOperation, Reduce>(beg1, it1, beg2, std::move(op_), std::move(red_))));
            }
            else
            {
                // Advance second iterator
                auto it2 = beg2;
                std::advance(it2, std::distance(beg1, it1));
                // Recursion
                auto red = red_;
                boost::asynchronous::create_callback_continuation_job<Job>(
                    // Combine the subtask results and set the result
                    [task_res, red, range1, range2](std::tuple<boost::asynchronous::expected<T>, boost::asynchronous::expected<T>> res) mutable
                    {
                        try
                        {
                            task_res.set_value(std::move(red(std::get<0>(res).get(), std::get<1>(res).get())));
                        }
                        catch(...)
                        {
                            task_res.set_exception(std::current_exception());
                        }
                    },
                    // Subtasks
                    parallel_inner_product_helper<decltype(beg1), decltype(beg2), T, BinaryOperation, Reduce, Job>(beg1, it1, beg2, op_, red_, cutoff_, this->get_name(), prio_),
                    parallel_inner_product_helper<decltype(beg1), decltype(beg2), T, BinaryOperation, Reduce, Job>(it1, end1, it2, op_, red_, cutoff_, this->get_name(), prio_)
                );
            }
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
        }
    }

    std::shared_ptr<Range1> range1_;
    std::shared_ptr<Range2> range2_;
    BinaryOperation op_;
    Reduce red_;
    long cutoff_;
    std::size_t prio_;
};

template <class Range1, class Range2, class T, class BinaryOperation, class Reduce, class Job>
struct parallel_inner_product_range_move_helper<Range1, Range2, T, BinaryOperation, Reduce, Job, typename std::enable_if<boost::asynchronous::detail::is_serializable<BinaryOperation>::value>::type, typename std::enable_if<boost::asynchronous::detail::is_serializable<Reduce>::value>::type>
        : public boost::asynchronous::continuation_task<T>
        , public boost::asynchronous::serializable_task
{
    typedef decltype(boost::begin(std::declval<Range1>())) Iterator1;
    typedef decltype(boost::begin(std::declval<Range2>())) Iterator2;

    // default constructor only when deserialized immediately after
    parallel_inner_product_range_move_helper() : boost::asynchronous::serializable_task("parallel_inner_product_range_move_helper") {}

    parallel_inner_product_range_move_helper(std::shared_ptr<Range1> range1, std::shared_ptr<Range2> range2, BinaryOperation op, Reduce red,
                                             long cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<T>(task_name)
        , boost::asynchronous::serializable_task(op.get_task_name())
        , range1_(range1)
        , range2_(range2)
        , op_(std::move(op))
        , red_(std::move(red))
        , cutoff_(cutoff)
        , task_name_(task_name)
        , prio_(prio)
        , beg1_(boost::begin(*range1_))
        , end1_(boost::end(*range1_))
        , beg2_(boost::begin(*range2_))
    {}

    parallel_inner_product_range_move_helper(std::shared_ptr<Range1> range1, std::shared_ptr<Range2> range2,
                                             Iterator1 beg1, Iterator1 end1, Iterator2 beg2,
                                             BinaryOperation op, Reduce red,
                                             long cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<T>(task_name)
        , boost::asynchronous::serializable_task(op.get_task_name())
        , range1_(range1)
        , range2_(range2)
        , op_(std::move(op))
        , red_(std::move(red))
        , cutoff_(cutoff)
        , task_name_(task_name)
        , prio_(prio)
        , beg1_(beg1)
        , end1_(end1)
        , beg2_(beg2)
    {}

    void operator()()
    {
        boost::asynchronous::continuation_result<T> task_res = this->this_task_result();
        try
        {
            // Advance iterator for range 1 based on the cutoff, but not past the end.
            auto it1 = boost::asynchronous::detail::find_cutoff(beg1_, cutoff_, end1_);
            // If the cutoff reaches the end, calculate the result here, otherwise, recurse
            if (it1 == end1_)
            {
                task_res.set_value(std::move(boost::asynchronous::detail::inner_product_helper<decltype(beg1_), decltype(beg2_), T, BinaryOperation, Reduce>(beg1_, it1, beg2_, std::move(op_), std::move(red_))));
            }
            else
            {
                // Advance second iterator
                Iterator2 it2 = beg2_;
                std::advance(it2, std::distance(beg1_, it1));
                // Recursion
                auto red = red_;
                boost::asynchronous::create_callback_continuation_job<Job>(
                    // Combine the subtask results and set the result
                    [task_res, red](std::tuple<boost::asynchronous::expected<T>, boost::asynchronous::expected<T>> res) mutable
                    {
                        try
                        {
                            task_res.set_value(std::move(red(std::get<0>(res).get(), std::get<1>(res).get())));
                        }
                        catch(...)
                        {
                            task_res.set_exception(std::current_exception());
                        }
                    },
                    // Subtasks
                    parallel_inner_product_range_move_helper<Range1, Range2, T, BinaryOperation, Reduce, Job>(range1_, range2_, beg1_, it1, beg2_, op_, red_, cutoff_, this->get_name(), prio_),
                    parallel_inner_product_range_move_helper<Range1, Range2, T, BinaryOperation, Reduce, Job>(range1_, range2_, it1, end1_, it2, op_, red_, cutoff_, this->get_name(), prio_)
                );
            }
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
        }
    }

    template <class Archive>
    void save(Archive & ar, const unsigned int /*version*/) const
    {
        Iterator2 end2 = beg2_;
        std::advance(end2, std::distance(beg1_, end1_));
        // only part range
        // TODO avoid copying
        auto r1 = std::move(boost::copy_range<Range1>(boost::make_iterator_range(beg1_, end1_)));
        auto r2 = std::move(boost::copy_range<Range2>(boost::make_iterator_range(beg2_, end2)));
        ar & r1;
        ar & r2;
        ar & op_;
        ar & red_;
        ar & cutoff_;
        ar & task_name_;
        ar & prio_;
    }

    template <class Archive>
    void load(Archive & ar, const unsigned int /*version*/)
    {
        range1_ = std::make_shared<Range1>();
        range2_ = std::make_shared<Range2>();
        ar & (*range1_);
        ar & (*range2_);
        ar & op_;
        ar & red_;
        ar & cutoff_;
        ar & task_name_;
        ar & prio_;
        beg1_ = boost::begin(*range1_);
        end1_ = boost::end(*range1_);
        beg2_ = boost::begin(*range2_);
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()

    std::shared_ptr<Range1> range1_;
    std::shared_ptr<Range2> range2_;
    BinaryOperation op_;
    Reduce red_;
    long cutoff_;
    std::string task_name_;
    std::size_t prio_;
    Iterator1 beg1_;
    Iterator1 end1_;
    Iterator2 beg2_;
};

}

// Moved ranges (with starting value)
template <class Range1,
          class Range2,
          class BinaryOperation,
          class Reduce,
          class Value,
          class Enable = typename std::enable_if<boost::has_range_iterator<Range1>::value && boost::has_range_iterator<Range2>::value &&
                                                     !boost::asynchronous::detail::has_is_continuation_task<Range1>::value &&
                                                     !boost::asynchronous::detail::has_is_continuation_task<Range2>::value>::type,
          class T = decltype( // Automatically deduce return type
                        std::declval<Reduce>()(
                            std::declval<BinaryOperation>()(
                                *boost::begin(std::declval<Range1>()),
                                *boost::begin(std::declval<Range2>())
                            ),
                            std::declval<BinaryOperation>()(
                                *boost::begin(std::declval<Range1>()),
                                *boost::begin(std::declval<Range2>())
                            )
                        )
                    ),
          class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<boost::has_range_iterator<Range1>::value && boost::has_range_iterator<Range2>::value &&
                            !boost::asynchronous::detail::has_is_continuation_task<Range1>::value &&
                            !boost::asynchronous::detail::has_is_continuation_task<Range2>::value,
                            boost::asynchronous::detail::callback_continuation<T, Job>>::type
parallel_inner_product(Range1 && range1, Range2 && range2, BinaryOperation op, Reduce red, const Value& value, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                       const std::string& task_name, std::size_t prio=0)
#else
                       const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto r1 = std::make_shared<Range1>(std::forward<Range1>(range1));
    auto r2 = std::make_shared<Range2>(std::forward<Range2>(range2));
    if (boost::begin(*r1) == boost::end(*r1)) {
        // There is no data, return the default value
        return boost::asynchronous::top_level_callback_continuation_job<T, Job>(boost::asynchronous::detail::default_value_helper<T>(value, task_name));
    }
    return boost::asynchronous::invoke(
        boost::asynchronous::top_level_callback_continuation_job<T, Job>(
            boost::asynchronous::detail::parallel_inner_product_range_move_helper<Range1, Range2, T, BinaryOperation, Reduce, Job>(r1, r2, op, red, cutoff, task_name, prio)
        ),
        [value, red](T const& t) {
            return red(value, t);
        }
    );
}

// Moved Ranges (without starting value)
template <class Range1,
          class Range2,
          class BinaryOperation,
          class Reduce,
          class Enable = typename std::enable_if<boost::has_range_iterator<Range1>::value && boost::has_range_iterator<Range2>::value &&
                                                     !boost::asynchronous::detail::has_is_continuation_task<Range1>::value &&
                                                     !boost::asynchronous::detail::has_is_continuation_task<Range2>::value>::type,
          class T = decltype( // Automatically deduce return type
                        std::declval<Reduce>()(
                            std::declval<BinaryOperation>()(
                                *boost::begin(std::declval<Range1>()),
                                *boost::begin(std::declval<Range2>())
                            ),
                            std::declval<BinaryOperation>()(
                                *boost::begin(std::declval<Range1>()),
                                *boost::begin(std::declval<Range2>())
                            )
                        )
                    ),
          class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>

typename std::enable_if<boost::has_range_iterator<Range1>::value && boost::has_range_iterator<Range2>::value &&
                            !boost::asynchronous::detail::has_is_continuation_task<Range1>::value &&
                            !boost::asynchronous::detail::has_is_continuation_task<Range2>::value,
                            boost::asynchronous::detail::callback_continuation<T, Job>>::type
parallel_inner_product(Range1 && range1, Range2 && range2, BinaryOperation op, Reduce red, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                       const std::string& task_name, std::size_t prio=0)
#else
                       const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto r1 = std::make_shared<Range1>(std::forward<Range1>(range1));
    auto r2 = std::make_shared<Range2>(std::forward<Range2>(range2));
    if (boost::begin(*r1) == boost::end(*r1)) {
        // There is no data, return the default value
        return boost::asynchronous::top_level_callback_continuation_job<T, Job>(boost::asynchronous::detail::default_value_helper<T>(task_name));
    }
    return boost::asynchronous::top_level_callback_continuation_job<T, Job>(
        boost::asynchronous::detail::parallel_inner_product_range_move_helper<Range1, Range2, T, BinaryOperation, Reduce, Job>(r1, r2, op, red, cutoff, task_name, prio)
    );
}

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ *
 * +++++++++++++++++++++++++++++++++++++++++++++ CONTINUATIONS +++++++++++++++++++++++++++++++++++++++++++++ *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

namespace detail
{

// Continuation task for parallel_inner_product with non-callback continuations
template <class Continuation1, class Continuation2, class T, class BinaryOperation, class Reduce, class Job, class Enable1=void, class Enable2=void>
struct parallel_inner_product_continuation_helper: public boost::asynchronous::continuation_task<T>
{
    parallel_inner_product_continuation_helper(Continuation1 && cont1, Continuation2 && cont2, BinaryOperation op, Reduce red,
                                             long cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<T>(task_name)
        , cont1_(std::forward<Continuation1>(cont1))
        , cont2_(std::forward<Continuation2>(cont2))
        , op_(op)
        , red_(red)
        , cutoff_(cutoff)
        , prio_(prio)
    {}

    void operator()()
    {
        boost::asynchronous::continuation_result<T> task_res = this->this_task_result();
        try
        {
            auto op = std::move(op_);
            auto red = std::move(red_);
            auto cutoff = cutoff_;
            auto prio = prio_;
            auto task_name = this->get_name();
            boost::asynchronous::create_callback_continuation(
                [task_res, op, red, cutoff, prio, task_name](std::tuple<std::future<typename Continuation1::return_type>,
                                                                        std::future<typename Continuation2::return_type>> && res) mutable
                {
                    try
                    {
                        auto range1 = std::move(std::get<0>(res).get());
                        auto range2 = std::move(std::get<1>(res).get());
                        boost::asynchronous::create_callback_continuation(
                            [task_res](std::tuple<boost::asynchronous::expected<T>> && res) mutable
                            {
                                try
                                {
                                    task_res.set_value(std::move(std::get<0>(res).get()));
                                }
                                catch (std::exception& e)
                                {
                                    task_res.set_exception(std::current_exception());
                                }
                            },
                            parallel_inner_product_range_move_helper<Continuation1, Continuation2, T, BinaryOperation, Reduce, Job>(std::move(range1), std::move(range2), std::move(op), std::move(red), cutoff, task_name, prio)
                        );
                    }
                    catch(...)
                    {
                        task_res.set_exception(std::current_exception());
                    }
                },
                std::move(cont1_),
                std::move(cont2_)
            );
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
        }
    }

    Continuation1 cont1_;
    Continuation2 cont2_;
    BinaryOperation op_;
    Reduce red_;
    long cutoff_;
    std::size_t prio_;
};

// Continuation task for parallel_inner_product with callback continuations
template <class Continuation1, class Continuation2, class T, class BinaryOperation, class Reduce, class Job>
struct parallel_inner_product_continuation_helper<Continuation1, Continuation2, T, BinaryOperation, Reduce, Job,
                                                  typename std::enable_if<has_is_callback_continuation_task<Continuation1>::value>::type,
                                                  typename std::enable_if<has_is_callback_continuation_task<Continuation2>::value>::type>
    : public boost::asynchronous::continuation_task<T>
{
    parallel_inner_product_continuation_helper(Continuation1 && cont1, Continuation2 && cont2, BinaryOperation op, Reduce red,
                                               long cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<T>(task_name)
        , cont1_(std::forward<Continuation1>(cont1))
        , cont2_(std::forward<Continuation2>(cont2))
        , op_(op)
        , red_(red)
        , cutoff_(cutoff)
        , prio_(prio)
    {}

    void operator()()
    {
        boost::asynchronous::continuation_result<T> task_res = this->this_task_result();
        try
        {
            auto op = std::move(op_);
            auto red = std::move(red_);
            auto cutoff = cutoff_;
            auto prio = prio_;
            auto task_name = this->get_name();
            boost::asynchronous::create_callback_continuation(
                [task_res, op, red, cutoff, prio, task_name](std::tuple<boost::asynchronous::expected<typename Continuation1::return_type>,
                                                                        boost::asynchronous::expected<typename Continuation2::return_type>> && res) mutable
                {
                    try
                    {
                        auto range1 = std::move(std::get<0>(res).get());
                        auto range2 = std::move(std::get<1>(res).get());
                        boost::asynchronous::create_callback_continuation(
                            [task_res](std::tuple<boost::asynchronous::expected<T>> && res) mutable
                            {
                                try
                                {
                                    task_res.set_value(std::move(std::get<0>(res).get()));
                                }
                                catch (std::exception& e)
                                {
                                    task_res.set_exception(std::current_exception());
                                }
                            },
                            parallel_inner_product(std::move(range1), std::move(range2), std::move(op), std::move(red), cutoff, task_name, prio)
                        );
                    }
                    catch(...)
                    {
                        task_res.set_exception(std::current_exception());
                    }
                },
                std::move(cont1_),
                std::move(cont2_)
            );
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
        }
    }

    Continuation1 cont1_;
    Continuation2 cont2_;
    BinaryOperation op_;
    Reduce red_;
    long cutoff_;
    std::size_t prio_;
};

}

// Continuations (with starting value)
template <class Continuation1,
          class Continuation2,
          class BinaryOperation,
          class Reduce,
          class Value,
          class Enable = typename std::enable_if<!boost::has_range_iterator<Continuation1>::value && !boost::has_range_iterator<Continuation2>::value &&
                                                      boost::asynchronous::detail::has_is_continuation_task<Continuation1>::value &&
                                                      boost::asynchronous::detail::has_is_continuation_task<Continuation2>::value>::type,
          class T = decltype( // Automatically deduce return type
                        std::declval<Reduce>()(
                            std::declval<BinaryOperation>()(
                                *boost::begin(std::declval<typename Continuation1::return_type>()),
                                *boost::begin(std::declval<typename Continuation2::return_type>())
                            ),
                            std::declval<BinaryOperation>()(
                                *boost::begin(std::declval<typename Continuation1::return_type>()),
                                *boost::begin(std::declval<typename Continuation2::return_type>())
                            )
                        )
                    ),
          class Job=typename std::conditional<std::is_same<typename Continuation1::job_type, typename Continuation2::job_type>::value, typename Continuation1::job_type, BOOST_ASYNCHRONOUS_DEFAULT_JOB>::type>
typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<Continuation1>::value &&
                            boost::asynchronous::detail::has_is_continuation_task<Continuation2>::value,
                            boost::asynchronous::detail::callback_continuation<T, Job>>::type
parallel_inner_product(Continuation1 && cont1, Continuation2 && cont2, BinaryOperation op, Reduce red, const Value& value, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                       const std::string& task_name, std::size_t prio=0)
#else
                       const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::invoke(
        boost::asynchronous::top_level_callback_continuation_job<T, Job>(
            boost::asynchronous::detail::parallel_inner_product_continuation_helper<Continuation1, Continuation2, T, BinaryOperation, Reduce, Job>(std::forward<Continuation1>(cont1), std::forward<Continuation2>(cont2), op, red, cutoff, task_name, prio)
        ),
        [value, red](T const& t) {
            return red(value, t);
        }
    );
}

// Continuations (without starting value)
template <class Continuation1,
          class Continuation2,
          class BinaryOperation,
          class Reduce,
          class Enable = typename std::enable_if<!boost::has_range_iterator<Continuation1>::value && !boost::has_range_iterator<Continuation2>::value &&
                                                      boost::asynchronous::detail::has_is_continuation_task<Continuation1>::value &&
                                                      boost::asynchronous::detail::has_is_continuation_task<Continuation2>::value>::type,
          class T = decltype( // Automatically deduce return type
                        std::declval<Reduce>()(
                            std::declval<BinaryOperation>()(
                                std::declval<decltype(*boost::begin(std::declval<typename Continuation1::return_type>()))>(),
                                std::declval<decltype(*boost::begin(std::declval<typename Continuation2::return_type>()))>()
                            ),
                            std::declval<BinaryOperation>()(
                                std::declval<decltype(*boost::begin(std::declval<typename Continuation1::return_type>()))>(),
                                std::declval<decltype(*boost::begin(std::declval<typename Continuation2::return_type>()))>()
                            )
                        )
                    ),
          class Job=typename std::conditional<std::is_same<typename Continuation1::job_type, typename Continuation2::job_type>::value, typename Continuation1::job_type, BOOST_ASYNCHRONOUS_DEFAULT_JOB>::type>
typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<Continuation1>::value &&
                            boost::asynchronous::detail::has_is_continuation_task<Continuation2>::value,
                            boost::asynchronous::detail::callback_continuation<T, Job>>::type
parallel_inner_product(Continuation1 && cont1, Continuation2 && cont2, BinaryOperation op, Reduce red, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                       const std::string& task_name, std::size_t prio=0)
#else
                       const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<T, Job>(
        boost::asynchronous::detail::parallel_inner_product_continuation_helper<Continuation1, Continuation2, T, BinaryOperation, Reduce, Job>(std::forward<Continuation1>(cont1), std::forward<Continuation2>(cont2), op, red, cutoff, task_name, prio)
    );
}

}}

#endif // BOOST_ASYNCHRONOUS_PARALLEL_INNER_PRODUCT_HPP
