/*
 * Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
 *
 * This file is part of CasparCG (www.casparcg.com).
 *
 * CasparCG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CasparCG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Niklas P Andersson, niklas.p.andersson@svt.se
 */
#include "geometry.h"

#include <common/except.h>

namespace caspar { namespace core {

frame_geometry::coord::coord(double vertex_x, double vertex_y, double texture_x, double texture_y)
    : vertex_x(vertex_x)
    , vertex_y(vertex_y)
    , texture_x(texture_x)
    , texture_y(texture_y)
{
}

bool frame_geometry::coord::operator==(const frame_geometry::coord& other) const
{
    return vertex_x == other.vertex_x && vertex_y == other.vertex_y && texture_x == other.texture_x &&
           texture_y == other.texture_y && texture_r == other.texture_r && texture_q == other.texture_q;
}

struct frame_geometry::impl
{
    impl(frame_geometry::geometry_type type, std::vector<coord> data)
        : type_(type)
    {
        if (type == geometry_type::quad && data.size() != 4)
            CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info("The number of coordinates needs to be 4"));

        data_ = std::move(data);
    }

    frame_geometry::geometry_type type_;
    std::vector<coord>            data_;
};

frame_geometry::frame_geometry(geometry_type type, std::vector<coord> data)
    : impl_(new impl(type, std::move(data)))
{
}

frame_geometry::geometry_type             frame_geometry::type() const { return impl_->type_; }
const std::vector<frame_geometry::coord>& frame_geometry::data() const { return impl_->data_; }

const frame_geometry& frame_geometry::get_default()
{
    static std::vector<frame_geometry::coord> data = {
        //    vertex    texture
        {0.0, 0.0, 0.0, 0.0}, // upper left
        {1.0, 0.0, 1.0, 0.0}, // upper right
        {1.0, 1.0, 1.0, 1.0}, // lower right
        {0.0, 1.0, 0.0, 1.0}  // lower left
    };
    static const frame_geometry g(frame_geometry::geometry_type::quad, data);

    return g;
}
const frame_geometry& frame_geometry::get_default_vflip()
{
    static std::vector<frame_geometry::coord> data = {
        //    vertex    texture
        {0.0, 0.0, 0.0, 1.0}, // upper left
        {1.0, 0.0, 1.0, 1.0}, // upper right
        {1.0, 1.0, 1.0, 0.0}, // lower right
        {0.0, 1.0, 0.0, 0.0}  // lower left
    };
    static const frame_geometry g(frame_geometry::geometry_type::quad, data);

    return g;
}

}} // namespace caspar::core
