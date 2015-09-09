// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_PARTIAL_SORT_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_PARTIAL_SORT_HPP

#include <algorithm>
#include <vector>

#include <boost/utility/enable_if.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>
#include <boost/asynchronous/algorithm/parallel_partition.hpp>
#include <boost/asynchronous/algorithm/parallel_sort.hpp>

namespace boost { namespace asynchronous
{
namespace detail
{

template <class Iterator, class Func, class Job>
struct parallel_partial_sort_helper: public boost::asynchronous::continuation_task<void>
{
    parallel_partial_sort_helper(Iterator beg, Iterator end,Iterator middle, Func func,long size_all_partitions, std::size_t original_size,
                                long cutoff, const uint32_t thread_num,
                                const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<void>(task_name)
        , beg_(beg),end_(end),middle_(middle)
        , func_(std::move(func)),size_all_partitions_(size_all_partitions),original_size_(original_size)
        , cutoff_(cutoff),thread_num_(thread_num),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<void> task_res = this_task_result();
        // if close enough, start sorting
        auto func(std::move(func_));
        auto cutoff = cutoff_;
        auto task_name = this->get_name();
        auto prio = prio_;
        auto thread_num = thread_num_;
        auto beg = beg_;
        auto end = end_;
        auto middle=middle_;
        auto size_all_partitions = size_all_partitions_;
        auto original_size = original_size_;

        // if we do not make enough progress, switch to sort
        if (size_all_partitions > (long)(4*original_size))
        {
            //std::cout << "switch to sorting" << std::endl;
            auto cont = boost::asynchronous::parallel_sort
                     (beg,end,std::move(func),cutoff,task_name,prio);
            cont.on_done([task_res](std::tuple<boost::asynchronous::expected<void> >&& sort_res) mutable
            {
                //std::cout << "end sorting" << std::endl;
                try
                {
                    // check for exceptions
                    std::get<0>(sort_res).get();
                    task_res.set_value();
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(boost::copy_exception(e));
                }
            });
        }
        else
        {
            // we "randomize" by taking the median of medians as pivot for the next partition run
            auto middleval = boost::asynchronous::detail::median_of_medians(beg,end,func);

            //std::cout << "*middle: " << middleval << std::endl;
            auto l = [middleval, func](const typename std::iterator_traits<Iterator>::value_type& i)
            {
                return func(i,middleval);
            };

            //std::cout << "start parallel_partition: " << end_ - beg_ << std::endl;
            auto cont = boost::asynchronous::parallel_partition<Iterator,decltype(l),Job>(beg_,end_,std::move(l),thread_num_);
            cont.on_done([task_res,beg,end,middle,func,size_all_partitions,original_size,cutoff,thread_num,task_name,prio]
                         (std::tuple<boost::asynchronous::expected<Iterator> >&& continuation_res) mutable
            {
                try
                {
                    auto res = std::move(std::get<0>(continuation_res).get());
                    //std::cout << "done parallel_partition: " << res-beg << " , " << middle-beg << " , " << size_all_partitions << std::endl;
                    auto dist_beg_res = std::distance(beg,res);

                    // if we are close enough (20% of size of original container) to middle, also
                    if ((std::size_t)(std::abs(std::distance(beg,middle) - dist_beg_res)) < (std::size_t) 20*original_size/100)
                    {
                        //std::cout << "switch to sorting" << std::endl;
                        auto cont = boost::asynchronous::parallel_sort
                                 (beg,std::max(res,middle),std::move(func),cutoff,task_name,prio);
                        cont.on_done([task_res](std::tuple<boost::asynchronous::expected<void> >&& sort_res) mutable
                        {
                            //std::cout << "end sorting" << std::endl;
                            try
                            {
                                // check for exceptions
                                std::get<0>(sort_res).get();
                                task_res.set_value();
                            }
                            catch(std::exception& e)
                            {
                                task_res.set_exception(boost::copy_exception(e));
                            }
                        });
                    }
                    else if (dist_beg_res >= std::distance(beg,middle))
                    {
                        // re-iterate on first part
                        //std::cout << "1st part: " << res-beg << std::endl;
                        auto cont = boost::asynchronous::top_level_callback_continuation_job<void,Job>
                                (boost::asynchronous::detail::parallel_partial_sort_helper<Iterator,Func,Job>
                                 (beg,res,middle,std::move(func),size_all_partitions+dist_beg_res,original_size,cutoff,thread_num,task_name,prio));
                        cont.on_done([task_res](std::tuple<boost::asynchronous::expected<void> >&& middle_element_res) mutable
                        {
                            try
                            {
                                // check for exceptions
                                std::get<0>(middle_element_res).get();
                                task_res.set_value();
                            }
                            catch(std::exception& e)
                            {
                                task_res.set_exception(boost::copy_exception(e));
                            }
                        });
                    }
                    else
                    {
                        // bad luck, no progress can be done on this iteration, re-iterate on the whole range
                        //std::cout << "2nd part: " << end-res << std::endl;
                        auto cont = boost::asynchronous::top_level_callback_continuation_job<void,Job>
                                (boost::asynchronous::detail::parallel_partial_sort_helper<Iterator,Func,Job>
                                 (beg,end,middle,std::move(func),size_all_partitions+std::distance(res,end),original_size,cutoff,thread_num,task_name,prio));
                        cont.on_done([task_res](std::tuple<boost::asynchronous::expected<void> >&& middle_element_res) mutable
                        {
                            try
                            {
                                // check for exceptions
                                std::get<0>(middle_element_res).get();
                                task_res.set_value();
                            }
                            catch(std::exception& e)
                            {
                                task_res.set_exception(boost::copy_exception(e));
                            }
                        });
                    }

                }
                catch(std::exception& e)
                {
                    task_res.set_exception(boost::copy_exception(e));
                }
            });
        }
    }

    Iterator beg_;
    Iterator end_;
    Iterator middle_;
    Func func_;
    long size_all_partitions_;
    std::size_t original_size_;
    long cutoff_;
    uint32_t thread_num_;
    std::size_t prio_;
};
}

template <class Iterator,class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_partial_sort(Iterator beg, Iterator middle, Iterator end, Func func,long cutoff,const uint32_t thread_num = 1,
                   #ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                   const std::string& task_name, std::size_t prio)
#else
                   const std::string& task_name="", std::size_t prio =0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_partial_sort_helper<Iterator,Func,Job>
             (beg,end,middle,std::move(func),0,std::distance(beg,end),cutoff,thread_num,task_name,prio));

}

}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_PARTIAL_SORT_HPP

