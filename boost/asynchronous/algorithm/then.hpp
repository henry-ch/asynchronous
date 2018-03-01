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
#include <boost/asynchronous/continuation_task.hpp>

namespace boost
{
namespace asynchronous
{
namespace detail
{

// Wrapper to avoid void&
struct rref_wrapper_void {};
template <class T>
struct rref_wrapper
{
    using type = T&&;
};
template <>
struct rref_wrapper<void>
{
    using type = rref_wrapper_void&&;
};

// Can the functor be invoked with the specified arguments?
template <class F, class... Args>
struct is_invocable_wrapper
{
#if __cpp_lib_is_invocable
    // C++17 and upwards
    constexpr static bool value = std::is_invocable_v<F, Args...>;
#else
private:
    static decltype(std::declval<F>()(std::declval<Args>()...), std::true_type{}) invocable_test(std::nullptr_t);
    static std::false_type invocable_test(...);
    using value_type = decltype(invocable_test(nullptr));

public:
    constexpr static bool value = value_type::value;
#endif
};

// Can we move arguments into the functor
template <class F, class Arg>
struct can_move_into_functor_helper
{
    constexpr static bool value = boost::asynchronous::detail::is_invocable_wrapper<F, typename std::remove_reference<Arg>::type&&>::value;
};
template <class F>
struct can_move_into_functor_helper<F, void> : std::false_type {};

// Get the preferred argument type (moved or not-moved)
template <class Arg, bool PreferMove = true>
struct functor_argument_type_helper
{
    using type = typename std::conditional<PreferMove, typename std::remove_reference<Arg>::type&&, Arg>::type;
};
template <bool PreferMove>
struct functor_argument_type_helper<void, PreferMove>
{
    using type = void;
};

// Get the functor return type
template <bool VoidInput, typename Functor, typename Arg>
struct functor_return_type_helper
{
#if __cpp_lib_is_invocable
    // C++17 and upwards: std::result_of is deprecated by P0604R0
    using type = std::invoke_result_t<Functor, Arg>;
#else
    using type = typename std::result_of<Functor(Arg)>::type;
#endif
};
template <typename Functor, typename Arg>
struct functor_return_type_helper<true, Functor, Arg>
{
#if __cpp_lib_is_invocable
    // C++17 and upwards: std::result_of is deprecated by P0604R0
    using type = std::invoke_result_t<Functor>;
#else
    using type = typename std::result_of<Functor()>::type;
#endif
};

// Get the functor return type
template <bool IsContinuation, typename T>
struct continuation_return_type_helper
{
    using type = T;
};
template <typename T>
struct continuation_return_type_helper<true, T>
{
    using type = typename T::return_type;
};

// The result of applying the functor to the given continuation
template <class Continuation, class Functor>
struct result_of_then
{
    // Handle continuations returning void
    constexpr static bool input_is_void = std::is_same<typename Continuation::return_type, void>::value;

    // Handle types that cannot be moved
    constexpr static bool can_move_into_functor = can_move_into_functor_helper<Functor, typename Continuation::return_type>::value;

    // Argument passed to the functor if not void
    using functor_argument_type = typename functor_argument_type_helper<typename Continuation::return_type, can_move_into_functor>::type;

    // Functor return type
    using functor_return_type = typename functor_return_type_helper<input_is_void, Functor, functor_argument_type>::type;

    // If the functor returns a continuation, handle that gracefully.
    constexpr static bool functor_returns_continuation = boost::asynchronous::detail::has_is_continuation_task<functor_return_type>::value;
    using return_type = typename continuation_return_type_helper<functor_returns_continuation, functor_return_type>::type;

