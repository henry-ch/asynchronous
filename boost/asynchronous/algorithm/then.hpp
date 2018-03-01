// Boost.Asynchronous library
//  Copyright (C) Christophe Henry, Tobias Holl 2018
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_THEN_HPP
#define BOOST_ASYNCHRONOUS_THEN_HPP

#include <cstddef>
#include <functional>
#include <type_traits>

#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>
#include <boost/asynchronous/continuation_task.hpp>

namespace boost
{
namespace asynchronous
{
namespace detail
{

// Wrapper to avoid void and void&& argument types
struct void_wrapper {};

template <class T> struct wrap { using type = T; };
template <> struct wrap<void> { using type = void_wrapper; };

// The result of applying the functor to the given continuation
template <class Continuation, class Functor>
struct then_traits
{
    // Handle functors taking boost::asynchronous::expected
    using InputType       = typename Continuation::return_type;
    using FunctorArgument = boost::asynchronous::expected<InputType>;

    // Functor return type
#if __cpp_lib_is_invocable
    // C++17 and upwards: std::result_of is deprecated by P0604R0
    using FunctorReturnType                  = std::invoke_result_t<Functor, FunctorArgument>;
#else
    using FunctorReturnType                  = typename std::result_of<Functor(FunctorArgument)>::type;
#endif
    using WrappedFunctorReturnType           = typename wrap<FunctorReturnType>::type;
    constexpr static bool FunctorReturnsVoid = std::is_same<FunctorReturnType, void>::value;

    // If the functor returns a continuation, handle that gracefully.
    using ReturnType                                 = typename boost::asynchronous::detail::get_return_type_if_possible_continuation<FunctorReturnType>::type;
    constexpr static bool FunctorReturnsContinuation = boost::asynchronous::detail::has_is_continuation_task<FunctorReturnType>::value;

    // Handle void returns
    constexpr static bool ReturnsVoid = std::is_same<ReturnType, void>::value;
};

// Move the argument only if it is move-constructible and -assignable
// This works like std::move_if_noexcept.
template <class T>
typename std::conditional<
    std::is_move_constructible<T>::value && std::is_move_assignable<T>::value,
    typename std::remove_reference<T>::type&&,
    const T&
>::type
move_where_possible(T& t) noexcept
{
    return std::move(t);
}

// Implementation of boost::asynchronous::then
template <class Continuation,
          class Functor,
          class Traits = boost::asynchronous::detail::then_traits<Continuation, Functor>>
struct then_helper : public boost::asynchronous::continuation_task<typename Traits::ReturnType>
{
    then_helper(Continuation cont, Functor func, std::string task_name)
        : boost::asynchronous::continuation_task<typename Traits::ReturnType>(std::move(task_name))
        , continuation_(boost::asynchronous::detail::move_where_possible(cont))
        , functor_(boost::asynchronous::detail::move_where_possible(func))
    {}

    // INVOKE: Call the functor. Void results are turned into a void_wrapper.

    // Version for functors returning something (including void continuations)
    template <bool FunctorReturnsVoid>
    static typename std::enable_if<!FunctorReturnsVoid, typename Traits::WrappedFunctorReturnType>::type invoke_then_functor(Functor functor, typename Traits::FunctorArgument arg)
    {
#if __cpp_lib_invoke
        return std::invoke(
            boost::asynchronous::detail::move_where_possible(functor),
            boost::asynchronous::detail::move_where_possible(arg)
        );
#else
        return functor(boost::asynchronous::detail::move_where_possible(arg));
#endif
    }

    // Version for functors returning void
    template <bool FunctorReturnsVoid>
    static typename std::enable_if<FunctorReturnsVoid, typename Traits::WrappedFunctorReturnType>::type invoke_then_functor(Functor functor, typename Traits::FunctorArgument arg)
    {
#if __cpp_lib_invoke
        std::invoke(
            boost::asynchronous::detail::move_where_possible(functor),
            boost::asynchronous::detail::move_where_possible(arg)
        );
#else
        functor(boost::asynchronous::detail::move_where_possible(arg));
#endif
        return void_wrapper{};
    }



    // DISPATCH: Set the task result properly after the functor has been called

