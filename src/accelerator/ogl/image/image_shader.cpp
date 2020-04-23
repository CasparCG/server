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
#include "image_shader.h"

#include "../util/device.h"
#include "../util/shader.h"

#include "ogl_image_fragment.h"
#include "ogl_image_vertex.h"

namespace caspar { namespace accelerator { namespace ogl {

std::weak_ptr<shader> g_shader;
std::mutex            g_shader_mutex;

std::shared_ptr<shader> get_image_shader(const spl::shared_ptr<device>& ogl)
{
    std::lock_guard<std::mutex> lock(g_shader_mutex);
    auto                        existing_shader = g_shader.lock();

    if (existing_shader) {
        return existing_shader;
    }

    // The deleter is alive until the weak pointer is destroyed, so we have
    // to weakly reference ogl, to not keep it alive until atexit
    std::weak_ptr<device> weak_ogl = ogl;

    auto deleter = [weak_ogl](shader* p) {
        auto ogl = weak_ogl.lock();

        if (ogl) {
            ogl->dispatch_async([=] { delete p; });
        }
    };

    existing_shader.reset(new shader(std::string(vertex_shader), std::string(fragment_shader)), deleter);

    g_shader = existing_shader;

    return existing_shader;
}

}}} // namespace caspar::accelerator::ogl
