// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_NTH_ELEMENT_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_NTH_ELEMENT_HPP

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
struct parallel_nth_element_helper: public boost::asynchronous::continuation_task<void>
{
    parallel_nth_element_helper(Iterator beg, Iterator end, Iterator nth, Func func,long size_all_partitions, std::size_t original_size,
                                long cutoff, const uint32_t thread_num,
                                const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<void>(task_name)
        , beg_(beg),end_(end),nth_(nth),func_(std::move(func)),size_all_partitions_(size_all_partitions),original_size_(original_size)
        , cutoff_(cutoff),thread_num_(thread_num),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<void> task_res = this_task_result();
        // advance up to cutoff
        Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
        // if not at end, recurse, otherwise execute here
        if (it == end_)
        {
            //std::cout << "start sequential nth_element" << std::endl;
            std::nth_element(beg_,nth_,end_,func_);
            task_res.set_value();
        }
        else
        {
            auto func(std::move(func_));
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            auto thread_num = thread_num_;
            auto beg = beg_;
            auto end = end_;
            auto nth=nth_;
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
                auto nthval = boost::asynchronous::detail::median_of_medians(beg,end,func);

                //std::cout << "*nth: " << nthval << std::endl;
                auto l = [nthval,func](const typename std::iterator_traits<Iterator>::value_type& i)
                {
                    return func(i,nthval);
                };

                //std::cout << "start parallel_partition: " << end_ - beg_ << std::endl;
                auto cont = boost::asynchronous::parallel_partition<Iterator,decltype(l),Job>(beg_,end_,std::move(l),thread_num_);
                cont.on_done([task_res,beg,end,nth,func,size_all_partitions,original_size,cutoff,thread_num,task_name,prio]
                             (std::tuple<boost::asynchronous::expected<Iterator> >&& continuation_res) mutable
                {
                    try
                    {
                        auto res = std::move(std::get<0>(continuation_res).get());
                        //std::cout << "done parallel_partition: " << res-beg << " , " << nth-beg << " , " << size_all_partitions << std::endl;

                        auto dist_beg_res = std::distance(beg,res);
                        if (dist_beg_res >= std::distance(beg,nth))
                        {
                            // re-iterate on first part
                            //std::cout << "1st part: " << res-beg << std::endl;
                            auto cont = boost::asynchronous::top_level_callback_continuation_job<void,Job>
                                    (boost::asynchronous::detail::parallel_nth_element_helper<Iterator,Func,Job>
                                     (beg,res,nth,std::move(func),size_all_partitions+dist_beg_res,original_size,cutoff,thread_num,task_name,prio));
                            cont.on_done([task_res](std::tuple<boost::asynchronous::expected<void> >&& nth_element_res) mutable
                            {
                                try
                                {
                                    // check for exceptions
                                    std::get<0>(nth_element_res).get();
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
                            // re-iterate on second part
                            //std::cout << "2nd part: " << end-res << std::endl;
                            auto cont = boost::asynchronous::top_level_callback_continuation_job<void,Job>
                                    (boost::asynchronous::detail::parallel_nth_element_helper<Iterator,Func,Job>
                                     (res,end,nth,std::move(func),size_all_partitions+std::distance(res,end),original_size,cutoff,thread_num,task_name,prio));
                            cont.on_done([task_res](std::tuple<boost::asynchronous::expected<void> >&& nth_element_res) mutable
                            {
                                try
                                {
                                    // check for exceptions
                                    std::get<0>(nth_element_res).get();
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

    }
    Iterator beg_;
    Iterator end_;
    Iterator nth_;
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
parallel_nth_element(Iterator beg, Iterator nth, Iterator end, Func func,long cutoff,const uint32_t thread_num = 1,
                   #ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                   const std::string& task_name, std::size_t prio)
#else
                   const std::string& task_name="", std::size_t prio =0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_nth_element_helper<Iterator,Func,Job>
             (beg,end,nth,std::move(func),0,std::distance(beg,end),cutoff,thread_num,task_name,prio));

}

}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_NTH_ELEMENT_HPP

