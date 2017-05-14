// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_PARTITION_COPY_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_PARTITION_COPY_HPP

#include <algorithm>
#include <vector>

#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>

namespace boost { namespace asynchronous
{
namespace detail
{
// tree structure containing accumulated results from part 1 (partitioning)
struct partition_copy_data
{
    partition_copy_data(std::size_t partition_true=0, std::size_t partition_false=0)
        : partition_true_(partition_true),partition_false_(partition_false)
        , data_()
    {}
    std::size_t partition_true_;
    std::size_t partition_false_;
    std::vector<partition_copy_data> data_;
};

template <class Iterator, class Func, class Job>
struct parallel_partition_copy_part1_helper: public boost::asynchronous::continuation_task<boost::asynchronous::detail::partition_copy_data>
{
    parallel_partition_copy_part1_helper(Iterator beg, Iterator end, Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<boost::asynchronous::detail::partition_copy_data>(task_name),
          beg_(beg),end_(end),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<boost::asynchronous::detail::partition_copy_data> task_res = this_task_result();
        try
        {
            // advance up to cutoff
            Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
            if (it == end_)
            {
                std::size_t count_true=0;
                std::size_t count_false=0;
                for (auto it = beg_; it != end_; ++it)
                {
                    if (func_(*it))
                        ++ count_true;
                    else
                        ++count_false;
                }
                partition_copy_data data(count_true,count_false);
                //Iterator it_part = std::partition(beg_,end_,std::move(func_));
                //partition_copy_data data(std::distance(beg_,it_part),std::distance(it_part,end_));
                task_res.set_value(std::move(data));
            }
            else
            {
                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [task_res](std::tuple<boost::asynchronous::expected<boost::asynchronous::detail::partition_copy_data>,
                                                  boost::asynchronous::expected<boost::asynchronous::detail::partition_copy_data> > res) mutable
                            {
                                try
                                {
                                    boost::asynchronous::detail::partition_copy_data res_left = std::move(std::get<0>(res).get());
                                    boost::asynchronous::detail::partition_copy_data res_right = std::move(std::get<1>(res).get());
                                    boost::asynchronous::detail::partition_copy_data res_all(res_left.partition_true_ + res_right.partition_true_,
                                                                                             res_left.partition_false_ + res_right.partition_false_);
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
                            parallel_partition_copy_part1_helper<Iterator,Func,Job>
                                (beg_,it,func_,cutoff_,this->get_name(),prio_),
                            parallel_partition_copy_part1_helper<Iterator,Func,Job>
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
boost::asynchronous::detail::callback_continuation<boost::asynchronous::detail::partition_copy_data,Job>
parallel_partition_copy_part1(Iterator beg, Iterator end, Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio=0)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<boost::asynchronous::detail::partition_copy_data,Job>
            (boost::asynchronous::detail::parallel_partition_copy_part1_helper<Iterator,Func,Job>
                (beg,end,std::move(func),cutoff,task_name,prio));

}

template <class Iterator, class OutputIt1, class OutputIt2, class Func, class Job>
struct parallel_partition_copy_part2_helper: public boost::asynchronous::continuation_task<std::pair<OutputIt1, OutputIt2>>
{
    parallel_partition_copy_part2_helper(Iterator beg, Iterator end, OutputIt1 out_true, OutputIt2 out_false,Func func,
                                    std::size_t offset_true, std::size_t offset_false,
                                    boost::asynchronous::detail::partition_copy_data data,
                                    long cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<std::pair<OutputIt1, OutputIt2>>(task_name),
          beg_(beg),end_(end),out_true_(out_true),out_false_(out_false),func_(std::move(func)),
          offset_true_(offset_true),offset_false_(offset_false),data_(std::move(data)),
          cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<std::pair<OutputIt1, OutputIt2>> task_res = this->this_task_result();
        try
        {
            // advance up to cutoff
            Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
            if (it == end_)
            {
                // write true part
                auto out_true = out_true_;
                auto beg = beg_;
                std::advance(out_true,offset_true_);
                auto out_false = out_false_;
                std::advance(out_false,offset_false_);
                while (beg != end_)
                {
                    if (func_(*beg))
                    {
                        *out_true++ = *beg;
                    }
                    else
                    {
                        *out_false++ = *beg;
                    }
                    beg++;
                }

                // done
                task_res.set_value(std::make_pair(out_true,out_false));
            }
            else
            {
                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [task_res]
                            (std::tuple<boost::asynchronous::expected<std::pair<OutputIt1, OutputIt2>>,
                                        boost::asynchronous::expected<std::pair<OutputIt1, OutputIt2>> > res) mutable
                            {
                                try
                                {
                                    // get to check that no exception
                                    std::get<0>(res).get();
                                    task_res.set_value(std::get<1>(res).get());
                                }
                                catch(std::exception& e)
                                {
                                    task_res.set_exception(boost::copy_exception(e));
                                }
                            },
                            // recursive tasks
                            parallel_partition_copy_part2_helper<Iterator,OutputIt1,OutputIt2,Func,Job>
                                    (beg_,it,out_true_,out_false_,func_,
                                     offset_true_,
                                     offset_false_,
                                     data_.data_[0],cutoff_,this->get_name(),prio_),
                            parallel_partition_copy_part2_helper<Iterator,OutputIt1,OutputIt2,Func,Job>
                                    (it,end_,out_true_,out_false_,func_,
                                     offset_true_ + data_.data_[0].partition_true_,
                                     offset_false_ + data_.data_[0].partition_false_,
                                     data_.data_[1],cutoff_,this->get_name(),prio_)
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
    OutputIt1 out_true_;
    OutputIt2 out_false_;
    Func func_;
    std::size_t offset_true_;
    std::size_t offset_false_;
    boost::asynchronous::detail::partition_copy_data data_;
    long cutoff_;
    std::size_t prio_;
};
template <class Iterator,class OutputIt1, class OutputIt2, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<std::pair<OutputIt1, OutputIt2>,Job>
parallel_partition_copy_part2(Iterator beg, Iterator end, OutputIt1 out_true, OutputIt2 out_false, Func func,
                         std::size_t offset_true, std::size_t offset_false, boost::asynchronous::detail::partition_copy_data data,
                         long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio=0)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<std::pair<OutputIt1, OutputIt2>,Job>
            (boost::asynchronous::detail::parallel_partition_copy_part2_helper<Iterator,OutputIt1,OutputIt2,Func,Job>
             (beg,end,out_true,out_false,std::move(func),offset_true,offset_false,std::move(data),cutoff,task_name,prio));

}

template <class Iterator, class OutputIt1, class OutputIt2, class Func, class Job>
struct parallel_partition_copy_helper: public boost::asynchronous::continuation_task<std::pair<OutputIt1, OutputIt2>>
{
    parallel_partition_copy_helper(Iterator beg, Iterator end, OutputIt1 out_true, OutputIt2 out_false, Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<std::pair<OutputIt1, OutputIt2>>(task_name),
          beg_(beg),end_(end),out_true_(out_true),out_false_(out_false),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<std::pair<OutputIt1, OutputIt2>> task_res = this->this_task_result();
        try
        {
            auto cont = boost::asynchronous::detail::parallel_partition_copy_part1<Iterator,Func,Job>
                            (beg_,end_,func_,cutoff_,this->get_name(),prio_);
            auto beg = beg_;
            auto end = end_;
            auto out_true = out_true_;
            auto out_false = out_false_;
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            auto func = func_;
            cont.on_done([task_res,beg,end,out_true,out_false,func,cutoff,task_name,prio]
                         (std::tuple<boost::asynchronous::expected<boost::asynchronous::detail::partition_copy_data> >&& res) mutable
            {
                try
                {
                    boost::asynchronous::detail::partition_copy_data data = std::move(std::get<0>(res).get());
                    auto cont =
                            boost::asynchronous::detail::parallel_partition_copy_part2<Iterator,OutputIt1,OutputIt2,Func,Job>
                            (beg,end,out_true,out_false,std::move(func),0,0,std::move(data),cutoff,task_name,prio);
                    cont.on_done([task_res](std::tuple<boost::asynchronous::expected<std::pair<OutputIt1, OutputIt2>> >&& res)
                    {
                        try
                        {
                            task_res.set_value(std::move(std::get<0>(res).get()));
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
    OutputIt1 out_true_;
    OutputIt2 out_false_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
}

template <class Iterator, class OutputIt1, class OutputIt2,class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<std::pair<OutputIt1, OutputIt2>,Job>
parallel_partition_copy(Iterator beg, Iterator end, OutputIt1 out_true, OutputIt2 out_false, Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio=0)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<std::pair<OutputIt1, OutputIt2>,Job>
            (boost::asynchronous::detail::parallel_partition_copy_helper<Iterator,OutputIt1,OutputIt2,Func,Job>
                (beg,end,out_true,out_false,std::move(func),cutoff,task_name,prio));

}
}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_PARTITION_COPY_HPP
