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

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/iterator_range.hpp>
#include <type_traits>
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
    partition_data(std::size_t partition_true=0, std::size_t partition_false=0, std::vector<bool> values = std::vector<bool>())
        : partition_true_(partition_true),partition_false_(partition_false),values_(std::move(values))
        , data_()
    {}
    partition_data(partition_data&& rhs) noexcept
        : partition_true_(rhs.partition_true_)
        , partition_false_(rhs.partition_false_)
        , values_(std::move(rhs.values_))
        , data_(std::move(rhs.data_))
    {
    }
    partition_data& operator=(partition_data&& rhs) noexcept
    {
        partition_true_ = std::move(rhs.partition_true_);
        partition_false_ = std::move(rhs.partition_false_);
        values_ = std::move(rhs.values_);
        data_ = std::move(rhs.data_);
        return *this;
    }
    // how many elelements yield true in this part of the tree (to know where the sibling node will start)
    std::size_t partition_true_;
    // how many elelements yield false in this part of the tree
    std::size_t partition_false_;
    // true/fase response of the elements to be partitioned, in this subnode
    std::vector<bool> values_;
    // subnodes (binary tree)
    std::vector<partition_data> data_;
};

// first pass, build a partition_data by calculating response of an element and offsets for each subnode
template <class Iterator, class Func, class Job>
struct parallel_stable_partition_part1_helper: public boost::asynchronous::continuation_task<boost::asynchronous::detail::partition_data>
{
    parallel_stable_partition_part1_helper(Iterator beg, Iterator end, Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<boost::asynchronous::detail::partition_data>(task_name),
          beg_(beg),end_(end),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<boost::asynchronous::detail::partition_data> task_res = this_task_result();
        try
        {
            // advance up to cutoff
            Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
            // elements < cutoff, sequential part
            if (it == end_)
            {
                std::size_t count_true=0;
                std::size_t count_false=0;
                std::vector<bool> values(std::distance(beg_,end_),true);
                std::size_t i=0;
                for (auto it = beg_; it != end_; ++it,++i)
                {
                    // calculate response, update offset
                    if (func_(*it))
                    {
                        ++count_true;
                    }
                    else
                    {
                        ++count_false;
                        values[i]=false;
                    }
                }
                partition_data data(count_true,count_false,std::move(values));
                task_res.set_value(std::move(data));
            }
            else
            {
                // parallel part, create a left and right part, post one, execute one
                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [task_res](std::tuple<boost::asynchronous::expected<boost::asynchronous::detail::partition_data>,
                                                  boost::asynchronous::expected<boost::asynchronous::detail::partition_data> > res)mutable
                            {
                                // continuation callback, both tasks are done, merge results
                                // add left and right into a new node, add offsets from left and right
                                try
                                {
                                    boost::asynchronous::detail::partition_data res_left = std::move(std::get<0>(res).get());
                                    boost::asynchronous::detail::partition_data res_right = std::move(std::get<1>(res).get());
                                    boost::asynchronous::detail::partition_data res_all(res_left.partition_true_ + res_right.partition_true_,
                                                                                        res_left.partition_false_ + res_right.partition_false_);
                                    res_all.data_.reserve(2);
                                    res_all.data_.emplace_back(std::move(res_left));
                                    res_all.data_.emplace_back(std::move(res_right));
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

// first pass, build a partition_data, returned as continuation
template <class Iterator,class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<boost::asynchronous::detail::partition_data,Job>
parallel_stable_partition_part1(Iterator beg, Iterator end, Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio=0)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<boost::asynchronous::detail::partition_data,Job>
            (boost::asynchronous::detail::parallel_stable_partition_part1_helper<Iterator,Func,Job>
                (beg,end,std::move(func),cutoff,task_name,prio));

}

// second pass, partition based on offsets from first pass
// we use the fact that the sequential part will work on the same elemenets that in first pass
// as cutoff stays unchanged
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
    void operator()()
    {
        boost::asynchronous::continuation_result<void> task_res = this->this_task_result();
        try
        {
            // advance up to cutoff
            Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
            // elements < cutoff, sequential part
            if (it == end_)
            {
                auto out = out_;
                std::advance(out,offset_true_);
                auto out2 = out_+offset_false_+ start_false_;
                std::size_t values_counter=0;
                for (auto it2 = beg_ ; it2 != end_ ; ++it2, ++values_counter)
                {
                    // write true elements
                    if (data_.values_[values_counter])
                    {
                        *out++ = *it2;
                    }
                    // write false elements
                    else
                    {
                        *out2++ = *it2;
                    }
                }

                // done
                task_res.set_value();
            }
            // parallel, post a task, execute one
            else
            {
                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [task_res](std::tuple<boost::asynchronous::expected<void>,boost::asynchronous::expected<void> > res)mutable
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
                                     std::move(data_.data_[0]),cutoff_,this->get_name(),prio_),
                            parallel_stable_partition_part2_helper<Iterator,Iterator2,Func,Job>
                                    (it,end_,out_,func_,start_false_,
                                     offset_true_ + data_.data_[0].partition_true_,
                                     offset_false_ + data_.data_[0].partition_false_,
                                     std::move(data_.data_[1]),cutoff_,this->get_name(),prio_)
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
    Iterator2 out_;
    Func func_;
    std::size_t start_false_;
    std::size_t offset_true_;
    std::size_t offset_false_;
    boost::asynchronous::detail::partition_data data_;
    long cutoff_;
    std::size_t prio_;
};

// second pass, partition based on offsets from first pass
template <class Iterator,class Iterator2, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_stable_partition_part2(Iterator beg, Iterator end, Iterator2 out, Func func, std::size_t start_false,
                         std::size_t offset_true, std::size_t offset_false, boost::asynchronous::detail::partition_data data,
                         long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio=0)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_stable_partition_part2_helper<Iterator,Iterator2,Func,Job>
             (beg,end,out,std::move(func),start_false,offset_true,offset_false,std::move(data),cutoff,task_name,prio));

}

// calls first pass, waits for continuation (on_done), then start second pass
template <class Iterator, class Iterator2, class Func, class Job>
struct parallel_stable_partition_helper: public boost::asynchronous::continuation_task<Iterator2>
{
    parallel_stable_partition_helper(Iterator beg, Iterator end, Iterator2 out, Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<Iterator2>(task_name),
          beg_(beg),end_(end),out_(out),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<Iterator2> task_res = this->this_task_result();
        try
        {
#ifdef BOOST_ASYNCHRONOUS_TIMING
            auto p1_start = boost::chrono::high_resolution_clock::now();
#endif
            // call first pass
            auto cont = boost::asynchronous::detail::parallel_stable_partition_part1<Iterator,Func,Job>(beg_,end_,func_,cutoff_,this->get_name(),prio_);
            auto beg = beg_;
            auto end = end_;
            auto out = out_;
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            auto func = func_;
            // this lambda will be called when first pass is done
            cont.on_done([task_res,beg,end,out,func,cutoff,task_name,prio
#ifdef BOOST_ASYNCHRONOUS_TIMING
                        ,p1_start
#endif
                        ]
                        (std::tuple<boost::asynchronous::expected<boost::asynchronous::detail::partition_data> >&& res)
            {
#ifdef BOOST_ASYNCHRONOUS_TIMING
                auto p1_stop = boost::chrono::high_resolution_clock::now();
                double p1_time = (boost::chrono::nanoseconds(p1_stop - p1_start).count() / 1000000);
                printf ("%50s: time = %.1f msec\n","p1_time", p1_time);
#endif
                try
                {
                    boost::asynchronous::detail::partition_data data = std::move(std::get<0>(res).get());
                    std::size_t start_false = data.partition_true_;
#ifdef BOOST_ASYNCHRONOUS_TIMING
                    auto p2_start = boost::chrono::high_resolution_clock::now();
#endif
                    // call second pass, get a continuation
                    auto cont =
                            boost::asynchronous::detail::parallel_stable_partition_part2<Iterator,Iterator2,Func,Job>
                            (beg,end,out,std::move(func),start_false,0,0,std::move(data),cutoff,task_name,prio);
                    Iterator2 ret = out;
                    std::advance(ret,start_false);
                    // this lambda will be called when second pass is done
                    cont.on_done([task_res,ret
#ifdef BOOST_ASYNCHRONOUS_TIMING
                                ,p2_start
#endif
                                 ](std::tuple<boost::asynchronous::expected<void> >&& res)mutable
                    {
#ifdef BOOST_ASYNCHRONOUS_TIMING
                        auto p2_stop = boost::chrono::high_resolution_clock::now();
                        double p2_time = (boost::chrono::nanoseconds(p2_stop - p2_start).count() / 1000000);
                        printf ("%50s: time = %.1f msec\n","p2_time", p2_time);
#endif
                        try
                        {
                            // get to check that no exception
                            std::get<0>(res).get();
                            // both passes are done, we now inform library that it can call callback / set future
                            task_res.set_value(ret);
                        }
                        catch(std::exception& e)
                        {
                            task_res.set_exception(boost::copy_exception(e));
                        }
                    });
                }
                //exceptions are propagated
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

// version with iterators
template <class Iterator, class Iterator2,class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<Iterator2,Job>
parallel_stable_partition(Iterator beg, Iterator end, Iterator2 out, Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio=0)
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
        boost::asynchronous::continuation_result<std::pair<Range,Iterator>> task_res = this->this_task_result();
        try
        {
            boost::shared_ptr<Range> range = range_;
            // TODO better ctor?
            boost::shared_ptr<Range> new_range = boost::make_shared<Range>(range->size());
            auto cont = boost::asynchronous::parallel_stable_partition<decltype(beg_),decltype(beg_),Func,Job>
                    (beg_,end_,boost::begin(*new_range),std::move(func_),cutoff_,this->get_name(),prio_);
            cont.on_done([task_res,range,new_range]
                          (std::tuple<boost::asynchronous::expected<Iterator> >&& continuation_res)mutable
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
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }
    boost::shared_ptr<Range> range_;
    Iterator beg_;
    Iterator end_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};

// version for moved ranges => will return the range as continuation
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
auto parallel_stable_partition(Range&& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio=0)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
-> typename std::enable_if<
              !boost::asynchronous::detail::has_is_continuation_task<Range>::value,
              // TODO make it work with boost::begin and clang
              boost::asynchronous::detail::callback_continuation<std::pair<Range,decltype(range.begin())>,Job> >::type

{
    auto r = boost::make_shared<Range>(std::forward<Range>(range));
    auto beg = boost::begin(*r);
    auto end = boost::end(*r);
    return boost::asynchronous::top_level_callback_continuation_job<std::pair<Range,decltype(boost::begin(range))>,Job>
            (boost::asynchronous::parallel_stable_partition_range_move_helper<Range,decltype(beg),Func,Job>
                (r,beg,end,func,cutoff,task_name,prio));
}

namespace detail
{
// Continuation is a callback continuation
template <class Continuation, class Iterator,class Func, class Job>
struct parallel_stable_partition_continuation_helper:
        public boost::asynchronous::continuation_task<
            std::pair<
                typename Continuation::return_type,
                Iterator>
       >
{
    parallel_stable_partition_continuation_helper(Continuation const& c,Func func, long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<
            std::pair<
                typename Continuation::return_type,
                Iterator>
          >(task_name)
        ,cont_(c),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        using return_type=
        std::pair<
            typename Continuation::return_type,
            Iterator>;

        boost::asynchronous::continuation_result<return_type> task_res = this->this_task_result();
        try
        {
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            auto func= std::move(func_);
            cont_.on_done([task_res,func,cutoff,task_name,prio]
                          (std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& continuation_res)mutable
            {
                try
                {
                    auto new_continuation =
                       boost::asynchronous::parallel_stable_partition<typename Continuation::return_type, Func, Job>
                            (std::move(std::get<0>(continuation_res).get()),std::move(func),cutoff,task_name,prio);
                    new_continuation.on_done(
                    [task_res]
                    (std::tuple<boost::asynchronous::expected<return_type> >&& new_continuation_res)
                    {
                        task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                    });
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
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
}

// version where the range is itself a continuation
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<
        boost::asynchronous::detail::has_is_continuation_task<Range>::value,
        boost::asynchronous::detail::callback_continuation<
              std::pair<typename Range::return_type,decltype(boost::begin(std::declval<typename Range::return_type&>()))>,
              Job>
>::type
parallel_stable_partition(Range&& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio=0)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<
                std::pair<typename Range::return_type,decltype(boost::begin(std::declval<typename Range::return_type&>()))>,
                Job
            >
            (boost::asynchronous::detail::parallel_stable_partition_continuation_helper<
                Range,
                decltype(boost::begin(std::declval<typename Range::return_type&>())),
                Func,
                Job>
                    (std::forward<Range>(range),func,cutoff,task_name,prio));
}

}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_STABLE_PARTITION_HPP
