// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_COPY_IF_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_COPY_IF_HPP

#include <algorithm>
#include <vector>

#include <boost/utility/enable_if.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>

namespace boost { namespace asynchronous
{
namespace detail
{
// tree structure containing accumulated results from part 1
struct copy_if_data
{
    copy_if_data(std::size_t true_size=0, std::vector<bool> values = std::vector<bool>())
        : true_size_(true_size)
        , values_(std::move(values))
        , data_()
    {}
    copy_if_data(copy_if_data&& rhs) =default;
    copy_if_data& operator=(copy_if_data&& rhs)=default;

    std::size_t true_size_;
    // true/fase response of the elements to be copied, in this subnode
    std::vector<bool> values_;
    // subnodes (binary tree)
    std::vector<copy_if_data> data_;
};

template <class Iterator, class Func, class Job>
struct parallel_copy_if_part1_helper: public boost::asynchronous::continuation_task<boost::asynchronous::detail::copy_if_data>
{
    parallel_copy_if_part1_helper(Iterator beg, Iterator end, Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<boost::asynchronous::detail::copy_if_data>(task_name),
          beg_(beg),end_(end),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<boost::asynchronous::detail::copy_if_data> task_res = this_task_result();
        try
        {
            // advance up to cutoff
            Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
            if (it == end_)
            {
                std::size_t count=0;
                std::vector<bool> values(std::distance(beg_,end_),true);
                std::size_t i=0;
                for (auto it = beg_; it != end_; ++it,++i)
                {
                    if (func_(*it))
                    {
                        ++ count;
                    }
                    else
                    {
                        values[i]=false;
                    }
                }
                copy_if_data data(count,std::move(values));
                task_res.set_value(std::move(data));
            }
            else
            {
                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [task_res](std::tuple<boost::asynchronous::expected<boost::asynchronous::detail::copy_if_data>,
                                                  boost::asynchronous::expected<boost::asynchronous::detail::copy_if_data> > res) mutable
                            {
                                try
                                {
                                    boost::asynchronous::detail::copy_if_data res_left = std::move(std::get<0>(res).get());
                                    boost::asynchronous::detail::copy_if_data res_right = std::move(std::get<1>(res).get());
                                    boost::asynchronous::detail::copy_if_data res_all(res_left.true_size_ + res_right.true_size_);
                                    res_all.data_.reserve(2);
                                    res_all.data_.push_back(std::move(res_left));
                                    res_all.data_.push_back(std::move(res_right));
                                    task_res.set_value(std::move(res_all));
                                }
                                catch(std::exception& e)
                                {
                                    task_res.set_exception(boost::copy_exception(e));
                                }
                            },
                            // recursive tasks
                            parallel_copy_if_part1_helper<Iterator,Func,Job>
                                (beg_,it,func_,cutoff_,this->get_name(),prio_),
                            parallel_copy_if_part1_helper<Iterator,Func,Job>
                                (it,end_,func_,cutoff_,this->get_name(),prio_)
                );
            }
        }
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }
    Iterator beg_;
    Iterator end_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
template <class Iterator,class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<boost::asynchronous::detail::copy_if_data,Job>
parallel_copy_if_part1(Iterator beg, Iterator end, Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio=0)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<boost::asynchronous::detail::copy_if_data,Job>
            (boost::asynchronous::detail::parallel_copy_if_part1_helper<Iterator,Func,Job>
                (beg,end,std::move(func),cutoff,task_name,prio));

}

template <class Iterator,class OutIterator, class Job>
struct parallel_copy_if_part2_helper: public boost::asynchronous::continuation_task<void>
{
    parallel_copy_if_part2_helper(Iterator beg, Iterator end, OutIterator outit,
                                  std::size_t offset, boost::asynchronous::detail::copy_if_data data,
                                  long cutoff,const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<void>(task_name),
          beg_(beg),end_(end),out_(outit),offset_(offset),data_(std::move(data)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<void> task_res = this_task_result();
        try
        {
            // advance up to cutoff
            Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
            if (it == end_)
            {
                auto out = out_;
                std::advance(out,offset_);
                std::size_t values_counter=0;
                for (auto it2 = beg_ ; it2 != end_ ; ++it2, ++values_counter)
                {
                    if (data_.values_[values_counter])
                    {
                        *out++ = *it2;
                    }
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
                            parallel_copy_if_part2_helper<Iterator,OutIterator,Job>
                                (beg_,it,out_,
                                 offset_,
                                 std::move(data_.data_[0]),
                                 cutoff_,this->get_name(),prio_),
                            parallel_copy_if_part2_helper<Iterator,OutIterator,Job>
                                (it,end_,out_,
                                 offset_ + data_.data_[0].true_size_,
                                 std::move(data_.data_[1]),
                                 cutoff_,this->get_name(),prio_)
                );
            }
        }
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }
    Iterator beg_;
    Iterator end_;
    OutIterator out_;
    std::size_t offset_;
    boost::asynchronous::detail::copy_if_data data_;

    long cutoff_;
    std::size_t prio_;
};
template <class Iterator,class Iterator2, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_copy_if_part2(Iterator beg, Iterator end, Iterator2 out, std::size_t offset, boost::asynchronous::detail::copy_if_data data,
                       long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio=0)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_copy_if_part2_helper<Iterator,Iterator2,Job>
             (beg,end,out,offset,std::move(data),cutoff,task_name,prio));

}

template <class Iterator, class Iterator2, class Func, class Job>
struct parallel_copy_if_helper: public boost::asynchronous::continuation_task<Iterator2>
{
    parallel_copy_if_helper(Iterator beg, Iterator end, Iterator2 out, Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<Iterator2>(task_name),
          beg_(beg),end_(end),out_(out),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<Iterator2> task_res = this->this_task_result();
        try
        {
            auto cont = boost::asynchronous::detail::parallel_copy_if_part1<Iterator,Func,Job>(beg_,end_,func_,cutoff_,this->get_name(),prio_);
            auto beg = beg_;
            auto end = end_;
            auto out = out_;
            auto func = func_;
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            cont.on_done([task_res,beg,end,out,func,cutoff,task_name,prio]
                         (std::tuple<boost::asynchronous::expected<boost::asynchronous::detail::copy_if_data> >&& res)
            {
                try
                {
                    boost::asynchronous::detail::copy_if_data data = std::move(std::get<0>(res).get());
                    std::size_t offset = data.true_size_;
                    auto cont =
                            boost::asynchronous::detail::parallel_copy_if_part2<Iterator,Iterator2,Job>
                            (beg,end,out,0,std::move(data),cutoff,task_name,prio);
                    Iterator2 ret = out;
                    std::advance(ret,offset);
                    cont.on_done([task_res,ret](std::tuple<boost::asynchronous::expected<void> >&& res)
                    {
                        try
                        {
                            // get to check that no exception
                            std::get<0>(res).get();
                            task_res.set_value(ret);
                        }
                        catch(std::exception& e)
                        {
                            task_res.set_exception(boost::copy_exception(e));
                        }
                    });
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(boost::copy_exception(e));
                }
            });
        }
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }
    Iterator beg_;
    Iterator end_;
    Iterator2 out_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};

}

template <class Iterator, class Iterator2,class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<Iterator2,Job>
parallel_copy_if(Iterator beg, Iterator end, Iterator2 out, Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio=0)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<Iterator2,Job>
            (boost::asynchronous::detail::parallel_copy_if_helper<Iterator,Iterator2,Func,Job>
                (beg,end,out,std::move(func),cutoff,task_name,prio));

}

}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_COPY_IF_HPP

