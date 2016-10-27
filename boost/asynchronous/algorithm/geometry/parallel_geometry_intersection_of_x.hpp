// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2014
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_GEOMETRY_INTERSECTION_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_GEOMETRY_INTERSECTION_HPP

#include <algorithm>

#include <boost/utility/enable_if.hpp>
#include <boost/geometry.hpp>

#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>
#include <boost/asynchronous/algorithm/geometry/parallel_intersection.hpp>

namespace boost { namespace asynchronous { namespace geometry
{
namespace detail
{

template <class Iterator,class Job>
struct parallel_geometry_intersection_of_x_helper:
        public boost::asynchronous::continuation_task<typename std::iterator_traits<Iterator>::value_type>
{
    parallel_geometry_intersection_of_x_helper(Iterator beg, Iterator end,
                            long cutoff,long overlay_cutoff,long partition_cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<typename std::iterator_traits<Iterator>::value_type>(task_name),
          beg_(beg),end_(end),cutoff_(cutoff),overlay_cutoff_(overlay_cutoff),partition_cutoff_(partition_cutoff),prio_(prio)
    {}    

    void operator()()
    {
        boost::asynchronous::continuation_result<typename std::iterator_traits<Iterator>::value_type> task_res = this->this_task_result();
        // advance up to cutoff
        Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
        if (it == end_)
        {
            typename std::iterator_traits<Iterator>::value_type final_output;
            bool first = true;
            for(auto it = beg_; it != end_; ++it)
            {
                if (first)
                {
                    final_output = *it;
                    first = false;
                }
                else
                {
                    typename std::iterator_traits<Iterator>::value_type result;
                    boost::geometry::intersection(final_output, *it, result);
                    final_output = std::move(result);
                }
            }
            task_res.set_value(std::move(final_output));
        }
        else
        {
            auto task_name = this->get_name();
            auto prio = prio_;
            auto overlay_cutoff = overlay_cutoff_;
            auto partition_cutoff = partition_cutoff_;

            boost::asynchronous::create_callback_continuation_job<Job>(
                        // called when subtasks are done, set our result
                        [task_res,task_name,prio,overlay_cutoff,partition_cutoff]
                        (std::tuple<boost::asynchronous::expected<typename std::iterator_traits<Iterator>::value_type>,
                                    boost::asynchronous::expected<typename std::iterator_traits<Iterator>::value_type> > res)
                        {
                            try
                            {
                                auto res0 = std::get<0>(res).get();
                                auto res1 = std::get<1>(res).get();
                                //typename std::iterator_traits<Iterator>::value_type result;
                                //boost::geometry::intersection(res0, res1, result);
                                //task_res.set_value(std::move(result));
                                typedef typename std::iterator_traits<Iterator>::value_type Return;
                                auto cont = boost::asynchronous::geometry::parallel_intersection<Return,Return,Return,Job>
                                        (std::move(res0),std::move(res1),overlay_cutoff,partition_cutoff);
                                cont.on_done([task_res](std::tuple<boost::asynchronous::expected<Return> >&& res_p_intersect)
                                {
                                    try
                                    {
                                        task_res.set_value(std::move(std::get<0>(res_p_intersect).get()));
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
                        },
                        // recursive tasks
                        parallel_geometry_intersection_of_x_helper<Iterator,Job>
                            (beg_,it,cutoff_,overlay_cutoff_,partition_cutoff_,this->get_name(),prio_),
                        parallel_geometry_intersection_of_x_helper<Iterator,Job>
                            (it,end_,cutoff_,overlay_cutoff_,partition_cutoff_,this->get_name(),prio_)
            );
        }
    }
    Iterator beg_;
    Iterator end_;
    long cutoff_;
    long overlay_cutoff_;
    long partition_cutoff_;
    std::size_t prio_;
};
}

template <class Iterator, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<typename std::iterator_traits<Iterator>::value_type,Job>
parallel_geometry_intersection_of_x(Iterator beg, Iterator end,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio=0,
#else
                    const std::string& task_name="", std::size_t prio=0,
#endif
                    long cutoff=300, long overlay_cutoff=1500, long partition_cutoff=80000)
{
    return boost::asynchronous::top_level_callback_continuation_job<typename std::iterator_traits<Iterator>::value_type,Job>
            (boost::asynchronous::geometry::detail::parallel_geometry_intersection_of_x_helper<Iterator,Job>
                (beg,end,cutoff,overlay_cutoff,partition_cutoff,task_name,prio));

}

namespace detail
{
template <class Range,class Return,class Job>
struct parallel_geometry_intersection_of_x_range_helper: public boost::asynchronous::continuation_task<Return>
{
    template <class Iterator>
    parallel_geometry_intersection_of_x_range_helper(boost::shared_ptr<Range> range,Iterator beg, Iterator end,
                            long cutoff,long overlay_cutoff,long partition_cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<Return>(task_name),
          range_(range),begin_(beg),end_(end),cutoff_(cutoff),overlay_cutoff_(overlay_cutoff),partition_cutoff_(partition_cutoff),prio_(prio)
    {}

    void operator()()
    {
        boost::asynchronous::continuation_result<Return> task_res = this->this_task_result();
        using Iterator = decltype(boost::begin(*range_));
        // advance up to cutoff
        auto it = boost::asynchronous::detail::find_cutoff(begin_,cutoff_,end_);
        if (it == end_)
        {
            Return final_output;
            bool first = true;
            for(auto it = begin_; it != end_; ++it)
            {
                if (first)
                {
                    final_output = *it;
                    first = false;
                }
                else
                {
                    typename std::iterator_traits<Iterator>::value_type result;
                    boost::geometry::intersection(final_output, *it, result);
                    final_output = std::move(result);
                }
            }
            task_res.set_value(std::move(final_output));
        }
        else
        {
            auto task_name = this->get_name();
            auto prio = prio_;
            auto overlay_cutoff = overlay_cutoff_;
            auto partition_cutoff = partition_cutoff_;

            boost::asynchronous::create_callback_continuation_job<Job>(
                        // called when subtasks are done, set our result
                        [task_res,task_name,prio,overlay_cutoff,partition_cutoff]
                        (std::tuple<boost::asynchronous::expected<Return>,boost::asynchronous::expected<Return> > res)
                        {
                            try
                            {
                                auto res0 = std::get<0>(res).get();
                                auto res1 = std::get<1>(res).get();
                                //Return result;
                                //boost::geometry::intersection(res0, res1, result);
                                //task_res.set_value(std::move(result));
                                auto cont = boost::asynchronous::geometry::parallel_intersection<Return,Return,Return,Job>
                                        (std::move(res0),std::move(res1),overlay_cutoff,partition_cutoff);
                                cont.on_done([task_res](std::tuple<boost::asynchronous::expected<Return> >&& res_p_intersect)
                                {
                                    try
                                    {
                                        task_res.set_value(std::move(std::get<0>(res_p_intersect).get()));
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
                        },
                        // recursive tasks
                        parallel_geometry_intersection_of_x_range_helper<Range,Return,Job>
                            (range_,begin_,it,cutoff_,overlay_cutoff_,partition_cutoff_,this->get_name(),prio_),
                        parallel_geometry_intersection_of_x_range_helper<Range,Return,Job>
                            (range_,it,end_,cutoff_,overlay_cutoff_,partition_cutoff_,this->get_name(),prio_)
            );
        }
    }

    boost::shared_ptr<Range> range_;
    decltype(boost::begin(*range_)) begin_;
    decltype(boost::end(*range_)) end_;
    long cutoff_;
    long overlay_cutoff_;
    long partition_cutoff_;
    std::size_t prio_;
};
}

template <class Range, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
auto
parallel_geometry_intersection_of_x(Range&& range,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio=0,
#else
                    const std::string& task_name="", std::size_t prio=0,
#endif
                    long cutoff=300, long overlay_cutoff=1500, long partition_cutoff=80000)
-> typename boost::disable_if<boost::asynchronous::detail::has_is_continuation_task<Range>,
                              boost::asynchronous::detail::callback_continuation<
                                typename std::remove_reference<decltype(*boost::begin(range))
                              >::type,Job>>::type
{
    auto r = boost::make_shared<Range>(std::forward<Range>(range));
    auto beg = boost::begin(*r);
    auto end = boost::end(*r);

    using Return = typename std::remove_reference<decltype(*boost::begin(range))>::type;

    return boost::asynchronous::top_level_callback_continuation_job<Return,Job>
            (boost::asynchronous::geometry::detail::parallel_geometry_intersection_of_x_range_helper
               <Range,Return,Job>
                (std::move(r),beg,end,cutoff,overlay_cutoff,partition_cutoff,task_name,prio));

}

// version for ranges given as continuation => will return the range as continuation
namespace detail
{
template <class Continuation, class Job,class Enable=void>
struct parallel_geometry_intersection_of_x_continuation_range_helper: public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_geometry_intersection_of_x_continuation_range_helper(Continuation c,long cutoff,long overlay_cutoff, long partition_cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<typename Continuation::return_type>(task_name)
        ,cont_(std::move(c)),cutoff_(cutoff),overlay_cutoff_(overlay_cutoff),partition_cutoff_(partition_cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<typename Continuation::return_type> task_res = this->this_task_result();
        auto cutoff = cutoff_;
        auto overlay_cutoff = overlay_cutoff_;
        auto partition_cutoff = partition_cutoff_;
        auto task_name = this->get_name();
        auto prio = prio_;
        cont_.on_done([task_res,cutoff,overlay_cutoff,partition_cutoff,task_name,prio]
                      (std::tuple<boost::future<typename Continuation::return_type> >&& continuation_res)
        {
            try
            {
                auto new_continuation = boost::asynchronous::geometry::parallel_geometry_intersection_of_x<typename Continuation::return_type,Job>
                        (std::move(std::get<0>(continuation_res).get()),task_name,prio,cutoff,overlay_cutoff,partition_cutoff);
                new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res)
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
    Continuation cont_;
    long cutoff_;
    long overlay_cutoff_;
    long partition_cutoff_;
    std::size_t prio_;
};
// Continuation is a callback continuation
template <class Continuation, class Job>
struct parallel_geometry_intersection_of_x_continuation_range_helper<Continuation,Job,
                                                                     typename ::boost::enable_if< boost::asynchronous::detail::has_is_callback_continuation_task<Continuation> >::type>
        : public boost::asynchronous::continuation_task<typename Continuation::return_type>
{
    parallel_geometry_intersection_of_x_continuation_range_helper(Continuation c,long cutoff,long overlay_cutoff, long partition_cutoff,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<typename Continuation::return_type>(task_name)
        ,cont_(std::move(c)),cutoff_(cutoff),overlay_cutoff_(overlay_cutoff),partition_cutoff_(partition_cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<typename Continuation::return_type> task_res = this->this_task_result();
        auto cutoff = cutoff_;
        auto overlay_cutoff = overlay_cutoff_;
        auto partition_cutoff = partition_cutoff_;
        auto task_name = this->get_name();
        auto prio = prio_;
        cont_.on_done([task_res,cutoff,overlay_cutoff,partition_cutoff,task_name,prio]
                      (std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& continuation_res)
        {
            try
            {
                auto new_continuation = boost::asynchronous::geometry::parallel_geometry_intersection_of_x<typename Continuation::return_type,Job>
                        (std::move(std::get<0>(continuation_res).get()),task_name,prio,cutoff,overlay_cutoff,partition_cutoff);
                new_continuation.on_done([task_res](std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& new_continuation_res)
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
    long cutoff_;
    long overlay_cutoff_;
    long partition_cutoff_;
    std::size_t prio_;
};
}
// version for ranges given as continuation => will return the range as continuation
template <class Range, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename boost::enable_if<boost::asynchronous::detail::has_is_continuation_task<Range>,boost::asynchronous::detail::callback_continuation<typename Range::return_type,Job> >::type
parallel_geometry_intersection_of_x(Range range,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                     const std::string& task_name, std::size_t prio=0,
#else
                     const std::string& task_name="", std::size_t prio=0,
                     long cutoff=300, long overlay_cutoff=1500, long partition_cutoff=80000)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<typename Range::return_type,Job>
            (boost::asynchronous::geometry::detail::parallel_geometry_intersection_of_x_continuation_range_helper<Range,Job>(
                 std::move(range),cutoff,overlay_cutoff,partition_cutoff,task_name,prio));
}

}}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_GEOMETRY_INTERSECTION_HPP
