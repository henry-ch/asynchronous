// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org
#ifndef BOOST_ASYNCHRONOUS_PARALLEL_QUICKSORT_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_QUICKSORT_HPP

#include <vector>
#include <iterator> // for std::iterator_traits

#include <boost/serialization/vector.hpp>

#include <boost/asynchronous/detail/any_interruptible.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/scheduler/serializable_task.hpp>
#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>
#include <boost/asynchronous/algorithm/parallel_merge.hpp>
#include <boost/asynchronous/algorithm/parallel_is_sorted.hpp>
#include <boost/asynchronous/algorithm/detail/parallel_sort_helper.hpp>
#include <boost/asynchronous/algorithm/parallel_partition.hpp>
#include <boost/asynchronous/algorithm/parallel_stable_partition.hpp>
#include <boost/asynchronous/algorithm/parallel_sort.hpp>
#include <boost/asynchronous/algorithm/parallel_is_sorted.hpp>

#ifdef BOOST_ASYNCHRONOUS_USE_BOOST_SORT
#include <boost/sort/parallel/sort.hpp>
#endif

namespace boost { namespace asynchronous
{
// fast version for iterators, will return nothing
namespace detail
{
template <class Iterator,class Func, class Job, class Sort>
struct parallel_quicksort_helper: public boost::asynchronous::continuation_task<void>
{