    // Version for functors that return normal continuations
    template <bool FunctorReturnsContinuation, bool ThenReturnsVoid>
    static typename std::enable_if<FunctorReturnsContinuation && !ThenReturnsVoid>::type dispatch(typename Traits::WrappedFunctorReturnType&& result, boost::asynchronous::continuation_result<typename Traits::ReturnType>& task_res)
    {
        result.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Traits::ReturnType>> inner_result) mutable
        {
            try
            {
                task_res.set_value(boost::asynchronous::detail::move_where_possible(std::get<0>(inner_result).get()));
            }
            catch (...)
            {
                task_res.set_exception(std::current_exception());
            }
        });
    }

    // Version for functors that return void continuations
    template <bool FunctorReturnsContinuation, bool ThenReturnsVoid>
    static typename std::enable_if<FunctorReturnsContinuation && ThenReturnsVoid>::type dispatch(typename Traits::WrappedFunctorReturnType&& result, boost::asynchronous::continuation_result<typename Traits::ReturnType>& task_res)
    {
        result.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Traits::ReturnType>> inner_result) mutable
        {
            try
            {
                std::get<0>(inner_result).get();
                task_res.set_value();
            }
            catch (...)
            {
                task_res.set_exception(std::current_exception());
            }
        });
    }

    // Version for normal functors
    template <bool FunctorReturnsContinuation, bool ThenReturnsVoid, class Value>
    static typename std::enable_if<!FunctorReturnsContinuation && !ThenReturnsVoid>::type dispatch(Value&& result, boost::asynchronous::continuation_result<typename Traits::ReturnType>& task_res)
    {
        task_res.set_value(std::forward<Value>(result));
    }

    // Version for functors returning void
    template <bool FunctorReturnsContinuation, bool ThenReturnsVoid>
    static typename std::enable_if<!FunctorReturnsContinuation && ThenReturnsVoid>::type dispatch(void_wrapper, boost::asynchronous::continuation_result<typename Traits::ReturnType>& task_res)
    {
        task_res.set_value();
    }


    // OPERATOR: Main logic. Attaches an on_done handler to the input continuation that invokes the functor and sets the result of this continuation.

    void operator()()
    {
        auto task_res = this->this_task_result();
        auto functor = boost::asynchronous::detail::move_where_possible(functor_);
        try
        {
            continuation_.on_done(
#if __cpp_init_captures
                [task_res, functor = boost::asynchronous::detail::move_where_possible(functor)](std::tuple<boost::asynchronous::expected<typename Traits::InputType>>&& res) mutable
#else
                [task_res, functor](std::tuple<boost::asynchronous::expected<typename Traits::InputType>>&& res) mutable
#endif
                {
                    try
                    {
                        dispatch<Traits::FunctorReturnsContinuation, Traits::ReturnsVoid>(
                            invoke_then_functor<Traits::FunctorReturnsVoid>(
                                boost::asynchronous::detail::move_where_possible(functor),
                                std::move(std::get<0>(res))
                            ),
                            task_res
                        );
                    }
                    catch (...)
                    {
                        task_res.set_exception(std::current_exception());
                    }
                }
            );
        }
        catch (...)
        {
            task_res.set_exception(std::current_exception());
        }
    }

    Continuation continuation_;
    Functor      functor_;
};

} // namespace detail

template <class Continuation, class Functor, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<
    boost::asynchronous::detail::has_is_continuation_task<Continuation>::value,
    boost::asynchronous::detail::callback_continuation<
        typename boost::asynchronous::detail::then_traits<Continuation, Functor>::ReturnType,
        Job
    >
>::type
then(Continuation continuation, Functor func,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name)
#else
             const std::string& task_name="")
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<typename boost::asynchronous::detail::then_traits<Continuation, Functor>::ReturnType, Job>(
        boost::asynchronous::detail::then_helper<Continuation, Functor>(
            boost::asynchronous::detail::move_where_possible(continuation),
            boost::asynchronous::detail::move_where_possible(func),
            task_name
        )
    );
}

}} // namespace boost::asynchronous

#endif // BOOST_ASYNCHRONOUS_THEN_HPP
