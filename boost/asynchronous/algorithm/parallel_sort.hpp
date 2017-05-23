// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_SORT_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_SORT_HPP

#include <vector>
#include <iterator> // for std::iterator_traits
#include <type_traits>

#include <boost/serialization/vector.hpp>
#include <boost/type_traits/has_trivial_destructor.hpp>

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
#include <boost/asynchronous/algorithm/parallel_reverse.hpp>
#include <boost/asynchronous/algorithm/detail/parallel_sort_helper.hpp>
#include <boost/asynchronous/algorithm/parallel_placement.hpp>
#include <boost/asynchronous/container/algorithms.hpp>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/iterator_range.hpp>

namespace boost { namespace asynchronous
{
// fast version for iterators (double memory costs) => will return nothing
namespace detail
{
template <class Iterator,class Func, class Job, class Sort>
struct parallel_sort_fast_helper: public boost::asynchronous::continuation_task<void>
{
    typedef typename std::iterator_traits<Iterator>::value_type value_type;

    parallel_sort_fast_helper(Iterator beg, Iterator end,unsigned int depth,std::shared_ptr<boost::asynchronous::placement_deleter<value_type,Job>> merge_memory,
                              value_type* beg2, value_type* end2,
                              Func func,long cutoff,const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<void>(task_name),
          beg_(beg),end_(end),depth_(depth),func_(std::move(func)),cutoff_(cutoff),prio_(prio),merge_memory_(merge_memory)
        , merge_beg_( beg2), merge_end_(end2)
    {
    }
    static void helper(Iterator beg, Iterator end,unsigned int depth,std::shared_ptr<boost::asynchronous::placement_deleter<value_type,Job>> merge_memory,
                       value_type* beg2, value_type* end2,
                       Func func,long cutoff,const std::string& task_name, std::size_t prio,
                       boost::asynchronous::continuation_result<void> task_res)
    {
        try
        {
            // advance up to cutoff
            Iterator it = boost::asynchronous::detail::find_cutoff(beg,cutoff,end);
            // if not at end, recurse, otherwise execute here
            if ((it == end)&&(depth %2 == 0))
            {
                // if already reverse sorted, only reverse
                if (std::is_sorted(beg,it,boost::asynchronous::detail::reverse_sorted<Func>(func)))
                {
                    std::reverse(beg,end);
                }
                // if already sorted, done
                else if (!std::is_sorted(beg,it,func))
                {
                    Sort()(beg,it,func);
                }
                task_res.set_value();
            }
            else
            {
                auto it2 = beg2+std::distance(beg,it);
                auto merge_task_name = task_name + "_merge";
                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [task_res,func,beg,end,it,beg2,end2,it2,depth,cutoff,merge_task_name,prio,merge_memory]
                            (std::tuple<boost::asynchronous::expected<void>,boost::asynchronous::expected<void> > res) mutable
                            {
                                try
                                {
                                    // get to check that no exception
                                    std::get<0>(res).get();
                                    std::get<1>(res).get();
                                    // merge both sorted sub-ranges
                                    auto on_done_fct = [task_res,depth,merge_memory](std::tuple<boost::asynchronous::expected<void> >&& merge_res)
                                    {
                                        try
                                        {
                                            // get to check that no exception
                                            std::get<0>(merge_res).get();
                                            // we need to clean up before returning value (in case a shutdown would be under way)
                                            if (depth == 0 && !boost::has_trivial_destructor<value_type>::value)
                                            {
                                                merge_memory->clear([task_res,merge_memory]()mutable{task_res.set_value();});
                                            }
                                            else
                                            {
                                                task_res.set_value();
                                            }
                                        }
                                        catch(std::exception& e)
                                        {
                                            if (depth == 0)
                                            {
                                                merge_memory->clear([task_res,merge_memory,e]()mutable
                                                                    {task_res.set_exception(std::make_exception_ptr(e));});
                                            }
                                            else
                                            {
                                                task_res.set_exception(std::make_exception_ptr(e));
                                            }
                                        }
                                    };
                                    if (depth%2 == 0)
                                    {
                                        // merge into first range
                                        auto c = boost::asynchronous::parallel_merge<value_type*,value_type*,Iterator,Func,Job>
                                                (beg2,it2,it2,end2,beg,func,cutoff,merge_task_name,prio);
                                        c.on_done(std::move(on_done_fct));
                                    }
                                    else
                                    {
                                        // merge into second range
                                        auto c = boost::asynchronous::parallel_merge<Iterator,Iterator,value_type*,Func,Job>
                                                (beg,it,it,end,beg2,func,cutoff,merge_task_name,prio);
                                        c.on_done(std::move(on_done_fct));
                                    }
                                }
                                catch(std::exception& e)
                                {
                                    if (depth == 0)
                                    {
                                        merge_memory->clear([task_res,merge_memory,e]()mutable
                                                            {task_res.set_exception(std::make_exception_ptr(e));});
                                    }
                                    else
                                    {
                                        task_res.set_exception(std::make_exception_ptr(e));
                                    }
                                }
                            },
                            // recursive tasks
                            parallel_sort_fast_helper<Iterator,Func,Job,Sort>
                                (beg,it,depth+1,merge_memory,beg2,it2,func,cutoff,task_name,prio),
                            parallel_sort_fast_helper<Iterator,Func,Job,Sort>
                                (it,end,depth+1,merge_memory,it2,end2,func,cutoff,task_name,prio)
                   );
            }
        }
        catch(std::exception& e)
        {
            if (depth == 0)
            {
                merge_memory->clear([task_res,merge_memory,e]()mutable
                                    {task_res.set_exception(std::make_exception_ptr(e));});
            }
            else
            {
                task_res.set_exception(std::make_exception_ptr(e));
            }
        }
    }

