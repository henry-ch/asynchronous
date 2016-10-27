// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org
#ifndef BOOST_ASYNCHRONOUS_PARALLEL_INPLACE_SORT_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_INPLACE_SORT_HPP

#include <vector>
#include <iterator> // for std::iterator_traits
#include <boost/smart_ptr/shared_array.hpp>

#include <boost/utility/enable_if.hpp>
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
#include <boost/asynchronous/algorithm/parallel_reverse.hpp>
#include <boost/asynchronous/algorithm/detail/parallel_sort_helper.hpp>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/iterator_range.hpp>

namespace boost { namespace asynchronous
{
// inplace sort variants
// version for iterators => will return nothing
namespace detail
{
template <class Iterator, class Func, class Job, class Sort>
struct parallel_sort_helper: public boost::asynchronous::continuation_task<void>
{
    parallel_sort_helper(Iterator beg, Iterator end,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<void>(task_name),
          beg_(beg),end_(end),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<void> task_res = this_task_result();
        try
        {
            // advance up to cutoff
            Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
            // if not at end, recurse, otherwise execute here
            if (it == end_)
            {
                Sort()(beg_,it,func_);
                task_res.set_value();
            }
            else
            {
                auto beg = beg_;
                auto end = end_;
                auto func = func_;
                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [task_res,func,beg,end,it](std::tuple<boost::asynchronous::expected<void>,boost::asynchronous::expected<void> > res) mutable
                            {
                                try
                                {
                                    // get to check that no exception
                                    std::get<0>(res).get();
                                    std::get<1>(res).get();
                                    // merge both sorted sub-ranges
                                    std::inplace_merge(beg,it,end,func);
                                    task_res.set_value();
                                }
                                catch(std::exception& e)
                                {
                                    task_res.set_exception(boost::copy_exception(e));
                                }
                            },
                            // recursive tasks
                            parallel_sort_helper<Iterator,Func,Job,Sort>(beg_,it,func_,cutoff_,this->get_name(),prio_),
                            parallel_sort_helper<Iterator,Func,Job,Sort>(it,end_,func_,cutoff_,this->get_name(),prio_)
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
}
// version for iterators => will return nothing
template <class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_sort_inplace(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio=0)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_sort_helper<Iterator,Func,Job,boost::asynchronous::std_sort>(beg,end,std::move(func),cutoff,task_name,prio));
}

template <class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_stable_sort_inplace(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio=0)
#else
                     const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_sort_helper<Iterator,Func,Job,boost::asynchronous::std_stable_sort>(beg,end,std::move(func),cutoff,task_name,prio));
}

#ifdef BOOST_ASYNCHRONOUS_USE_BOOST_SPREADSORT
template <class Iterator, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<void,Job>
parallel_spreadsort_inplace(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio=0)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<void,Job>
            (boost::asynchronous::detail::parallel_sort_helper<Iterator,Func,Job,boost::asynchronous::boost_spreadsort>(beg,end,std::move(func),cutoff,task_name,prio));
}
#endif

// version for ranges held only by reference => will return nothing (void)
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::disable_if<boost::asynchronous::detail::has_is_continuation_task<Range>,boost::asynchronous::detail::callback_continuation<void,Job> >::type
parallel_sort_inplace(Range& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio=0)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::parallel_sort_inplace<decltype(boost::begin(range)),Func,Job>
            (boost::begin(range),boost::end(range),std::move(func),cutoff,task_name,prio);
}
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::disable_if<boost::asynchronous::detail::has_is_continuation_task<Range>,boost::asynchronous::detail::callback_continuation<void,Job> >::type
parallel_stable_sort_inplace(Range& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio=0)
#else
                     const std::string& task_name="", std::size_t prio=0)
#endif
{
   return boost::asynchronous::parallel_stable_sort_inplace<decltype(boost::begin(range)),Func,Job>
           (boost::begin(range),boost::end(range),std::move(func),cutoff,task_name,prio);
}

#ifdef BOOST_ASYNCHRONOUS_USE_BOOST_SPREADSORT
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::disable_if<boost::asynchronous::detail::has_is_continuation_task<Range>,boost::asynchronous::detail::callback_continuation<void,Job> >::type
parallel_spreadsort_inplace(Range& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio=0)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
   return boost::asynchronous::parallel_spreadsort_inplace<decltype(boost::begin(range)),Func,Job>
           (boost::begin(range),boost::end(range),std::move(func),cutoff,task_name,prio);
}
#endif

