// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_IF_CONTINUATION_HPP
#define BOOST_ASYNCHRONOUS_IF_CONTINUATION_HPP

#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/continuation_task.hpp>

namespace boost { namespace asynchronous
{
namespace detail
{
template <class Continuation, class IfClause, class ThenClause>
struct if_continuation_helper :
        public boost::asynchronous::continuation_task<std::tuple<bool,typename Continuation::return_type>>
{
    if_continuation_helper(Continuation const& c,IfClause if_clause,ThenClause then_clause,
                   const std::string& task_name)
        :boost::asynchronous::continuation_task<std::tuple<bool,typename Continuation::return_type>>(task_name)
        ,cont_(c)
        ,if_clause_(if_clause)
        ,then_clause_(then_clause)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<std::tuple<bool,typename Continuation::return_type>> task_res =
                this->this_task_result();
        try
        {
            auto if_clause = if_clause_;
            auto then_clause = then_clause_;
            // TODO C++14 move capture if possible
            cont_.on_done(
            [task_res,if_clause,then_clause]
            (std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& continuation_res)
            {
                try
                {
                    auto res = std::get<0>(continuation_res).get();
                    if (if_clause(res))
                    {
                        auto then_cont = then_clause(std::move(res));
                        then_cont.on_done(
                            [task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& then_res)
                            {
                                task_res.set_value(std::make_tuple(true,std::move(std::get<0>(then_res).get())));
                            }
                        );
                    }
                    else
                    {
                        task_res.set_value(std::make_tuple(false,std::move(res)));
                    }
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(boost::copy_exception(e));
                }
            }
            );
        }
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }
    Continuation cont_;
    IfClause if_clause_;
    ThenClause then_clause_;
};

template <class Continuation, class ElseClause>
struct else_continuation_helper :
        public boost::asynchronous::continuation_task<typename std::tuple_element<1,typename Continuation::return_type>::type>
{
    else_continuation_helper(Continuation const& c,ElseClause else_clause,
                   const std::string& task_name)
        :boost::asynchronous::continuation_task<typename std::tuple_element<1,typename Continuation::return_type>::type>(task_name)
        ,cont_(c)
        ,else_clause_(else_clause)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<typename std::tuple_element<1,typename Continuation::return_type>::type>
                task_res = this->this_task_result();
        try
        {
            auto else_clause = else_clause_;
            // TODO C++14 move capture if possible
            cont_.on_done(
            [task_res,else_clause]
            (std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& continuation_res)
            {
                try
                {
                    auto res = std::get<0>(continuation_res).get();
                    if (std::get<0>(res))
                    {
                        // if handled, just forward
                        task_res.set_value(std::move(std::get<1>(res)));
                    }
                    else
                    {
                        auto else_cond = else_clause(std::move(std::get<1>(res)));
                        else_cond.on_done(
                            [task_res](std::tuple<boost::asynchronous::expected<
                                                typename std::tuple_element<1,typename Continuation::return_type>::type> >&& prev_res)
                            {
                                task_res.set_value(std::move(std::get<0>(prev_res).get()));
                            }
                        );
                    }
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(boost::copy_exception(e));
                }
            }
            );
        }
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }
    Continuation cont_;
    ElseClause else_clause_;
};

template <class Continuation, class IfClause, class ThenClause>
struct else_if_continuation_helper :
        public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    else_if_continuation_helper(Continuation const& c,IfClause if_clause,ThenClause then_clause,
                   const std::string& task_name)
        :boost::asynchronous::continuation_task<typename Continuation::return_type>(task_name)
        ,cont_(c)
        ,if_clause_(if_clause)
        ,then_clause_(then_clause)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<typename Continuation::return_type>
                task_res = this->this_task_result();
        try
        {
            auto if_clause = if_clause_;
            auto then_clause = then_clause_;
            // TODO C++14 move capture if possible
            cont_.on_done(
            [task_res,if_clause,then_clause]
            (std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& continuation_res)
            {
                try
                {
                    auto res = std::get<0>(continuation_res).get();
                    if (std::get<0>(res))
                    {
                        // if handled, just forward
                        task_res.set_value(std::move(res));
                    }
                    else if(if_clause(std::get<1>(res)))
                    {
                        auto then_cont = then_clause(std::move(std::get<1>(res)));
                        then_cont.on_done(
                            [task_res](std::tuple<boost::asynchronous::expected<
                                            typename std::tuple_element<1,typename Continuation::return_type>::type> >&& then_res)
                            {
                                task_res.set_value(std::make_tuple(true,std::move(std::get<0>(then_res).get())));
                            }
                        );
                    }
                    else
                    {
                        task_res.set_value(std::move(res));
                    }
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(boost::copy_exception(e));
                }
            }
            );
        }
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }
    Continuation cont_;
    IfClause if_clause_;
    ThenClause then_clause_;
};

}

template <class Continuation, class IfClause, class ThenClause,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<std::tuple<bool,typename Continuation::return_type>,Job>
if_(Continuation cont,IfClause if_clause, ThenClause then_clause,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name)
#else
             const std::string& task_name="")
#endif
{

    return boost::asynchronous::top_level_callback_continuation_job<std::tuple<bool,typename Continuation::return_type>,Job>
            (boost::asynchronous::detail::if_continuation_helper<Continuation,IfClause,ThenClause>
                (std::move(cont),std::move(if_clause),std::move(then_clause),task_name));
}

template <class Continuation, class ElseClause,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<typename std::tuple_element<1,typename Continuation::return_type>::type,Job>
else_(Continuation cont,ElseClause else_clause,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name)
#else
             const std::string& task_name="")
#endif
{

    return boost::asynchronous::top_level_callback_continuation_job<
            typename std::tuple_element<1,typename Continuation::return_type>::type,Job>
                (boost::asynchronous::detail::else_continuation_helper<Continuation,ElseClause>
                    (std::move(cont),std::move(else_clause),task_name));
}

template <class Continuation, class IfClause, class ThenClause,class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<typename Continuation::return_type,Job>
else_if(Continuation cont,IfClause if_clause, ThenClause then_clause,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name)
#else
             const std::string& task_name="")
#endif
{

    return boost::asynchronous::top_level_callback_continuation_job<typename Continuation::return_type,Job>
                (boost::asynchronous::detail::else_if_continuation_helper<Continuation,IfClause,ThenClause>
                    (std::move(cont),std::move(if_clause),std::move(then_clause),task_name));
}

}}

#endif // BOOST_ASYNCHRONOUS_IF_CONTINUATION_HPP