    void operator()()
    {
        boost::asynchronous::continuation_result<void> task_res = this_task_result();
        try
        {
            // do we need to parallelize? If not, no need to allocate memory and we save ourselves the else clause
            auto it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
            if ((depth_ == 0) && (end_ == it))
            {
                // if already reverse sorted, only reverse
                if (std::is_sorted(beg_,it,boost::asynchronous::detail::reverse_sorted<Func>(func_)))
                {
                    std::reverse(beg_,end_);
                }
                // if already sorted, done
                else if (!std::is_sorted(beg_,it,func_))
                {
                    Sort()(beg_,it,func_);
                }
                task_res.set_value();
                return;
            }
            else if (depth_ == 0)
            {
                // optimization for cases where we are already sorted
                auto beg = beg_;
                auto end = end_;
                auto depth = depth_;
                auto func = func_;
                auto cutoff = cutoff_;
                auto task_name = this->get_name();
                auto cont = boost::asynchronous::parallel_is_sorted<Iterator,Func,Job>(beg_,end_,func_,cutoff_,task_name+"_is_sorted",prio_);
                auto prio = prio_;
// GCC 4.7 imagines needing "this"
#if BOOST_GCC_VERSION < 40800
                cont.on_done([this,beg,end,depth,func,cutoff,task_name,prio,task_res]
#else
                cont.on_done([beg,end,depth,func,cutoff,task_name,prio,task_res]
#endif

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
                        auto cont2 = boost::asynchronous::parallel_is_reverse_sorted<Iterator,Func,Job>
                                (beg,end,func,cutoff,task_name+"_is_reverse_sorted",prio);
#if BOOST_GCC_VERSION < 40800
                        cont2.on_done([this,beg,end,depth,func,cutoff,task_name,prio,task_res]
#else
                        cont2.on_done([beg,end,depth,func,cutoff,task_name,prio,task_res]
#endif
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
                                            task_res.set_exception(std::make_exception_ptr(e));
                                        }
                                    });
                                    return;
                                }
                                // create extra memory for merge
                                auto size = std::distance(beg,end);
#ifdef BOOST_ASYNCHRONOUS_TIMING
                                auto alloc_start = std::chrono::high_resolution_clock::now();
#endif
                                std::shared_ptr<char> merge_memory_ (
                                            new char[size * sizeof(typename std::iterator_traits<Iterator>::value_type)],[](char* p){delete[] p;});

#ifdef BOOST_ASYNCHRONOUS_TIMING
                                auto alloc_stop = std::chrono::high_resolution_clock::now();
                                double alloc_time = (std::chrono::nanoseconds(alloc_stop - alloc_start).count() / 1000000);
                                printf ("%50s: time = %.1f msec\n","alloc_time", alloc_time);
                                auto placement_start = std::chrono::high_resolution_clock::now();
#endif
                                auto cont = boost::asynchronous::parallel_placement<value_type,Job>
                                        (0,size,merge_memory_,cutoff,task_name+"_placement",prio);
                                cont.on_done([
#ifdef BOOST_ASYNCHRONOUS_TIMING
                                             placement_start,
#endif
                                             task_res,merge_memory_,size,beg,end,depth,func,cutoff,task_name,prio]
                                              (std::tuple<boost::asynchronous::expected<boost::asynchronous::detail::parallel_placement_helper_result> >&& continuation_res) mutable
                                {
#ifdef BOOST_ASYNCHRONOUS_TIMING
                                    auto placement_stop = std::chrono::high_resolution_clock::now();
                                    double placement_time = (std::chrono::nanoseconds(placement_stop - placement_start).count() / 1000000);
                                    printf ("%50s: time = %.1f msec\n","placement_time", placement_time);
#endif
                                    try
                                    {
                                        // get to check that no exception
                                        auto res = std::get<0>(continuation_res).get();
                                        if (res.first != boost::asynchronous::detail::parallel_placement_helper_enum::success)
                                        {
                                            task_res.set_exception(res.second);
                                        }
                                        else
                                        {
                                            auto merge_memory =
                                                    std::make_shared<boost::asynchronous::placement_deleter<value_type,Job>>(size,merge_memory_,cutoff,task_name,prio);
                                            helper(beg,end,depth,merge_memory,(value_type*)merge_memory_.get(),((value_type*)merge_memory_.get())+size,func,cutoff,task_name,prio,task_res);
                                        }
                                    }
                                    catch(std::exception& e)
                                    {
                                        task_res.set_exception(std::make_exception_ptr(e));
                                    }
                                });
                            }
                            catch(std::exception& e)
                            {
                                task_res.set_exception(std::make_exception_ptr(e));
                            }
                        });
                    }
                    catch(std::exception& e)
                    {
                        task_res.set_exception(std::make_exception_ptr(e));
                    }
                });
                return;
            }
            helper(beg_,end_,depth_,merge_memory_,merge_beg_,merge_end_,func_,cutoff_,this->get_name(),prio_,std::move(task_res));
        }
        catch(std::exception& e)
        {
            task_res.set_exception(std::make_exception_ptr(e));
        }
    }
    Iterator beg_;
    Iterator end_;
    unsigned int depth_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
    std::shared_ptr<boost::asynchronous::placement_deleter<value_type,Job>> merge_memory_;
    value_type* merge_beg_;
    value_type* merge_end_;
};
}
// fast version for iterators => will return nothing
template <class Iterator,class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_sort(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio=0)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_sort_fast_helper<Iterator,Func,Job,boost::asynchronous::std_sort>
              (beg,end,0, std::shared_ptr<boost::asynchronous::placement_deleter<typename std::iterator_traits<Iterator>::value_type,Job>>(),
               nullptr,nullptr,std::move(func),cutoff,task_name,prio));
}

template <class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_stable_sort(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio=0)
#else
                     const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_sort_fast_helper<Iterator,Func,Job,boost::asynchronous::std_stable_sort>
               (beg,end,0,std::shared_ptr<boost::asynchronous::placement_deleter<typename std::iterator_traits<Iterator>::value_type,Job>>(),
                nullptr,nullptr,std::move(func),cutoff,task_name,prio));
}
// test boost sort
#ifdef BOOST_ASYNCHRONOUS_USE_BOOST_SORT
template <class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_boost_stable_sort(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio=0)
#else
                     const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_sort_fast_helper<Iterator,Func,Job,boost::asynchronous::boost_stable_sort>
               (beg,end,0,std::shared_ptr<boost::asynchronous::placement_deleter<typename std::iterator_traits<Iterator>::value_type,Job>>(),
                nullptr,nullptr,std::move(func),cutoff,task_name,prio));
}
#endif

#ifdef BOOST_ASYNCHRONOUS_USE_BOOST_SPREADSORT
template <class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_spreadsort(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio=0)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_sort_fast_helper<Iterator,Func,Job,boost::asynchronous::boost_spreadsort>
               (beg,end,0,std::shared_ptr<boost::asynchronous::placement_deleter<typename std::iterator_traits<Iterator>::value_type,Job>>(),
                nullptr,nullptr,std::move(func),cutoff,task_name,prio));
}
#endif

