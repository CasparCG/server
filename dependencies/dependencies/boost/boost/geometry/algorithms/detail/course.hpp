// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2014.
// Modifications copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_COURSE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_COURSE_HPP

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/radian_access.hpp>
#include <boost/geometry/util/math.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

/// Calculate course (bearing) between two points.
template <typename ReturnType, typename Point1, typename Point2>
inline ReturnType course(Point1 const& p1, Point2 const& p2)
{
    // http://williams.best.vwh.net/avform.htm#Crs
    ReturnType dlon = get_as_radian<0>(p2) - get_as_radian<0>(p1);
    ReturnType cos_p2lat = cos(get_as_radian<1>(p2));

    // An optimization which should kick in often for Boxes
    //if ( math::equals(dlon, ReturnType(0)) )
    //if ( get<0>(p1) == get<0>(p2) )
    //{
    //    return - sin(get_as_radian<1>(p1)) * cos_p2lat);
    //}

    // "An alternative formula, not requiring the pre-computation of d"
    // In the formula below dlon is used as "d"
    return atan2(sin(dlon) * cos_p2lat,
        cos(get_as_radian<1>(p1)) * sin(get_as_radian<1>(p2))
        - sin(get_as_radian<1>(p1)) * cos_p2lat * cos(dlon));
}

} // namespace detail
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_COURSE_HPP