    parallel_quicksort_helper(Iterator beg, Iterator end,Func func,uint32_t thread_num,long size_all_partitions, std::size_t original_size,
                              long cutoff,const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<void>(task_name),
          beg_(beg),end_(end),func_(std::move(func)),thread_num_(thread_num)
        , size_all_partitions_(size_all_partitions),original_size_(original_size),cutoff_(cutoff),prio_(prio)
    {
    }
    void operator()()
    {
        boost::asynchronous::continuation_result<void> task_res = this_task_result();
        try
        {
            // advance up to cutoff
            Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
            // if not at end, recurse, otherwise sort here
            if (it == end_)
            {
                // if already reverse sorted, only reverse
                if (std::is_sorted(beg_,it,boost::asynchronous::detail::reverse_sorted<Func>(func_)))
                {
                    std::reverse(beg_,end_);
                }
                // if already sorted, done
                else if (!std::is_sorted(beg_,it,func_))
                {
                    Sort()(beg_,end_,func_);
                }
                task_res.set_value();
            }
            // if we do not make enough progress, also switch to parallel_sort
            else if (size_all_partitions_ > (long)(std::log10(original_size_)*original_size_))
            {
                auto cont = boost::asynchronous::parallel_sort
                         (beg_,end_,std::move(func_),cutoff_,this->get_name(),prio_);
                cont.on_done([task_res](std::tuple<boost::asynchronous::expected<void> >&& sort_res) mutable
                {
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
                auto beg = beg_;
                auto end = end_;
                auto func = func_;
                auto thread_num = thread_num_;
                auto task_name = this->get_name();
                auto prio = prio_;
                auto cutoff = cutoff_;
                auto size_all_partitions = size_all_partitions_;
                auto original_size = original_size_;

                // optimization for cases where we are already sorted
                auto cont = boost::asynchronous::parallel_is_sorted<Iterator,Func,Job>(beg,end,func,cutoff,task_name+"_is_sorted",prio);
                cont.on_done([task_res,beg,end,func,thread_num,size_all_partitions,original_size,cutoff,task_name,prio]
                    (std::tuple<boost::asynchronous::expected<bool> >&& res) mutable
                    {
                        try
                        {
                            bool sorted = std::get<0>(res).get();
                            if (sorted)
                            {
                                task_res.set_value();
                                return;
                            }
                        }
                        catch(std::exception& e)
                        {
                            task_res.set_exception(boost::copy_exception(e));
                            return;
                        }
                       //check if reverse sorted
                        auto cont2 = boost::asynchronous::parallel_is_reverse_sorted<Iterator,Func,Job>
                            (beg,end,func,cutoff,task_name+"_is_reverse_sorted",prio);
                        cont2.on_done([task_res,beg,end,func,thread_num,size_all_partitions,original_size,cutoff,task_name,prio]
                                      (std::tuple<boost::asynchronous::expected<bool> >&& res) mutable
                        {
                            try
                            {
                                bool sorted = std::get<0>(res).get();
                                if (sorted)
                                {
                                    // reverse sorted
                                    auto cont3 = boost::asynchronous::parallel_reverse<Iterator,Job>(beg,end,cutoff,task_name+"_reverse",prio);
                                    cont3.on_done([task_res](std::tuple<boost::asynchronous::expected<void> >&& res) mutable
                                    {
                                        try
                                        {
                                            std::get<0>(res).get();
                                            task_res.set_value();
                                        }
                                        catch(std::exception& e)
                                        {
                                            task_res.set_exception(boost::copy_exception(e));
                                        }
                                    });
                                    return;
                                }
                                // we "randomize" by taking the median of medians as pivot for the next partition run
                                auto middleval = boost::asynchronous::detail::median_of_medians(beg,end,func);

                                auto l = [middleval, func](const typename std::iterator_traits<Iterator>::value_type& i)
                                {
                                    return func(i,middleval);
                                };

#ifdef BOOST_ASYNCHRONOUS_QUICKSORT_USE_STABLE_PARTITION
                                auto cont3 = boost::asynchronous::parallel_stable_partition<Iterator,decltype(l),Job>(beg,end,std::move(l),cutoff);
#else
                                auto cont3 = boost::asynchronous::parallel_partition<Iterator,decltype(l),Job>(beg,end,std::move(l),thread_num);
#endif
                                cont3.on_done([task_res,beg,end,func,thread_num,size_all_partitions,original_size,cutoff,task_name,prio]
                                             (std::tuple<boost::asynchronous::expected<Iterator> >&& continuation_res)
                                {
                                    try
                                    {
                                        // get our middle iterator
                                        Iterator it = std::get<0>(continuation_res).get();
                                        auto dist_beg_it = std::distance(beg,it);
                                        auto dist_it_end = std::distance(it,end);
                                        // partition our sub-partitions
                                        boost::asynchronous::create_callback_continuation_job<Job>(
                                            [task_res]
                                            (std::tuple<boost::asynchronous::expected<void>,boost::asynchronous::expected<void> > res) mutable
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
                                            boost::asynchronous::detail::parallel_quicksort_helper<Iterator,Func,Job,Sort>
                                                    (beg,it,func,thread_num,size_all_partitions+dist_beg_it,original_size,cutoff,task_name,prio),
                                            boost::asynchronous::detail::parallel_quicksort_helper<Iterator,Func,Job,Sort>
                                                    (it,end,func,thread_num,size_all_partitions+dist_it_end,original_size,cutoff,task_name,prio)
                                        );
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
                });
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
    uint32_t thread_num_;
    long size_all_partitions_;
    std::size_t original_size_;
    long cutoff_;
    std::size_t prio_;
};

}

// fast version for iterators => will return nothing
template <class Iterator,class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_quicksort(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const uint32_t thread_num,const std::string& task_name, std::size_t prio=0)
#else
              const uint32_t thread_num = boost::thread::hardware_concurrency(),const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_quicksort_helper<Iterator,Func,Job,boost::asynchronous::std_sort>
              (beg,end,std::move(func),thread_num,0,std::distance(beg,end),cutoff,task_name,prio));
}

#ifdef BOOST_ASYNCHRONOUS_USE_BOOST_SPREADSORT
template <class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_quick_spreadsort(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const uint32_t thread_num,const std::string& task_name, std::size_t prio=0)
#else
              const uint32_t thread_num = boost::thread::hardware_concurrency(),const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_quicksort_helper<Iterator,Func,Job,boost::asynchronous::boost_spreadsort>
              (beg,end,std::move(func),thread_num,0,std::distance(beg,end),cutoff,task_name,prio));
}
#endif

// test boost sort
#ifdef BOOST_ASYNCHRONOUS_USE_BOOST_SORT
template <class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_quick_spin_sort(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const uint32_t thread_num,const std::string& task_name, std::size_t prio=0)
#else
              const uint32_t thread_num = boost::thread::hardware_concurrency(),const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_quicksort_helper<Iterator,Func,Job,boost::asynchronous::boost_spin_sort>
              (beg,end,std::move(func),thread_num,0,std::distance(beg,end),cutoff,task_name,prio));
}
template <class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_quick_indirect_sort(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const uint32_t thread_num,const std::string& task_name, std::size_t prio=0)
#else
              const uint32_t thread_num = boost::thread::hardware_concurrency(),const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_quicksort_helper<Iterator,Func,Job,boost::asynchronous::boost_indirect_sort>
              (beg,end,std::move(func),thread_num,0,std::distance(beg,end),cutoff,task_name,prio));
}
template <class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_quick_intro_sort(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const uint32_t thread_num,const std::string& task_name, std::size_t prio=0)
#else
              const uint32_t thread_num = boost::thread::hardware_concurrency(),const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_quicksort_helper<Iterator,Func,Job,boost::asynchronous::boost_intro_sort>
              (beg,end,std::move(func),thread_num,0,std::distance(beg,end),cutoff,task_name,prio));
}
#endif
}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_QUICKSORT_HPP