// test boost sort
#ifdef BOOST_ASYNCHRONOUS_USE_BOOST_SORT
template <class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_indirect_sort(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio=0)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_sort_fast_helper<Iterator,Func,Job,boost::asynchronous::boost_indirect_sort>
               (beg,end,0,std::shared_ptr<boost::asynchronous::placement_deleter<typename std::iterator_traits<Iterator>::value_type,Job>>(),
                nullptr,nullptr,std::move(func),cutoff,task_name,prio));
}
template <class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_intro_sort(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio=0)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_sort_fast_helper<Iterator,Func,Job,boost::asynchronous::boost_intro_sort>
               (beg,end,0,std::shared_ptr<boost::asynchronous::placement_deleter<typename std::iterator_traits<Iterator>::value_type,Job>>(),
                nullptr,nullptr,std::move(func),cutoff,task_name,prio));
}
template <class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_boost_spin_sort(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio=0)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_sort_fast_helper<Iterator,Func,Job,boost::asynchronous::boost_spin_sort>
               (beg,end,0,std::shared_ptr<boost::asynchronous::placement_deleter<typename std::iterator_traits<Iterator>::value_type,Job>>(),
                nullptr,nullptr,std::move(func),cutoff,task_name,prio));
}
#endif

// version for moved ranges => will return the range as continuation
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<!(boost::asynchronous::detail::is_serializable<Func>::value ||
                          boost::asynchronous::detail::has_is_continuation_task<Range>::value),
                           boost::asynchronous::detail::callback_continuation<Range,Job> >::type
parallel_sort(Range&& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                   const std::string& task_name, std::size_t prio=0)
#else
                   const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto r = std::make_shared<Range>(std::forward<Range>(range));
    auto beg = boost::begin(*r);
    auto end = boost::end(*r);
    return boost::asynchronous::top_level_callback_continuation_job<Range,Job>
            (boost::asynchronous::detail::parallel_sort_range_move_helper<boost::asynchronous::detail::callback_continuation<void,Job>,Range>
             (boost::asynchronous::parallel_sort<decltype(boost::begin(*r)),Func,Job>
                                (beg,end,std::move(func),cutoff,task_name,prio),r,task_name));
}

template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<!(boost::asynchronous::detail::is_serializable<Func>::value ||
                          boost::asynchronous::detail::has_is_continuation_task<Range>::value),
                           boost::asynchronous::detail::callback_continuation<Range,Job> >::type
parallel_stable_sort(Range&& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                          const std::string& task_name, std::size_t prio=0)
#else
                          const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto r = std::make_shared<Range>(std::forward<Range>(range));
    auto beg = boost::begin(*r);
    auto end = boost::end(*r);
    return boost::asynchronous::top_level_callback_continuation_job<Range,Job>
            (boost::asynchronous::detail::parallel_sort_range_move_helper<boost::asynchronous::detail::callback_continuation<void,Job>,Range>
             (boost::asynchronous::parallel_stable_sort<decltype(boost::begin(*r)),Func,Job>
              (beg,end,std::move(func),cutoff,task_name,prio),r,task_name));
}
#ifdef BOOST_ASYNCHRONOUS_USE_BOOST_SPREADSORT
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<!(boost::asynchronous::detail::is_serializable<Func>::value ||
                          boost::asynchronous::detail::has_is_continuation_task<Range>::value),
                           boost::asynchronous::detail::callback_continuation<Range,Job> >::type
parallel_spreadsort(Range&& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                   const std::string& task_name, std::size_t prio=0)
#else
                   const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto r = std::make_shared<Range>(std::forward<Range>(range));
    auto beg = boost::begin(*r);
    auto end = boost::end(*r);
    return boost::asynchronous::top_level_callback_continuation_job<Range,Job>
            (boost::asynchronous::detail::parallel_sort_range_move_helper<boost::asynchronous::detail::callback_continuation<void,Job>,Range>
             (boost::asynchronous::parallel_spreadsort<decltype(boost::begin(*r)),Func,Job>(beg,end,std::move(func),cutoff,task_name,prio),r,task_name));
}
#endif

