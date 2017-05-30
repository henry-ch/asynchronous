// Boost.Asynchronous library
//  Copyright (C) Christophe Henry, Tobias Holl 2014
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRON_PARALLEL_FIND_ALL_HPP
#define BOOST_ASYNCHRON_PARALLEL_FIND_ALL_HPP

#include <algorithm>
#include <utility>
#include <type_traits>
#include <iterator>


#include <type_traits>
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
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>

namespace boost { namespace asynchronous
{
    
namespace detail {
    
template <class Range, class Func, class ReturnRange>
void find_all(Range rng, Func fn, ReturnRange& ret) {
    boost::copy(rng | boost::adaptors::filtered(fn), std::back_inserter(ret));
}

template <class Func>
struct not_
{
    not_(Func&& f):func_(std::forward<Func>(f)){}
    template <typename... Arg>
    bool operator()(Arg... arg)const
    {
        return !func_(arg...);
    }
    Func func_;
};

template <class Range, class Func>
void find_all(Range& rng, Func fn) {
    boost::remove_erase_if(rng , boost::asynchronous::detail::not_<Func>(std::move(fn)));
}
    
}

// version for iterators
namespace detail
{
template <class Iterator, class Func, class ReturnRange, class Job>
struct parallel_find_all_helper: public boost::asynchronous::continuation_task<ReturnRange>
{
    parallel_find_all_helper(Iterator beg, Iterator end,Func func,long cutoff,
                             std::string task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<ReturnRange>(task_name)
        , beg_(beg),end_(end),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()const
    {
        boost::asynchronous::continuation_result<ReturnRange> task_res = this->this_task_result();
        try
        {
            // advance up to cutoff
            Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
            // if not at end, recurse, otherwise execute here
            if (it == end_)
            {
                ReturnRange ret(beg_,it);
                boost::asynchronous::detail::find_all(ret,func_);
                task_res.set_value(std::move(ret));
            }
            else
            {
                boost::asynchronous::create_callback_continuation_job<Job>(
                    // called when subtasks are done, set our result
                    [task_res]
                    (std::tuple<boost::asynchronous::expected<ReturnRange>,boost::asynchronous::expected<ReturnRange>> res) mutable
                    {
                        try
                        {
                            ReturnRange rt = std::move(std::get<0>(res).get());
                            boost::range::push_back(rt, std::move(std::get<1>(res).get()));
                            task_res.set_value(std::move(rt));
                        }
                        catch(...)
                        {
                            task_res.set_exception(std::current_exception());
                        }
                    },
                    // recursive tasks
                    parallel_find_all_helper<Iterator,Func,ReturnRange,Job>(beg_,it,func_,cutoff_,this->get_name(),prio_),
                    parallel_find_all_helper<Iterator,Func,ReturnRange,Job>(it,end_,func_,cutoff_,this->get_name(),prio_)
                );
            }
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
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
          class ReturnRange=std::vector<typename std::iterator_traits<Iterator>::value_type>,
          class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<ReturnRange,Job>
parallel_find_all(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                  const std::string& task_name, std::size_t prio=0)
#else
                  const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<ReturnRange,Job>
            (boost::asynchronous::detail::parallel_find_all_helper<Iterator,Func,ReturnRange,Job>(beg,end,func,cutoff,task_name,prio));
}

// version for ranges held only by reference
namespace detail
{
template <class Range, class Func, class ReturnRange, class Job>
struct parallel_find_all_range_helper: public boost::asynchronous::continuation_task<ReturnRange>
{
    parallel_find_all_range_helper(Range const& range,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<ReturnRange>(task_name)
        ,range_(range),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<ReturnRange> task_res = this->this_task_result();
        try
        {
            // advance up to cutoff
            auto it = boost::asynchronous::detail::find_cutoff(boost::begin(range_),cutoff_,boost::end(range_));
            // if not at end, recurse, otherwise execute here
            if (it == boost::end(range_))
            {
                ReturnRange ret(boost::begin(range_),it);
                boost::asynchronous::detail::find_all(ret,func_);
                task_res.set_value(std::move(ret));
            }
            else
            {
                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [task_res]
                            (std::tuple<boost::asynchronous::expected<ReturnRange>,boost::asynchronous::expected<ReturnRange>> res) mutable
                            {
                                try
                                {
                                    ReturnRange rt = std::move(std::get<0>(res).get());
                                    boost::range::push_back(rt, std::move(std::get<1>(res).get()));
                                    task_res.set_value(std::move(rt));
                                }
                                catch(...)
                                {
                                    task_res.set_exception(std::current_exception());
                                }
                            },
                            // recursive tasks
                            parallel_find_all_helper<decltype(boost::begin(range_)),Func,ReturnRange,Job>(
                                        boost::begin(range_),it,func_,cutoff_,this->get_name(),prio_),
                            parallel_find_all_helper<decltype(boost::begin(range_)),Func,ReturnRange,Job>(
                                        it,boost::end(range_),func_,cutoff_,this->get_name(),prio_)
                        );
            }
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
        }
    }
    Range const& range_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
}
template <class Range, class Func, class ReturnRange=Range, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<!boost::asynchronous::detail::has_is_continuation_task<Range>::value,boost::asynchronous::detail::callback_continuation<ReturnRange,Job> >::type
parallel_find_all(Range const& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                  const std::string& task_name, std::size_t prio=0)
#else
                  const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<ReturnRange,Job>
            (boost::asynchronous::detail::parallel_find_all_range_helper<Range,Func,ReturnRange,Job>(range,func,cutoff,task_name,prio));
}

// version for moved ranges
template <class Range, class Func, class ReturnRange, class Job,class Enable=void>
struct parallel_find_all_range_move_helper: public boost::asynchronous::continuation_task<ReturnRange>
{
    template <class Iterator>
    parallel_find_all_range_move_helper(std::shared_ptr<Range> range,Iterator , Iterator ,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<ReturnRange>(task_name)
        ,range_(range),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<ReturnRange> task_res = this->this_task_result();
        try
        {
            std::shared_ptr<Range> range = std::move(range_);
            // advance up to cutoff
            auto it = boost::asynchronous::detail::find_cutoff(boost::begin(*range),cutoff_,boost::end(*range));
            // if not at end, recurse, otherwise execute here
            if (it == boost::end(*range))
            {
                ReturnRange ret(boost::begin(*range),it);
                boost::asynchronous::detail::find_all(ret,func_);
                task_res.set_value(std::move(ret));
            }
            else
            {
                boost::asynchronous::create_callback_continuation_job<Job>(
                    // called when subtasks are done, set our result
                    [task_res,range]
                    (std::tuple<boost::asynchronous::expected<ReturnRange>,boost::asynchronous::expected<ReturnRange>> res) mutable
                    {
                        try
                        {
                            ReturnRange rt = std::move(std::get<0>(res).get());
                            boost::range::push_back(rt, std::move(std::get<1>(res).get()));
                            task_res.set_value(std::move(rt));
                        }
                        catch(...)
                        {
                            task_res.set_exception(std::current_exception());
                        }
                    },
                    // recursive tasks
                    boost::asynchronous::detail::parallel_find_all_helper<decltype(boost::begin(*range)),Func,ReturnRange,Job>(
                                boost::begin(*range),it,func_,cutoff_,this->get_name(),prio_),
                    boost::asynchronous::detail::parallel_find_all_helper<decltype(boost::begin(*range)),Func,ReturnRange,Job>(
                                it,boost::end(*range),func_,cutoff_,this->get_name(),prio_)
                );
            }
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
        }
    }
    std::shared_ptr<Range> range_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};

template <class Range, class Func, class ReturnRange, class Job>
struct parallel_find_all_range_move_helper<Range,Func,ReturnRange,Job,typename std::enable_if<boost::asynchronous::detail::is_serializable<Func>::value >::type>
        : public boost::asynchronous::continuation_task<ReturnRange>
        , public boost::asynchronous::serializable_task
{
    //default ctor only when deserialized immediately after
    parallel_find_all_range_move_helper():boost::asynchronous::serializable_task("parallel_find_all_range_move_helper")
    {
    }
    template <class Iterator>
    parallel_find_all_range_move_helper(std::shared_ptr<Range> range,Iterator beg, Iterator end,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<ReturnRange>(task_name)
        , boost::asynchronous::serializable_task(func.get_task_name())
        , range_(range),func_(std::move(func)),cutoff_(cutoff),task_name_(task_name),prio_(prio), begin_(beg), end_(end)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<ReturnRange> task_res = this->this_task_result();
        try
        {
            // advance up to cutoff
            auto it = boost::asynchronous::detail::find_cutoff(begin_,cutoff_,end_);
            // if not at end, recurse, otherwise execute here
            if (it == end_)
            {
                ReturnRange ret(begin_,it);
                boost::asynchronous::detail::find_all(ret,func_);
                task_res.set_value(std::move(ret));
            }
            else
            {
                boost::asynchronous::create_callback_continuation_job<Job>(
                    // called when subtasks are done, set our result
                    [task_res]
                    (std::tuple<boost::asynchronous::expected<ReturnRange>,boost::asynchronous::expected<ReturnRange>> res) mutable
                    {
                        try
                        {
                            ReturnRange rt = std::move(std::get<0>(res).get());
                            boost::range::push_back(rt, std::move(std::get<1>(res).get()));
                            task_res.set_value(std::move(rt));
                        }
                        catch(...)
                        {
                            task_res.set_exception(std::current_exception());
                        }
                    },
                    // recursive tasks
                    parallel_find_all_range_move_helper<Range,Func,ReturnRange,Job>(
                                range_,begin_,it,
                                func_,cutoff_,task_name_,prio_),
                    parallel_find_all_range_move_helper<Range,Func,ReturnRange,Job>(
                                range_,it,end_,
                                func_,cutoff_,task_name_,prio_)
                );
            }
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
        }
    }
    template <class Archive>
    void save(Archive & ar, const unsigned int /*version*/)const
    {
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
        range_ = std::make_shared<Range>();
        ar & (*range_);
        ar & func_;
        ar & cutoff_;
        ar & task_name_;
        ar & prio_;
        begin_ = boost::begin(*range_);
        end_ = boost::end(*range_);
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
    std::shared_ptr<Range> range_;
    Func func_;
    long cutoff_;
    std::string task_name_;
    std::size_t prio_;
    decltype(boost::begin(*range_)) begin_;
    decltype(boost::end(*range_)) end_;
};

template <class Range, class Func, class ReturnRange=Range, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<!boost::asynchronous::detail::has_is_continuation_task<Range>::value,boost::asynchronous::detail::callback_continuation<ReturnRange,Job> >::type
parallel_find_all(Range&& range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                  const std::string& task_name, std::size_t prio=0)
#else
                  const std::string& task_name="", std::size_t prio=0)
#endif
{
    auto r = std::make_shared<Range>(std::forward<Range>(range));
    auto beg = boost::begin(*r);
    auto end = boost::end(*r);
    return boost::asynchronous::top_level_callback_continuation_job<ReturnRange,Job>
            (boost::asynchronous::parallel_find_all_range_move_helper<Range,Func,ReturnRange,Job>(r,beg,end,func,cutoff,task_name,prio));
}

// version for ranges given as continuation
namespace detail
{
// adapter to non-callback continuations
template <class Continuation, class Func, class ReturnRange, class Job,class Enable=void>
struct parallel_find_all_continuation_range_helper: public boost::asynchronous::continuation_task<ReturnRange>
{
    parallel_find_all_continuation_range_helper(Continuation const& c,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<ReturnRange>(task_name)
        ,cont_(c),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<ReturnRange> task_res = this->this_task_result();
        try
        {
            auto func(std::move(func_));
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            cont_.on_done([task_res,func,cutoff,task_name,prio]
                          (std::tuple<std::future<typename Continuation::return_type> >&& continuation_res) mutable
            {
                try
                {
                    auto new_continuation = boost::asynchronous::parallel_find_all<typename Continuation::return_type, Func, ReturnRange, Job>(std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<ReturnRange> >&& new_continuation_res)
                    {
                        task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                    });
                }
                catch(...)
                {
                    task_res.set_exception(std::current_exception());
                }
            }
            );
            boost::asynchronous::any_continuation ac(std::move(cont_));
            boost::asynchronous::get_continuations().emplace_front(std::move(ac));
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
        }
    }
    Continuation cont_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
// Continuation is a callback continuation
template <class Continuation, class Func, class ReturnRange, class Job>
struct parallel_find_all_continuation_range_helper<Continuation,Func,ReturnRange,Job,
                                                   typename std::enable_if< boost::asynchronous::detail::has_is_callback_continuation_task<Continuation>::value >::type>
        : public boost::asynchronous::continuation_task<ReturnRange>
{
    parallel_find_all_continuation_range_helper(Continuation const& c,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<ReturnRange>(task_name)
        ,cont_(c),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<ReturnRange> task_res = this->this_task_result();
        try
        {
            auto func(std::move(func_));
            auto cutoff = cutoff_;
            auto task_name = this->get_name();
            auto prio = prio_;
            cont_.on_done([task_res,func,cutoff,task_name,prio]
                          (std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& continuation_res) mutable
            {
                try
                {
                    auto new_continuation = boost::asynchronous::parallel_find_all<typename Continuation::return_type, Func, ReturnRange, Job>(std::move(std::get<0>(continuation_res).get()),func,cutoff,task_name,prio);
                    new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<ReturnRange> >&& new_continuation_res)
                    {
                        task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                    });
                }
                catch(...)
                {
                    task_res.set_exception(std::current_exception());
                }
            }
            );
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
        }
    }
    Continuation cont_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
}

template <class Range, class Func, class ReturnRange=typename Range::return_type, class Job=typename BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<Range>::value, boost::asynchronous::detail::callback_continuation<ReturnRange, Job>>::type
parallel_find_all(Range range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                  const std::string& task_name, std::size_t prio=0)
#else
                  const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<ReturnRange,Job>
            (boost::asynchronous::detail::parallel_find_all_continuation_range_helper<Range,Func,ReturnRange,Job>(range,func,cutoff,task_name,prio));
}

}}
#endif // BOOST_ASYNCHRON_PARALLEL_FIND_ALL_HPP
