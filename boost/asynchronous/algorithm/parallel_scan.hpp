// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_SCAN_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_SCAN_HPP

#include <algorithm>

#include <boost/utility/enable_if.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>

namespace boost { namespace asynchronous
{
namespace detail
{
// tree structure containing accumulated results from part 1 (partitioning)
template <class T>
struct scan_data
{
    scan_data(T val=T())
        : value_(std::move(val))
        , data_()
    {}
    scan_data(scan_data&&)=default;
    scan_data& operator=(scan_data&&)=default;
    T value_;
    std::vector<scan_data<T>> data_;
};

// part 1: reduce and combine (result of combining is in scan_data tree)
template <class Iterator, class T, class Reduce, class Combine, class Job>
struct parallel_scan_part1_helper: public boost::asynchronous::continuation_task<boost::asynchronous::detail::scan_data<T>>
{
    parallel_scan_part1_helper(Iterator beg, Iterator end, Reduce r, Combine c,
                            long cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<boost::asynchronous::detail::scan_data<T>>(task_name)
        , beg_(beg),end_(end), reduce_(std::move(r)), combine_(std::move(c)), cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<boost::asynchronous::detail::scan_data<T>> task_res = this->this_task_result();
        // advance up to cutoff
        Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
        if (it == end_)
        {
            task_res.set_value(boost::asynchronous::detail::scan_data<T>(std::move(reduce_(beg_,end_))));
        }
        else
        {
            auto c = combine_;
            boost::asynchronous::create_callback_continuation_job<Job>(
                        // called when subtasks are done, set our result
                        [task_res,c](std::tuple<boost::asynchronous::expected<boost::asynchronous::detail::scan_data<T>>,
                                                boost::asynchronous::expected<boost::asynchronous::detail::scan_data<T>> > res)mutable
                        {
                            try
                            {
                                boost::asynchronous::detail::scan_data<T> res_left = std::move(std::get<0>(res).get());
                                boost::asynchronous::detail::scan_data<T> res_right = std::move(std::get<1>(res).get());
                                boost::asynchronous::detail::scan_data<T> res_all(c(res_left.value_ , res_right.value_));
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
                        parallel_scan_part1_helper<Iterator,T,Reduce,Combine,Job>
                            (beg_,it,reduce_,combine_,cutoff_,this->get_name(),prio_),
                        parallel_scan_part1_helper<Iterator,T,Reduce,Combine,Job>
                            (it,end_,reduce_,combine_,cutoff_,this->get_name(),prio_)
            );
        }
    }

    Iterator beg_;
    Iterator end_;
    Reduce reduce_;
    Combine combine_;
    long cutoff_;
    std::size_t prio_;
};

template <class Iterator, class T, class Reduce, class Combine, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<boost::asynchronous::detail::scan_data<T>,Job>
parallel_scan_part1(Iterator beg, Iterator end, T /*init*/,
                    Reduce r, Combine c,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<boost::asynchronous::detail::scan_data<T>,Job>
            (boost::asynchronous::detail::parallel_scan_part1_helper<Iterator,T,Reduce,Combine,Job>
                (beg,end,std::move(r),std::move(c),cutoff,task_name,prio));

}

// part 2: scan
template <class Iterator, class OutIterator, class T, class Scan, class Combine, class Job>
struct parallel_scan_part2_helper: public boost::asynchronous::continuation_task<OutIterator>
{
    parallel_scan_part2_helper(Iterator beg, Iterator end, OutIterator out, T init, Scan s, Combine c,
                               boost::asynchronous::detail::scan_data<T> d,
                               long cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<OutIterator>(task_name)
        , beg_(beg), end_(end), out_(out), init_(std::move(init)), scan_(std::move(s)), combine_(std::move(c))
        , data_(std::move(d)), cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<OutIterator> task_res = this->this_task_result();
        // advance up to cutoff
        Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
        auto out = out_;
        std::advance(out,std::distance(beg_,it));
        if (it == end_)
        {
            scan_(beg_,end_,out_,init_);
            task_res.set_value(out);
        }
        else
        {
            // init of right side => init
            // init of left side = init + value of current node.left
            auto init_right = combine_(init_,data_.data_[0].value_);
            boost::asynchronous::create_callback_continuation_job<Job>(
                        // called when subtasks are done, set our result
                        [task_res](std::tuple<boost::asynchronous::expected<OutIterator>,
                                              boost::asynchronous::expected<OutIterator>> res)mutable
                        {
                            try
                            {
                                // we are not interested in the first iterator, we just get() in case of an exception
                                std::move(std::get<0>(res).get());
                                OutIterator right = std::move(std::get<1>(res).get());;
                                task_res.set_value(std::move(right));
                            }
                            catch(std::exception& e)
                            {
                                task_res.set_exception(boost::copy_exception(e));
                            }
                        },
                        // recursive tasks
                        parallel_scan_part2_helper<Iterator,OutIterator,T,Scan,Combine,Job>
                                (beg_,it,out_,init_,scan_,combine_,std::move(data_.data_[0]),cutoff_,this->get_name(),prio_),
                        parallel_scan_part2_helper<Iterator,OutIterator,T,Scan,Combine,Job>
                                (it,end_,out,init_right,scan_,combine_,std::move(data_.data_[1]),cutoff_,this->get_name(),prio_)
            );
        }
    }

    Iterator beg_;
    Iterator end_;
    OutIterator out_;
    T init_;
    Scan scan_;
    Combine combine_;
    boost::asynchronous::detail::scan_data<T> data_;
    long cutoff_;
    std::size_t prio_;
};

template <class Iterator, class OutIterator, class T, class Scan, class Combine, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<OutIterator,Job>
parallel_scan_part2(Iterator beg, Iterator end, OutIterator out, T init,
                    Scan s, Combine c, boost::asynchronous::detail::scan_data<T> d, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<OutIterator,Job>
            (boost::asynchronous::detail::parallel_scan_part2_helper<Iterator,OutIterator,T,Scan,Combine,Job>
             (beg,end,out,std::move(init),std::move(s),std::move(c),std::move(d),cutoff,task_name,prio));

}

// calls part 1, then part 2
template <class Iterator, class OutIterator, class T, class Reduce, class Combine, class Scan, class Job>
struct parallel_scan_helper: public boost::asynchronous::continuation_task<OutIterator>
{
    parallel_scan_helper(Iterator beg, Iterator end,OutIterator out, T init, Reduce r, Combine c, Scan s,
                            long cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<OutIterator>(task_name)
        , beg_(beg),end_(end), out_(out), init_(std::move(init))
        , reduce_(std::move(r)), combine_(std::move(c)), scan_(std::move(s))
        , cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<OutIterator> task_res = this->this_task_result();
        auto cont = boost::asynchronous::detail::parallel_scan_part1<Iterator,T,Reduce,Combine,Job>
                (beg_,end_,init_,reduce_,combine_,cutoff_,this->get_name()+"_part1",prio_);
        auto beg = beg_;
        auto end = end_;
        auto out = out_;
        auto cutoff = cutoff_;
        auto task_name = this->get_name();
        auto prio = prio_;
        auto scan = scan_;
        auto init = init_;
        auto combine = combine_;
        cont.on_done([task_res,beg,end,out,init,scan,combine,cutoff,task_name,prio]
                     (std::tuple<boost::asynchronous::expected<boost::asynchronous::detail::scan_data<T>>>&& res)mutable
        {
            try
            {
                boost::asynchronous::detail::scan_data<T> data = std::move(std::get<0>(res).get());
                auto cont = boost::asynchronous::detail::parallel_scan_part2<Iterator,OutIterator,T,Scan,Combine,Job>
                        (beg,end,out,std::move(init),std::move(scan),std::move(combine),std::move(data),cutoff,task_name,prio);
                cont.on_done([task_res](std::tuple<boost::asynchronous::expected<OutIterator> >&& res)mutable
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

    Iterator beg_;
    Iterator end_;
    OutIterator out_;
    T init_;
    Reduce reduce_;
    Combine combine_;
    Scan scan_;
    long cutoff_;
    std::size_t prio_;
};

}

template <class Iterator, class OutIterator, class T, class Reduce, class Combine, class Scan,
          class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<OutIterator,Job>
parallel_scan(Iterator beg, Iterator end, OutIterator out, T init,
              Reduce r, Combine c, Scan s,
              long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<OutIterator,Job>
            (boost::asynchronous::detail::parallel_scan_helper<Iterator,OutIterator,T,Reduce,Combine,Scan,Job>
                (beg,end,out,std::move(init),std::move(r),std::move(c),std::move(s),cutoff,task_name,prio));

}

}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_SCAN_HPP