template <class Range, class Func, class Job, class Sort>
struct parallel_sort_range_move_helper_serializable
        : public boost::asynchronous::continuation_task<Range>
        , public boost::asynchronous::serializable_task
{
    //default ctor only when deserialized immediately after
    parallel_sort_range_move_helper_serializable():boost::asynchronous::serializable_task("parallel_sort_range_move_helper")
    {
    }
    template <class Iterator>
    parallel_sort_range_move_helper_serializable(std::shared_ptr<Range> range,Iterator beg, Iterator end,Func func,unsigned int depth, long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<Range>(task_name)
        , boost::asynchronous::serializable_task(func.get_task_name())
        , range_(range),func_(std::move(func))
        , cutoff_(cutoff),task_name_(task_name),prio_(prio),depth_(depth)
        , begin_(beg)
        , end_(end)
    {
    }
    static void helper(std::shared_ptr<Range> full_range,decltype(boost::begin(*full_range)) beg, decltype(boost::begin(*full_range)) end,unsigned int depth,
                       Func func,long cutoff,const std::string& task_name, std::size_t prio,
                       boost::asynchronous::continuation_result<Range> task_res)
    {
        // advance up to cutoff
        auto it = boost::asynchronous::detail::find_cutoff(beg,cutoff,end);
        try
        {
            // if not at end, recurse, otherwise execute here
            if (it == end)
            {
                // if already reverse sorted, only reverse
                if (std::is_sorted(beg,it,boost::asynchronous::detail::reverse_sorted<Func>(func)))
                {
                    std::reverse(beg,end);
                }
                // if already sorted, done
                else if (!std::is_sorted(beg,it,func))
                {
                    Sort()(beg,it,func);
                }
                Range res (std::distance(beg,end));
                std::move(beg,it,boost::begin(res));
                task_res.set_value(std::move(res));
            }
            else
            {
                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [full_range,task_res,it,cutoff,func,task_name,prio]
                            (std::tuple<boost::asynchronous::expected<Range>,boost::asynchronous::expected<Range> > res) mutable
                            {
                                try
                                {
                                    auto r1 = std::move(std::get<0>(res).get());
                                    auto r2 = std::move(std::get<1>(res).get());
                                    Range range(r1.size()+r2.size());
                                    std::merge(r1.begin(),r1.end(),r2.begin(),r2.end(),range.begin(),func);
                                    task_res.set_value(std::move(range));
                                    //TODO reactivate parallel_merge when it supports serialization
                                    /*std::shared_ptr<Range> r1 =  std::make_shared<Range>(std::move(std::get<0>(res).get()));
                                    std::shared_ptr<Range> r2 =  std::make_shared<Range>(std::move(std::get<1>(res).get()));
                                    std::shared_ptr<Range> range = std::make_shared<Range>(r1->size()+r2->size());

                                    // merge both sorted sub-ranges
                                    auto on_done_fct = [full_range,task_res,r1,r2,range](std::tuple<boost::asynchronous::expected<void> >&& merge_res)
                                    {
                                        try
                                        {
                                            // get to check that no exception
                                            std::get<0>(merge_res).get();
                                            task_res.set_value(std::move(*range));
                                        }
                                        catch(std::exception& e)
                                        {
                                            task_res.set_exception(std::make_exception_ptr(e));
                                        }
                                    };
                                    auto c = boost::asynchronous::parallel_merge<decltype(boost::begin(*r1)),decltype(boost::begin(*r1)),
                                                                                 decltype(boost::begin(*range)),Func,Job>
                                            (boost::begin(*r1),boost::end(*r1),boost::begin(*r2),boost::end(*r2), boost::begin(*range),func,
                                             cutoff,task_name+"_merge",prio);
                                    c.on_done(std::move(on_done_fct));*/
                                }
                                catch(std::exception& e)
                                {
                                    task_res.set_exception(std::make_exception_ptr(e));
                                }
                            },
                            // recursive tasks
                            parallel_sort_range_move_helper_serializable<Range,Func,Job,Sort>(
                                        full_range,beg,it,
                                        func,depth+1,cutoff,task_name,prio),
                            parallel_sort_range_move_helper_serializable<Range,Func,Job,Sort>(
                                        full_range,it,end,
                                        func,depth+ 1,cutoff,task_name,prio)
                );
            }
        }
        catch(std::exception& e)
        {
            task_res.set_exception(std::make_exception_ptr(e));
        }
    }

    void operator()()
    {
        auto task_res = this->this_task_result();
        helper(range_,begin_,end_,depth_,std::move(func_),cutoff_,task_name_,prio_,task_res);
    }

    template <class Archive>
    void save(Archive & ar, const unsigned int /*version*/)const
    {
        // only part range
        // TODO avoid copying
        auto r = std::move(boost::copy_range< Range>(boost::make_iterator_range(begin_,end_)));
        ar & r;
        ar & func_;
        ar & cutoff_;
        ar & task_name_;
        ar & prio_;
        ar & depth_;
    }
    template <class Archive>
    void load(Archive & ar, const unsigned int /*version*/)
    {
        range_ = std::make_shared<Range>();
        ar & (*range_);
        ar & func_;
        ar & cutoff_;
        ar & task_name_;
        ar & prio_;
        ar & depth_;
        begin_ = boost::begin(*range_);
        end_ = boost::end(*range_);
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()

    std::shared_ptr<Range> range_;
    Func func_;
    long cutoff_;
    std::string task_name_;
    std::size_t prio_;
    unsigned int depth_;
    decltype(boost::begin(*range_)) begin_;
    decltype(boost::end(*range_)) end_;
};

template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if< boost::asynchronous::detail::is_serializable<Func>::value &&
                        !boost::asynchronous::detail::has_is_continuation_task<Range>::value,
                         boost::asynchronous::detail::callback_continuation<Range,Job> >::type
parallel_sort(Range&& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                   const std::string& task_name, std::size_t prio=0)
#else
                   const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto r = std::make_shared<Range>(std::forward<Range>(range));
    auto beg = boost::begin(*r);
    auto end = boost::end(*r);
    return boost::asynchronous::top_level_callback_continuation_job<Range,Job>
            (boost::asynchronous::parallel_sort_range_move_helper_serializable<Range,Func,Job,boost::asynchronous::std_sort>
                (r,beg,end,std::move(func),0,cutoff,task_name,prio));
}
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if< boost::asynchronous::detail::is_serializable<Func>::value &&
                        !boost::asynchronous::detail::has_is_continuation_task<Range>::value,
                          boost::asynchronous::detail::callback_continuation<Range,Job> >::type
parallel_stable_sort(Range&& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                          const std::string& task_name, std::size_t prio=0)
#else
                          const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto r = std::make_shared<Range>(std::forward<Range>(range));
    auto beg = boost::begin(*r);
    auto end = boost::end(*r);
    return boost::asynchronous::top_level_callback_continuation_job<Range,Job>
            (boost::asynchronous::parallel_sort_range_move_helper_serializable<Range,Func,Job,boost::asynchronous::std_stable_sort>
                (r,beg,end,std::move(func),0,cutoff,task_name,prio));
}
#ifdef BOOST_ASYNCHRONOUS_USE_BOOST_SPREADSORT
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if< boost::asynchronous::detail::is_serializable<Func>::value &&
                        !boost::asynchronous::detail::has_is_continuation_task<Range>::value,
                          boost::asynchronous::detail::callback_continuation<Range,Job> >::type
parallel_spreadsort(Range&& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                   const std::string& task_name, std::size_t prio=0)
#else
                   const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto r = std::make_shared<Range>(std::forward<Range>(range));
    auto beg = boost::begin(*r);
    auto end = boost::end(*r);
    return boost::asynchronous::top_level_callback_continuation_job<Range,Job>
            (boost::asynchronous::parallel_sort_range_move_helper_serializable<Range,Func,Job,boost::asynchronous::boost_spreadsort>
                (r,beg,end,std::move(func),0,cutoff,task_name,prio));
}
#endif

