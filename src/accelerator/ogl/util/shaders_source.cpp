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
 * Author: Julian Waller, julian@superfly.tv
 */

#include "shaders_source.h"

#include "ogl_image_shader_frag.h"
#include "ogl_image_shader_vert.h"

#include "ogl_image_shader_compute_from_rgba_comp.h"
#include "ogl_image_shader_compute_to_rgba_comp.h"

namespace caspar::accelerator::ogl {

const std::string shaders_source::image_fragment = std::string(ogl_image_shader_frag);
const std::string shaders_source::image_vertex   = std::string(ogl_image_shader_vert);

const std::string shaders_source::compute_from_rgba = std::string(ogl_image_shader_compute_from_rgba_comp);
const std::string shaders_source::compute_to_rgba   = std::string(ogl_image_shader_compute_to_rgba_comp);

} // namespace caspar::accelerator::ogl
