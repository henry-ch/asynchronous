// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_SINUS_BUFFER_HPP
#define BOOST_GEOMETRY_SINUS_BUFFER_HPP

//static double M_PI = 3.14159265;

// Function to let buffer-distance depend on alpha, e.g.:
inline double corrected_distance(double distance, double alpha)
{
    return distance * 1.0 + 0.2 * sin(alpha * 6.0);
}

class buffer_point_strategy_sample
{
public :

    template
    <
        typename Point,
        typename OutputRange,
        typename DistanceStrategy
    >
    void apply(Point const& point,
            DistanceStrategy const& distance_strategy,
            OutputRange& output_range) const
    {
        namespace bg = boost::geometry;
        const int point_count = 90; // points for a full circle

        double const distance = distance_strategy.apply(point, point,
                        bg::strategy::buffer::buffer_side_left);

        double const angle_increment = 2.0 * M_PI / double(point_count);

        double alpha = 0;
        for (std::size_t i = 0; i <= point_count; i++, alpha -= angle_increment)
        {
            double const cd = corrected_distance(distance, alpha);

            typename boost::range_value<OutputRange>::type output_point;
            bg::set<0>(output_point, bg::get<0>(point) + cd * cos(alpha));
            bg::set<1>(output_point, bg::get<1>(point) + cd * sin(alpha));
            output_range.push_back(output_point);
        }
    }
};

class buffer_join_strategy_sample
{
private :
    template
    <
        typename Point,
        typename DistanceType,
        typename RangeOut
    >
    inline void generate_points(Point const& vertex,
                Point const& perp1, Point const& perp2,
                DistanceType const& buffer_distance,
                RangeOut& range_out) const
    {
        namespace bg = boost::geometry;
        const int point_count = 90; // points for a full circle

        double dx1 = bg::get<0>(perp1) - bg::get<0>(vertex);
        double dy1 = bg::get<1>(perp1) - bg::get<1>(vertex);
        double dx2 = bg::get<0>(perp2) - bg::get<0>(vertex);
        double dy2 = bg::get<1>(perp2) - bg::get<1>(vertex);

        // Assuming the corner is convex, angle2 < angle1
        double const angle1 = atan2(dy1, dx1);
        double angle2 = atan2(dy2, dx2);

        while (angle2 > angle1)
        {
            angle2 -= 2 * M_PI;
        }

        double const angle_increment = 2.0 * M_PI / double(point_count);
        double alpha = angle1 - angle_increment;

        for (int i = 0; alpha >= angle2 && i < point_count; i++, alpha -= angle_increment)
        {
            double cd = corrected_distance(buffer_distance, alpha);

            Point p;
            bg::set<0>(p, bg::get<0>(vertex) + cd * cos(alpha));
            bg::set<1>(p, bg::get<1>(vertex) + cd * sin(alpha));
            range_out.push_back(p);
        }
    }

public :

    template <typename Point, typename DistanceType, typename RangeOut>
    inline bool apply(Point const& , Point const& vertex,
                Point const& perp1, Point const& perp2,
                DistanceType const& buffer_distance,
                RangeOut& range_out) const
    {
        generate_points(vertex, perp1, perp2, buffer_distance, range_out);
        return true;
    }

    template <typename NumericType>
    static inline NumericType max_distance(NumericType const& distance)
    {
        return distance;
    }

};

class buffer_side_sample
{
public :
    template
    <
        typename Point,
        typename OutputRange,
        typename DistanceStrategy
    >
    static inline void apply(
                Point const& input_p1, Point const& input_p2,
                boost::geometry::strategy::buffer::buffer_side_selector side,
                DistanceStrategy const& distance,
                OutputRange& output_range)
    {
        namespace bg = boost::geometry;
        // Generate a block along (left or right of) the segment

        double const dx = bg::get<0>(input_p2) - bg::get<0>(input_p1);
        double const dy = bg::get<1>(input_p2) - bg::get<1>(input_p1);

        // For normalization [0,1] (=dot product d.d, sqrt)
        double const length = bg::math::sqrt(dx * dx + dy * dy);

        if (bg::math::equals(length, 0))
        {
            return;
        }

        // Generate the normalized perpendicular p, to the left (ccw)
        double const px = -dy / length;
        double const py = dx / length;

        // Both vectors perpendicular to input p1 and input p2 have same angle
        double const alpha = atan2(py, px);

        double const d = distance.apply(input_p1, input_p2, side);

        double const cd = corrected_distance(d, alpha);

        output_range.resize(2);

        bg::set<0>(output_range.front(), bg::get<0>(input_p1) + px * cd);
        bg::set<1>(output_range.front(), bg::get<1>(input_p1) + py * cd);
        bg::set<0>(output_range.back(), bg::get<0>(input_p2) + px * cd);
        bg::set<1>(output_range.back(), bg::get<1>(input_p2) + py * cd);
    }
};

#endif // BOOST_GEOMETRY_SINUS_BUFFER_HPP

