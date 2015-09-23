// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_PLACEMENT_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_PLACEMENT_HPP


#include <boost/utility/enable_if.hpp>
#include <boost/shared_array.hpp>

#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>

namespace boost { namespace asynchronous
{
namespace detail
{
enum class parallel_placement_helper_enum
{
    success,
    error_handled,
    error_not_handled
};
using parallel_placement_helper_result = std::pair<boost::asynchronous::detail::parallel_placement_helper_enum,boost::exception_ptr>;

template <class T, class Job>
struct parallel_placement_helper: public boost::asynchronous::continuation_task<boost::asynchronous::detail::parallel_placement_helper_result>
{
    parallel_placement_helper(std::size_t beg, std::size_t end, char* data,long cutoff,const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<boost::asynchronous::detail::parallel_placement_helper_result>(task_name),
          beg_(beg),end_(end),data_(data),cutoff_(cutoff),prio_(prio)
    {
    }
    void operator()()
    {
        boost::asynchronous::continuation_result<boost::asynchronous::detail::parallel_placement_helper_result> task_res = this_task_result();
        try
        {
            // advance up to cutoff
            auto it =(end_-beg_ <= (std::size_t)cutoff_)? end_: beg_ + (end_-beg_)/2;
            // if not at end, recurse, otherwise execute here
            if (it == end_)
            {
                for (std::size_t i = 0; i < (end_ - beg_); ++i)
                {
                    try
                    {
                        new (((T*)data_)+ (i+beg_)) T;
                    }
                    catch (std::exception& e)
                    {
                        // we need to cleanup
                        for (std::size_t j = 0; j < i; ++j)
                        {
                            ((T*)(data_)+(j+beg_))->~T();
                        }
                        task_res.set_value(std::make_pair(boost::asynchronous::detail::parallel_placement_helper_enum::error_not_handled,boost::copy_exception(e)));
                    }
                    catch (...)
                    {
                        // we need to cleanup
                        for (std::size_t j = 0; j < i; ++j)
                        {
                            ((T*)(data_)+(j+beg_))->~T();
                        }
                        task_res.set_value(std::make_pair(boost::asynchronous::detail::parallel_placement_helper_enum::error_not_handled,boost::current_exception()));
                    }
                }
                task_res.set_value(std::make_pair(boost::asynchronous::detail::parallel_placement_helper_enum::success,boost::exception_ptr()));
            }
            else
            {
                auto beg = beg_;
                auto end = end_;
                auto data = data_;
                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [task_res,beg,it,end,data](std::tuple<boost::asynchronous::expected<boost::asynchronous::detail::parallel_placement_helper_result>,
                                                                  boost::asynchronous::expected<boost::asynchronous::detail::parallel_placement_helper_result> > res) mutable
                            {
                                try
                                {
                                    // get to check that no exception
                                    auto res1 = std::get<0>(res).get();
                                    auto res2 = std::get<1>(res).get();
                                    // if no exception, proceed
                                    if (res1.first == boost::asynchronous::detail::parallel_placement_helper_enum::success &&
                                        res2.first == boost::asynchronous::detail::parallel_placement_helper_enum::success)
                                    {
                                        task_res.set_value(std::make_pair(boost::asynchronous::detail::parallel_placement_helper_enum::success,boost::exception_ptr()));
                                    }
                                    // if both have an error and error not handled (i.e propagated), keep any exception and proceed
                                    else if (res1.first == boost::asynchronous::detail::parallel_placement_helper_enum::error_not_handled &&
                                             res2.first == boost::asynchronous::detail::parallel_placement_helper_enum::error_not_handled)
                                    {
                                        task_res.set_value(std::make_pair(boost::asynchronous::detail::parallel_placement_helper_enum::error_handled,res1.second));
                                    }
                                    // if first part has an error and second no, call destructor on second
                                    else if (res1.first == boost::asynchronous::detail::parallel_placement_helper_enum::error_not_handled)
                                    {
                                        for (std::size_t j = 0; j < (end-it); ++j)
                                        {
                                            ((T*)(data)+(j+beg))->~T();
                                        }
                                        task_res.set_value(std::make_pair(boost::asynchronous::detail::parallel_placement_helper_enum::error_handled,res1.second));
                                    }
                                    // if second part has an error and first no, call destructor on first
                                    else
                                    {
                                        for (std::size_t j = 0; j < (it-beg); ++j)
                                        {
                                            ((T*)(data)+(j+beg))->~T();
                                        }
                                        task_res.set_value(std::make_pair(boost::asynchronous::detail::parallel_placement_helper_enum::error_handled,res2.second));
                                    }
                                }
                                catch(std::exception& e)
                                {
                                    task_res.set_value(std::make_pair(boost::asynchronous::detail::parallel_placement_helper_enum::error_not_handled,boost::copy_exception(e)));
                                }
                                catch (...)
                                {
                                    task_res.set_value(std::make_pair(boost::asynchronous::detail::parallel_placement_helper_enum::error_not_handled,boost::current_exception()));
                                }
                            },
                            // recursive tasks
                            boost::asynchronous::detail::parallel_placement_helper<T,Job>
                                    (beg_,it,data_,cutoff_,this->get_name(),prio_),
                            boost::asynchronous::detail::parallel_placement_helper<T,Job>
                                    (it,end_,data_,cutoff_,this->get_name(),prio_)
                 );
            }
        }
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }
    std::size_t beg_;
    std::size_t end_;
    char* data_;
    long cutoff_;
    std::size_t prio_;
};
}

template <class T, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<boost::asynchronous::detail::parallel_placement_helper_result,Job>
parallel_placement(std::size_t beg, std::size_t end, char* data,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
   return boost::asynchronous::top_level_callback_continuation_job<boost::asynchronous::detail::parallel_placement_helper_result,Job>
            (boost::asynchronous::detail::parallel_placement_helper<T,Job>(beg,end,data,cutoff,task_name,prio));
}

namespace detail
{
template <class T, class Job>
struct parallel_placement_delete_helper: public boost::asynchronous::continuation_task<void>
{
    parallel_placement_delete_helper(std::size_t beg, std::size_t end, boost::shared_array<char> data,long cutoff,const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<void>(task_name),
          beg_(beg),end_(end),data_(data),cutoff_(cutoff),prio_(prio)
    {
    }
    void operator()()
    {
        boost::asynchronous::continuation_result<void> task_res = this_task_result();
        try
        {
            // advance up to cutoff
            auto it =(end_-beg_ <= (std::size_t)cutoff_)? end_: beg_ + (end_-beg_)/2;
            // if not at end, recurse, otherwise execute here
            if (it == end_)
            {
                for (std::size_t i = 0; i < (end_ - beg_); ++i)
                {
                    ((T*)(data_.get())+(i+beg_))->~T();
                }
                task_res.set_value();
            }
            else
            {
                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [task_res](std::tuple<boost::asynchronous::expected<void>,boost::asynchronous::expected<void> > res) mutable
                            {
                                try
                                {
                                    // get to check that no exception
                                    std::get<0>(res).get();
                                    std::get<1>(res).get();
                                    task_res.set_value();
                                }
                                catch(std::exception& e)
                                {
                                    task_res.set_exception(boost::copy_exception(e));
                                }
                            },
                            // recursive tasks
                            boost::asynchronous::detail::parallel_placement_delete_helper<T,Job>
                                    (beg_,it,data_,cutoff_,this->get_name(),prio_),
                            boost::asynchronous::detail::parallel_placement_delete_helper<T,Job>
                                    (it,end_,data_,cutoff_,this->get_name(),prio_)
                 );
            }
        }
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }
    std::size_t beg_;
    std::size_t end_;
    boost::shared_array<char> data_;
    long cutoff_;
    std::size_t prio_;
};
}

template <class T, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_placement_delete(boost::shared_array<char> data,std::size_t size,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
   return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_placement_delete_helper<T,Job>(0,size,data,cutoff,task_name,prio));
}

template <class T, class Job>
struct placement_deleter
{
    placement_deleter(std::size_t s, char* d,long cutoff,const std::string& task_name, std::size_t prio):size_(s),data_(d),cutoff_(cutoff),task_name_(task_name),prio_(prio){}
    placement_deleter(placement_deleter const&)=delete;
    placement_deleter(placement_deleter &&)=delete;
    placement_deleter& operator=(placement_deleter const&)=delete;
    placement_deleter& operator=(placement_deleter &&)=delete;
    ~placement_deleter()
    {
        boost::shared_array<char> d (data_);
        auto cont = boost::asynchronous::parallel_placement_delete<T,Job>(d,size_,cutoff_,task_name_,prio_);
        cont.on_done(
            [d](std::tuple<boost::asynchronous::expected<void> >&&)
             {
              // ignore
             });
    }

    std::size_t size_;
    char* data_;
    long cutoff_;
    std::string task_name_;
    std::size_t prio_;
};

}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_PLACEMENT_HPP

