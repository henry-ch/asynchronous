// Boost.Geometry (aka GGL, Generic Geometry Library)
// Other Test

// Copyright (c) 2015 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/geometry.hpp>
#include <boost/geometry/multi/geometries/multi_geometries.hpp>

#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>

#include <boost/foreach.hpp> // sorry I'm still at C++03 by default
#include <boost/timer.hpp>

#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/algorithm/geometry/parallel_geometry_union_of_x.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>

#include "sinus_buffer.hpp"

#ifdef TEST_WITH_SVG
#include <fstream>
#include <boost/geometry/io/svg/svg_mapper.hpp>
#endif

namespace bg = boost::geometry;


#ifdef TEST_WITH_SVG
template <typename Collection, typename Geometry>
void create_svg(std::string const& filename, Collection const& collection, Geometry const& result)
{
    typedef typename boost::range_value<Collection>::type Element;
    typedef typename bg::point_type<Element>::type point_type;
    std::ofstream svg(filename.c_str());

    bg::svg_mapper<point_type> mapper(svg, 800, 800);

    mapper.add(result);
    BOOST_FOREACH(Element const& element, collection)
    {
        mapper.add(element);
    }

    typedef typename boost::range_value<Collection>::type Element;
    BOOST_FOREACH(Element const& element, collection)
    {
        mapper.map(element, "fill-opacity:0.5;fill:rgb(51,51,153);stroke:rgb(0,0,0);stroke-width:1");
    }

    // Comment if you want to see only input
    mapper.map(result, "fill:none;stroke:rgb(255,128,0);stroke-width:3");
}
#endif

template <typename Collection>
void pairwise_unions(Collection const& input_collection, Collection& output_collection)
{
    output_collection.clear();

    typedef typename boost::range_value<Collection>::type Element;

    std::size_t n = boost::size(input_collection);
    for (std::size_t i = 1; i < n; i += 2)
    {
        Element result;
        bg::union_(input_collection[i - 1], input_collection[i], result);
        output_collection.push_back(result);
    }
    if (n % 2 == 1)
    {
        output_collection.push_back(input_collection.back());
    }
}

