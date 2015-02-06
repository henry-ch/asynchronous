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

#include <boost/geometry/algorithms/area.hpp>
#include <boost/geometry/algorithms/within.hpp>
#include <boost/geometry/algorithms/detail/interior_iterator.hpp>
#include <boost/geometry/algorithms/detail/point_on_border.hpp>
#include <boost/geometry/algorithms/detail/ring_identifier.hpp>
#include <boost/geometry/algorithms/detail/overlay/ring_properties.hpp>
#include <boost/geometry/algorithms/detail/overlay/overlay_type.hpp>

#include <boost/asynchronous/algorithm/parallel_for_each.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{

/*
namespace dispatch
{

    template <typename Tag, typename Geometry>
    struct select_rings
    {};

    template <typename Box>
    struct select_rings<box_tag, Box>
    {
        template <typename Geometry, typename Map>
        static inline void apply(Box const& box, Geometry const& ,
                ring_identifier const& id, Map& map, bool midpoint)
        {
            map[id] = typename Map::mapped_type(box, midpoint);
        }

        template <typename Map>
        static inline void apply(Box const& box,
                ring_identifier const& id, Map& map, bool midpoint)
        {
            map[id] = typename Map::mapped_type(box, midpoint);
        }
    };

    template <typename Ring>
    struct select_rings<ring_tag, Ring>
    {
        template <typename Geometry, typename Map>
        static inline void apply(Ring const& ring, Geometry const& ,
                    ring_identifier const& id, Map& map, bool midpoint)
        {
            if (boost::size(ring) > 0)
            {
                map[id] = typename Map::mapped_type(ring, midpoint);
            }
        }

        template <typename Map>
        static inline void apply(Ring const& ring,
                    ring_identifier const& id, Map& map, bool midpoint)
        {
            if (boost::size(ring) > 0)
            {
                map[id] = typename Map::mapped_type(ring, midpoint);
            }
        }
    };


    template <typename Polygon>
    struct select_rings<polygon_tag, Polygon>
    {
        template <typename Geometry, typename Map>
        static inline void apply(Polygon const& polygon, Geometry const& geometry,
                    ring_identifier id, Map& map, bool midpoint)
        {
            typedef typename geometry::ring_type<Polygon>::type ring_type;
            typedef select_rings<ring_tag, ring_type> per_ring;

            per_ring::apply(exterior_ring(polygon), geometry, id, map, midpoint);

            typename interior_return_type<Polygon const>::type
                rings = interior_rings(polygon);
            for (typename detail::interior_iterator<Polygon const>::type
                    it = boost::begin(rings); it != boost::end(rings); ++it)
            {
                id.ring_index++;
                per_ring::apply(*it, geometry, id, map, midpoint);
            }
        }

        template <typename Map>
        static inline void apply(Polygon const& polygon,
                ring_identifier id, Map& map, bool midpoint)
        {
            typedef typename geometry::ring_type<Polygon>::type ring_type;
            typedef select_rings<ring_tag, ring_type> per_ring;

            per_ring::apply(exterior_ring(polygon), id, map, midpoint);

            typename interior_return_type<Polygon const>::type
                rings = interior_rings(polygon);
            for (typename detail::interior_iterator<Polygon const>::type
                    it = boost::begin(rings); it != boost::end(rings); ++it)
            {
                id.ring_index++;
                per_ring::apply(*it, id, map, midpoint);
            }
        }
    };

    template <typename Multi>
    struct select_rings<multi_polygon_tag, Multi>
    {
        template <typename Geometry, typename Map>
        static inline void apply(Multi const& multi, Geometry const& geometry,
                    ring_identifier id, Map& map, bool midpoint)
        {
            typedef typename boost::range_iterator
                <
                    Multi const
                >::type iterator_type;

            typedef select_rings<polygon_tag, typename boost::range_value<Multi>::type> per_polygon;

            id.multi_index = 0;
            for (iterator_type it = boost::begin(multi); it != boost::end(multi); ++it)
            {
                id.ring_index = -1;
                per_polygon::apply(*it, geometry, id, map, midpoint);
                id.multi_index++;
            }
        }
    };

} // namespace dispatch

template<overlay_type OverlayType>
struct decide
{};

template<>
struct decide<overlay_union>
{
    template <typename Code>
    static bool include(ring_identifier const& , Code const& code)
    {
        return code.within_code * -1 == 1;
    }

    template <typename Code>
    static bool reversed(ring_identifier const& , Code const& )
    {
        return false;
    }
};

template<>
struct decide<overlay_difference>
{
    template <typename Code>
    static bool include(ring_identifier const& id, Code const& code)
    {
        bool is_first = id.source_index == 0;
        return code.within_code * -1 * (is_first ? 1 : -1) == 1;
    }

    template <typename Code>
    static bool reversed(ring_identifier const& id, Code const& code)
    {
        return include(id, code) && id.source_index == 1;
    }
};

template<>
struct decide<overlay_intersection>
{
    template <typename Code>
    static bool include(ring_identifier const& , Code const& code)
    {
        return code.within_code * 1 == 1;
    }

    template <typename Code>
    static bool reversed(ring_identifier const& , Code const& )
    {
        return false;
    }
};
*/