namespace detail
{
template <class Range, class Func, class Job, class Sort>
struct parallel_sort_range_move_helper2 : public boost::asynchronous::continuation_task<Range>
{
    //default ctor only when deserialized immediately after
    parallel_sort_range_move_helper2()
    {
    }
    template <class Iterator>
    parallel_sort_range_move_helper2(std::shared_ptr<Range> range,Iterator beg, Iterator end,Func func,unsigned int depth, long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<Range>(task_name)
        , range_(range),func_(std::move(func))
        , cutoff_(cutoff),task_name_(task_name),prio_(prio),depth_(depth)
        , begin_(beg)
        , end_(end)
    {
    }
    static void helper(std::shared_ptr<Range> full_range,decltype(boost::begin(*full_range)) beg, decltype(boost::begin(*full_range)) end,unsigned int depth,
                       Func func,long cutoff,const std::string& task_name, std::size_t prio,
                       boost::asynchronous::continuation_result<Range> task_res)
    {
        // advance up to cutoff
        auto it = boost::asynchronous::detail::find_cutoff(beg,cutoff,end);
        // if not at end, recurse, otherwise execute here
        if (it == end)
        {
            // if already reverse sorted, only reverse
            if (std::is_sorted(beg,it,boost::asynchronous::detail::reverse_sorted<Func>(func)))
            {
                std::reverse(beg,end);
            }
            // if already sorted, done
            else if (!std::is_sorted(beg,it,func))
            {
                Sort()(beg,it,func);
            }
            auto cont = boost::asynchronous::make_asynchronous_range<Range,Job> (std::distance(beg,end),cutoff);
            cont.on_done([task_res,beg,it](std::tuple<boost::asynchronous::expected<Range>>&& continuation_res)
            {
                try
                {
                    auto res = std::move(std::get<0>(continuation_res).get());
                    std::move(beg,it,boost::begin(res));
                    task_res.set_value(std::move(res));
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(std::make_exception_ptr(e));
                }
            });
        }
        else
        {
            boost::asynchronous::create_callback_continuation_job<Job>(
                        // called when subtasks are done, set our result
                        [full_range,task_res,it,cutoff,func,task_name,prio](
                        std::tuple<boost::asynchronous::expected<Range>,boost::asynchronous::expected<Range> > res) mutable
                        {
                            try
                            {
                                std::shared_ptr<Range> r1 =  std::make_shared<Range>(std::move(std::get<0>(res).get()));
                                std::shared_ptr<Range> r2 =  std::make_shared<Range>(std::move(std::get<1>(res).get()));
                                auto vec_cont = boost::asynchronous::make_asynchronous_range<Range,Job>(r1->size()+r2->size(),cutoff);
                                vec_cont.on_done([full_range,task_res,it,cutoff,func,task_name,prio,r1,r2]
                                                 (std::tuple<boost::asynchronous::expected<Range>>&& vec_res)
                                {
                                    try
                                    {
                                        std::shared_ptr<Range> range = std::make_shared<Range>(std::move(std::get<0>(vec_res).get()));
                                        // merge both sorted sub-ranges
                                        auto on_done_fct = [full_range,task_res,r1,r2,range]
                                                           (std::tuple<boost::asynchronous::expected<void> >&& merge_res) mutable
                                        {
                                            try
                                            {
                                                // get to check that no exception
                                                std::get<0>(merge_res).get();
                                                task_res.set_value(std::move(*range));
                                            }
                                            catch(std::exception& e)
                                            {
                                                task_res.set_exception(std::make_exception_ptr(e));
                                            }
                                        };
                                        auto c = boost::asynchronous::parallel_merge<decltype(boost::begin(*r1)),decltype(boost::begin(*r1)),
                                                                                     decltype(boost::begin(*range)),Func,Job>
                                                (boost::begin(*r1),boost::end(*r1),boost::begin(*r2),boost::end(*r2), boost::begin(*range),func,
                                                 cutoff,task_name+"_merge",prio);
                                        c.on_done(std::move(on_done_fct));
                                    }
                                    catch(std::exception& e)
                                    {
                                        task_res.set_exception(std::make_exception_ptr(e));
                                    }
                                });
                            }
                            catch(std::exception& e)
                            {
                                task_res.set_exception(std::make_exception_ptr(e));
                            }
                        },
                        // recursive tasks
                        parallel_sort_range_move_helper2<Range,Func,Job,Sort>(
                                    full_range,beg,it,
                                    func,depth+1,cutoff,task_name,prio),
                        parallel_sort_range_move_helper2<Range,Func,Job,Sort>(
                                    full_range,it,end,
                                    func,depth+ 1,cutoff,task_name,prio)
            );
        }
    }

    void operator()()
    {
        auto task_res = this->this_task_result();
        try
        {
            if (depth_ == 0)
            {
                // optimization for cases where we are already sorted
                auto range = range_;
                auto beg = begin_;
                auto end = end_;
                auto depth = depth_;
                auto func = func_;
                auto cutoff = cutoff_;
                auto task_name = this->get_name();
                auto cont = boost::asynchronous::parallel_is_sorted<decltype(boost::begin(*range_)),Func,Job>
                        (begin_,end_,func_,cutoff_,task_name+"_is_sorted",prio_);
                auto prio = prio_;
#if BOOST_GCC_VERSION < 40800
                cont.on_done([this,range,beg,end,depth,func,cutoff,task_name,prio,task_res]
#else
                cont.on_done([range,beg,end,depth,func,cutoff,task_name,prio,task_res]
#endif
                             (std::tuple<boost::asynchronous::expected<bool> >&& res) mutable
                {
                    try
                    {
                        bool sorted = std::get<0>(res).get();
                        if (sorted)
                        {
                            task_res.set_value(std::move(*range));
                            return;
                        }
                        auto cont2 = boost::asynchronous::parallel_is_reverse_sorted<decltype(boost::begin(*range_)),Func,Job>
                                (beg,end,func,cutoff,task_name+"_is_reverse_sorted",prio);
#if BOOST_GCC_VERSION < 40800
                        cont2.on_done([this,range,beg,end,depth,func,cutoff,task_name,prio,task_res]
#else
                        cont2.on_done([range,beg,end,depth,func,cutoff,task_name,prio,task_res]
#endif
                                      (std::tuple<boost::asynchronous::expected<bool> >&& res)
                        {
                            try
                            {
                                bool sorted = std::get<0>(res).get();
                                if (sorted)
                                {
                                    // reverse sorted
                                    auto cont3 = boost::asynchronous::parallel_reverse<decltype(boost::begin(*range_)),Job>
                                            (beg,end,cutoff,task_name+"_reverse",prio);
                                    cont3.on_done([range,task_res](std::tuple<boost::asynchronous::expected<void> >&& res) mutable
                                    {
                                        try
                                        {
                                            std::get<0>(res).get();
                                            task_res.set_value(std::move(*range));
                                        }
                                        catch(std::exception& e)
                                        {
                                            task_res.set_exception(std::make_exception_ptr(e));
                                        }
                                    });
                                    return;
                                }
                                helper(range,beg,end,depth,std::move(func),cutoff,task_name,prio,task_res);
                            }
                            catch(std::exception& e)
                            {
                                task_res.set_exception(std::make_exception_ptr(e));
                            }
                        });
                    }
                    catch(std::exception& e)
                    {
                        task_res.set_exception(std::make_exception_ptr(e));
                    }
                });
                return;
            }
            helper(range_,begin_,end_,depth_,std::move(func_),cutoff_,task_name_,prio_,task_res);
        }
        catch(std::exception& e)
        {
            task_res.set_exception(std::make_exception_ptr(e));
        }
    }
    std::shared_ptr<Range> range_;
    Func func_;
    long cutoff_;
    std::string task_name_;
    std::size_t prio_;
    unsigned int depth_;
    decltype(boost::begin(*range_)) begin_;
    decltype(boost::end(*range_)) end_;
};
}

template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<!boost::asynchronous::detail::has_is_continuation_task<Range>::value,
                          boost::asynchronous::detail::callback_continuation<Range,Job> >::type
parallel_sort2(Range&& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                   const std::string& task_name, std::size_t prio=0)
#else
                   const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto r = std::make_shared<Range>(std::forward<Range>(range));
    auto beg = boost::begin(*r);
    auto end = boost::end(*r);
    return boost::asynchronous::top_level_callback_continuation_job<Range,Job>
            (boost::asynchronous::detail::parallel_sort_range_move_helper2<Range,Func,Job,boost::asynchronous::std_sort>
                (r,beg,end,std::move(func),0,cutoff,task_name,prio));
}
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<!boost::asynchronous::detail::has_is_continuation_task<Range>::value,
                          boost::asynchronous::detail::callback_continuation<Range,Job> >::type
parallel_stable_sort2(Range&& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                          const std::string& task_name, std::size_t prio=0)
#else
                          const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto r = std::make_shared<Range>(std::forward<Range>(range));
    auto beg = boost::begin(*r);
    auto end = boost::end(*r);
    return boost::asynchronous::top_level_callback_continuation_job<Range,Job>
            (boost::asynchronous::detail::parallel_sort_range_move_helper2<Range,Func,Job,boost::asynchronous::std_stable_sort>
                (r,beg,end,std::move(func),0,cutoff,task_name,prio));
}
#ifdef BOOST_ASYNCHRONOUS_USE_BOOST_SPREADSORT
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<!boost::asynchronous::detail::has_is_continuation_task<Range>::value,
                          boost::asynchronous::detail::callback_continuation<Range,Job> >::type
parallel_spreadsort2(Range&& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                   const std::string& task_name, std::size_t prio=0)
#else
                   const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto r = std::make_shared<Range>(std::forward<Range>(range));
    auto beg = boost::begin(*r);
    auto end = boost::end(*r);
    return boost::asynchronous::top_level_callback_continuation_job<Range,Job>
            (boost::asynchronous::detail::parallel_sort_range_move_helper2<Range,Func,Job,boost::asynchronous::boost_spreadsort>
                (r,beg,end,std::move(func),0,cutoff,task_name,prio));
}
#endif


