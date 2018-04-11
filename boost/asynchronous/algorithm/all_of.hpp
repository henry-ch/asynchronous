// Boost.Asynchronous library
//  Copyright (C) Tobias Holl 2018
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_ALL_OF_CONT_HPP
#define BOOST_ASYNCHRONOUS_ALL_OF_CONT_HPP

#include <atomic>
#include <memory>
#include <tuple>
#include <type_traits>

#include <boost/mpl/and.hpp>

#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/detail/job_type_from.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/algorithm/detail/helpers.hpp>

namespace boost
{
namespace asynchronous
{

namespace detail
{

template <typename ResultTuple>
struct all_of_task_counter
{
    all_of_task_counter()
        : task_count(std::tuple_size<ResultTuple>::value)
    {}

    ResultTuple         values;
    std::atomic<size_t> task_count;
};

template <typename... Continuations>
struct all_of_return_type
{
    // Replace void by boost::asynchronous::detail::void_wrapper, because void is not allowed in tuples
    using type = std::tuple<typename boost::asynchronous::detail::wrap<typename Continuations::return_type>::type...>;
};

template <typename Job, typename... Continuations>
struct all_of_helper : public boost::asynchronous::continuation_task<typename boost::asynchronous::detail::all_of_return_type<Continuations...>::type>
{
    using return_type = typename boost::asynchronous::detail::all_of_return_type<Continuations...>::type;
    using task_counter_type = boost::asynchronous::detail::all_of_task_counter<return_type>;

    all_of_helper(Continuations... continuations)
        : boost::asynchronous::continuation_task<return_type>("boost::asynchronous::all_of(...)")
        , continuations_(std::move(continuations)...)
    {}

    // Dispatch recursion
    template <size_t Index>
    typename std::enable_if<(Index > 0)>::type dispatch_next(std::shared_ptr<task_counter_type> task_counter, boost::asynchronous::continuation_result<return_type> task_res)
    {
        dispatch<Index>(task_counter, task_res);
        dispatch_next<Index - 1>(task_counter, task_res);
    }

    template <size_t Index>
    typename std::enable_if<(Index == 0)>::type dispatch_next(std::shared_ptr<task_counter_type> task_counter, boost::asynchronous::continuation_result<return_type> task_res)
    {
        dispatch<Index>(task_counter, task_res);
        // End of recursion.
    }

    // Store a non-void result
    template <size_t Index>
    typename std::enable_if<
        !std::is_same<typename std::tuple_element<Index, std::tuple<Continuations...>>::type::return_type, void>::value
    >::type
    dispatch(std::shared_ptr<task_counter_type> task_counter, boost::asynchronous::continuation_result<return_type> task_res)
    {
        std::get<Index>(continuations_).on_done(
            [task_counter, task_res](std::tuple<boost::asynchronous::expected<typename std::tuple_element<Index, std::tuple<Continuations...>>::type::return_type>>&& args)
            {
                try
                {
                    std::get<Index>(task_counter->values) = std::move(std::get<0>(args).get());
                    size_t remaining = --(task_counter->task_count);
                    if (remaining == 0)
                    {
                        task_res.set_value(std::move(task_counter->values));
                    }
                }
                catch (...)
                {
                    task_res.set_exception(std::current_exception());
                }
            }
        );
    }

    // "Store" a void result
    template <size_t Index>
    typename std::enable_if<
        std::is_same<typename std::tuple_element<Index, std::tuple<Continuations...>>::type::return_type, void>::value
    >::type
    dispatch(std::shared_ptr<task_counter_type> task_counter, boost::asynchronous::continuation_result<return_type> task_res)
    {
        std::get<Index>(continuations_).on_done(
            [task_counter, task_res](std::tuple<boost::asynchronous::expected<typename std::tuple_element<Index, std::tuple<Continuations...>>::type::return_type>>&& args)
            {
                try
                {
                    std::get<0>(args).get();
                    std::get<Index>(task_counter->values) = boost::asynchronous::detail::void_wrapper {};
                    size_t remaining = --(task_counter->task_count);
                    if (remaining == 0)
                    {
                        task_res.set_value(std::move(task_counter->values));
                    }
                }
                catch (...)
                {
                    task_res.set_exception(std::current_exception());
                }
            }
        );
    }

    void operator()()
    {
        boost::asynchronous::continuation_result<return_type> task_res = this->this_task_result();
        auto task_counter = std::make_shared<task_counter_type>();

        dispatch_next<sizeof...(Continuations) - 1>(task_counter, task_res);
    }

    std::tuple<Continuations...> continuations_;
};

} // namespace detail

template <typename Job, typename... Continuations>
typename std::enable_if<
    boost::mpl::and_<boost::asynchronous::detail::has_is_continuation_task<Continuations>...>::type::value,
    boost::asynchronous::detail::callback_continuation<
        typename boost::asynchronous::detail::all_of_return_type<Continuations...>::type,
        Job
    >
>::type
all_of_job(Continuations... continuations)
{
    return boost::asynchronous::top_level_callback_continuation_job<typename boost::asynchronous::detail::all_of_return_type<Continuations...>::type, Job>(
        boost::asynchronous::detail::all_of_helper<Job, Continuations...>(std::move(continuations)...)
    );
}

template <typename... Continuations>
auto all_of(Continuations... continuations)
    -> decltype(all_of_job<typename boost::asynchronous::detail::job_type_from<Continuations...>::type, Continuations...>(std::move(continuations)...))
{
    return all_of_job<typename boost::asynchronous::detail::job_type_from<Continuations...>::type, Continuations...>(std::move(continuations)...);
}

} // namespace asynchronous
} // namespace boost

#endif // BOOST_ASYNCHRONOUS_ALL_OF_CONT_HPP
