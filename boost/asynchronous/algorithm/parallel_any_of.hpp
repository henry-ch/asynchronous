// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2014
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_ANY_OF_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_ANY_OF_HPP

#include <algorithm>
#include <utility>
#include <type_traits>
#include <iterator>


#include <type_traits>
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

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>

namespace boost { namespace asynchronous
{
namespace detail
{
struct any_of_operator
{
    template <class Iterator, class Func>
    bool algorithm(Iterator beg, Iterator end, Func& f)
    {
        return std::any_of(beg,end,f);
    }
    bool merge(bool b1, bool b2)const
    {
        return b1 || b2;
    }
};
}
// version for iterators
template <class Iterator, class Func,
          class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<bool,Job>
parallel_any_of(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio=0)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<bool,Job>
            (boost::asynchronous::detail::parallel_all_of_helper<Iterator,Func,boost::asynchronous::detail::any_of_operator,Job>
                (beg,end,func,cutoff,task_name,prio));
}

// version for ranges returned as continuations
namespace detail
{
template <class Continuation, class Func,class Job>
struct parallel_any_of_continuation_range_helper: public boost::asynchronous::continuation_task<bool>
{
    parallel_any_of_continuation_range_helper(Continuation c,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<bool>(task_name)
        , cont_(std::move(c)),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<bool> task_res = this->this_task_result();
        try
        {
            auto func(std::move(func_));
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            cont_.on_done([task_res,func,cutoff,task_name,prio]
                          (std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& continuation_res) mutable
            {
                try
                {
                    auto res = std::make_shared<typename Continuation::return_type>(std::move(std::get<0>(continuation_res).get()));
                    auto new_continuation = boost::asynchronous::parallel_any_of
                            <decltype(boost::begin(std::declval<typename Continuation::return_type>())), Func, Job>
                                (boost::begin(*res),boost::end(*res),func,cutoff,task_name,prio);
                    new_continuation.on_done([res,task_res](std::tuple<boost::asynchronous::expected<bool> >&& new_continuation_res)
                    {
                        task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                    });
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(std::make_exception_ptr(e));
                }
            });
        }
        catch(std::exception& e)
        {
            task_res.set_exception(std::make_exception_ptr(e));
        }
    }
    Continuation cont_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};

}
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<Range>::value,boost::asynchronous::detail::callback_continuation<bool,Job> >::type
parallel_any_of(Range range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio=0)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<bool,Job>
            (boost::asynchronous::detail::parallel_any_of_continuation_range_helper<Range,Func,Job>(range,std::move(func),cutoff,task_name,prio));
}
}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_ANY_OF_HPP