// version for ranges given as continuation => will return the range as continuation
namespace detail
{
// adapter to non-callback continuations
template <class Continuation, class Func, class Job,class Enable=void>
struct parallel_sort_continuation_range_helper: public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_sort_continuation_range_helper(Continuation const& c,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<typename Continuation::return_type>(task_name)
        ,cont_(c),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<typename Continuation::return_type> task_res = this->this_task_result();
        try
        {
            auto func(std::move(func_));
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            cont_.on_done([task_res,func,cutoff,task_name,prio](std::tuple<std::future<typename Continuation::return_type> >&& continuation_res)
            {
                try
                {
                    auto new_continuation = boost::asynchronous::parallel_sort<typename Continuation::return_type, Func, Job>(std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res) mutable
                    {
                        task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                    });
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(std::make_exception_ptr(e));
                }
            }
            );
            boost::asynchronous::any_continuation ac(std::move(cont_));
            boost::asynchronous::get_continuations().emplace_front(std::move(ac));
        }
        catch(std::exception& e)
        {
            task_res.set_exception(std::make_exception_ptr(e));
        }
    }
    Continuation cont_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
// Continuation is a callback continuation
template <class Continuation, class Func, class Job>
struct parallel_sort_continuation_range_helper<Continuation,Func,Job,
                                               typename std::enable_if< boost::asynchronous::detail::has_is_callback_continuation_task<Continuation>::value >::type>:
        public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_sort_continuation_range_helper(Continuation const& c,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<typename Continuation::return_type>(task_name)
        ,cont_(c),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<typename Continuation::return_type> task_res = this->this_task_result();
        try
        {
            auto func(std::move(func_));
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            cont_.on_done([task_res,func,cutoff,task_name,prio](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& continuation_res)
            {
                try
                {
                    auto new_continuation = boost::asynchronous::parallel_sort<typename Continuation::return_type, Func, Job>(std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res) mutable
                    {
                        task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                    });
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(std::make_exception_ptr(e));
                }
            }
            );
        }
        catch(std::exception& e)
        {
            task_res.set_exception(std::make_exception_ptr(e));
        }
    }
    Continuation cont_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};

// adapter to non-callback continuations
template <class Continuation, class Func, class Job,class Enable=void>
struct parallel_stable_sort_continuation_range_helper: public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_stable_sort_continuation_range_helper(Continuation const& c,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<typename Continuation::return_type>(task_name)
        ,cont_(c),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<typename Continuation::return_type> task_res = this->this_task_result();
        try
        {
            auto func(std::move(func_));
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            cont_.on_done([task_res,func,cutoff,task_name,prio](std::tuple<std::future<typename Continuation::return_type> >&& continuation_res)
            {
                try
                {
                    auto new_continuation = boost::asynchronous::parallel_stable_sort<typename Continuation::return_type,Func,Job>
                            (std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res) mutable
                    {
                        task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                    });
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(std::make_exception_ptr(e));
                }
            }
            );
            boost::asynchronous::any_continuation ac(std::move(cont_));
            boost::asynchronous::get_continuations().emplace_front(std::move(ac));
        }
        catch(std::exception& e)
        {
            task_res.set_exception(std::make_exception_ptr(e));
        }
    }
    Continuation cont_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
