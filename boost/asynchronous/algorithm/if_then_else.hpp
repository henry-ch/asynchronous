// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_IF_THEN_ELSE_CONTINUATION_HPP
#define BOOST_ASYNCHRONOUS_IF_THEN_ELSE_CONTINUATION_HPP

#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/continuation_task.hpp>

namespace boost { namespace asynchronous
{
namespace detail
{

template <class Continuation, class IfClause, class ThenClause, class ElseClause>
struct if_then_else_continuation_helper :
        public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    if_then_else_continuation_helper(Continuation const& c,IfClause if_clause,ThenClause then_clause,ElseClause else_clause,
                   std::string& task_name)
        :boost::asynchronous::continuation_task<typename Continuation::return_type>(std::move(task_name))
        ,cont_(c)
        ,if_clause_(if_clause)
        ,then_clause_(then_clause)
        ,else_clause_(else_clause)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<typename Continuation::return_type> task_res =
                this->this_task_result();
        auto if_clause = if_clause_;
        auto then_clause = then_clause_;
        auto else_clause = else_clause_;
        // TODO C++14 move capture if possible
        cont_.on_done(
        [task_res,if_clause,then_clause,else_clause]
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
                            task_res.set_value(std::move(std::get<0>(then_res).get()));
                        }
                    );
                }
                else
                {
                    auto else_cont = else_clause(std::move(res));
                    else_cont.on_done(
                        [task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& then_res)
                        {
                            task_res.set_value(std::move(std::get<0>(then_res).get()));
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
    Continuation cont_;
    IfClause if_clause_;
    ThenClause then_clause_;
    ElseClause else_clause_;
};

template <class IfClause, class ThenClause, class ElseClause, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
struct if_then_else_fwd
{
    if_then_else_fwd(IfClause& if_clause, ThenClause& then_clause, ElseClause& else_clause,const std::string& task_name)
    : if_clause_(std::move(if_clause))
    , then_clause_(std::move(then_clause))
    , else_clause_(std::move(else_clause))
    , task_name_(task_name)
    {}

    IfClause if_clause_;
    ThenClause then_clause_;
    ElseClause else_clause_;
    std::string task_name_;

    template <class Continuation>
    boost::asynchronous::detail::callback_continuation<typename Continuation::return_type,Job>
    operator()(Continuation c)
    {
        return boost::asynchronous::top_level_callback_continuation_job<typename Continuation::return_type,Job>
                (boost::asynchronous::detail::if_then_else_continuation_helper<Continuation,IfClause,ThenClause,ElseClause>
                    (std::move(c),std::move(if_clause_),std::move(then_clause_),std::move(else_clause_),task_name_));
    }

};
}

template <class IfClause, class ThenClause, class ElseClause, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::if_then_else_fwd<IfClause,ThenClause,ElseClause,Job>
if_then_else(IfClause if_clause, ThenClause then_clause, ElseClause else_clause,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name)
#else
             const std::string& task_name="")
#endif
{
    return boost::asynchronous::detail::if_then_else_fwd<IfClause,ThenClause,ElseClause,Job>
                (if_clause,then_clause,else_clause,task_name);
}

}}

#endif // BOOST_ASYNCHRONOUS_IF_THEN_ELSE_CONTINUATION_HPP
