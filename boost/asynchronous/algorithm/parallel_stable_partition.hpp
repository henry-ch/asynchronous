// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_STABLE_PARTITION_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_STABLE_PARTITION_HPP

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
// tree structure containing accumulated results from part 1 (partitioning)
struct partition_data
{
    partition_data(std::size_t partition_true=0, std::size_t partition_false=0)
        : partition_true_(partition_true),partition_false_(partition_false)
        , data_()
    {}
    std::size_t partition_true_;
    std::size_t partition_false_;
    std::vector<partition_data> data_;
};

template <class Iterator, class Func, class Job>
struct parallel_stable_partition_part1_helper: public boost::asynchronous::continuation_task<boost::asynchronous::detail::partition_data>
{
    parallel_stable_partition_part1_helper(Iterator beg, Iterator end, Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<boost::asynchronous::detail::partition_data>(task_name),
          beg_(beg),end_(end),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()const
    {
        boost::asynchronous::continuation_result<boost::asynchronous::detail::partition_data> task_res = this_task_result();
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
            partition_data data(count_true,count_false);
            //Iterator it_part = std::partition(beg_,end_,std::move(func_));
            //partition_data data(std::distance(beg_,it_part),std::distance(it_part,end_));
            task_res.set_value(std::move(data));
        }
        else
        {
            boost::asynchronous::create_callback_continuation_job<Job>(
                        // called when subtasks are done, set our result
                        [task_res](std::tuple<boost::asynchronous::expected<boost::asynchronous::detail::partition_data>,
                                              boost::asynchronous::expected<boost::asynchronous::detail::partition_data> > res)
                        {
                            try
                            {
                                boost::asynchronous::detail::partition_data res_left = std::move(std::get<0>(res).get());
                                boost::asynchronous::detail::partition_data res_right = std::move(std::get<1>(res).get());
                                boost::asynchronous::detail::partition_data res_all(res_left.partition_true_ + res_right.partition_true_,
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
                        parallel_stable_partition_part1_helper<Iterator,Func,Job>
                            (beg_,it,func_,cutoff_,this->get_name(),prio_),
                        parallel_stable_partition_part1_helper<Iterator,Func,Job>
                            (it,end_,func_,cutoff_,this->get_name(),prio_)
            );
        }
    }
    Iterator beg_;
    Iterator end_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};

template <class Iterator,class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<boost::asynchronous::detail::partition_data,Job>
parallel_stable_partition_part1(Iterator beg, Iterator end, Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<boost::asynchronous::detail::partition_data,Job>
            (boost::asynchronous::detail::parallel_stable_partition_part1_helper<Iterator,Func,Job>
                (beg,end,std::move(func),cutoff,task_name,prio));

}

template <class Iterator, class Iterator2, class Func, class Job>
struct parallel_stable_partition_part2_helper: public boost::asynchronous::continuation_task<void>
{
    parallel_stable_partition_part2_helper(Iterator beg, Iterator end, Iterator2 out,Func func,
                                    std::size_t start_false,
                                    std::size_t offset_true, std::size_t offset_false,
                                    boost::asynchronous::detail::partition_data data,
                                    long cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<void>(task_name),
          beg_(beg),end_(end),out_(out),func_(std::move(func)),start_false_(start_false),offset_true_(offset_true),offset_false_(offset_false),data_(std::move(data)),
          cutoff_(cutoff),prio_(prio)
    {}
    void operator()()const
    {
        boost::asynchronous::continuation_result<void> task_res = this_task_result();
        // advance up to cutoff
        Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
        if (it == end_)
        {
            // write true part
            auto out = out_;
            auto beg = beg_;
            std::advance(out,offset_true_);
            auto out2 = out_+offset_false_+ start_false_;
            while (beg != end_)
            {
                if (func_(*beg))
                {
                    *out++ = *beg;
                }
                else
                {
                    *out2++ = *beg;
                }
                beg++;
            }

            // done
            task_res.set_value();
        }
        else
        {
            boost::asynchronous::create_callback_continuation_job<Job>(
                        // called when subtasks are done, set our result
                        [task_res](std::tuple<boost::asynchronous::expected<void>,boost::asynchronous::expected<void> > res)
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
                        parallel_stable_partition_part2_helper<Iterator,Iterator2,Func,Job>
                                (beg_,it,out_,func_,start_false_,
                                 offset_true_,
                                 offset_false_,
                                 data_.data_[0],cutoff_,this->get_name(),prio_),
                        parallel_stable_partition_part2_helper<Iterator,Iterator2,Func,Job>
                                (it,end_,out_,func_,start_false_,
                                 offset_true_ + data_.data_[0].partition_true_,
                                 offset_false_ + data_.data_[0].partition_false_,
                                 data_.data_[1],cutoff_,this->get_name(),prio_)
            );
        }
    }
    Iterator beg_;
    Iterator end_;
    Iterator2 out_;
    Func func_;
    std::size_t start_false_;
    std::size_t offset_true_;
    std::size_t offset_false_;
    boost::asynchronous::detail::partition_data data_;
    long cutoff_;
    std::size_t prio_;
};
template <class Iterator,class Iterator2, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_stable_partition_part2(Iterator beg, Iterator end, Iterator2 out, Func func, std::size_t start_false,
                         std::size_t offset_true, std::size_t offset_false, boost::asynchronous::detail::partition_data data,
                         long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_stable_partition_part2_helper<Iterator,Iterator2,Func,Job>
             (beg,end,out,std::move(func),start_false,offset_true,offset_false,std::move(data),cutoff,task_name,prio));

}

template <class Iterator, class Iterator2, class Func, class Job>
struct parallel_stable_partition_helper: public boost::asynchronous::continuation_task<Iterator2>
{
    parallel_stable_partition_helper(Iterator beg, Iterator end, Iterator2 out, Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<Iterator2>(task_name),
          beg_(beg),end_(end),out_(out),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()const
    {
        boost::asynchronous::continuation_result<Iterator2> task_res = this->this_task_result();
        //auto p1_start = boost::chrono::high_resolution_clock::now();
        auto cont = boost::asynchronous::detail::parallel_stable_partition_part1<Iterator,Func,Job>(beg_,end_,func_,cutoff_,this->get_name(),prio_);
        auto beg = beg_;
        auto end = end_;
        auto out = out_;
        auto cutoff = cutoff_;
        auto task_name = this->get_name();
        auto prio = prio_;
        auto func = func_;
        cont.on_done([task_res,beg,end,out,func,cutoff,task_name,prio/*,p1_start*/]
                     (std::tuple<boost::asynchronous::expected<boost::asynchronous::detail::partition_data> >&& res)
        {
//            auto p1_stop = boost::chrono::high_resolution_clock::now();
//            double p1_time = (boost::chrono::nanoseconds(p1_stop - p1_start).count() / 1000000);
//            printf ("%50s: time = %.1f msec\n","p1_time", p1_time);
            try
            {
                boost::asynchronous::detail::partition_data data = std::move(std::get<0>(res).get());
                std::size_t start_false = data.partition_true_;
//                auto p2_start = boost::chrono::high_resolution_clock::now();
                auto cont =
                        boost::asynchronous::detail::parallel_stable_partition_part2<Iterator,Iterator2,Func,Job>
                        (beg,end,out,std::move(func),start_false,0,0,std::move(data),cutoff,task_name,prio);
                Iterator2 ret = out;
                std::advance(ret,start_false);
                cont.on_done([task_res,ret/*,p2_start*/](std::tuple<boost::asynchronous::expected<void> >&& res)
                {
//                    auto p2_stop = boost::chrono::high_resolution_clock::now();
//                    double p2_time = (boost::chrono::nanoseconds(p2_stop - p2_start).count() / 1000000);
//                    printf ("%50s: time = %.1f msec\n","p2_time", p2_time);
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
parallel_stable_partition(Iterator beg, Iterator end, Iterator2 out, Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<Iterator2,Job>
            (boost::asynchronous::detail::parallel_stable_partition_helper<Iterator,Iterator2,Func,Job>
                (beg,end,out,std::move(func),cutoff,task_name,prio));

}

// version for moved ranges => will return the range as continuation
template <class Range, class Iterator,class Func, class Job,class Enable=void>
struct parallel_stable_partition_range_move_helper:
        public boost::asynchronous::continuation_task<std::pair<Range,Iterator>>
{
    parallel_stable_partition_range_move_helper(boost::shared_ptr<Range> range,Iterator beg, Iterator end,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<std::pair<Range,Iterator>>(task_name)
        ,range_(range),beg_(beg),end_(end),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {
    }
    parallel_stable_partition_range_move_helper(parallel_stable_partition_range_move_helper&&)=default;
    parallel_stable_partition_range_move_helper& operator=(parallel_stable_partition_range_move_helper&&)=default;
    parallel_stable_partition_range_move_helper(parallel_stable_partition_range_move_helper const&)=delete;
    parallel_stable_partition_range_move_helper& operator=(parallel_stable_partition_range_move_helper const&)=delete;

    void operator()()
    {
        boost::shared_ptr<Range> range = range_;
        // TODO better ctor?
        boost::shared_ptr<Range> new_range = boost::make_shared<Range>(range->size());
        boost::asynchronous::continuation_result<std::pair<Range,Iterator>> task_res
                                                                                              = this->this_task_result();
        auto cont = boost::asynchronous::parallel_stable_partition<decltype(beg_),decltype(beg_),Func,Job>
                (beg_,end_,boost::begin(*new_range),std::move(func_),cutoff_,this->get_name(),prio_);
        cont.on_done([task_res,range,new_range]
                      (std::tuple<boost::asynchronous::expected<Iterator> >&& continuation_res)
        {
            try
            {
                task_res.set_value(std::make_pair(std::move(*new_range),std::get<0>(continuation_res).get()));
            }
            catch(std::exception& e)
            {
                task_res.set_exception(boost::copy_exception(e));
            }
        });

    }
    boost::shared_ptr<Range> range_;
    Iterator beg_;
    Iterator end_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};


template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
auto parallel_stable_partition(Range&& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
-> typename boost::disable_if<
              has_is_continuation_task<Range>,
              boost::asynchronous::detail::callback_continuation<std::pair<Range,decltype(boost::begin(range))>,Job> >::type

{
    auto r = boost::make_shared<Range>(std::forward<Range>(range));
    auto beg = boost::begin(*r);
    auto end = boost::end(*r);
    return boost::asynchronous::top_level_callback_continuation_job<std::pair<Range,decltype(boost::begin(range))>,Job>
            (boost::asynchronous::parallel_stable_partition_range_move_helper<Range,decltype(beg),Func,Job>
                (r,beg,end,func,cutoff,task_name,prio));
}

}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_STABLE_PARTITION_HPP