// Continuation is a callback continuation
template <class Continuation, class Func, class Job>
struct parallel_stable_sort_continuation_range_helper<Continuation,Func,Job,
                                                      typename std::enable_if< boost::asynchronous::detail::has_is_callback_continuation_task<Continuation>::value >::type>
        : public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_stable_sort_continuation_range_helper(Continuation const& c,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<typename Continuation::return_type>(task_name)
        ,cont_(c),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<typename Continuation::return_type> task_res = this->this_task_result();
        try
        {
            auto func(std::move(func_));
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            cont_.on_done([task_res,func,cutoff,task_name,prio](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& continuation_res) mutable
            {
                try
                {
                    auto new_continuation = boost::asynchronous::parallel_stable_sort<typename Continuation::return_type,Func,Job>
                            (std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res) mutable
                    {
                        task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                    });
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(std::make_exception_ptr(e));
                }
            }
            );
        }
        catch(std::exception& e)
        {
            task_res.set_exception(std::make_exception_ptr(e));
        }
    }
    Continuation cont_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
#ifdef BOOST_ASYNCHRONOUS_USE_BOOST_SPREADSORT
// adapter to non-callback continuations
template <class Continuation, class Func, class Job,class Enable=void>
struct parallel_spreadsort_continuation_range_helper: public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_spreadsort_continuation_range_helper(Continuation const& c,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<typename Continuation::return_type>(task_name)
        ,cont_(c),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<typename Continuation::return_type> task_res = this->this_task_result();
        try
        {
            auto func(std::move(func_));
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            cont_.on_done([task_res,func,cutoff,task_name,prio](std::tuple<std::future<typename Continuation::return_type> >&& continuation_res) mutable
            {
                try
                {
                    auto new_continuation = boost::asynchronous::parallel_spreadsort<typename Continuation::return_type,Func,Job>
                            (std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res) mutable
                    {
                        task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                    });
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(std::make_exception_ptr(e));
                }
            }
            );
            boost::asynchronous::any_continuation ac(std::move(cont_));
            boost::asynchronous::get_continuations().emplace_front(std::move(ac));
        }
        catch(std::exception& e)
        {
            task_res.set_exception(std::make_exception_ptr(e));
        }
    }
    Continuation cont_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
// Continuation is a callback continuation
template <class Continuation, class Func, class Job>
struct parallel_spreadsort_continuation_range_helper<Continuation,Func,Job,
                                                     typename std::enable_if< boost::asynchronous::detail::has_is_callback_continuation_task<Continuation>::value >::type>
        : public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_spreadsort_continuation_range_helper(Continuation const& c,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<typename Continuation::return_type>(task_name)
        ,cont_(c),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<typename Continuation::return_type> task_res = this->this_task_result();
        try
        {
            auto func(std::move(func_));
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            cont_.on_done([task_res,func,cutoff,task_name,prio](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& continuation_res) mutable
            {
                try
                {
                    auto new_continuation = boost::asynchronous::parallel_spreadsort<typename Continuation::return_type,Func,Job>
                            (std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res) mutable
                    {
                        task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                    });
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(std::make_exception_ptr(e));
                }
            }
            );
        }
        catch(std::exception& e)
        {
            task_res.set_exception(std::make_exception_ptr(e));
        }
    }
    Continuation cont_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
#endif
}
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<Range>::value,boost::asynchronous::detail::callback_continuation<typename Range::return_type,Job> >::type
parallel_sort(Range range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio=0)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<typename Range::return_type,Job>
            (boost::asynchronous::detail::parallel_sort_continuation_range_helper<Range,Func,Job>(range,std::move(func),cutoff,task_name,prio));
}
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<Range>::value,boost::asynchronous::detail::callback_continuation<typename Range::return_type,Job> >::type
parallel_stable_sort(Range range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio=0)
#else
                     const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<typename Range::return_type,Job>
            (boost::asynchronous::detail::parallel_stable_sort_continuation_range_helper<Range,Func,Job>(range,std::move(func),cutoff,task_name,prio));
}
#ifdef BOOST_ASYNCHRONOUS_USE_BOOST_SPREADSORT
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<Range>::value,boost::asynchronous::detail::callback_continuation<typename Range::return_type,Job> >::type
parallel_spreadsort(Range range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio=0)
#else
                     const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<typename Range::return_type,Job>
            (boost::asynchronous::detail::parallel_spreadsort_continuation_range_helper<Range,Func,Job>(range,std::move(func),cutoff,task_name,prio));
}
#endif


// version for ranges given as continuation => will return the range as continuation
namespace detail
{
// adapter to non-callback continuations
template <class Continuation, class Func, class Job,class Enable=void>
struct parallel_sort_continuation_range_helper2: public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_sort_continuation_range_helper2(Continuation const& c,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<typename Continuation::return_type>(task_name)
        ,cont_(c),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<typename Continuation::return_type> task_res = this->this_task_result();
        try
        {
            auto func(std::move(func_));
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            cont_.on_done([task_res,func,cutoff,task_name,prio](std::tuple<std::future<typename Continuation::return_type> >&& continuation_res) mutable
            {
                try
                {
                    auto new_continuation = boost::asynchronous::parallel_sort2<typename Continuation::return_type, Func, Job>(std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res) mutable
                    {
                        task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                    });
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(std::make_exception_ptr(e));
                }
            }
            );
            boost::asynchronous::any_continuation ac(std::move(cont_));
            boost::asynchronous::get_continuations().emplace_front(std::move(ac));
        }
        catch(std::exception& e)
        {
            task_res.set_exception(std::make_exception_ptr(e));
        }
    }
    Continuation cont_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
