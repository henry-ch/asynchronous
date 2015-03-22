// Boost.Geometry (aka GGL, Generic Geometry Library)
// Other Test

// Copyright (c) 2015 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/timer.hpp>

#include <boost/geometry.hpp>
#include <boost/geometry/multi/geometries/multi_geometries.hpp>

#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>

#include <boost/foreach.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/algorithm/geometry/parallel_geometry_intersection_of_x.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include "sinus_buffer2.hpp"

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


// Function to let buffer-distance depend on alpha, e.g.:
struct intersection_distance_modifier
{
    static inline double apply(double distance, double alpha)
    {
        return distance * 1.0 + 0.2 * distance * sin(alpha * 6.0);
    }
};

template <typename Collection>
void pairwise_intersections(Collection const& input_collection, Collection& output_collection)
{
    output_collection.clear();

    typedef typename boost::range_value<Collection>::type Element;

    std::size_t n = boost::size(input_collection);
    for (std::size_t i = 1; i < n; i += 2)
    {
        Element result;
        bg::intersection(input_collection[i - 1], input_collection[i], result);
        output_collection.push_back(result);
    }
    if (n % 2 == 1)
    {
        output_collection.push_back(input_collection.back());
    }
}

void test_many_intersections(int count_x, int count_y, double distance, bool method1, bool method2,
                             int tpsize,long intersection_of_x_cutoff)
{
    typedef bg::model::point<double, 2, bg::cs::cartesian> point;
    typedef bg::model::polygon<point> polygon_type;
    typedef bg::model::multi_polygon<polygon_type> multi_polygon_type;

    // Declare main object: a vector of (multi) polygons which will be intersected
    std::vector<multi_polygon_type> many_polygons;

    // Predefined strategies
    bg::strategy::buffer::distance_symmetric<double> distance_strategy(distance);
    bg::strategy::buffer::end_flat end_strategy; // not effectively used

    // Own strategies
    buffer_join_strategy_sample<intersection_distance_modifier> join_strategy;
    buffer_point_strategy_sample<intersection_distance_modifier> point_strategy;
    buffer_side_sample<intersection_distance_modifier> side_strategy;

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
            double x = i * 1.0 + random();
            double y = j * 1.0 + random();

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
    double elapsed=0.0;
    if (method1)
    {
        auto start = boost::chrono::high_resolution_clock::now();

        // Sequentially intersection all input geometries
        multi_polygon_type final_output;

        // Other than union, take care the first one is just assigned
        bool first = true;

        BOOST_FOREACH(multi_polygon_type const& mp, many_polygons)
        {
            if (first)
            {
                final_output = mp;
                first = false;
            }
            else
            {
                multi_polygon_type result;
                bg::intersection(final_output, mp, result);
                final_output = result;
            }
        }
        elapsed = (double)(boost::chrono::nanoseconds(boost::chrono::high_resolution_clock::now() - start).count() / 1000000000.0);
        std::cout << "Method 1: " << elapsed << " " << bg::area(final_output) << std::endl;

        #ifdef TEST_WITH_SVG
        create_svg("/tmp/many_intersections1.svg", many_polygons, final_output);
        #endif
    }

    // Method 2: add geometries pair-wise (much faster)
    double elapsed2=0.0;
    if (method2)
    {
        auto start = boost::chrono::high_resolution_clock::now();

        std::vector<multi_polygon_type> final_output;
        pairwise_intersections(many_polygons, final_output);

        while(final_output.size() > 1)
        {
            std::vector<multi_polygon_type> input = final_output;
            pairwise_intersections(input, final_output);
        }

        BOOST_ASSERT(final_output.size() == 1);
        elapsed2 = (double)(boost::chrono::nanoseconds(boost::chrono::high_resolution_clock::now() - start).count() / 1000000000.0);
        std::cout << "Method 2: " << elapsed2 << " " << bg::area(final_output.front()) << std::endl;


        #ifdef TEST_WITH_SVG
        create_svg("/tmp/many_intersections2.svg", many_polygons, final_output.front());
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

        /*auto beg = many_polygons.begin();
        auto end = many_polygons.end();

        boost::future<multi_polygon_type> fu = boost::asynchronous::post_future(pool,
        [beg,end,intersection_of_x_cutoff]()
        {
            return boost::asynchronous::geometry::parallel_geometry_intersection_of_x<decltype(beg),BOOST_ASYNCHRONOUS_DEFAULT_JOB>
                    (beg,end,"",0,intersection_of_x_cutoff);
        }
        ,"",0);*/

        boost::future<multi_polygon_type> fu = boost::asynchronous::post_future(pool,
        [intersection_of_x_cutoff,many_polygons=std::move(many_polygons)/*,overlay_cutoff,partition_cutoff*/]()mutable
        {
            return boost::asynchronous::geometry::parallel_geometry_intersection_of_x<std::vector<multi_polygon_type>,BOOST_ASYNCHRONOUS_DEFAULT_JOB>
                    (std::move(many_polygons),"",0,intersection_of_x_cutoff/*,overlay_cutoff,partition_cutoff*/);
        }
        ,"",0);
        multi_polygon_type final_output = std::move(fu.get());
        elapsed3 = (double)(boost::chrono::nanoseconds(boost::chrono::high_resolution_clock::now() - start).count() / 1000000000.0);

        std::cout << "Method 3: " << elapsed3 << " " << bg::area(final_output) << std::endl;
        #ifdef TEST_WITH_SVG
        create_svg("/tmp/many_intersections3.svg", many_polygons, final_output);
        #endif
    }

    std::cout << "speedup Method3/Method1: " << elapsed / elapsed3 << std::endl;
}

int main(int argc, char** argv)
{
    int count_x = argc > 1 ? atol(argv[1]) : 50;
    int count_y = argc > 2 ? atol(argv[2]) : count_x;
    double distance = argc > 3 ? atof(argv[3]) : double(count_x);
    // for asynchronous
    int tpsize = argc > 4 ? atol(argv[4]) : 8;
    long intersection_of_x_cutoff = argc > 5 ? atol(argv[5]) : 300;

    std::cout << "tpsize=" << tpsize << std::endl;
    std::cout << "intersection_of_x_cutoff=" << intersection_of_x_cutoff << std::endl;

    std::cout << "Testing for " << count_x << " x " << count_y <<  " with " << distance << std::endl;
    test_many_intersections(count_x, count_y, distance, true, false,tpsize,intersection_of_x_cutoff);

    return 0;
}
