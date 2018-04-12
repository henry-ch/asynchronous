// Boost.Asynchronous library
//  Copyright (C) Tobias Holl 2018
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_ANY_OF_CONT_HPP
#define BOOST_ASYNCHRONOUS_ANY_OF_CONT_HPP

#include <limits>
#include <memory>
#include <tuple>
#include <type_traits>
#include <vector>

#include <boost/mpl/contains.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/variant.hpp>

#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/exceptions.hpp>
#include <boost/asynchronous/algorithm/detail/helpers.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/detail/job_type_from.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>

namespace boost
{
namespace asynchronous
{

// In all_of, it is simple enough to just return a tuple of values. However, any_of can only return one value, so there are a number of options that we can pursue:
// Either, we use a variant type (like Boost.Variant) to encapsulate all the different possible return types. Unfortunately, there are use cases where we need to
// know which of the continuations returned, even if multiple of them have the same return type. A better solution would be to store an index into a tuple, but if
// the tuple contains large types with slow default constructors, we suffer a significant performance penalty just from creating the type. Additionally, std::tuple
// only allows compile-time indexing - not the runtime indexing we need for any_of because the target index only becomes known when a continuation finishes.
// We need some form of type erasure (either through Boost.Variant or Boost.Any) *and* the index of the finished continuation to provide the users with what they
// need from any_of.
// There is an additional complication with Boost.Variant because it may only contain *distinct* types, which we need to fix manually

namespace detail
{

// Filter out non-distinct types, and void, from a boost::variant<Ts...>
template <typename Complete, typename... Ts>
struct distinct_variant_impl
{
    using type = Complete;
};

template <typename... Cs, typename T, typename... Ts>
struct distinct_variant_impl<boost::variant<Cs...>, T, Ts...>
{
private:
    // Iff T is not equal to any of Cs..., add it.
    template <typename U>
    static typename std::enable_if<
        !boost::mpl::contains<typename boost::variant<Cs...>::types, typename std::remove_cv<U>::type>::type::value,
        typename distinct_variant_impl<boost::variant<Cs..., typename std::remove_cv<U>::type>, Ts...>::type
    >::type next();

    template <typename U>
    static typename std::enable_if<
        boost::mpl::contains<typename boost::variant<Cs...>::types, typename std::remove_cv<U>::type>::type::value,
        typename distinct_variant_impl<boost::variant<Cs...>, Ts...>::type
    >::type next();

public:
    using type = decltype(next<T>());
};


template <typename... Cs, typename... Ts>
struct distinct_variant_impl<boost::variant<Cs...>, void, Ts...>
{
    using type = typename distinct_variant_impl<boost::variant<Cs...>, Ts...>::type;
};

template <typename... Ts> struct distinct_variant;

template <typename T, typename... Ts>
struct distinct_variant<T, Ts...>
{
    using type = typename distinct_variant_impl<boost::variant<T>, Ts...>::type;
};

template <typename... Ts>
struct distinct_variant<void, Ts...>
{
    using type = typename distinct_variant<Ts...>::type;
};

template <>
struct distinct_variant<>
{
    // There were only void types - use void_wrapper as a dummy type.
    using type = boost::variant<boost::asynchronous::detail::void_wrapper>;
};

} // namespace detail

// The return type of any_of stores the index of the returning continuation, and a variant containing the actual value.
template <typename... Types>
struct any_of_result
{
    // The number of continuations that contributed to this result
    constexpr static size_t count = sizeof...(Types);
    // Boost.Variant needs distinct types, so we need to resolve what variant we actually want here
    using variant_type = typename boost::asynchronous::detail::distinct_variant<Types...>::type;

    // The index of the continuation that returned (if any)
    size_t& index() { return m_index; }
    const size_t& index() const { return m_index; }

