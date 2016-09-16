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
#include <boost/asynchronous/detail/function_traits.hpp>

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
        try
        {
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
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
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

template <class Func, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4>
void func_arity_helper(Func& f,
                       Arg0&& arg0,Arg1&& arg1,Arg2&& arg2,Arg3&& arg3,Arg4&& ,
                       typename boost::enable_if_c<boost::asynchronous::function_traits<Func>::arity == 4>::type* =0)
{
    f(std::forward<Arg0>(arg0),std::forward<Arg1>(arg1),std::forward<Arg2>(arg2),std::forward<Arg3>(arg3));
}
template <class Func, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4>
void func_arity_helper(Func& f,
                       Arg0&& arg0,Arg1&& arg1,Arg2&& arg2,Arg3&& arg3,Arg4&& arg4,
                       typename boost::enable_if_c<boost::asynchronous::function_traits<Func>::arity == 5>::type* =0)
{
    f(std::forward<Arg0>(arg0),std::forward<Arg1>(arg1),std::forward<Arg2>(arg2),std::forward<Arg3>(arg3),std::forward<Arg4>(arg4));
}

// part 2: scan
template <class Iterator, class OutIterator, class T, class Scan, class Combine, class Job>
struct parallel_scan_part2_helper: public boost::asynchronous::continuation_task<T>
{
    parallel_scan_part2_helper(Iterator beg, Iterator end, OutIterator out, T init, Scan s, Combine c,int depth,
                               boost::shared_ptr<boost::asynchronous::detail::scan_data<T>> part1_accumulated,
                               boost::asynchronous::detail::scan_data<T>* d,
                               long cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<T>(task_name)
        , beg_(beg), end_(end), out_(out), init_(std::move(init)), scan_(std::move(s)), combine_(std::move(c))
        , depth_(depth),part1_accumulated_(part1_accumulated), data_(d), cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<T> task_res = this->this_task_result();
        try
        {
            // advance up to cutoff
            Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
            auto out = out_;
            std::advance(out,std::distance(beg_,it));
            if (it == end_)
            {
                //scan_(beg_,end_,out_,init_);
                boost::asynchronous::detail::func_arity_helper(scan_,beg_,end_,out_,init_,part1_accumulated_->value_);
                // save performance by only setting result when top-level task
                if (depth_ == 0)
                    task_res.set_value(part1_accumulated_->value_);
                else
                    task_res.set_value(T());
            }
            else
            {
                auto part1_accumulated = part1_accumulated_;
                // init of right side => init
                // init of left side = init + value of current node.left
                auto init_right = combine_(init_,data_->data_[0].value_);
                auto depth =depth_;
                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [task_res,part1_accumulated,depth](std::tuple<boost::asynchronous::expected<T>,
                                                                    boost::asynchronous::expected<T>> res)mutable
                            {
                                try
                                {
                                    // we are not interested in the first part results, we just get() in case of an exception
                                    std::get<0>(res).get();
                                    std::get<1>(res).get();
                                    if (depth == 0)
                                        task_res.set_value(part1_accumulated->value_);
                                    else
                                        task_res.set_value(T());
                                }
                                catch(std::exception& e)
                                {
                                    task_res.set_exception(boost::copy_exception(e));
                                }
                            },
                            // recursive tasks
                            parallel_scan_part2_helper<Iterator,OutIterator,T,Scan,Combine,Job>
                                    (beg_,it,out_,init_,scan_,combine_,depth_+1,part1_accumulated_,&data_->data_[0],cutoff_,this->get_name(),prio_),
                            parallel_scan_part2_helper<Iterator,OutIterator,T,Scan,Combine,Job>
                                    (it,end_,out,init_right,scan_,combine_,depth_+1,part1_accumulated_,&data_->data_[1],cutoff_,this->get_name(),prio_)
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
    T init_;
    Scan scan_;
    Combine combine_;
    int depth_;
    boost::shared_ptr<boost::asynchronous::detail::scan_data<T>> part1_accumulated_;
    boost::asynchronous::detail::scan_data<T>* data_;
    long cutoff_;
    std::size_t prio_;
};

template <class Iterator, class OutIterator, class T, class Scan, class Combine, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<T,Job>
parallel_scan_part2(Iterator beg, Iterator end, OutIterator out, T init,
                    Scan s, Combine c, boost::shared_ptr<boost::asynchronous::detail::scan_data<T>> part1_accumulated,
                    boost::asynchronous::detail::scan_data<T>* d, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<T,Job>
            (boost::asynchronous::detail::parallel_scan_part2_helper<Iterator,OutIterator,T,Scan,Combine,Job>
             (beg,end,out,std::move(init),std::move(s),std::move(c),0,part1_accumulated,d,cutoff,task_name,prio));

}

// calls part 1, then part 2
template <class Iterator, class OutIterator, class T, class Reduce, class Combine, class Scan, class Job>
struct parallel_scan_helper: public boost::asynchronous::continuation_task<T>
{
    parallel_scan_helper(Iterator beg, Iterator end,OutIterator out, T init, Reduce r, Combine c, Scan s,
                            long cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<T>(task_name)
        , beg_(beg),end_(end), out_(out), init_(std::move(init))
        , reduce_(std::move(r)), combine_(std::move(c)), scan_(std::move(s))
        , cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<T> task_res = this->this_task_result();
        try
        {
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
                    boost::shared_ptr<boost::asynchronous::detail::scan_data<T>> data =
                            boost::make_shared<boost::asynchronous::detail::scan_data<T>>(std::move(std::get<0>(res).get()));
                    auto cont = boost::asynchronous::detail::parallel_scan_part2<Iterator,OutIterator,T,Scan,Combine,Job>
                            (beg,end,out,std::move(init),std::move(scan),std::move(combine),
                             data, data.get(),cutoff,task_name,prio);
                    cont.on_done([task_res](std::tuple<boost::asynchronous::expected<T> >&& res)mutable
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
boost::asynchronous::detail::callback_continuation<T,Job>
parallel_scan(Iterator beg, Iterator end, OutIterator out, T init,
              Reduce r, Combine c, Scan s,
              long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<T,Job>
            (boost::asynchronous::detail::parallel_scan_helper<Iterator,OutIterator,T,Reduce,Combine,Scan,Job>
                (beg,end,out,std::move(init),std::move(r),std::move(c),std::move(s),cutoff,task_name,prio));

}
// version for moved ranges => will return the ranges (input + output) as continuation
template <class Range, class OutRange, class T, class Reduce, class Combine, class Scan, class Job,class Enable=void>
struct parallel_scan_range_move_helper: public boost::asynchronous::continuation_task<std::pair<Range,OutRange>>
{
    parallel_scan_range_move_helper(boost::shared_ptr<Range> range,boost::shared_ptr<OutRange> out_range, T init,
                                    Reduce r, Combine c, Scan s,
                                    long cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<std::pair<Range,OutRange>>(task_name)
        , range_(range), out_range_(out_range),init_(std::move(init))
        , reduce_(std::move(r)), combine_(std::move(c)), scan_(std::move(s))
        , cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<std::pair<Range,OutRange>> task_res = this->this_task_result();
        try
        {
            auto cont = boost::asynchronous::parallel_scan<
                    decltype(boost::begin(*range_)),
                    decltype(boost::begin(*out_range_)),
                    T,Reduce,Combine,Scan,Job>
                    (boost::begin(*range_),boost::end(*range_),boost::begin(*out_range_),std::move(init_),
                     std::move(reduce_),std::move(combine_),std::move(scan_),cutoff_,this->get_name(),prio_);

            auto range = range_;
            auto out_range = out_range_;
            cont.on_done([task_res,range,out_range]
                          (std::tuple<boost::asynchronous::expected<T>>&& continuation_res) mutable
            {
                try
                {
                    // get to check that no exception
                    std::get<0>(continuation_res).get();
                    task_res.set_value(std::make_pair(std::move(*range),std::move(*out_range)));
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

    boost::shared_ptr<Range> range_;
    boost::shared_ptr<OutRange> out_range_;
    T init_;
    Reduce reduce_;
    Combine combine_;
    Scan scan_;
    long cutoff_;
    std::size_t prio_;
};
// version for moved ranges
template <class Range, class OutRange, class T, class Reduce, class Combine, class Scan, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::disable_if<boost::asynchronous::detail::has_is_continuation_task<Range>,
                           boost::asynchronous::detail::callback_continuation<std::pair<Range,OutRange>,Job> >::type
parallel_scan(Range&& range,OutRange&& out_range,T init,Reduce r, Combine c, Scan s,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto pr = boost::make_shared<Range>(std::forward<Range>(range));
    auto pout = boost::make_shared<OutRange>(std::forward<OutRange>(out_range));
    return boost::asynchronous::top_level_callback_continuation_job<std::pair<Range,OutRange>,Job>
            (boost::asynchronous::parallel_scan_range_move_helper<Range,OutRange,T,Reduce,Combine,Scan,Job>
                (pr,pout,std::move(init),std::move(r),std::move(c),std::move(s),cutoff,task_name,prio));
}

// version for a single moved range (in/out) => will return the range as continuation
template <class Range, class T, class Reduce, class Combine, class Scan, class Job,class Enable=void>
struct parallel_scan_range_move_single_helper: public boost::asynchronous::continuation_task<Range>
{
    parallel_scan_range_move_single_helper(boost::shared_ptr<Range> range, T init,
                                    Reduce r, Combine c, Scan s,
                                    long cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<Range>(task_name)
        , range_(range), init_(std::move(init))
        , reduce_(std::move(r)), combine_(std::move(c)), scan_(std::move(s))
        , cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<Range> task_res = this->this_task_result();
        try
        {
            auto cont = boost::asynchronous::parallel_scan<
                    decltype(boost::begin(*range_)),
                    decltype(boost::begin(*range_)),
                    T,Reduce,Combine,Scan,Job>
                    (boost::begin(*range_),boost::end(*range_),boost::begin(*range_),std::move(init_),
                     std::move(reduce_),std::move(combine_),std::move(scan_),cutoff_,this->get_name(),prio_);

            auto range = range_;
            cont.on_done([task_res,range]
                          (std::tuple<boost::asynchronous::expected<T>>&& continuation_res) mutable
            {
                try
                {
                    // get to check that no exception
                    std::get<0>(continuation_res).get();
                    task_res.set_value(std::move(*range));
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

    boost::shared_ptr<Range> range_;
    T init_;
    Reduce reduce_;
    Combine combine_;
    Scan scan_;
    long cutoff_;
    std::size_t prio_;
};
// version for a single moved range (in/out) => will return the range as continuation
template <class Range, class T, class Reduce, class Combine, class Scan, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::disable_if<boost::asynchronous::detail::has_is_continuation_task<Range>,
                           boost::asynchronous::detail::callback_continuation<Range,Job> >::type
parallel_scan(Range&& range,T init,Reduce r, Combine c, Scan s,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto pr = boost::make_shared<Range>(std::forward<Range>(range));
    return boost::asynchronous::top_level_callback_continuation_job<Range,Job>
            (boost::asynchronous::parallel_scan_range_move_single_helper<Range,T,Reduce,Combine,Scan,Job>
                (pr,std::move(init),std::move(r),std::move(c),std::move(s),cutoff,task_name,prio));
}


namespace
{
// version for ranges given as continuation => will return the range as continuation
template <class Continuation, class T, class Reduce, class Combine, class Scan, class Job,class Enable=void>
struct parallel_scan_range_continuation_helper: public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_scan_range_continuation_helper(Continuation range, T init,
                                    Reduce r, Combine c, Scan s,
                                    long cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<typename Continuation::return_type>(task_name)
        , cont_(std::move(range)), init_(std::move(init))
        , reduce_(std::move(r)), combine_(std::move(c)), scan_(std::move(s))
        , cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<typename Continuation::return_type> task_res = this->this_task_result();
        try
        {
            auto init(std::move(init_));
            auto reduce(std::move(reduce_));
            auto combine(std::move(combine_));
            auto scan(std::move(scan_));
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;

            cont_.on_done([task_res,init,reduce,combine,scan,cutoff,task_name,prio]
                          (std::tuple<boost::asynchronous::expected<typename Continuation::return_type>>&& continuation_res) mutable
            {
                try
                {
                    auto new_continuation = boost::asynchronous::parallel_scan<
                            typename Continuation::return_type,
                            T,
                            Reduce,Combine,Scan,
                            Job>
                            (std::move(std::get<0>(continuation_res).get()),
                             std::move(init),
                             std::move(reduce),std::move(combine),std::move(scan),
                             cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res)
                    {
                        try
                        {
                            task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
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
            );
        }
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }

    Continuation cont_;
    T init_;
    Reduce reduce_;
    Combine combine_;
    Scan scan_;
    long cutoff_;
    std::size_t prio_;
};
}
// version for ranges given as continuation => will return the range as continuation
template <class Range, class T, class Reduce, class Combine, class Scan, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::enable_if<boost::asynchronous::detail::has_is_continuation_task<Range>,
                          boost::asynchronous::detail::callback_continuation<typename Range::return_type,Job> >::type
parallel_scan(Range range,T init,Reduce r, Combine c, Scan s,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<typename Range::return_type,Job>
            (boost::asynchronous::parallel_scan_range_continuation_helper<Range,T,Reduce,Combine,Scan,Job>
                (std::move(range),std::move(init),std::move(r),std::move(c),std::move(s),cutoff,task_name,prio));
}

}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_SCAN_HPP
