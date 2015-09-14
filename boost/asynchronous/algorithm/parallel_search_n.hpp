// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org
#ifndef BOOST_ASYNCHRONOUS_PARALLEL_SEARCH_N_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_SEARCH_N_HPP

#include <algorithm>
#include <vector>
#include <iterator> // for std::iterator_traits

#include <boost/utility/enable_if.hpp>
#include <boost/serialization/vector.hpp>

#include <boost/asynchronous/detail/any_interruptible.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/scheduler/serializable_task.hpp>
#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
namespace boost { namespace asynchronous
{

// version for iterators => will return nothing
namespace detail
{
template <class Iterator1,class Size, class T, class Func, class Job>
struct parallel_search_n_helper: public boost::asynchronous::continuation_task<Iterator1>
{
    parallel_search_n_helper(Iterator1 beg1, Iterator1 end1,Size count, const T& value,Func func,
                                  long cutoff,const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<Iterator1>(task_name)
        , beg1_(beg1),end1_(end1),count_(count),value_(value),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}

    void operator()()
    {
        boost::asynchronous::continuation_result<Iterator1> task_res = this->this_task_result();
        try
        {
            // advance up to cutoff
            Iterator1 it = boost::asynchronous::detail::find_cutoff(beg1_,cutoff_,end1_);
            // if not at end, recurse, otherwise execute here
            if (it == end1_)
            {
                task_res.set_value(std::search_n(beg1_,end1_,count_,value_,func_));
            }
            else
            {
                auto beg1 = beg1_;
                auto end1 = end1_;
                auto count = count_;
                auto value = value_;
                auto func = std::move(func_);
                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [task_res,it,beg1,end1,count,value,func]
                            (std::tuple<boost::asynchronous::expected<Iterator1>,boost::asynchronous::expected<Iterator1>> res) mutable
                            {
                                try
                                {
                                    auto r1 = std::move(std::get<0>(res).get());
                                    auto r2 = std::move(std::get<1>(res).get());
                                    if (r1 != it)
                                    {
                                        // found in first part
                                        task_res.set_value(std::move(r1));
                                    }
                                    else
                                    {
                                        // check in overlap region
                                        auto itbeg = beg1;
                                        auto itend = it;
                                        std::advance(itbeg, std::distance(beg1,it) - count);
                                        std::advance(itend, count);
                                        auto itoverlap = std::search_n(itbeg,itend,count,value,func);
                                        if(itoverlap != itend)
                                        {
                                            task_res.set_value(std::move(itoverlap));
                                        }
                                        // check in second part
                                        else if (r2 != end1)
                                        {
                                            task_res.set_value(std::move(r2));
                                        }
                                        // nowhere found => end
                                        else
                                        {
                                            task_res.set_value(std::move(end1));
                                        }
                                    }
                                }
                                catch(std::exception& e)
                                {
                                    task_res.set_exception(boost::copy_exception(e));
                                }
                            },
                            // recursive tasks
                            parallel_search_n_helper<Iterator1,Size,T,Func,Job>
                                    (beg1_,it,count_,value_,func_,cutoff_,this->get_name(),prio_),
                            parallel_search_n_helper<Iterator1,Size,T,Func,Job>
                                    (it,end1_,count_,value_,func_,cutoff_,this->get_name(),prio_)
                   );
            }
        }
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }

    Iterator1 beg1_;
    Iterator1 end1_;
    Size count_;
    T value_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
}

template <class Iterator1, class Size, class T, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<Iterator1,Job>
parallel_search_n(Iterator1 beg1,Iterator1 end1, Size count, const T& value,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<Iterator1,Job>
            (boost::asynchronous::detail::parallel_search_n_helper<Iterator1,Size,T,Func,Job>
               (beg1,end1,count,value,std::move(func),cutoff,task_name,prio));
}

template <class Iterator1, class Size, class T, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<Iterator1,Job>
parallel_search_n(Iterator1 beg1,Iterator1 end1, Size count, const T& value,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto l = [](const typename std::iterator_traits<Iterator1>::value_type& i,
                const T& j)
    {
        return i == j;
    };

    return boost::asynchronous::top_level_callback_continuation_job<Iterator1,Job>
            (boost::asynchronous::detail::parallel_search_n_helper<Iterator1,Size,T,decltype(l),Job>
               (beg1,end1,count,value,std::move(l),cutoff,task_name,prio));
}


}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_SEARCH_N_HPP