    // Handle void returns
    constexpr static bool output_is_void = std::is_same<return_type, void>::value;
};

// Moves the argument if and only if the condition is true
// This works like std::move_if_noexcept.
template <bool Condition, class T>
typename std::conditional<Condition, typename std::remove_reference<T>::type&&, const T&>::type
move_if(T& t) noexcept
{
    return std::move(t);
}

// Move the argument only if it is move-constructible and -assignable
template <class T>
auto move_where_possible(T& t) noexcept -> decltype(boost::asynchronous::detail::move_if<std::is_move_constructible<T>::value && std::is_move_assignable<T>::value>(t))
{
    return boost::asynchronous::detail::move_if<std::is_move_constructible<T>::value && std::is_move_assignable<T>::value>(t);
}

// Move if possible, with an extra condition
template <bool Condition, class T>
auto move_where_possible_if(T& t) noexcept -> decltype(boost::asynchronous::detail::move_if<std::is_move_constructible<T>::value && std::is_move_assignable<T>::value && Condition>(t))
{
    return boost::asynchronous::detail::move_if<std::is_move_constructible<T>::value && std::is_move_assignable<T>::value && Condition>(t);
}

// Implementation of boost::asynchronous::then
template <class Continuation,
          class Functor,
          class ResultOfThen = boost::asynchronous::detail::result_of_then<Continuation, Functor>>
struct then_helper : public boost::asynchronous::continuation_task<typename ResultOfThen::return_type>
{
    using InputType         = typename Continuation::return_type;         // Return type of the input continuation
    using FunctorReturnType = typename ResultOfThen::functor_return_type; // Return type of the functor
    using ReturnType        = typename ResultOfThen::return_type;         // Return type of this continuation (special-cased for functors returning continuations)

    constexpr static bool HasVoidInput         = ResultOfThen::input_is_void;
    constexpr static bool HasVoidIntermediate  = std::is_same<FunctorReturnType, void>::value;
    constexpr static bool HasVoidOutput        = ResultOfThen::output_is_void;
    constexpr static bool HasInnerContinuation = ResultOfThen::functor_returns_continuation;
    constexpr static bool CanMoveIntoFunctor   = ResultOfThen::can_move_into_functor;

    then_helper(Continuation cont, Functor func, std::string task_name)
        : boost::asynchronous::continuation_task<ReturnType>(std::move(task_name))
        , continuation_(boost::asynchronous::detail::move_where_possible(cont))
        , functor_(boost::asynchronous::detail::move_where_possible(func))
    {}

    // DISPATCH: Set the task result properly after the functor has been called

