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

#pragma once

#include <common/memory.h>

#include <vector>

namespace caspar { namespace core {

class frame_geometry
{
  public:
    enum class geometry_type
    {
        quad
    };

    struct coord
    {
        double vertex_x  = 0.0;
        double vertex_y  = 0.0;
        double texture_x = 0.0;
        double texture_y = 0.0;
        double texture_r = 0.0;
        double texture_q = 1.0;

        coord() = default;
        coord(double vertex_x, double vertex_y, double texture_x, double texture_y);

        bool operator==(const coord& other) const;
    };

    frame_geometry(geometry_type type, std::vector<coord> data);

    geometry_type             type() const;
    const std::vector<coord>& data() const;

    static const frame_geometry& get_default();
    static const frame_geometry& get_default_vflip();

  private:
    struct impl;
    spl::shared_ptr<impl> impl_;
};

}} // namespace caspar::core
