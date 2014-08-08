// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_INVOKE_HPP
#define BOOST_ASYNCHRONOUS_INVOKE_HPP

#include <algorithm>
#include <vector>

#include <boost/thread/future.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/serialization/vector.hpp>

#include <boost/asynchronous/detail/any_interruptible.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/scheduler/serializable_task.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>


namespace boost { namespace asynchronous
{

namespace detail
{
template <class Continuation, class Func, class Job,class Return,class Enable=void>
struct invoke_helper: public boost::asynchronous::continuation_task<Return>
{
    invoke_helper(Continuation const& c,Func func)
        : boost::asynchronous::continuation_task<Return>()
        , cont_(c),func_(std::move(func))
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<Return> task_res = this->this_task_result();
        auto func(std::move(func_));
        cont_.on_done([task_res,func](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& continuation_res)
        {
            try
            {
                task_res.emplace_value(func(std::move(std::get<0>(continuation_res).get())));
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
private:
    Continuation cont_;
    Func func_;
};
template <class Continuation, class Func, class Job,class Return>
struct invoke_helper<Continuation,Func,Job,Return,typename ::boost::enable_if<boost::asynchronous::detail::is_serializable<Func> >::type>
        : public boost::asynchronous::continuation_task<Return>
        , public boost::asynchronous::serializable_task
{
    invoke_helper(Continuation const& c,Func func)
        : boost::asynchronous::continuation_task<Return>()
        , boost::asynchronous::serializable_task(func.get_task_name())
        , cont_(c),func_(std::move(func))
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<Return> task_res = this->this_task_result();
        auto func(std::move(func_));
        cont_.on_done([task_res,func](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& continuation_res)
        {
            try
            {
                task_res.emplace_value(func(std::move(std::get<0>(continuation_res).get())));
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
    template <class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/)
    {
        ar & cont_;
        ar & func_;
    }
private:
    Continuation cont_;
    Func func_;
};
}
// Notice: return value of Continuation must have a default-ctor
template <class Continuation, class Func, class Job=boost::asynchronous::any_callable>
auto invoke(Continuation c,Func func)
    -> typename boost::enable_if<has_is_continuation_task<Continuation>,
            boost::asynchronous::detail::callback_continuation<decltype(func(typename Continuation::return_type())),Job> >::type
{
    return boost::asynchronous::top_level_callback_continuation_job<decltype(func(typename Continuation::return_type())),Job>
            (boost::asynchronous::detail::invoke_helper<Continuation,Func,Job,decltype(func(typename Continuation::return_type()))>
                (std::move(c),std::move(func)));
}

}}

#endif //BOOST_ASYNCHRONOUS_INVOKE_HPP