// version for moved ranges => will return the range as continuation
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::disable_if<boost::asynchronous::detail::is_serializable<Func>,boost::asynchronous::detail::callback_continuation<Range,Job> >::type
parallel_sort_move_inplace(Range&& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                   const std::string& task_name, std::size_t prio=0)
#else
                   const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto r = boost::make_shared<Range>(std::forward<Range>(range));
    auto beg = boost::begin(*r);
    auto end = boost::end(*r);
    return boost::asynchronous::top_level_callback_continuation_job<Range,Job>
            (boost::asynchronous::detail::parallel_sort_range_move_helper<boost::asynchronous::detail::callback_continuation<void,Job>,Range>
             (boost::asynchronous::parallel_sort_inplace<decltype(boost::begin(*r)),Func,Job>
              (beg,end,std::move(func),cutoff,task_name,prio),r,task_name));
}
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::disable_if<boost::asynchronous::detail::is_serializable<Func>,boost::asynchronous::detail::callback_continuation<Range,Job> >::type
parallel_stable_sort_move_inplace(Range&& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                          const std::string& task_name, std::size_t prio=0)
#else
                          const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto r = boost::make_shared<Range>(std::forward<Range>(range));
    auto beg = boost::begin(*r);
    auto end = boost::end(*r);
    return boost::asynchronous::top_level_callback_continuation_job<Range,Job>
            (boost::asynchronous::detail::parallel_sort_range_move_helper<boost::asynchronous::detail::callback_continuation<void,Job>,Range>
             (boost::asynchronous::parallel_stable_sort_inplace<decltype(boost::begin(*r)),Func,Job>
              (beg,end,std::move(func),cutoff,task_name,prio),r,task_name));
}
#ifdef BOOST_ASYNCHRONOUS_USE_BOOST_SPREADSORT
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::disable_if<boost::asynchronous::detail::is_serializable<Func>,boost::asynchronous::detail::callback_continuation<Range,Job> >::type
parallel_spreadsort_move_inplace(Range&& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                   const std::string& task_name, std::size_t prio=0)
#else
                   const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto r = boost::make_shared<Range>(std::forward<Range>(range));
    auto beg = boost::begin(*r);
    auto end = boost::end(*r);
    return boost::asynchronous::top_level_callback_continuation_job<Range,Job>
            (boost::asynchronous::detail::parallel_sort_range_move_helper<boost::asynchronous::detail::callback_continuation<void,Job>,Range>
             (boost::asynchronous::parallel_spreadsort_inplace<decltype(boost::begin(*r)),Func,Job>
              (beg,end,std::move(func),cutoff,task_name,prio),r,task_name));
}
#endif

