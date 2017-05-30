// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_FOR_EACH_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_FOR_EACH_HPP

#include <algorithm>
#include <vector>
#include <iterator> // for std::iterator_traits

#include <type_traits>
#include <boost/serialization/vector.hpp>

#include <boost/asynchronous/detail/any_interruptible.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/scheduler/serializable_task.hpp>
#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>
#include <boost/asynchronous/detail/function_traits.hpp>


namespace boost { namespace asynchronous
{

// version for iterators => will return nothing
namespace detail
{
template <int args, class Iterator, class Func, class Enable=void>
struct for_each_helper
{
    Func operator()(Iterator beg, Iterator end, Func&& func)
    {
        return std::for_each(beg,end,std::forward<Func>(func));
    }
};
template <class Iterator, class Func>
struct for_each_helper<1,Iterator,Func,typename std::enable_if<std::is_integral<Iterator>::value>::type>
{
    Func operator()(Iterator beg, Iterator end, Func&& func)
    {   
        for (; beg != end; ++beg)
        {   
            func(beg);
        }
        return std::forward<Func>(func);
    }   
};
template <class Iterator, class Func>
struct for_each_helper<2,Iterator,Func,void>
{
    Func operator()(Iterator beg, Iterator end, Func&& func)
    {
        func(beg,end);
        return std::forward<Func>(func);
    }
};

template <class Iterator, class Func, class Job>
struct parallel_for_each_helper: public boost::asynchronous::continuation_task<Func>
{
    parallel_for_each_helper(Iterator beg, Iterator end,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<Func>(task_name)
        , beg_(beg),end_(end),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<Func> task_res = this->this_task_result();
        try
        {
            // advance up to cutoff
            Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
            // if not at end, recurse, otherwise execute here
            if (it == end_)
            {
                task_res.set_value(std::move(boost::asynchronous::detail::for_each_helper<
                                             boost::asynchronous::function_traits<Func>::arity,Iterator,Func>()
                                             (beg_,it,std::move(func_))));
            }
            else
            {
                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [task_res]
                            (std::tuple<boost::asynchronous::expected<Func>,boost::asynchronous::expected<Func> > res) mutable
                            {
                                try
                                {
                                    // get to check that no exception
                                    Func f1 (std::move(std::get<0>(res).get()));
                                    Func f2 (std::move(std::get<1>(res).get()));
                                    f1.merge(std::move(f2));
                                    task_res.set_value(std::move(f1));
                                }
                                catch(...)
                                {
                                    task_res.set_exception(std::current_exception());
                                }
                            },
                            // recursive tasks
                            parallel_for_each_helper<Iterator,Func,Job>(beg_,it,func_,cutoff_,this->get_name(),prio_),
                            parallel_for_each_helper<Iterator,Func,Job>(it,end_,func_,cutoff_,this->get_name(),prio_)
                   );
            }
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
        }
    }
    Iterator beg_;
    Iterator end_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
}

template <class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<Func,Job>
parallel_for_each(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio=0)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<Func,Job>
            (boost::asynchronous::detail::parallel_for_each_helper<Iterator,Func,Job>(beg,end,std::move(func),cutoff,task_name,prio));
}
}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_FOR_EACH_HPP