    // Version for functors that return normal continuations
    template <bool InnerContinuation, bool VoidOutput>
    static typename std::enable_if<InnerContinuation && !VoidOutput>::type dispatch(typename boost::asynchronous::detail::rref_wrapper<FunctorReturnType>::type result, boost::asynchronous::continuation_result<ReturnType>& task_res)
    {
        result.on_done([task_res](std::tuple<boost::asynchronous::expected<ReturnType>> inner_result) mutable
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
    template <bool InnerContinuation, bool VoidOutput>
    static typename std::enable_if<InnerContinuation && VoidOutput>::type dispatch(typename boost::asynchronous::detail::rref_wrapper<FunctorReturnType>::type result, boost::asynchronous::continuation_result<ReturnType>& task_res)
    {
        result.on_done([task_res](std::tuple<boost::asynchronous::expected<ReturnType>> inner_result) mutable
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
    template <bool InnerContinuation, bool VoidOutput, class Value>
    static typename std::enable_if<!InnerContinuation && !VoidOutput>::type dispatch(Value&& result, boost::asynchronous::continuation_result<ReturnType>& task_res)
    {
        task_res.set_value(std::forward<Value>(result));
    }

    // Version for functors returning void
    template <bool InnerContinuation, bool VoidOutput>
    static typename std::enable_if<!InnerContinuation && VoidOutput>::type dispatch(boost::asynchronous::continuation_result<ReturnType>& task_res)
    {
        task_res.set_value();
    }

#if __cpp_lib_invoke
// C++17 and upwards allows the use of more callable types here via std::invoke
#define BOOST_ASYNCHRONOUS_CALL_THEN_FUNCTOR_1(functor) \
    std::invoke( \
        boost::asynchronous::detail::move_where_possible(functor) \
    )
#define BOOST_ASYNCHRONOUS_CALL_THEN_FUNCTOR_2(functor, arg) \
    std::invoke( \
        boost::asynchronous::detail::move_where_possible(functor), \
        boost::asynchronous::detail::move_where_possible_if<CanMoveIntoFunctor>(arg) \
    )
#else
// C++11 equivalent
#define BOOST_ASYNCHRONOUS_CALL_THEN_FUNCTOR_1(functor) \
    functor()
#define BOOST_ASYNCHRONOUS_CALL_THEN_FUNCTOR_2(functor, arg) \
    functor( \
        boost::asynchronous::detail::move_where_possible_if<CanMoveIntoFunctor>(arg) \
    )
#endif

// Usual preamble
#if __cpp_init_captures
#define BOOST_ASYNCHRONOUS_THEN_IMPL_MOVE_FUNCTOR = boost::asynchronous::detail::move_where_possible(functor)
#else
#define BOOST_ASYNCHRONOUS_THEN_IMPL_MOVE_FUNCTOR
#endif
#define BOOST_ASYNCHRONOUS_THEN_IMPL_PREAMBLE \
    auto task_res = this->this_task_result(); \
    auto functor = boost::asynchronous::detail::move_where_possible(functor_); \
    try \
    { \
        continuation_.on_done( \
            [task_res, functor BOOST_ASYNCHRONOUS_THEN_IMPL_MOVE_FUNCTOR](std::tuple<boost::asynchronous::expected<InputType>>&& res) mutable \
            { \
                try \
                {

// Handle input data (from the continuation)
#define BOOST_ASYNCHRONOUS_THEN_IMPL_DISCARD_INPUT     std::get<0>(res).get();
#define BOOST_ASYNCHRONOUS_THEN_IMPL_GET_NONVOID_INPUT InputType input(boost::asynchronous::detail::move_where_possible(std::get<0>(res).get()));

// Call the functor
#define BOOST_ASYNCHRONOUS_THEN_IMPL_CALL_WITHOUT_INPUT BOOST_ASYNCHRONOUS_CALL_THEN_FUNCTOR_1(functor)
#define BOOST_ASYNCHRONOUS_THEN_IMPL_CALL_WITH_INPUT    BOOST_ASYNCHRONOUS_CALL_THEN_FUNCTOR_2(functor, input)

// Handle output
#define BOOST_ASYNCHRONOUS_THEN_IMPL_NONVOID_DISPATCH(call) dispatch<HasInnerContinuation, HasVoidOutput>(call, task_res);
#define BOOST_ASYNCHRONOUS_THEN_IMPL_VOID_DISPATCH(call)    call; dispatch<HasInnerContinuation, HasVoidOutput>(task_res);

// Cleanup
#define BOOST_ASYNCHRONOUS_THEN_IMPL_EPILOGUE \
                } \
                catch (...) \
                { \
                    task_res.set_exception(std::current_exception()); \
                } \
            } \
        ); \
    } \
    catch (...) \
    { \
        task_res.set_exception(std::current_exception()); \
    }

    template <bool VoidInput, bool VoidIntermediate>
    typename std::enable_if<VoidInput && VoidIntermediate>::type impl()
    {
        BOOST_ASYNCHRONOUS_THEN_IMPL_PREAMBLE
        BOOST_ASYNCHRONOUS_THEN_IMPL_DISCARD_INPUT
        BOOST_ASYNCHRONOUS_THEN_IMPL_VOID_DISPATCH(BOOST_ASYNCHRONOUS_THEN_IMPL_CALL_WITHOUT_INPUT)
        BOOST_ASYNCHRONOUS_THEN_IMPL_EPILOGUE
    }

    template <bool VoidInput, bool VoidIntermediate>
    typename std::enable_if<VoidInput && !VoidIntermediate>::type impl()
    {
        BOOST_ASYNCHRONOUS_THEN_IMPL_PREAMBLE
        BOOST_ASYNCHRONOUS_THEN_IMPL_DISCARD_INPUT
        BOOST_ASYNCHRONOUS_THEN_IMPL_NONVOID_DISPATCH(BOOST_ASYNCHRONOUS_THEN_IMPL_CALL_WITHOUT_INPUT)
        BOOST_ASYNCHRONOUS_THEN_IMPL_EPILOGUE
    }

    template <bool VoidInput, bool VoidIntermediate>
    typename std::enable_if<!VoidInput && VoidIntermediate>::type impl()
    {
        BOOST_ASYNCHRONOUS_THEN_IMPL_PREAMBLE
        BOOST_ASYNCHRONOUS_THEN_IMPL_GET_NONVOID_INPUT
        BOOST_ASYNCHRONOUS_THEN_IMPL_VOID_DISPATCH(BOOST_ASYNCHRONOUS_THEN_IMPL_CALL_WITH_INPUT)
        BOOST_ASYNCHRONOUS_THEN_IMPL_EPILOGUE
    }

    template <bool VoidInput, bool VoidIntermediate>
    typename std::enable_if<!VoidInput && !VoidIntermediate>::type impl()
    {
        BOOST_ASYNCHRONOUS_THEN_IMPL_PREAMBLE
        BOOST_ASYNCHRONOUS_THEN_IMPL_GET_NONVOID_INPUT
        BOOST_ASYNCHRONOUS_THEN_IMPL_NONVOID_DISPATCH(BOOST_ASYNCHRONOUS_THEN_IMPL_CALL_WITH_INPUT)
        BOOST_ASYNCHRONOUS_THEN_IMPL_EPILOGUE
    }

    void operator()()
    {
        this->impl<HasVoidInput, HasVoidIntermediate>();
    }

#undef BOOST_ASYNCHRONOUS_CALL_THEN_FUNCTOR_1
#undef BOOST_ASYNCHRONOUS_CALL_THEN_FUNCTOR_2
#undef BOOST_ASYNCHRONOUS_THEN_IMPL_MOVE_FUNCTOR
#undef BOOST_ASYNCHRONOUS_THEN_IMPL_PREAMBLE
#undef BOOST_ASYNCHRONOUS_THEN_IMPL_DISCARD_INPUT
#undef BOOST_ASYNCHRONOUS_THEN_IMPL_GET_NONVOID_INPUT
#undef BOOST_ASYNCHRONOUS_THEN_IMPL_CALL_WITHOUT_INPUT
#undef BOOST_ASYNCHRONOUS_THEN_IMPL_CALL_WITH_INPUT
#undef BOOST_ASYNCHRONOUS_THEN_IMPL_NONVOID_DISPATCH
#undef BOOST_ASYNCHRONOUS_THEN_IMPL_VOID_DISPATCH
#undef BOOST_ASYNCHRONOUS_THEN_IMPL_EPILOGUE

    Continuation continuation_;
    Functor      functor_;
};

} // namespace detail

template <class Continuation, class Functor, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<
    boost::asynchronous::detail::has_is_continuation_task<Continuation>::value,
    boost::asynchronous::detail::callback_continuation<
        typename boost::asynchronous::detail::result_of_then<Continuation, Functor>::return_type,
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
    return boost::asynchronous::top_level_callback_continuation_job<typename boost::asynchronous::detail::result_of_then<Continuation, Functor>::return_type, Job>(
        boost::asynchronous::detail::then_helper<Continuation, Functor>(
            boost::asynchronous::detail::move_where_possible(continuation),
            boost::asynchronous::detail::move_where_possible(func),
            task_name
        )
    );
}

}} // namespace boost::asynchronous

#endif // BOOST_ASYNCHRONOUS_THEN_HPP
