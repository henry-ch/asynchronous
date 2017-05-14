// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2014 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ASYNCHRONOUS_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_SELECT_RINGS_HPP
#define BOOST_ASYNCHRONOUS_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_SELECT_RINGS_HPP


#include <map>

#include <boost/range.hpp>

#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/algorithms/detail/overlay/select_rings.hpp>

#include <boost/asynchronous/algorithm/parallel_for_each.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{
template
<
    overlay_type OverlayType,
    typename Fct,
    typename Job,
    typename Geometry1, typename Geometry2,
    typename RingTurnInfoMap,
    typename RingPropertyMap
>
inline boost::asynchronous::detail::callback_continuation<Fct,Job>
update_selection_map(Geometry1 geometry1,
                     Geometry2 geometry2,
                     RingTurnInfoMap turn_info_per_ring,
                     std::shared_ptr<RingPropertyMap> map_with_all,
                     long cutoff)
{
    typedef decltype(boost::begin(*map_with_all)) Iterator;
    //TODO name + prio
    return boost::asynchronous::parallel_for_each<Iterator,Fct,Job>
            (boost::begin(*map_with_all),boost::end(*map_with_all),
             Fct(std::move(geometry1),std::move(geometry2),std::move(turn_info_per_ring),map_with_all),
             cutoff,"geometry::update_selection_map",0);
}


/*!
\brief The function select_rings select rings based on the overlay-type (union,intersection)
*/
template
<
    overlay_type OverlayType,
    typename RingPropertyMap,
    typename Fct,
    typename Job,
    typename Geometry1, 
    typename Geometry2,
    typename RingTurnInfoMap
>
inline boost::asynchronous::detail::callback_continuation<Fct,Job>
parallel_select_rings(Geometry1 geometry1, Geometry2 geometry2,
                      RingTurnInfoMap turn_info_per_ring,
                      long cutoff)
{
    typedef typename geometry::tag<Geometry1>::type tag1;
    typedef typename geometry::tag<Geometry2>::type tag2;
    std::shared_ptr<RingPropertyMap> map_with_all(std::make_shared<RingPropertyMap>());
    dispatch::select_rings<tag1, Geometry1>::apply(geometry1, geometry2,
                ring_identifier(0, -1, -1), *map_with_all);
    dispatch::select_rings<tag2, Geometry2>::apply(geometry2, geometry1,
                ring_identifier(1, -1, -1), *map_with_all);
    return update_selection_map<OverlayType,Fct,Job>(std::move(geometry1), std::move(geometry2), std::move(turn_info_per_ring),map_with_all,cutoff);
}

template
<
    overlay_type OverlayType,
    typename SelectionMap,
    typename Fct,
    typename Job,
    typename Geometry,
    typename IntersectionMap
>
inline boost::asynchronous::detail::callback_continuation<Fct,Job>
parallel_select_rings(Geometry& geometry,
             IntersectionMap intersection_map,
             bool midpoint, long cutoff)
{
    typedef typename geometry::tag<Geometry>::type tag;

    std::shared_ptr<SelectionMap> map_with_all(std::make_shared<SelectionMap>());
    dispatch::select_rings<tag, Geometry>::apply(geometry,
                ring_identifier(0, -1, -1), *map_with_all, midpoint);

    return update_selection_map<OverlayType,Fct,Job>(geometry, geometry, std::move(intersection_map),map_with_all,cutoff);
}


}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_ASYNCHRONOUS_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_SELECT_RINGS_HPP