void test_many_unions(int count_x, int count_y, double distance, bool method1,
                      int tpsize,long union_of_x_cutoff,long overlay_cutoff,long partition_cutoff)
{
    typedef bg::model::point<double, 2, bg::cs::cartesian> point;
    typedef bg::model::polygon<point> polygon_type;
    typedef bg::model::multi_polygon<polygon_type> multi_polygon_type;

    // Declare main object: a vector of (multi) polygons which will be unioned
    std::vector<multi_polygon_type> many_polygons;

    // Predefined strategies
    bg::strategy::buffer::distance_symmetric<double> distance_strategy(distance);
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

    for (int i = 0; i < count_x; i++)
    {
        for (int j = 0; j < count_y; j++)
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



    // Method 1: add geometries one-by-one to the final output (SLOW)
    if (method1)
    {
        boost::timer t;

        // Sequentially union all input geometries
        multi_polygon_type final_output;

        BOOST_FOREACH(multi_polygon_type const& mp, many_polygons)
        {
            multi_polygon_type result;
            bg::union_(final_output, mp, result);
            final_output = result;
        }

        std::cout << "Method 1: " << t.elapsed() << " " << bg::area(final_output) << std::endl;

        #ifdef TEST_WITH_SVG
        create_svg("/tmp/many_polygons1.svg", many_polygons, final_output);
        #endif
    }

    // Method 2: add geometries pair-wise (much faster)
   double elapsed2=0.0;
     {
        auto start = boost::chrono::high_resolution_clock::now();

        std::vector<multi_polygon_type> final_output;
        pairwise_unions(many_polygons, final_output);

        while(final_output.size() > 1)
        {
            std::vector<multi_polygon_type> input = final_output;
            pairwise_unions(input, final_output);
        }

        elapsed2 = (double)(boost::chrono::nanoseconds(boost::chrono::high_resolution_clock::now() - start).count() / 1000000000.0);
        BOOST_ASSERT(final_output.size() == 1);

        std::cout << "Method 2: " << elapsed2 << " " << bg::area(final_output.front()) << std::endl;


        #ifdef TEST_WITH_SVG
        create_svg("/tmp/many_polygons2.svg", many_polygons, final_output.front());
        #endif
    }

    // Method 3: in parallel
    double elapsed3=0.0;
    auto pool = boost::asynchronous::create_shared_scheduler_proxy(
                new boost::asynchronous::multiqueue_threadpool_scheduler<
                        boost::asynchronous::lockfree_queue<>/*,
                        boost::asynchronous::default_find_position< boost::asynchronous::sequential_push_policy>,
                        boost::asynchronous::no_cpu_load_saving*/
                    >(tpsize,64));

    {
        auto start = boost::chrono::high_resolution_clock::now();

/*        auto beg = many_polygons.begin();
        auto end = many_polygons.end();

        boost::future<std::vector<multi_polygon_type>> fu = boost::asynchronous::post_future(pool,
        [union_of_x_cutoff,beg,end,overlay_cutoff,partition_cutoff]()
        {
            return boost::asynchronous::geometry::parallel_geometry_union_of_x<decltype(beg),
                                                                    std::vector<multi_polygon_type>,
                                                                    BOOST_ASYNCHRONOUS_DEFAULT_JOB>
                    (beg,end,"",0,union_of_x_cutoff,overlay_cutoff,partition_cutoff);
        }
        ,"",0);*/

        boost::future<std::vector<multi_polygon_type>> fu = boost::asynchronous::post_future(pool,
        [union_of_x_cutoff,many_polygons=std::move(many_polygons),overlay_cutoff,partition_cutoff]()mutable
        {
            return boost::asynchronous::geometry::parallel_geometry_union_of_x<std::vector<multi_polygon_type>,
                                                                     BOOST_ASYNCHRONOUS_DEFAULT_JOB>
                    (std::move(many_polygons),"",0,union_of_x_cutoff,overlay_cutoff,partition_cutoff);
        }
        ,"",0);
        std::vector<multi_polygon_type> final_output = std::move(fu.get());
        elapsed3 = (double)(boost::chrono::nanoseconds(boost::chrono::high_resolution_clock::now() - start).count() / 1000000000.0);

        BOOST_ASSERT(final_output.size() == 1);
        std::cout << "Method 3: " << elapsed3 << " " << bg::area(final_output.front()) << std::endl;
        #ifdef TEST_WITH_SVG
        create_svg("/tmp/many_polygons3.svg", many_polygons, final_output.front());
        #endif
    }

    std::cout << "speedup Method3/Method2: " << elapsed2 / elapsed3 << std::endl;
}

int main(int argc, char** argv)
{
    int count_x = argc > 1 ? atol(argv[1]) : 51;
    int count_y = argc > 2 ? atol(argv[2]) : 51;
    double distance = argc > 3 ? atof(argv[3]) : 1.85; // Leaves many holes in union
    // for asynchronous
    int tpsize = argc > 4 ? atol(argv[4]) : 8;
    long union_of_x_cutoff = argc > 5 ? atol(argv[5]) : 300;
    long overlay_cutoff = argc > 6 ? atol(argv[6]) : 1500;
    long partition_cutoff = argc > 7 ? atol(argv[7]) : 80000;
    std::cout << "tpsize=" << tpsize << std::endl;
    std::cout << "union_of_x_cutoff=" << union_of_x_cutoff << std::endl;
    std::cout << "overlay_cutoff=" << overlay_cutoff << std::endl;
    std::cout << "partition_cutoff=" << partition_cutoff << std::endl;

    std::cout << "Testing for " << count_x << " x " << count_y <<  " with " << distance << std::endl;
    test_many_unions(count_x, count_y, distance, false,tpsize,union_of_x_cutoff,overlay_cutoff,partition_cutoff);

    return 0;
}
