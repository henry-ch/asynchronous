// Boost.Asynchronous library
//  Copyright (C) Christophe Henry, Tobias Holl 2014
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_MOVE_BIND_HPP
#define BOOST_ASYNCHRONOUS_MOVE_BIND_HPP

#include <tuple>

namespace boost { namespace asynchronous
{
    template <class Func,typename... Params>
    struct moveable_binder
    {
        template<typename... Args>
        moveable_binder(Func f, Args&&... params)
            : m_func(std::move(f)),m_params(std::forward<Args>(params)...)  {}

        moveable_binder(moveable_binder&& rhs)noexcept
            : m_func(std::move(rhs.m_func)), m_params(std::move(rhs.m_params)){}
        moveable_binder& operator= (moveable_binder&& rhs) noexcept
        {
            std::swap(m_func,rhs.m_func);
            std::swap(m_params,rhs.m_params);
            return *this;
        }
        moveable_binder(moveable_binder const& rhs)noexcept
            : m_func(std::move(const_cast<moveable_binder&>(rhs).m_func)), m_params(std::move(const_cast<moveable_binder&>(rhs).m_params)){}
        moveable_binder& operator= (moveable_binder const& rhs) noexcept
        {
            std::swap(m_func,const_cast<moveable_binder&>(rhs).m_func);
            std::swap(m_params,const_cast<moveable_binder&>(rhs).m_params);
            return *this;
        }

        template <class Tuple, int i>
        struct helper
        {
            template<typename... Args>
            static auto doit(Func* f, std::tuple<Params...>* t, Args... args)
            -> decltype(helper<Tuple,i-1>::doit(f,t,std::get<i-1>(*t),std::move(args)...))
            {
                return helper<Tuple,i-1>::doit(f,t,std::get<i-1>(*t),std::move(args)...);
            }
        };
        template <class Tuple>
        struct helper<Tuple,1>
        {
            template<typename... Args>
            static auto doit(Func* f, std::tuple<Params...>* t,Args... args)
            -> decltype((*f)(std::move(std::get<0>(*t)),std::move(args)...))
            {
                return (*f)(std::move(std::get<0>(*t)),std::move(args)...);
            }
        };
        template <class Tuple>
        struct helper<Tuple,0>
        {
            template<typename... Args>
            static auto doit(Func* f, std::tuple<Params...>* ,Args... args)
            -> decltype((*f)(std::move(args)...))
            {
                return (*f)(std::move(args)...);
            }
        };

        template <typename... Args>
        auto operator()(Args... args)
        -> decltype(helper<std::tuple<Params...>,sizeof...(Params)>::doit(std::declval<Func*>(),std::declval<std::tuple<Params...>*>(),std::move(args)...))
        {
            return helper<std::tuple<Params...>,sizeof...(Params)>::doit(&m_func,&m_params,std::move(args)...);
        }

        Func m_func;
        std::tuple<Params...> m_params;
    };

    template <typename Func,typename... Args>
    boost::asynchronous::moveable_binder<Func,Args...> move_bind(Func f,Args... args)
    {
        return boost::asynchronous::moveable_binder<Func,Args...>(std::move(f),std::move(args)...);
    }
}} //boost::asynchronous


#endif // BOOST_ASYNCHRONOUS_MOVE_BIND_HPP
