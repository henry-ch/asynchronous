// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2013 Adam Wulkiewicz, Lodz, Poland

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ASYNCHRONOUS_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_OVERLAY_HPP
#define BOOST_ASYNCHRONOUS_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_OVERLAY_HPP


#include <deque>
#include <map>

#include <boost/range.hpp>
#include <boost/mpl/assert.hpp>


#include <boost/geometry/algorithms/detail/overlay/enrich_intersection_points.hpp>
#include <boost/geometry/algorithms/detail/overlay/enrichment_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turns.hpp>
#include <boost/geometry/algorithms/detail/overlay/overlay_type.hpp>
#include <boost/geometry/algorithms/detail/overlay/traverse.hpp>
#include <boost/geometry/algorithms/detail/overlay/traversal_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>

#include <boost/geometry/algorithms/detail/recalculate.hpp>

#include <boost/geometry/algorithms/num_points.hpp>
#include <boost/geometry/algorithms/reverse.hpp>

#include <boost/geometry/algorithms/detail/overlay/add_rings.hpp>
#include <boost/geometry/algorithms/detail/overlay/assign_parents.hpp>
#include <boost/geometry/algorithms/detail/overlay/ring_properties.hpp>
#include <boost/asynchronous/algorithm/geometry/detail/select_rings.hpp>
#include <boost/asynchronous/algorithm/geometry/detail/get_turns.hpp>
#include <boost/asynchronous/algorithm/geometry/detail/assign_parents.hpp>
#include <boost/geometry/algorithms/detail/overlay/do_reverse.hpp>

#include <boost/geometry/policies/robustness/segment_ratio_type.hpp>


#ifdef BOOST_GEOMETRY_DEBUG_ASSEMBLE
#  include <boost/geometry/io/dsv/write.hpp>
#endif