// Continuation is a callback continuation
template <class Continuation, class Func, class Job>
struct parallel_sort_continuation_range_helper2<Continuation,Func,Job,
                                                typename std::enable_if< boost::asynchronous::detail::has_is_callback_continuation_task<Continuation>::value >::type>:
        public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_sort_continuation_range_helper2(Continuation const& c,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<typename Continuation::return_type>(task_name)
        ,cont_(c),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<typename Continuation::return_type> task_res = this->this_task_result();
        try
        {
            auto func(std::move(func_));
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            cont_.on_done([task_res,func,cutoff,task_name,prio](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& continuation_res) mutable
            {
                try
                {
                    auto new_continuation = boost::asynchronous::parallel_sort2<typename Continuation::return_type, Func, Job>(std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res) mutable
                    {
                        task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                    });
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(std::make_exception_ptr(e));
                }
            }
            );
        }
        catch(std::exception& e)
        {
            task_res.set_exception(std::make_exception_ptr(e));
        }
    }
    Continuation cont_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};

// adapter to non-callback continuations
template <class Continuation, class Func, class Job,class Enable=void>
struct parallel_stable_sort_continuation_range_helper2: public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_stable_sort_continuation_range_helper2(Continuation const& c,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<typename Continuation::return_type>(task_name)
        ,cont_(c),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<typename Continuation::return_type> task_res = this->this_task_result();
        try
        {
            auto func(std::move(func_));
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            cont_.on_done([task_res,func,cutoff,task_name,prio](std::tuple<std::future<typename Continuation::return_type> >&& continuation_res) mutable
            {
                try
                {
                    auto new_continuation = boost::asynchronous::parallel_stable_sort2<typename Continuation::return_type,Func,Job>
                            (std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res) mutable
                    {
                        task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                    });
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(std::make_exception_ptr(e));
                }
            }
            );
            boost::asynchronous::any_continuation ac(std::move(cont_));
            boost::asynchronous::get_continuations().emplace_front(std::move(ac));
        }
        catch(std::exception& e)
        {
            task_res.set_exception(std::make_exception_ptr(e));
        }
    }
    Continuation cont_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
// Continuation is a callback continuation
template <class Continuation, class Func, class Job>
struct parallel_stable_sort_continuation_range_helper2<Continuation,Func,Job,
                                                       typename std::enable_if< boost::asynchronous::detail::has_is_callback_continuation_task<Continuation>::value >::type>
        : public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_stable_sort_continuation_range_helper2(Continuation const& c,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<typename Continuation::return_type>(task_name)
        ,cont_(c),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<typename Continuation::return_type> task_res = this->this_task_result();
        try
        {
            auto func(std::move(func_));
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            cont_.on_done([task_res,func,cutoff,task_name,prio](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& continuation_res) mutable
            {
                try
                {
                    auto new_continuation = boost::asynchronous::parallel_stable_sort2<typename Continuation::return_type,Func,Job>
                            (std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res) mutable
                    {
                        task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                    });
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(std::make_exception_ptr(e));
                }
            }
            );
        }
        catch(std::exception& e)
        {
            task_res.set_exception(std::make_exception_ptr(e));
        }
    }
    Continuation cont_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
#ifdef BOOST_ASYNCHRONOUS_USE_BOOST_SPREADSORT
// adapter to non-callback continuations
template <class Continuation, class Func, class Job,class Enable=void>
struct parallel_spreadsort_continuation_range_helper2: public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_spreadsort_continuation_range_helper2(Continuation const& c,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<typename Continuation::return_type>(task_name)
        ,cont_(c),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<typename Continuation::return_type> task_res = this->this_task_result();
        try
        {
            auto func(std::move(func_));
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            cont_.on_done([task_res,func,cutoff,task_name,prio](std::tuple<std::future<typename Continuation::return_type> >&& continuation_res) mutable
            {
                try
                {
                    auto new_continuation = boost::asynchronous::parallel_spreadsort2<typename Continuation::return_type,Func,Job>
                            (std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res) mutable
                    {
                        task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                    });
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(std::make_exception_ptr(e));
                }
            }
            );
            boost::asynchronous::any_continuation ac(std::move(cont_));
            boost::asynchronous::get_continuations().emplace_front(std::move(ac));
        }
        catch(std::exception& e)
        {
            task_res.set_exception(std::make_exception_ptr(e));
        }
    }
    Continuation cont_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
// Continuation is a callback continuation
template <class Continuation, class Func, class Job>
struct parallel_spreadsort_continuation_range_helper2<Continuation,Func,Job,
                                                      typename std::enable_if< boost::asynchronous::detail::has_is_callback_continuation_task<Continuation>::value >::type>
        : public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_spreadsort_continuation_range_helper2(Continuation const& c,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<typename Continuation::return_type>(task_name)
        ,cont_(c),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<typename Continuation::return_type> task_res = this->this_task_result();
        try
        {
            auto func(std::move(func_));
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            cont_.on_done([task_res,func,cutoff,task_name,prio](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& continuation_res) mutable
            {
                try
                {
                    auto new_continuation = boost::asynchronous::parallel_spreadsort2<typename Continuation::return_type,Func,Job>
                            (std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res) mutable
                    {
                        task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                    });
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(std::make_exception_ptr(e));
                }
            }
            );
        }
        catch(std::exception& e)
        {
            task_res.set_exception(std::make_exception_ptr(e));
        }
    }
    Continuation cont_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
#endif
}
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<Range>::value,boost::asynchronous::detail::callback_continuation<typename Range::return_type,Job> >::type
parallel_sort2(Range range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio=0)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<typename Range::return_type,Job>
            (boost::asynchronous::detail::parallel_sort_continuation_range_helper2<Range,Func,Job>(range,std::move(func),cutoff,task_name,prio));
}
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<Range>::value,boost::asynchronous::detail::callback_continuation<typename Range::return_type,Job> >::type
parallel_stable_sort2(Range range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio=0)
#else
                     const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<typename Range::return_type,Job>
            (boost::asynchronous::detail::parallel_stable_sort_continuation_range_helper2<Range,Func,Job>(range,std::move(func),cutoff,task_name,prio));
}
#ifdef BOOST_ASYNCHRONOUS_USE_BOOST_SPREADSORT
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<Range>::value,boost::asynchronous::detail::callback_continuation<typename Range::return_type,Job> >::type
parallel_spreadsort2(Range range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio=0)
#else
                     const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<typename Range::return_type,Job>
            (boost::asynchronous::detail::parallel_spreadsort_continuation_range_helper2<Range,Func,Job>(range,std::move(func),cutoff,task_name,prio));
}
#endif
}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_SORT_HPP
