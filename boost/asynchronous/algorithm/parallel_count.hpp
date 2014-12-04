// Boost.Asynchronous library
//  Copyright (C) Christophe Henry, Tobias Holl 2014
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRON_PARALLEL_COUNT_HPP
#define BOOST_ASYNCHRON_PARALLEL_COUNT_HPP

#include <algorithm>
#include <utility>
#include <type_traits>
#include <iterator>

#include <boost/thread/future.hpp>
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

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>

namespace boost { namespace asynchronous
{
    
namespace detail {

/* Did not work.
template <class Range, class Func>
typename boost::range_difference<Range>::type count(Range const& r, Func fn) {
    return boost::size(r | boost::adaptors::filtered(fn));
}
*/

template <class Range, class Func>
long count(Range const& r, Func fn) {
    long c = 0;
    for (auto it = boost::begin(r); it != boost::end(r); ++it) {
        if (fn(*it))
            ++c;
    }
    return c;
}
    
}

// version for iterators
namespace detail
{
template <class Iterator, class Func, class Job>
struct parallel_count_helper: public boost::asynchronous::continuation_task<long>
{
    parallel_count_helper(Iterator beg, Iterator end,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<long>(task_name)
        , beg_(beg),end_(end),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()const
    {
        boost::asynchronous::continuation_result<long> task_res = this->this_task_result();
        // advance up to cutoff
        Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
        // if not at end, recurse, otherwise execute here
        if (it == end_)
        {
            task_res.set_value(std::move(boost::asynchronous::detail::count(boost::make_iterator_range(beg_, it),func_)));
        }
        else
        {
            boost::asynchronous::create_callback_continuation_job<Job>(
                        // called when subtasks are done, set our result
                        [task_res](std::tuple<boost::asynchronous::expected<long>,boost::asynchronous::expected<long>> res)
                        {
                            try
                            {
                                long rt = std::get<0>(res).get();
                                rt += std::get<1>(res).get();
                                task_res.set_value(std::move(rt));
                            }
                            catch(std::exception& e)
                            {
                                task_res.set_exception(boost::copy_exception(e));
                            }
                        },
                        // recursive tasks
                        parallel_count_helper<Iterator,Func,Job>(beg_,it,func_,cutoff_,this->get_name(),prio_),
                        parallel_count_helper<Iterator,Func,Job>(it,end_,func_,cutoff_,this->get_name(),prio_)
            );
        }
    }
    Iterator beg_;
    Iterator end_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
}
template <class Iterator, class Func,
          class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<long,Job>
parallel_count(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<long,Job>
            (boost::asynchronous::detail::parallel_count_helper<Iterator,Func,Job>(beg,end,func,cutoff,task_name,prio));
}

// version for ranges held only by reference
namespace detail
{
template <class Range, class Func, class Job>
struct parallel_count_range_helper: public boost::asynchronous::continuation_task<long>
{
    parallel_count_range_helper(Range const& range,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<long>(task_name)
        ,range_(range),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()const
    {
        boost::asynchronous::continuation_result<long> task_res = this->this_task_result();
        // advance up to cutoff
        auto it = boost::asynchronous::detail::find_cutoff(boost::begin(range_),cutoff_,boost::end(range_));
        // if not at end, recurse, otherwise execute here
        if (it == boost::end(range_))
        {
            task_res.set_value(std::move(boost::asynchronous::detail::count(boost::make_iterator_range(boost::begin(range_), it),std::move(func_))));
        }
        else
        {
            boost::asynchronous::create_callback_continuation_job<Job>(
                        // called when subtasks are done, set our result
                        [task_res](std::tuple<boost::asynchronous::expected<long>,boost::asynchronous::expected<long>> res)
                        {
                            try
                            {
                                long rt = std::get<0>(res).get();
                                rt += std::get<1>(res).get();
                                task_res.set_value(std::move(rt));
                            }
                            catch(std::exception& e)
                            {
                                task_res.set_exception(boost::copy_exception(e));
                            }
                        },
                        // recursive tasks
                        parallel_count_helper<decltype(boost::begin(range_)),Func,Job>(boost::begin(range_),it,func_,cutoff_,this->get_name(),prio_),
                        parallel_count_helper<decltype(boost::begin(range_)),Func,Job>(it,boost::end(range_),func_,cutoff_,this->get_name(),prio_)
            );
        }
    }
    Range const& range_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
}
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::disable_if<has_is_continuation_task<Range>,boost::asynchronous::detail::callback_continuation<long,Job> >::type
parallel_count(Range const& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
               const std::string& task_name, std::size_t prio)
#else
               const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<long,Job>
            (boost::asynchronous::detail::parallel_count_range_helper<Range,Func,Job>(range,func,cutoff,task_name,prio));
}

// version for moved ranges
template <class Range, class Func, class Job,class Enable=void>
struct parallel_count_range_move_helper: public boost::asynchronous::continuation_task<long>
{
    template <class Iterator>
    parallel_count_range_move_helper(boost::shared_ptr<Range> range,Iterator , Iterator,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<long>(task_name)
        ,range_(range),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()const
    {
        boost::shared_ptr<Range> range = std::move(range_);        
        boost::asynchronous::continuation_result<long> task_res = this->this_task_result();
        // advance up to cutoff
        auto it = boost::asynchronous::detail::find_cutoff(boost::begin(*range),cutoff_,boost::end(*range));
        // if not at end, recurse, otherwise execute here
        if (it == boost::end(*range))
        {
            task_res.set_value(std::move(boost::asynchronous::detail::count(boost::make_iterator_range(boost::begin(*range), it),std::move(func_))));
        }
        else
        {
            boost::asynchronous::create_callback_continuation_job<Job>(
                        // called when subtasks are done, set our result
                        [task_res,range](std::tuple<boost::asynchronous::expected<long>,boost::asynchronous::expected<long>> res)
                        {
                            try
                            {
                                long rt = std::get<0>(res).get();
                                rt += std::get<1>(res).get();
                                task_res.set_value(std::move(rt));
                            }
                            catch(std::exception& e)
                            {
                                task_res.set_exception(boost::copy_exception(e));
                            }
                        },
                        // recursive tasks
                        boost::asynchronous::detail::parallel_count_helper<decltype(boost::begin(*range_)),Func,Job>(
                            boost::begin(*range),it,func_,cutoff_,this->get_name(),prio_),
                        boost::asynchronous::detail::parallel_count_helper<decltype(boost::begin(*range_)),Func,Job>(
                            it,boost::end(*range),func_,cutoff_,this->get_name(),prio_)
            );
        }
    }
    boost::shared_ptr<Range> range_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};

template <class Range, class Func, class Job>
struct parallel_count_range_move_helper<Range,Func,Job,typename ::boost::enable_if<boost::asynchronous::detail::is_serializable<Func> >::type>
        : public boost::asynchronous::continuation_task<long>
        , public boost::asynchronous::serializable_task
{
    //default ctor only when deserialized immediately after
    parallel_count_range_move_helper():boost::asynchronous::serializable_task("parallel_count_range_move_helper")
    {
    }
    template <class Iterator>
    parallel_count_range_move_helper(boost::shared_ptr<Range> range,Iterator beg, Iterator end,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<long>(task_name)
        , boost::asynchronous::serializable_task(func.get_task_name())
        , range_(range),func_(std::move(func)),cutoff_(cutoff),task_name_(task_name),prio_(prio),begin_(beg), end_(end)
    {}
    void operator()()const
    {
        boost::asynchronous::continuation_result<long> task_res = this->this_task_result();
        // advance up to cutoff
        auto it = boost::asynchronous::detail::find_cutoff(begin_,cutoff_,end_);
        // if not at end, recurse, otherwise execute here
        if (it == end_)
        {
            task_res.set_value(std::move(boost::asynchronous::detail::count(boost::make_iterator_range(begin_, it),std::move(func_))));
        }
        else
        {
            boost::asynchronous::create_callback_continuation_job<Job>(
                        // called when subtasks are done, set our result
                        [task_res](std::tuple<boost::asynchronous::expected<long>,boost::asynchronous::expected<long>> res)
                        {
                            try
                            {
                                long rt = std::get<0>(res).get();
                                rt += std::get<1>(res).get();
                                task_res.set_value(std::move(rt));
                            }
                            catch(std::exception& e)
                            {
                                task_res.set_exception(boost::copy_exception(e));
                            }
                        },
                        // recursive tasks
                        boost::asynchronous::parallel_count_range_move_helper<Range,Func,Job>(
                                    range_,begin_,it,
                                    func_,cutoff_,task_name_,prio_),
                        boost::asynchronous::parallel_count_range_move_helper<Range,Func,Job>(
                                    range_,it,end_,
                                    func_,cutoff_,task_name_,prio_)
            );
        }
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
    }
    template <class Archive>
    void load(Archive & ar, const unsigned int /*version*/)
    {
        range_ = boost::make_shared<Range>();
        ar & (*range_);
        ar & func_;
        ar & cutoff_;
        ar & task_name_;
        ar & prio_;
        begin_ = boost::begin(*range_);
        end_ = boost::end(*range_);
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
    boost::shared_ptr<Range> range_;
    Func func_;
    long cutoff_;
    std::string task_name_;
    std::size_t prio_;
    decltype(boost::begin(*range_)) begin_;
    decltype(boost::end(*range_)) end_;
};

template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::disable_if<has_is_continuation_task<Range>,boost::asynchronous::detail::callback_continuation<long,Job> >::type
parallel_count(Range&& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
               const std::string& task_name, std::size_t prio)
#else
               const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto r = boost::make_shared<Range>(std::forward<Range>(range));
    auto beg = boost::begin(*r);
    auto end = boost::end(*r);
    return boost::asynchronous::top_level_callback_continuation_job<long,Job>
            (boost::asynchronous::parallel_count_range_move_helper<Range,Func,Job>(r,beg,end,func,cutoff,task_name,prio));
}


// version for ranges given as continuation
namespace detail
{
// adapter to non-callback continuations
template <class Continuation, class Func, class Job,class Enable=void>
struct parallel_count_continuation_range_helper: public boost::asynchronous::continuation_task<long>
{
    parallel_count_continuation_range_helper(Continuation const& c,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<long>(task_name)
        ,cont_(c),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<long> task_res = this->this_task_result();
        auto func(std::move(func_));
        auto cutoff = cutoff_;
        auto task_name = this->get_name();
        auto prio = prio_;
        cont_.on_done([task_res,func,cutoff,task_name,prio](std::tuple<boost::future<typename Continuation::return_type> >&& continuation_res)
        {
            try
            {
                auto new_continuation = boost::asynchronous::parallel_count<typename Continuation::return_type, Func, Job>(std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                new_continuation.on_done([task_res](std::tuple<boost::future<long> >&& new_continuation_res)
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
        boost::asynchronous::any_continuation ac(cont_);
        boost::asynchronous::get_continuations().push_front(ac);
    }
    Continuation cont_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
// Continuation is a callback continuation
template <class Continuation, class Func, class Job>
struct parallel_count_continuation_range_helper<Continuation,Func,Job,typename ::boost::enable_if< has_is_callback_continuation_task<Continuation> >::type>
        : public boost::asynchronous::continuation_task<long>
{
    parallel_count_continuation_range_helper(Continuation const& c,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<long>(task_name)
        ,cont_(c),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<long> task_res = this->this_task_result();
        auto func(std::move(func_));
        auto cutoff = cutoff_;
        auto task_name = this->get_name();
        auto prio = prio_;
        cont_.on_done([task_res,func,cutoff,task_name,prio](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& continuation_res)
        {
            try
            {
                auto new_continuation = boost::asynchronous::parallel_count<typename Continuation::return_type, Func, Job>(std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                new_continuation.on_done([task_res](std::tuple<boost::future<long> >&& new_continuation_res)
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
    Continuation cont_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
}

template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::enable_if<has_is_continuation_task<Range>, boost::asynchronous::detail::continuation<long, Job>>::type
parallel_count(Range range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
               const std::string& task_name, std::size_t prio)
#else
               const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_continuation_job<long,Job>
            (boost::asynchronous::detail::parallel_count_continuation_range_helper<Range,Func,Job>(range,func,cutoff,task_name,prio));
}


}}
#endif // BOOST_ASYNCHRON_PARALLEL_COUNT_HPP
