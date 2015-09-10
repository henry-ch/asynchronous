// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2014
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org
// Thanks to Herr Prof. Dr. Joel Falcou "Crazy Frenchman" for this clever piece of code

#ifndef BOOST_ASYNCHRONOUS_FUNCTION_TRAITS_HPP
#define BOOST_ASYNCHRONOUS_FUNCTION_TRAITS_HPP

#include <functional>
#include <tuple>

namespace boost { namespace asynchronous
{
template <typename F>
struct function_traits : public function_traits<decltype(&F::operator())>
{};

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...)const>
{
    enum {arity = sizeof...(Args)};
    using result_type = ReturnType;
    using function_type = std::function<ReturnType(Args...)>;

    template <std::size_t i>
    struct arg_
    {
        typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
    };
    template <std::size_t i>
    using arg = typename arg_<i>::type;
};
template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...)>
{
    enum {arity = sizeof...(Args)};
    using result_type = ReturnType;
    using function_type = std::function<ReturnType(Args...)>;

    template <std::size_t i>
    struct arg_
    {
        typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
    };
    template <std::size_t i>
    using arg = typename arg_<i>::type;
};

/*
template<class F>
struct min_function_traits
{
    enum {arity = F::arity};
};
template <typename ClassType, typename ReturnType, typename... Args>
struct min_function_traits<ReturnType(ClassType::*)(Args...)const>
{
    enum {arity = sizeof...(Args)};
};
template <typename ClassType, typename ReturnType, typename... Args>
struct min_function_traits<ReturnType(ClassType::*)(Args...)>
{
    enum {arity = sizeof...(Args)};
};
*/

template <class T>
auto make_function(T t) -> typename boost::asynchronous::function_traits<T>::function_type
{
    return typename boost::asynchronous::function_traits<T>::function_type (std::move(t));
}

}}
#endif // BOOST_ASYNCHRONOUS_FUNCTION_TRAITS_HPP
