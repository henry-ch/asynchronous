// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <vector>
#include <set>
#include <functional>
#include <random>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>

#include <boost/geometry.hpp>
#include <boost/geometry/multi/geometries/multi_geometries.hpp>

#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>


#include <boost/asynchronous/algorithm/geometry/parallel_geometry_union_of_x.hpp>


#include "test_common.hpp"
#include "sinus_buffer.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;
namespace bg = boost::geometry;


BOOST_AUTO_TEST_CASE( test_geometry_union_of_x_200_200_2 )
{
    typedef bg::model::point<double, 2, bg::cs::cartesian> point;
    typedef bg::model::polygon<point> polygon_type;
    typedef bg::model::multi_polygon<polygon_type> multi_polygon_type;

    // Declare main object: a vector of (multi) polygons which will be unioned
    std::vector<multi_polygon_type> many_polygons;

    // Predefined strategies
    bg::strategy::buffer::distance_symmetric<double> distance_strategy(2.0);
    bg::strategy::buffer::end_flat end_strategy; // not effectively used

    // Own strategies
    buffer_join_strategy_sample join_strategy;
    buffer_point_strategy_sample point_strategy;
    buffer_side_sample side_strategy;

    // Use a bit of random disturbance in the to be generated grid
    typedef boost::minstd_rand base_generator_type;
    base_generator_type generator(12345);
    boost::uniform_real<> random_range(0.0, 0.5);
    boost::variate_generator
    <
        base_generator_type&,
        boost::uniform_real<>
    > random(generator, random_range);

    for (int i = 0; i < 200; i++)
    {
        for (int j = 0; j < 200; j++)
        {
            double x = i * 3.0 + random();
            double y = j * 3.0 + random();

            point p(x, y);

            multi_polygon_type buffered;
            // Create the buffer of a point
            bg::buffer(p, buffered,
                        distance_strategy, side_strategy,
                        join_strategy, end_strategy, point_strategy);

            many_polygons.push_back(buffered);
        }
    }

    auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                                boost::asynchronous::lockfree_queue<> >(
                                                                                    boost::thread::hardware_concurrency()));

    // make a copy and execute in pool
    boost::future<std::vector<multi_polygon_type>> fu = boost::asynchronous::post_future(scheduler,
    [many_polygons=std::move(many_polygons)]()mutable
    {
        return boost::asynchronous::geometry::parallel_geometry_union_of_x<std::vector<multi_polygon_type>,
                                                                 BOOST_ASYNCHRONOUS_DEFAULT_JOB>
                (std::move(many_polygons),"",0);
    }
    ,"",0);

    try
    {
        std::vector<multi_polygon_type> final_output = std::move(fu.get());
        BOOST_CHECK_MESSAGE(final_output.size() == 1,"parallel_geometry_union_of_x gave more than one result.");
        BOOST_CHECK_MESSAGE((long)bg::area(final_output.front()) == (long)358564,"parallel_geometry_union_of_x gave a wrong value.");
    }
    catch(...)
    {
        BOOST_FAIL( "unexpected exception" );
    }
}

BOOST_AUTO_TEST_CASE( test_geometry_union_of_x_200_200_2_continuation )
{
    typedef bg::model::point<double, 2, bg::cs::cartesian> point;
    typedef bg::model::polygon<point> polygon_type;
    typedef bg::model::multi_polygon<polygon_type> multi_polygon_type;

    // Declare main object: a vector of (multi) polygons which will be unioned
    std::vector<multi_polygon_type> many_polygons;

    // Predefined strategies
    bg::strategy::buffer::distance_symmetric<double> distance_strategy(2.0);
    bg::strategy::buffer::end_flat end_strategy; // not effectively used

    // Own strategies
    buffer_join_strategy_sample join_strategy;
    buffer_point_strategy_sample point_strategy;
    buffer_side_sample side_strategy;

    // Use a bit of random disturbance in the to be generated grid
    typedef boost::minstd_rand base_generator_type;
    base_generator_type generator(12345);
    boost::uniform_real<> random_range(0.0, 0.5);
    boost::variate_generator
    <
        base_generator_type&,
        boost::uniform_real<>
    > random(generator, random_range);

    for (int i = 0; i < 200; i++)
    {
        for (int j = 0; j < 200; j++)
        {
            double x = i * 3.0 + random();
            double y = j * 3.0 + random();

            point p(x, y);

            multi_polygon_type buffered;
            // Create the buffer of a point
            bg::buffer(p, buffered,
                        distance_strategy, side_strategy,
                        join_strategy, end_strategy, point_strategy);

            many_polygons.push_back(buffered);
        }
    }

    auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::multiqueue_threadpool_scheduler<
                                                                                boost::asynchronous::lockfree_queue<> >(
                                                                                    boost::thread::hardware_concurrency()));

    // make a copy and execute in pool
    boost::future<std::vector<multi_polygon_type>> fu = boost::asynchronous::post_future(scheduler,
    [many_polygons=std::move(many_polygons)]()mutable
    {
        return boost::asynchronous::geometry::parallel_geometry_union_of_x(
            boost::asynchronous::geometry::parallel_geometry_union_of_x<std::vector<multi_polygon_type>>
                (std::move(many_polygons),"",0));
    }
    ,"",0);

    try
    {
        std::vector<multi_polygon_type> final_output = std::move(fu.get());
        BOOST_CHECK_MESSAGE(final_output.size() == 1,"parallel_geometry_union_of_x gave more than one result.");
        BOOST_CHECK_MESSAGE((long)bg::area(final_output.front()) == (long)358564,"parallel_geometry_union_of_x gave a wrong value.");
    }
    catch(...)
    {
        BOOST_FAIL( "unexpected exception" );
    }
}
