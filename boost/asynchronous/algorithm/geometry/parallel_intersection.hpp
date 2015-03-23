// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2014.
// Modifications copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ASYNCHRONOUS_GEOMETRY_ALGORITHMS_DETAIL_INTERSECTION_INTERFACE_HPP
#define BOOST_ASYNCHRONOUS_GEOMETRY_ALGORITHMS_DETAIL_INTERSECTION_INTERFACE_HPP

#include <boost/geometry/policies/robustness/no_rescale_policy.hpp>
#include <boost/geometry/policies/robustness/rescale_policy.hpp>

#include <boost/geometry/algorithms/detail/intersection/interface.hpp>
#include <boost/asynchronous/algorithm/geometry/detail/intersection_insert.hpp>


namespace boost { namespace asynchronous { namespace geometry
{


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

// By default, all is forwarded to the intersection_insert-dispatcher
template
<
    typename Job,
    typename Geometry1, typename Geometry2,typename GeometryOut,
    typename Tag1 = typename boost::geometry::tag<Geometry1>::type,
    typename Tag2 = typename boost::geometry::tag<Geometry2>::type,
    bool Reverse = boost::geometry::reverse_dispatch<Geometry1, Geometry2>::type::value
>
struct intersection
{
    template <typename TaskRes,typename RobustPolicy, typename Strategy>
    static inline void apply(TaskRes task_res,
                             Geometry1 const& geometry1,
                             Geometry2 const& geometry2,
                             RobustPolicy const& robust_policy,
                             Strategy const& strategy,
                             long overlay_cutoff,
                             long partition_cutoff)
    {
        typedef typename boost::range_value<GeometryOut>::type OneOut;
/*        typename TaskRes::return_type geometry_out;


        boost::geometry::dispatch::parallel_intersection_insert
        <
            Job,
            Geometry1, Geometry2, OneOut,
            boost::geometry::overlay_intersection
        >::apply(geometry1, geometry2, robust_policy, std::back_inserter(geometry_out), strategy);

        task_res.set_value(std::move(geometry_out));*/

        boost::geometry::dispatch::parallel_intersection_insert
        <
            Job,
            Geometry1, Geometry2, OneOut,
            boost::geometry::overlay_intersection
        >::apply(task_res,geometry1, geometry2, robust_policy, strategy,overlay_cutoff,partition_cutoff);

    }

};


// If reversal is needed, perform it
/*template
<
    typename Geometry1, typename Geometry2,
    typename Tag1, typename Tag2
>
struct intersection
<
    Geometry1, Geometry2,
    Tag1, Tag2,
    true
>
    : intersection<Geometry2, Geometry1, Tag2, Tag1, false>
{
    template <typename RobustPolicy, typename GeometryOut, typename Strategy>
    static inline bool apply(
        Geometry1 const& g1,
        Geometry2 const& g2,
        RobustPolicy const& robust_policy,
        GeometryOut& out,
        Strategy const& strategy)
    {
        return intersection<
                   Geometry2, Geometry1,
                   Tag2, Tag1,
                   false
               >::apply(g2, g1, robust_policy, out, strategy);
    }
};*/


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

    
namespace resolve_variant
{

template <typename Job, typename Geometry1, typename Geometry2>
struct intersection
{
    template <typename TaskRes,typename GeometryOut>
    static inline void
    apply(
          TaskRes task_res,
          const Geometry1& geometry1,
          const Geometry2& geometry2,
          long overlay_cutoff,
          long partition_cutoff)
    {
        boost::geometry::concept::check<Geometry1 const>();
        boost::geometry::concept::check<Geometry2 const>();

        typedef typename boost::geometry::rescale_overlay_policy_type
        <
            Geometry1,
            Geometry2
        >::type rescale_policy_type;

        rescale_policy_type robust_policy
        = boost::geometry::get_rescale_policy<rescale_policy_type>(geometry1, geometry2);

        typedef boost::geometry::strategy_intersection
        <
            typename boost::geometry::cs_tag<Geometry1>::type,
            Geometry1,
            Geometry2,
            typename boost::geometry::point_type<Geometry1>::type,
            rescale_policy_type
        > strategy;
        
        dispatch::intersection
        <
            Job,
            Geometry1,
            Geometry2,
            GeometryOut
        >::apply(task_res,geometry1, geometry2, robust_policy, strategy(),overlay_cutoff,partition_cutoff);
    }
};

/*
template <BOOST_VARIANT_ENUM_PARAMS(typename T), typename Geometry2>
struct intersection<variant<BOOST_VARIANT_ENUM_PARAMS(T)>, Geometry2>
{
    template <typename GeometryOut>
    struct visitor: static_visitor<bool>
    {
        Geometry2 const& m_geometry2;
        GeometryOut& m_geometry_out;
        
        visitor(Geometry2 const& geometry2,
                GeometryOut& geometry_out)
        : m_geometry2(geometry2),
        m_geometry_out(geometry_out)
        {}
        
        template <typename Geometry1>
        result_type operator()(Geometry1 const& geometry1) const
        {
            return intersection
            <
                Geometry1,
                Geometry2
            >::template apply
            <
                GeometryOut
            >
            (geometry1, m_geometry2, m_geometry_out);
        }
    };
    
    template <typename GeometryOut>
    static inline bool
    apply(variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry1,
          Geometry2 const& geometry2,
          GeometryOut& geometry_out)
    {
        return apply_visitor(visitor<GeometryOut>(geometry2, geometry_out), geometry1);
    }
};


template <typename Geometry1, BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct intersection<Geometry1, variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
{
    template <typename GeometryOut>
    struct visitor: static_visitor<bool>
    {
        Geometry1 const& m_geometry1;
        GeometryOut& m_geometry_out;
        
        visitor(Geometry1 const& geometry1,
                GeometryOut& geometry_out)
        : m_geometry1(geometry1),
          m_geometry_out(geometry_out)
        {}
        
        template <typename Geometry2>
        result_type operator()(Geometry2 const& geometry2) const
        {
            return intersection
            <
                Geometry1,
                Geometry2
            >::template apply
            <
                GeometryOut
            >
            (m_geometry1, geometry2, m_geometry_out);
        }
    };
    
    template <typename GeometryOut>
    static inline bool
    apply(
          Geometry1 const& geometry1,
          const variant<BOOST_VARIANT_ENUM_PARAMS(T)>& geometry2,
          GeometryOut& geometry_out)
    {
        return apply_visitor(visitor<GeometryOut>(geometry1, geometry_out), geometry2);
    }
};


template <BOOST_VARIANT_ENUM_PARAMS(typename T1), BOOST_VARIANT_ENUM_PARAMS(typename T2)>
struct intersection<variant<BOOST_VARIANT_ENUM_PARAMS(T1)>, variant<BOOST_VARIANT_ENUM_PARAMS(T2)> >
{
    template <typename GeometryOut>
    struct visitor: static_visitor<bool>
    {
        GeometryOut& m_geometry_out;
        
        visitor(GeometryOut& geometry_out)
        : m_geometry_out(geometry_out)
        {}
        
        template <typename Geometry1, typename Geometry2>
        result_type operator()(
                               Geometry1 const& geometry1,
                               Geometry2 const& geometry2) const
        {
            return intersection
            <
                Geometry1,
                Geometry2
            >::template apply
            <
                GeometryOut
            >
            (geometry1, geometry2, m_geometry_out);
        }
    };
    
    template <typename GeometryOut>
    static inline bool
    apply(
          const variant<BOOST_VARIANT_ENUM_PARAMS(T1)>& geometry1,
          const variant<BOOST_VARIANT_ENUM_PARAMS(T2)>& geometry2,
          GeometryOut& geometry_out)
    {
        return apply_visitor(visitor<GeometryOut>(geometry_out), geometry1, geometry2);
    }
};*/
    
} // namespace resolve_variant
    

/*!
\brief \brief_calc2{intersection}
\ingroup intersection
\details \details_calc2{intersection, spatial set theoretic intersection}.
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\tparam GeometryOut Collection of geometries (e.g. std::vector, std::deque, boost::geometry::multi*) of which
    the value_type fulfills a \p_l_or_c concept, or it is the output geometry (e.g. for a box)
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param geometry_out The output geometry, either a multi_point, multi_polygon,
    multi_linestring, or a box (for intersection of two boxes)

\qbk{[include reference/algorithms/intersection.qbk]}
*/
template
<
    typename TaskRes,
    typename Geometry1,
    typename Geometry2,
    typename GeometryOut,
    typename Job
>
inline void intersection(TaskRes task_res,
                         Geometry1 const& geometry1,
                         Geometry2 const& geometry2,
                         long overlay_cutoff,
                         long partition_cutoff)
{
    resolve_variant::intersection
        <
           Job,
           Geometry1,
           Geometry2
        >::template apply
        <
            TaskRes,GeometryOut
        >
        (task_res,geometry1, geometry2,overlay_cutoff,partition_cutoff);
}

namespace detail
{
template <class Geometry1, class Geometry2, class GeometryOut,class Job>
struct parallel_intersection_helper: public boost::asynchronous::continuation_task<GeometryOut>
{
    parallel_intersection_helper(Geometry1 geometry1,Geometry2 geometry2,
                          long overlay_cutoff,long partition_cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<GeometryOut>(task_name),
          geometry1_(geometry1),geometry2_(geometry2),overlay_cutoff_(overlay_cutoff),partition_cutoff_(partition_cutoff),prio_(prio)
    {}

    void operator()()
    {
        boost::asynchronous::continuation_result<GeometryOut> task_res = this->this_task_result();
        boost::asynchronous::geometry::intersection<boost::asynchronous::continuation_result<GeometryOut>,
                                                    Geometry1,Geometry2,GeometryOut,Job>
                (task_res,geometry1_,geometry2_,overlay_cutoff_,partition_cutoff_);
    }
    Geometry1 geometry1_;
    Geometry2 geometry2_;
    long overlay_cutoff_;
    long partition_cutoff_;
    std::size_t prio_;
};
}

template <class Geometry1, class Geometry2, class GeometryOut, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<GeometryOut,Job>
parallel_intersection(Geometry1 geometry1,
               Geometry2 geometry2,
               long overlay_cutoff,
               long partition_cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
               const std::string& task_name, std::size_t prio)
#else
               const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<GeometryOut,Job>
            (boost::asynchronous::geometry::detail::parallel_intersection_helper<Geometry1,Geometry2,GeometryOut,Job>
                (std::move(geometry1),std::move(geometry2),overlay_cutoff,partition_cutoff,task_name,prio));

}

}}} // namespace boost::asynchronous::geometry


#endif // BOOST_ASYNCHRONOUS_GEOMETRY_ALGORITHMS_DETAIL_INTERSECTION_INTERFACE_HPP