#ifdef BOOST_ASYNCHRONOUS_GEOMETRY_TIME_OVERLAY
# include <boost/timer.hpp>
#endif


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{

template
<
    overlay_type OverlayType,
    typename Geometry1, typename Geometry2,
    typename TurnInfoMap, 
    typename RingPropertyMap
>
struct update_selection_map_fct
{
    update_selection_map_fct(){}
    update_selection_map_fct(Geometry1 geometry1,
                             Geometry2 geometry2,
                             TurnInfoMap turn_info_map,
                             boost::shared_ptr<RingPropertyMap> all_ring_properties)
        : geometry1_(boost::make_shared<Geometry1>(std::move(geometry1)))
        , geometry2_(boost::make_shared<Geometry2>(std::move(geometry2)))
        , turn_info_map_(boost::make_shared<TurnInfoMap>(std::move(turn_info_map)))
        , all_ring_properties_(all_ring_properties)
    {}
    update_selection_map_fct(update_selection_map_fct&& rhs)noexcept
        : geometry1_(std::move(rhs.geometry1_))
        , geometry2_(std::move(rhs.geometry2_))
        , turn_info_map_(std::move(rhs.turn_info_map_))
        , all_ring_properties_(std::move(rhs.all_ring_properties_))
        , selected_ring_properties_(std::move(rhs.selected_ring_properties_))
    {
    }
    update_selection_map_fct& operator=(update_selection_map_fct&& rhs)noexcept
    {
        geometry1_ = std::move(rhs.geometry1_);
        geometry2_ = std::move(rhs.geometry2_);
        turn_info_map_ = std::move(rhs.turn_info_map_);
        all_ring_properties_ = std::move(rhs.all_ring_properties_);
        selected_ring_properties_ = std::move(rhs.selected_ring_properties_);
        return *this;
    }
    update_selection_map_fct(update_selection_map_fct const& rhs)noexcept
        : geometry1_(rhs.geometry1_)
        , geometry2_(rhs.geometry2_)
        , turn_info_map_(rhs.turn_info_map_)
        , all_ring_properties_(rhs.all_ring_properties_)
        , selected_ring_properties_(rhs.selected_ring_properties_)
    {
    }
    update_selection_map_fct& operator=(update_selection_map_fct const& rhs)noexcept
    {
        geometry1_ = rhs.geometry1_;
        geometry2_ = rhs.geometry2_;
        turn_info_map_ = rhs.turn_info_map_;
        all_ring_properties_ = rhs.all_ring_properties_;
        selected_ring_properties_ = rhs.selected_ring_properties_;
        return *this;
    }
    void operator()(std::pair<ring_identifier,typename RingPropertyMap::mapped_type> const& i)
    {
        ring_identifier const& id = i.first;

        ring_turn_info info;

        typename TurnInfoMap::iterator tcit = turn_info_map_->find(id);
        if (tcit != turn_info_map_->end())
        {
            info = std::move(tcit->second);
        }

        if (info.has_normal_turn)
        {
            // There are normal turns on this ring. It should be traversed, we
            // don't include the original ring
            return;
        }

        if (! info.has_uu_turn)
        {
            // Check if the ring is within the other geometry, by taking
            // a point lying on the ring
            switch(id.source_index)
            {
                case 0 :
                    info.within_other = geometry::within(i.second.point, *geometry2_);
                    break;
                case 1 :
                    info.within_other = geometry::within(i.second.point, *geometry1_);
                    break;
            }
        }

        if (decide<OverlayType>::include(id, info))
        {
            typename RingPropertyMap::mapped_type properties = i.second; // Copy by value
            properties.reversed = decide<OverlayType>::reversed(id, info);
            selected_ring_properties_[id] = properties;
        }
    }
    void merge(update_selection_map_fct const& rhs)
    {
        // TODO move possible?
        selected_ring_properties_.insert(rhs.selected_ring_properties_.begin(),rhs.selected_ring_properties_.end());
    }
    boost::shared_ptr<Geometry1> geometry1_;
    boost::shared_ptr<Geometry2> geometry2_;
    boost::shared_ptr<TurnInfoMap> turn_info_map_;
    boost::shared_ptr<RingPropertyMap> all_ring_properties_;
    RingPropertyMap selected_ring_properties_;
};

template
<
    typename Job,
    typename Geometry1, typename Geometry2,
    bool Reverse1, bool Reverse2, bool ReverseOut,
    typename GeometryOut,
    overlay_type Direction
>
struct parallel_overlay
{
    template <typename TaskRes,typename RobustPolicy, typename Strategy>
    static inline void apply(
                TaskRes task_res,
                Geometry1 const& geometry1, Geometry2 const& geometry2,
                RobustPolicy& robust_policy,
                Strategy const& ,
                long overlay_cutoff,
                long partition_cutoff)
    {
        boost::shared_ptr<typename TaskRes::return_type> output_collection(boost::make_shared<typename TaskRes::return_type>());
        auto out = std::back_inserter(*output_collection);

        if ( geometry::num_points(geometry1) == 0
          && geometry::num_points(geometry2) == 0 )
        {
            task_res.set_value(std::move(*output_collection));
            return;
        }

        if ( geometry::num_points(geometry1) == 0
          || geometry::num_points(geometry2) == 0 )
        {
            task_res.set_value(std::move(*output_collection));
            return_if_one_input_is_empty
                <
                    GeometryOut, Direction, ReverseOut
                >(geometry1, geometry2, out);
            return;
        }

        typedef typename geometry::point_type<GeometryOut>::type point_type;
        typedef detail::overlay::traversal_turn_info
        <
            point_type,
            typename geometry::segment_ratio_type<point_type, RobustPolicy>::type
        > turn_info;
        typedef std::deque<turn_info> container_type;

        typedef std::deque
            <
                typename geometry::ring_type<GeometryOut>::type
            > ring_container_type;

        container_type turn_points;
#ifdef BOOST_ASYNCHRONOUS_GEOMETRY_TIME_OVERLAY
        auto start = boost::chrono::high_resolution_clock::now();
#endif
#ifdef BOOST_GEOMETRY_DEBUG_ASSEMBLE
std::cout << "get turns" << std::endl;
#endif

        detail::get_turns::no_interrupt_policy policy;
        auto cont =
        geometry::parallel_get_turns
            <
                Reverse1, Reverse2,
                detail::overlay::assign_null_policy,Job
            >(geometry1, geometry2, robust_policy, turn_points, policy,partition_cutoff);
#ifdef BOOST_ASYNCHRONOUS_GEOMETRY_TIME_OVERLAY
        cont.on_done([start,task_res,geometry1, geometry2, robust_policy,overlay_cutoff,partition_cutoff,output_collection]
#else
        cont.on_done([task_res,geometry1, geometry2, robust_policy,overlay_cutoff,partition_cutoff,output_collection]
#endif
                     (std::tuple<boost::asynchronous::expected<container_type> >&& continuation_res)mutable
        {
            try
            {
            container_type turn_points( std::move(std::get<0>(continuation_res).get()));
#ifdef BOOST_ASYNCHRONOUS_GEOMETRY_TIME_OVERLAY
            double elapsed = (double)(boost::chrono::nanoseconds(boost::chrono::high_resolution_clock::now() - start).count() / 1000000.0);
            std::cout << "get_turns ms: " << elapsed <<std::endl;
            start = boost::chrono::high_resolution_clock::now();
#endif

#ifdef BOOST_GEOMETRY_DEBUG_ASSEMBLE
std::cout << "enrich" << std::endl;
#endif
            typename Strategy::side_strategy_type side_strategy;
            geometry::enrich_intersection_points<Reverse1, Reverse2>(turn_points,
                Direction == overlay_union
                    ? geometry::detail::overlay::operation_union
                    : geometry::detail::overlay::operation_intersection,
                    geometry1, geometry2,
                    robust_policy,
                    side_strategy);

#ifdef BOOST_ASYNCHRONOUS_GEOMETRY_TIME_OVERLAY
            elapsed = (double)(boost::chrono::nanoseconds(boost::chrono::high_resolution_clock::now() - start).count() / 1000000.0);
            std::cout << "enrich_intersection_points ms: " << elapsed <<std::endl;
            start = boost::chrono::high_resolution_clock::now();
#endif

#ifdef BOOST_GEOMETRY_DEBUG_ASSEMBLE
std::cout << "traverse" << std::endl;
#endif
            // Traverse through intersection/turn points and create rings of them.
            // Note that these rings are always in clockwise order, even in CCW polygons,
            // and are marked as "to be reversed" below
            boost::shared_ptr<ring_container_type> rings(boost::make_shared<ring_container_type>());
            traverse<Reverse1, Reverse2, Geometry1, Geometry2>::apply
                (
                    geometry1, geometry2,
                    Direction == overlay_union
                        ? geometry::detail::overlay::operation_union
                        : geometry::detail::overlay::operation_intersection,
                    robust_policy,
                    turn_points, *rings
                );
#ifdef BOOST_ASYNCHRONOUS_GEOMETRY_TIME_OVERLAY
            elapsed = (double)(boost::chrono::nanoseconds(boost::chrono::high_resolution_clock::now() - start).count() / 1000000.0);
            std::cout << "traverse ms: " << elapsed <<std::endl;
            start = boost::chrono::high_resolution_clock::now();
#endif

            std::map<ring_identifier, ring_turn_info> turn_info_per_ring;
            get_ring_turn_info(turn_info_per_ring, turn_points);

#ifdef BOOST_ASYNCHRONOUS_GEOMETRY_TIME_OVERLAY
            elapsed = (double)(boost::chrono::nanoseconds(boost::chrono::high_resolution_clock::now() - start).count() / 1000000.0);
            std::cout << "map_turns ms: " << elapsed <<std::endl;
            start = boost::chrono::high_resolution_clock::now();
#endif

            typedef ring_properties
            <
                typename geometry::point_type<GeometryOut>::type
            > properties;
    
            // Select all rings which are NOT touched by any intersection point
            typedef update_selection_map_fct<Direction,Geometry1,Geometry2,
                                         std::map<ring_identifier, ring_turn_info>,
                                         std::map<ring_identifier, properties>> select_ring_fct;

            auto cont = parallel_select_rings<Direction,std::map<ring_identifier, properties>,select_ring_fct,Job>(
                    std::move(geometry1), std::move(geometry2), std::move(turn_info_per_ring/*map*/),overlay_cutoff);

            cont.on_done(
#ifdef BOOST_ASYNCHRONOUS_GEOMETRY_TIME_OVERLAY
            [start,task_res,rings,partition_cutoff,output_collection](std::tuple<boost::asynchronous::expected<select_ring_fct> >&& res)mutable
#else
            [task_res,rings,partition_cutoff,output_collection](std::tuple<boost::asynchronous::expected<select_ring_fct> >&& res)mutable
#endif
            {
                try
                {
#ifdef BOOST_ASYNCHRONOUS_GEOMETRY_TIME_OVERLAY
                    double elapsed = (double)(boost::chrono::nanoseconds(boost::chrono::high_resolution_clock::now() - start).count() / 1000000.0);
                    std::cout << "select_rings ms: " << elapsed <<std::endl;
                    start = boost::chrono::high_resolution_clock::now();
#endif
                    select_ring_fct all_fct (std::move(std::get<0>(res).get()));
                    // Add rings created during traversal
                    {
                        ring_identifier id(2, 0, -1);
                        for (typename boost::range_iterator<ring_container_type>::type
                             it = boost::begin(*rings);
                            it != boost::end(*rings);
                            ++it)
                        {
                            (all_fct.selected_ring_properties_)[id] = properties(*it/*, true*/);
                            (all_fct.selected_ring_properties_)[id].reversed = ReverseOut;
                            id.multi_index++;
                        }
                    }

                    auto geometry1 = all_fct.geometry1_;
                    auto geometry2 = all_fct.geometry2_;
                    auto cont = parallel_assign_parents<Job>(*all_fct.geometry1_, *all_fct.geometry2_, *rings,
                                                              all_fct.selected_ring_properties_,partition_cutoff);
                    cont.on_done(
#ifdef BOOST_ASYNCHRONOUS_GEOMETRY_TIME_OVERLAY
                    [start,task_res,rings,output_collection,geometry1,geometry2]
                    (std::tuple<boost::asynchronous::expected<std::pair<std::map<ring_identifier, properties>,bool>>>&& res)mutable
#else
                    [task_res,rings,output_collection,geometry1,geometry2]
                    (std::tuple<boost::asynchronous::expected<std::pair<std::map<ring_identifier, properties>,bool>>>&& res)mutable
#endif
                    {
                        try
                        {
#ifdef BOOST_ASYNCHRONOUS_GEOMETRY_TIME_OVERLAY
                            double elapsed = (double)(boost::chrono::nanoseconds(boost::chrono::high_resolution_clock::now() - start).count() / 1000000.0);
                            std::cout << "assign_parents ms: " << elapsed <<std::endl;
                            start = boost::chrono::high_resolution_clock::now();
#endif
                            auto res_parents = std::move(std::get<0>(res).get());
                            auto ring_map = std::move(res_parents.first);
                            if (res_parents.second)
                            {
                                parallel_assign_parents_handle_result<Job>(ring_map);
                            }

                            add_rings<GeometryOut>(ring_map, *geometry1, *geometry2,
                                                  *rings, std::back_inserter(*output_collection));

#ifdef BOOST_ASYNCHRONOUS_GEOMETRY_TIME_OVERLAY
                            elapsed = (double)(boost::chrono::nanoseconds(boost::chrono::high_resolution_clock::now() - start).count() / 1000000.0);
                            std::cout << "add_rings ms: " << elapsed <<std::endl;
#endif
                            task_res.set_value(std::move(*output_collection));
                        }
                        catch(std::exception& e)
                        {
                            std::cout << "overlay. exception 0" << std::endl;
                            task_res.set_exception(boost::copy_exception(e));
                        }
                    }
                    );

                }
                catch(std::exception& e)
                {
                    std::cout << "overlay. exception 1" << std::endl;
                    task_res.set_exception(boost::copy_exception(e));
                }
            });
            }
            catch(std::exception& e)
            {
                std::cout << "overlay. exception 2" << std::endl;
                task_res.set_exception(boost::copy_exception(e));
            }
        });




    }
};


}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_ASYNCHRONOUS_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_OVERLAY_HPP
