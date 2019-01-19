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
 * Author: Robert Nagy, ronag89@gmail.com
 */

#pragma once

#include <core/mixer/image/blend_modes.h>

#include <common/memory.h>

#include <core/frame/frame_transform.h>
#include <core/frame/geometry.h>
#include <core/frame/pixel_format.h>

namespace caspar { namespace accelerator { namespace ogl {

enum class keyer
{
    linear = 0,
    additive,
};

struct draw_params final
{
    core::pixel_format_desc                     pix_desc = core::pixel_format::invalid;
    std::vector<spl::shared_ptr<class texture>> textures;
    core::image_transform                       transform;
    core::frame_geometry                        geometry   = core::frame_geometry::get_default();
    core::blend_mode                            blend_mode = core::blend_mode::normal;
    ogl::keyer                                  keyer      = ogl::keyer::linear;
    std::shared_ptr<class texture>              background;
    std::shared_ptr<class texture>              local_key;
    std::shared_ptr<class texture>              layer_key;
    double                                      aspect_ratio = 1.0;
};

class image_kernel final
{
    image_kernel(const image_kernel&);
    image_kernel& operator=(const image_kernel&);

  public:
    explicit image_kernel(const spl::shared_ptr<class device>& ogl);
    ~image_kernel();

    void draw(const draw_params& params);

  private:
    struct impl;
    spl::unique_ptr<impl> impl_;
};

}}} // namespace caspar::accelerator::ogl