template
<
    overlay_type OverlayType,
    typename Fct,
    typename Job,
    typename Geometry1, typename Geometry2,
    typename IntersectionMap, typename SelectionMap
>
inline boost::asynchronous::detail::callback_continuation<Fct,Job>
update_selection_map(Geometry1& geometry1,
                     Geometry2& geometry2,
                     IntersectionMap& intersection_map,
                     SelectionMap& map_with_all, long cutoff)
{
    typedef decltype(boost::begin(map_with_all)) Iterator;

    boost::shared_ptr<SelectionMap> pmap_with_all(boost::make_shared<SelectionMap>(std::move(map_with_all)));
    return boost::asynchronous::parallel_for_each<Iterator,Fct,Job>
            (boost::begin(*pmap_with_all),boost::end(*pmap_with_all),
             Fct(geometry1,geometry2,intersection_map,pmap_with_all),
             cutoff);
}


/*!
\brief The function select_rings select rings based on the overlay-type (union,intersection)
*/
template
<
    overlay_type OverlayType,
    typename SelectionMap,
    typename Fct,
    typename Job,
    typename Geometry1, typename Geometry2,
    typename IntersectionMap
>
inline boost::asynchronous::detail::callback_continuation<Fct,Job>
pselect_rings(Geometry1& geometry1, Geometry2& geometry2,
             IntersectionMap& intersection_map,
             bool midpoint, long cutoff)
{
    typedef typename geometry::tag<Geometry1>::type tag1;
    typedef typename geometry::tag<Geometry2>::type tag2;

    SelectionMap map_with_all;
    dispatch::select_rings<tag1, Geometry1>::apply(geometry1, geometry2,
                ring_identifier(0, -1, -1), map_with_all, midpoint);
    dispatch::select_rings<tag2, Geometry2>::apply(geometry2, geometry1,
                ring_identifier(1, -1, -1), map_with_all, midpoint);

    return update_selection_map<OverlayType,Fct,Job>(geometry1, geometry2, intersection_map,map_with_all,cutoff);
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
pselect_rings(Geometry& geometry,
             IntersectionMap& intersection_map,
             bool midpoint, long cutoff)
{
    typedef typename geometry::tag<Geometry>::type tag;

    SelectionMap map_with_all;
    dispatch::select_rings<tag, Geometry>::apply(geometry,
                ring_identifier(0, -1, -1), map_with_all, midpoint);

    return update_selection_map<OverlayType,Fct,Job>(geometry, geometry, intersection_map,map_with_all,cutoff);
}


}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_ASYNCHRONOUS_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_SELECT_RINGS_HPP