namespace detail
{
// adapter to non-callback continuations
template <class Continuation, class Func, class Job,class Enable=void>
struct parallel_sort_continuation_range_inplace_helper: public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_sort_continuation_range_inplace_helper(Continuation const& c,Func func,long cutoff,
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
            cont_.on_done([task_res,func,cutoff,task_name,prio](std::tuple<boost::future<typename Continuation::return_type> >&& continuation_res) mutable
            {
                try
                {
                    auto new_continuation = boost::asynchronous::parallel_sort_move_inplace<typename Continuation::return_type,Func,Job>
                            (std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res) mutable
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
            boost::asynchronous::any_continuation ac(std::move(cont_));
            boost::asynchronous::get_continuations().emplace_front(std::move(ac));
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
// Continuation is a callback continuation
template <class Continuation, class Func, class Job>
struct parallel_sort_continuation_range_inplace_helper<Continuation,Func,Job,
                                                       typename ::boost::enable_if< boost::asynchronous::detail::has_is_callback_continuation_task<Continuation> >::type>
        : public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_sort_continuation_range_inplace_helper(Continuation const& c,Func func,long cutoff,
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
                    auto new_continuation = boost::asynchronous::parallel_sort_move_inplace<typename Continuation::return_type,Func,Job>
                            (std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res) mutable
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

// adapter to non-callback continuations
template <class Continuation, class Func, class Job,class Enable=void>
struct parallel_stable_sort_continuation_range_inplace_helper: public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_stable_sort_continuation_range_inplace_helper(Continuation const& c,Func func,long cutoff,
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
            cont_.on_done([task_res,func,cutoff,task_name,prio](std::tuple<boost::future<typename Continuation::return_type> >&& continuation_res) mutable
            {
                try
                {
                    auto new_continuation = boost::asynchronous::parallel_stable_sort_move_inplace<typename Continuation::return_type,Func,Job>
                            (std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res) mutable
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
            boost::asynchronous::any_continuation ac(std::move(cont_));
            boost::asynchronous::get_continuations().emplace_front(std::move(ac));
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
// Continuation is a callback continuation
template <class Continuation, class Func, class Job>
struct parallel_stable_sort_continuation_range_inplace_helper<Continuation,Func,Job,
                                                              typename ::boost::enable_if< boost::asynchronous::detail::has_is_callback_continuation_task<Continuation> >::type>
        : public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_stable_sort_continuation_range_inplace_helper(Continuation const& c,Func func,long cutoff,
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
                    auto new_continuation = boost::asynchronous::parallel_stable_sort_move_inplace<typename Continuation::return_type,Func,Job>
                            (std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res) mutable
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
#ifdef BOOST_ASYNCHRONOUS_USE_BOOST_SPREADSORT
// adapter to non-callback continuations
template <class Continuation, class Func, class Job,class Enable=void>
struct parallel_spreadsort_continuation_range_inplace_helper: public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_spreadsort_continuation_range_inplace_helper(Continuation const& c,Func func,long cutoff,
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
            cont_.on_done([task_res,func,cutoff,task_name,prio](std::tuple<boost::future<typename Continuation::return_type> >&& continuation_res) mutable
            {
                try
                {
                    auto new_continuation = boost::asynchronous::parallel_spreadsort_move_inplace<typename Continuation::return_type,Func,Job>
                            (std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res) mutable
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
            boost::asynchronous::any_continuation ac(std::move(cont_));
            boost::asynchronous::get_continuations().emplace_front(std::move(ac));
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
// Continuation is a callback continuation
template <class Continuation, class Func, class Job>
struct parallel_spreadsort_continuation_range_inplace_helper<Continuation,Func,Job,
                                                             typename ::boost::enable_if< boost::asynchronous::detail::has_is_callback_continuation_task<Continuation> >::type>
        : public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_spreadsort_continuation_range_inplace_helper(Continuation const& c,Func func,long cutoff,
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
                    auto new_continuation = boost::asynchronous::parallel_spreadsort_move_inplace<typename Continuation::return_type,Func,Job>
                            (std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res) mutable
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
#endif
}
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::enable_if<boost::asynchronous::detail::has_is_continuation_task<Range>,boost::asynchronous::detail::callback_continuation<typename Range::return_type,Job> >::type
parallel_sort_inplace(Range range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
              const std::string& task_name, std::size_t prio=0)
#else
              const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<typename Range::return_type,Job>
            (boost::asynchronous::detail::parallel_sort_continuation_range_inplace_helper<Range,Func,Job>(range,std::move(func),cutoff,task_name,prio));
}
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::enable_if<boost::asynchronous::detail::has_is_continuation_task<Range>,boost::asynchronous::detail::callback_continuation<typename Range::return_type,Job> >::type
parallel_stable_sort_inplace(Range range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio=0)
#else
                     const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<typename Range::return_type,Job>
            (boost::asynchronous::detail::parallel_stable_sort_continuation_range_inplace_helper<Range,Func,Job>(range,std::move(func),cutoff,task_name,prio));
}
#ifdef BOOST_ASYNCHRONOUS_USE_BOOST_SPREADSORT
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::enable_if<boost::asynchronous::detail::has_is_continuation_task<Range>,boost::asynchronous::detail::callback_continuation<typename Range::return_type,Job> >::type
parallel_spreadsort_inplace(Range range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio=0)
#else
                     const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<typename Range::return_type,Job>
            (boost::asynchronous::detail::parallel_spreadsort_continuation_range_inplace_helper<Range,Func,Job>(range,std::move(func),cutoff,task_name,prio));
}
#endif

}}


#endif // BOOST_ASYNCHRONOUS_PARALLEL_INPLACE_SORT_HPP
