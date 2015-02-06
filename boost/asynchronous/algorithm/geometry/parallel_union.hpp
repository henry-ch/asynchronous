// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2014.
// Modifications copyright (c) 2014 Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_PARALLEL_UNION_HPP
#define BOOST_GEOMETRY_ALGORITHMS_PARALLEL_UNION_HPP


#include <boost/range/metafunctions.hpp>

#include <boost/geometry/core/is_areal.hpp>
#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/core/reverse_dispatch.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/algorithms/not_implemented.hpp>
#include <boost/asynchronous/algorithm/geometry/detail/overlay.hpp>
#include <boost/geometry/policies/robustness/get_rescale_policy.hpp>

#include <boost/geometry/algorithms/detail/overlay/linear_linear.hpp>
#include <boost/geometry/algorithms/detail/overlay/pointlike_pointlike.hpp>

#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>

namespace boost { namespace asynchronous { namespace geometry
{

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Job,
    typename Geometry1, typename Geometry2, typename GeometryOut,
    typename TagIn1 = typename boost::geometry::tag<Geometry1>::type,
    typename TagIn2 = typename boost::geometry::tag<Geometry2>::type,
    typename TagOut = typename boost::geometry::tag<GeometryOut>::type,
    bool Areal1 = boost::geometry::is_areal<Geometry1>::value,
    bool Areal2 = boost::geometry::is_areal<Geometry2>::value,
    bool ArealOut = boost::geometry::is_areal<GeometryOut>::value,
    bool Reverse1 = boost::geometry::detail::overlay::do_reverse<boost::geometry::point_order<Geometry1>::value>::value,
    bool Reverse2 = boost::geometry::detail::overlay::do_reverse<boost::geometry::point_order<Geometry2>::value>::value,
    bool ReverseOut = boost::geometry::detail::overlay::do_reverse<boost::geometry::point_order<GeometryOut>::value>::value,
    bool Reverse = boost::geometry::reverse_dispatch<Geometry1, Geometry2>::type::value
>
struct union_insert: boost::geometry::not_implemented<TagIn1, TagIn2, TagOut>
{};


// If reversal is needed, perform it first

template
< 
    typename Job,
    typename Geometry1, typename Geometry2, typename GeometryOut,
    typename TagIn1, typename TagIn2, typename TagOut,
    bool Areal1, bool Areal2, bool ArealOut,
    bool Reverse1, bool Reverse2, bool ReverseOut
>
struct union_insert
    <
        Job,Geometry1, Geometry2, GeometryOut,
        TagIn1, TagIn2, TagOut,
        Areal1, Areal2, ArealOut,
        Reverse1, Reverse2, ReverseOut,
        true
    >: union_insert<Job,Geometry2, Geometry1, GeometryOut>
{
    template <typename TaskRes,typename RobustPolicy, typename Strategy>
    static inline void apply(TaskRes task_res,
                             Geometry1& g1,
                             Geometry2& g2,
                             RobustPolicy& robust_policy,
                             Strategy const& strategy,
                             long)
    {
        typename TaskRes::return_type output_collection;
        union_insert
            <
                Job,Geometry2, Geometry1, GeometryOut
            >::apply(task_res,g2, g1, robust_policy, std::back_inserter(output_collection), strategy);
        task_res.set_value(std::move(output_collection));
    }
};


template
<
    typename Job,
    typename Geometry1, typename Geometry2, typename GeometryOut,
    typename TagIn1, typename TagIn2, typename TagOut,
    bool Reverse1, bool Reverse2, bool ReverseOut
>
struct union_insert
    <
        Job,Geometry1, Geometry2, GeometryOut,
        TagIn1, TagIn2, TagOut,
        true, true, true,
        Reverse1, Reverse2, ReverseOut,
        false
    > : boost::geometry::detail::overlay::parallel_overlay
        <Job,Geometry1, Geometry2, Reverse1, Reverse2, ReverseOut, GeometryOut, boost::geometry::overlay_union>
{
    template <typename TaskRes,typename RobustPolicy, typename Strategy>
    static inline void apply(TaskRes task_res,
                             Geometry1& g1,
                             Geometry2& g2,
                             RobustPolicy& robust_policy,
                             Strategy const& strategy,
                             long cutoff)
    {
        // TODO we will start with this one for parallelization
        //typename TaskRes::return_type output_collection;
        boost::geometry::detail::overlay::parallel_overlay
            <
                Job,Geometry1, Geometry2, Reverse1, Reverse2, ReverseOut,GeometryOut, boost::geometry::overlay_union
            >::apply(std::move(task_res),g1, g2, robust_policy, strategy,cutoff);
        //task_res.set_value(std::move(output_collection));
    }
};


// dispatch for union of non-areal geometries
template
<
    typename Job,
    typename Geometry1, typename Geometry2, typename GeometryOut,
    typename TagIn1, typename TagIn2, typename TagOut,
    bool Reverse1, bool Reverse2, bool ReverseOut
>
struct union_insert
    <
        Job,Geometry1, Geometry2, GeometryOut,
        TagIn1, TagIn2, TagOut,
        false, false, false,
        Reverse1, Reverse2, ReverseOut,
        false
    > : union_insert
        <
            Job,Geometry1, Geometry2, GeometryOut,
            typename boost::geometry::tag_cast<TagIn1, boost::geometry::pointlike_tag, boost::geometry::linear_tag>::type,
            typename boost::geometry::tag_cast<TagIn2, boost::geometry::pointlike_tag, boost::geometry::linear_tag>::type,
            TagOut,
            false, false, false,
            Reverse1, Reverse2, ReverseOut,
            false
        >
{
    template <typename TaskRes,typename RobustPolicy, typename Strategy>
    static inline void apply(TaskRes task_res,
                             Geometry1& g1,
                             Geometry2& g2,
                             RobustPolicy& robust_policy,
                             Strategy const& strategy,
                             long)
    {
        typename TaskRes::return_type output_collection;
        union_insert
                <
                    Job,Geometry1, Geometry2, GeometryOut,
                    typename boost::geometry::tag_cast<TagIn1, boost::geometry::pointlike_tag, boost::geometry::linear_tag>::type,
                    typename boost::geometry::tag_cast<TagIn2, boost::geometry::pointlike_tag, boost::geometry::linear_tag>::type,
                    TagOut,
                    false, false, false,
                    Reverse1, Reverse2, ReverseOut,
                    false
                >::apply(g1, g2, robust_policy, std::back_inserter(output_collection), strategy);
        task_res.set_value(std::move(output_collection));
    }
};


// dispatch for union of linear geometries
template
<
    typename Job,
    typename Linear1, typename Linear2, typename LineStringOut,
    bool Reverse1, bool Reverse2, bool ReverseOut
>
struct union_insert
    <
        Job,Linear1, Linear2, LineStringOut,
        boost::geometry::linear_tag, boost::geometry::linear_tag, boost::geometry::linestring_tag,
        false, false, false,
        Reverse1, Reverse2, ReverseOut,
        false
    > : boost::geometry::detail::overlay::linear_linear_linestring
        <
            Linear1, Linear2, LineStringOut, boost::geometry::overlay_union
        >
{
    template <typename TaskRes,typename RobustPolicy, typename Strategy>
    static inline void apply(TaskRes task_res,
                             Linear1& g1,
                             Linear2& g2,
                             RobustPolicy& robust_policy,
                             Strategy const& strategy,
                             long)
    {
        typename TaskRes::return_type output_collection;
        boost::geometry::detail::overlay::linear_linear_linestring
                <
                    Linear1, Linear2, LineStringOut, boost::geometry::overlay_union
                >::apply(g1, g2, robust_policy, std::back_inserter(output_collection), strategy);
        task_res.set_value(std::move(output_collection));
    }
};


// dispatch for point-like geometries
template
<
    typename Job,
    typename PointLike1, typename PointLike2, typename PointOut,
    bool Reverse1, bool Reverse2, bool ReverseOut
>
struct union_insert
    <
        Job,PointLike1, PointLike2, PointOut,
        boost::geometry::pointlike_tag, boost::geometry::pointlike_tag, boost::geometry::point_tag,
        false, false, false,
        Reverse1, Reverse2, ReverseOut,
        false
    > : boost::geometry::detail::overlay::union_pointlike_pointlike_point
        <
            PointLike1, PointLike2, PointOut
        >
{
    template <typename TaskRes,typename RobustPolicy, typename Strategy>
    static inline void apply(TaskRes task_res,
                             PointLike1& g1,
                             PointLike2& g2,
                             RobustPolicy& robust_policy,
                             Strategy const& strategy,
                             long)
    {
        typename TaskRes::return_type output_collection;
        boost::geometry::detail::overlay::union_pointlike_pointlike_point
                <
                    PointLike1, PointLike2, PointOut
                >::apply(g1, g2, robust_policy, std::back_inserter(output_collection), strategy);
        task_res.set_value(std::move(output_collection));
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace union_
{

/*!
\brief_calc2{union}
\ingroup union
\details \details_calc2{union_insert, spatial set theoretic union}.
    \details_insert{union}
\tparam GeometryOut output geometry type, must be specified
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param out \param_out{union}
\return \void
*/
template
<    
    typename GeometryOut,
    typename Job,
    typename Geometry1,
    typename Geometry2,
    typename TaskRes
>
inline void union_insert(TaskRes task_res,
                         Geometry1& geometry1,
                         Geometry2& geometry2,
                         long cutoff)
{
    boost::geometry::concept::check<Geometry1 const>();
    boost::geometry::concept::check<Geometry2 const>();
    boost::geometry::concept::check<GeometryOut>();

    typedef typename boost::geometry::rescale_overlay_policy_type
        <
            Geometry1,
            Geometry2
        >::type rescale_policy_type;

    typedef boost::geometry::strategy_intersection
        <
            typename boost::geometry::cs_tag<GeometryOut>::type,
            Geometry1,
            Geometry2,
            typename boost::geometry::point_type<GeometryOut>::type,
            rescale_policy_type
        > strategy;

    rescale_policy_type robust_policy
            = boost::geometry::get_rescale_policy<rescale_policy_type>(geometry1, geometry2);

    dispatch::union_insert
           <
               Job,Geometry1, Geometry2, GeometryOut
           >::apply(task_res,geometry1, geometry2, robust_policy, strategy(),cutoff);
}


}} // namespace detail::union_
#endif // DOXYGEN_NO_DETAIL




/*!
\brief Combines two geometries which each other
\ingroup union
\details \details_calc2{union, spatial set theoretic union}.
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\tparam Collection output collection, either a multi-geometry,
    or a std::vector<Geometry> / std::deque<Geometry> etc
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param output_collection the output collection
\note Called union_ because union is a reserved word.

\qbk{[include reference/algorithms/union.qbk]}
*/
template
<
    typename TaskRes,
    typename Geometry1,
    typename Geometry2,
    typename Collection,
    typename Job
>
inline void union_(TaskRes task_res,
                   Geometry1& geometry1,
                   Geometry2& geometry2,
                   long cutoff)
{
    boost::geometry::concept::check<Geometry1 const>();
    boost::geometry::concept::check<Geometry2 const>();

    typedef typename boost::range_value<Collection>::type geometry_out;
    boost::geometry::concept::check<geometry_out>();

    detail::union_::union_insert<geometry_out,Job>(task_res,geometry1, geometry2,cutoff);
}

namespace detail
{
template <class Geometry1, class Geometry2, class Collection,class Job>
struct parallel_union_helper: public boost::asynchronous::continuation_task<Collection>
{
    parallel_union_helper(Geometry1 geometry1,Geometry2 geometry2,
                          long cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<Collection>(task_name),
          geometry1_(geometry1),geometry2_(geometry2),cutoff_(cutoff),prio_(prio)
    {}

    void operator()()
    {
        boost::asynchronous::continuation_result<Collection> task_res = this->this_task_result();
        boost::asynchronous::geometry::union_<boost::asynchronous::continuation_result<Collection>,Geometry1,Geometry2,Collection,Job>
                (task_res,geometry1_,geometry2_,cutoff_);
    }
    Geometry1 geometry1_;
    Geometry2 geometry2_;
    long cutoff_;
    std::size_t prio_;
};
}

template <class Geometry1, class Geometry2, class Collection, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<Collection,Job>
parallel_union(Geometry1 geometry1,
               Geometry2 geometry2,
               long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
               const std::string& task_name, std::size_t prio)
#else
               const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<Collection,Job>
            (boost::asynchronous::geometry::detail::parallel_union_helper<Geometry1,Geometry2,Collection,Job>
                (std::move(geometry1),std::move(geometry2),cutoff,task_name,prio));

}

}}} // namespace boost::asynchronous::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_PARALLEL_UNION_HPP