    // The result of that continuation
    variant_type& value() { return m_value; }
    const variant_type& value() const { return m_value; }

private:
    size_t       m_index = std::numeric_limits<size_t>::max();
    variant_type m_value;
};

namespace detail
{

// The result storage. We want to fail with an exception if and only if all tasks failed, which is why we still need to store the full number of tasks
template <typename Storage>
struct any_of_task_counter
{
    any_of_task_counter()
        : exceptions(Storage::count)
        , failures_remaining(Storage::count)
    {}

    Storage storage;
    boost::asynchronous::combined_exception exceptions;
    std::atomic<size_t> failures_remaining;
    std::atomic_flag has_value;

    template <size_t Index, typename T>
    bool set(T&& t)
    {
        bool was_empty = !has_value.test_and_set();
        if (was_empty)
        {
            storage.index() = Index;
            storage.value() = std::forward<T>(t);
        }
        return was_empty;
    }

    template <size_t Index>
    bool set()
    {
        bool was_empty = !has_value.test_and_set();
        if (was_empty)
            storage.index() = Index;
        return was_empty;
    }

    template <size_t Index>
    size_t store_exception(std::exception_ptr exception)
    {
        exceptions.underlying_exceptions()[Index] = exception;
        return --failures_remaining;
    }
};

template <typename Job, typename... Continuations>
struct any_of_helper : public boost::asynchronous::continuation_task<boost::asynchronous::any_of_result<typename Continuations::return_type...>>
{
    using return_type = boost::asynchronous::any_of_result<typename Continuations::return_type...>;
    using task_counter_type = boost::asynchronous::detail::any_of_task_counter<return_type>;

    any_of_helper(Continuations... continuations)
        : boost::asynchronous::continuation_task<return_type>("boost::asynchronous::any_of(...)")
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
                    bool done = task_counter->template set<Index>(std::move(std::get<0>(args).get()));
                    if (done)
                        task_res.set_value(std::move(task_counter->storage));
                    // TODO: On completion, interrupt other continuations if possible
                }
                catch (...)
                {
                    // Store the exception_ptr. If all tasks failed, throw the exception storage with all underlying exceptions
                    size_t failures_remaining = task_counter->template store_exception<Index>(std::current_exception());
                    if (failures_remaining == 0)
                        task_res.set_exception(ASYNCHRONOUS_CREATE_EXCEPTION(task_counter->exceptions));
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
                    bool done = task_counter->template set<Index>();
                    if (done)
                        task_res.set_value(std::move(task_counter->storage));
                    // TODO: On completion, interrupt other continuations if possible
                }
                catch (...)
                {
                    // Store the exception_ptr. If all tasks failed, throw the exception storage with all underlying exceptions
                    size_t failures_remaining = task_counter->template store_exception<Index>(std::current_exception());
                    if (failures_remaining == 0)
                        task_res.set_exception(ASYNCHRONOUS_CREATE_EXCEPTION(task_counter->exceptions));
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

// When using any_of, manage object lifetimes carefully - any_of may return a value before all continuations have finished.

template <typename Job, typename... Continuations>
typename std::enable_if<
    boost::mpl::and_<boost::asynchronous::detail::has_is_continuation_task<Continuations>...>::type::value,
    boost::asynchronous::detail::callback_continuation<
        boost::asynchronous::any_of_result<typename Continuations::return_type...>,
        Job
    >
>::type
any_of_job(Continuations... continuations)
{
    return boost::asynchronous::top_level_callback_continuation_job<boost::asynchronous::any_of_result<typename Continuations::return_type...>, Job>(
        boost::asynchronous::detail::any_of_helper<Job, Continuations...>(std::move(continuations)...)
    );
}

template <typename... Continuations>
auto any_of(Continuations... continuations)
    -> decltype(any_of_job<typename boost::asynchronous::detail::job_type_from<Continuations...>::type, Continuations...>(std::move(continuations)...))
{
    return any_of_job<typename boost::asynchronous::detail::job_type_from<Continuations...>::type, Continuations...>(std::move(continuations)...);
}

} // namespace asynchronous
} // namespace boost

#endif // BOOST_ASYNCHRONOUS_ANY_OF_CONT_HPP
