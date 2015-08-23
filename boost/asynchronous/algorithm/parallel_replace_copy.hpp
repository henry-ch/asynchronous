// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_REPLACE_COPY_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_REPLACE_COPY_HPP

#include <boost/asynchronous/algorithm/parallel_transform.hpp>


namespace boost { namespace asynchronous
{

// versions with iterators
template <class Iterator, class Iterator2, class T, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<Iterator2,Job>
parallel_replace_copy_if(Iterator beg,Iterator end,Iterator2 beg2, Func func, T const& new_value,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto l = [func, new_value](T const& t)
    {
        if(func(t))
        {
            return new_value;
        }
        else
        {
            return t;
        }
    };

    return boost::asynchronous::parallel_transform<Iterator,Iterator2,decltype(l),Job>
            (beg,end,beg2,std::move(l),cutoff,task_name,prio);
}

// version for ranges returned as continuations
namespace detail
{
template <class Continuation, class Iterator2, class Func, class T,class Job>
struct parallel_replace_copy_if_continuation_helper: public boost::asynchronous::continuation_task<Iterator2>
{
    parallel_replace_copy_if_continuation_helper(Continuation c,Iterator2 beg2,Func func, T const& new_value,long cutoff,
                                                 const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<Iterator2>(task_name)
        , cont_(std::move(c)),res_it_(beg2),func_(std::move(func)),new_value_(new_value),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<Iterator2> task_res = this->this_task_result();
        auto res_it = res_it_;
        auto func(std::move(func_));
        auto new_value = new_value_;
        auto cutoff = cutoff_;
        auto task_name = this->get_name();
        auto prio = prio_;
        cont_.on_done([task_res,res_it,func,new_value,cutoff,task_name,prio]
                      (std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& continuation_res) mutable
        {
            try
            {
                auto res = boost::make_shared<typename Continuation::return_type>(std::move(std::get<0>(continuation_res).get()));
                auto new_continuation = boost::asynchronous::parallel_replace_copy_if
                        <decltype(boost::begin(std::declval<typename Continuation::return_type>())), Iterator2,T,Func, Job>
                            (boost::begin(*res),boost::end(*res),res_it,func,new_value,cutoff,task_name,prio);
                new_continuation.on_done([res,task_res](std::tuple<boost::asynchronous::expected<Iterator2> >&& new_continuation_res)
                {
                    task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                });
            }
            catch(std::exception& e)
            {
                task_res.set_exception(boost::copy_exception(e));
            }
        });
    }
    Continuation cont_;
    Iterator2 res_it_;
    Func func_;
    T new_value_;
    long cutoff_;
    std::size_t prio_;
};
}

template <class Range, class Iterator2, class Func, class T, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::enable_if<has_is_continuation_task<Range>,boost::asynchronous::detail::callback_continuation<Iterator2,Job> >::type
parallel_replace_copy_if(Range range,Iterator2 beg2,Func func, T const& new_value, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<Iterator2,Job>
            (boost::asynchronous::detail::parallel_replace_copy_if_continuation_helper<Range,Iterator2,Func,T,Job>
               (std::move(range),beg2,std::move(func),new_value,cutoff,task_name,prio));
}


template <class Iterator, class Iterator2, class T, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<Iterator2,Job>
parallel_replace_copy(Iterator beg,Iterator end,Iterator2 beg2, T const& old_value, T const& new_value,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto l = [old_value, new_value](T const& t)
    {
        if(t == old_value)
        {
            return new_value;
        }
        else
        {
            return t;
        }
    };

    return boost::asynchronous::parallel_transform<Iterator,Iterator2,decltype(l),Job>
            (beg,end,beg2,std::move(l),cutoff,task_name,prio);
}

// version for ranges returned as continuations
namespace detail
{
template <class Continuation, class Iterator2, class T,class Job>
struct parallel_replace_copy_continuation_helper: public boost::asynchronous::continuation_task<Iterator2>
{
    parallel_replace_copy_continuation_helper(Continuation c,Iterator2 beg2,T const& old_value, T const& new_value,long cutoff,
                                                 const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<Iterator2>(task_name)
        , cont_(std::move(c)),res_it_(beg2),old_value_(old_value),new_value_(new_value),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<Iterator2> task_res = this->this_task_result();
        auto res_it = res_it_;
        auto old_value = old_value_;
        auto new_value = new_value_;
        auto cutoff = cutoff_;
        auto task_name = this->get_name();
        auto prio = prio_;
        cont_.on_done([task_res,res_it,old_value,new_value,cutoff,task_name,prio]
                      (std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& continuation_res)
        {
            try
            {
                auto res = boost::make_shared<typename Continuation::return_type>(std::move(std::get<0>(continuation_res).get()));
                auto new_continuation = boost::asynchronous::parallel_replace_copy
                        <decltype(boost::begin(std::declval<typename Continuation::return_type>())), Iterator2,T, Job>
                            (boost::begin(*res),boost::end(*res),res_it,old_value,new_value,cutoff,task_name,prio);
                new_continuation.on_done([res,task_res]
                                         (std::tuple<boost::asynchronous::expected<Iterator2> >&& new_continuation_res) mutable
                {
                    task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                });
            }
            catch(std::exception& e)
            {
                task_res.set_exception(boost::copy_exception(e));
            }
        });
    }
    Continuation cont_;
    Iterator2 res_it_;
    T old_value_;
    T new_value_;
    long cutoff_;
    std::size_t prio_;
};
}

template <class Range, class Iterator2, class T, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::enable_if<has_is_continuation_task<Range>,boost::asynchronous::detail::callback_continuation<Iterator2,Job> >::type
parallel_replace_copy(Range range,Iterator2 beg2, T const& old_value, T const& new_value, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<Iterator2,Job>
            (boost::asynchronous::detail::parallel_replace_copy_continuation_helper<Range,Iterator2,T,Job>
               (std::move(range),beg2,old_value,new_value,cutoff,task_name,prio));
}

}}

#endif // BOOST_ASYNCHRONOUS_PARALLEL_REPLACE_COPY_HPP

