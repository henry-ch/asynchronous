// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_INVOKE_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_INVOKE_HPP

#include <algorithm>
#include <vector>
#include <tuple>

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
template <class ReturnType, class Job>
struct parallel_invoke_helper: public boost::asynchronous::continuation_task<ReturnType>
{
    template <typename... Args>
    parallel_invoke_helper(Args&&... args)
    {
        typedef decltype(boost::asynchronous::detail::make_future_tuple(args...)) sp_future_type;
        typedef typename sp_future_type::element_type future_type;
        boost::asynchronous::continuation_result<ReturnType> task_res = this->this_task_result();
        boost::asynchronous::create_continuation_job<Job>(
                    // called when subtasks are done, set our result
                    [task_res](future_type res)
                    {
                        task_res.emplace_value(std::move(res));
                    },
                    // recursive tasks
                    args...);
    }
    void operator()()const
    {
    }
    template <class Archive>
    void serialize(Archive & /*ar*/, const unsigned int /*version*/)
    {
    }
};
template <class ReturnType, class Job,class Duration>
struct parallel_invoke_helper_timeout: public boost::asynchronous::continuation_task<ReturnType>
{
    template <typename... Args>
    parallel_invoke_helper_timeout(Duration const& d,Args&&... args)
    {
        typedef decltype(boost::asynchronous::detail::make_future_tuple(args...)) sp_future_type;
        typedef typename sp_future_type::element_type future_type;
        boost::asynchronous::continuation_result<ReturnType> task_res = this->this_task_result();
        boost::asynchronous::create_continuation_job_timeout<Job>(
                    // called when subtasks are done, set our result
                    [task_res](future_type res)
                    {
                        task_res.emplace_value(std::move(res));
                    },
                    d,
                    // recursive tasks
                    args...);
    }
    void operator()()const
    {
    }
    template <class Archive>
    void serialize(Archive & /*ar*/, const unsigned int /*version*/)
    {
    }
};
}
template <class Return, class Func>
struct continuation_task_wrapper : public boost::asynchronous::continuation_task<Return>
{
    continuation_task_wrapper(Func&& f):func_(std::forward<Func>(f)){}
    void operator()()const
    {
        boost::asynchronous::continuation_result<Return> task_res = this->this_task_result();
        try
        {
            task_res.emplace_value(std::move(func_()));
        }
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }
    template <class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/)
    {
        ar & func_;
    }
    Func func_;
};
template <class Func>
struct continuation_task_wrapper<void,Func> : public boost::asynchronous::continuation_task<void>
{
    continuation_task_wrapper(Func&& f):func_(std::forward<Func>(f)){}
    void operator()()const
    {
        boost::asynchronous::continuation_result<void> task_res = this->this_task_result();
        try
        {
            func_();
            task_res.emplace_value();
        }
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }
    template <class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/)
    {
        ar & func_;
    }
    Func func_;
};
template <class T>
auto to_continuation_task(T task)
-> boost::asynchronous::continuation_task_wrapper<decltype(task()),T>
{
    return boost::asynchronous::continuation_task_wrapper<decltype(task()),T>(std::move(task));
}

template <class Job, typename... Args>
auto parallel_invoke(Args&&... args)
    -> boost::asynchronous::detail::continuation<typename decltype(boost::asynchronous::detail::make_future_tuple(args...))::element_type,Job>
{
    typedef typename decltype(boost::asynchronous::detail::make_future_tuple(args...))::element_type ReturnType;

    return boost::asynchronous::top_level_continuation_log<ReturnType,Job>
            (boost::asynchronous::detail::parallel_invoke_helper<ReturnType,Job>(std::forward<Args>(args)...));
}
// version with timeout
template <class Job, class Duration, typename... Args>
auto parallel_invoke_timeout(Duration const& d, Args&&... args)
    -> boost::asynchronous::detail::continuation<typename decltype(boost::asynchronous::detail::make_future_tuple(args...))::element_type,Job>
{
    typedef typename decltype(boost::asynchronous::detail::make_future_tuple(args...))::element_type ReturnType;

    return boost::asynchronous::top_level_continuation_log<ReturnType,Job>
            (boost::asynchronous::detail::parallel_invoke_helper_timeout<ReturnType,Job,Duration>(d,std::forward<Args>(args)...));
}
}}

#endif //BOOST_ASYNCHRONOUS_PARALLEL_INVOKE_HPP
