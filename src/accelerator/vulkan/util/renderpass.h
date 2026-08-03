/*
 * Copyright 2025
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
 * Author: Niklas Andersson, niklas@niklaspandersson.se
 */

#pragma once

#include <common/bit_depth.h>
#include <common/memory.h>
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "draw_params.h"
#include "uniform_block.h"

namespace caspar { namespace accelerator { namespace vulkan {

using draw_data = std::pair<std::vector<core::frame_geometry::coord>, uniform_block>;
struct frame_context
{
    virtual vk::Buffer                      upload_vertex_data(const std::vector<float>& data) = 0;
    virtual draw_data                       create_draw_data(const draw_params& params)        = 0;
    virtual std::shared_ptr<class pipeline> get_pipeline()                                     = 0;
    virtual vk::CommandBuffer               get_command_buffer()                               = 0;
    virtual void                            submit()                                           = 0;
    virtual std::shared_ptr<class texture>
    create_attachment(uint32_t width, uint32_t height, uint32_t components_count) = 0;
};

class renderpass
{
    frame_context*                  _ctx;
    std::shared_ptr<class pipeline> _pipeline;
    uint32_t                        _width;
    uint32_t                        _height;

    std::shared_ptr<class texture> _default_attachment;

    struct layer_info
    {
        std::shared_ptr<class texture>           attachment;
        std::shared_ptr<class texture>           local_key_attachment;
        std::shared_ptr<class texture>           layer_key_attachment;
        std::array<vk::ImageView, 7>             textures;
        std::vector<core::frame_geometry::coord> coords;
        uniform_block                            uniforms;
        uint32_t                                 vertex_buffer_offset = 0;
    };
    std::vector<layer_info> layers_;

  public:
    renderpass(frame_context* ctx, uint32_t width, uint32_t height);

    renderpass()                             = delete;
    renderpass(const renderpass&)            = delete;
    renderpass& operator=(const renderpass&) = delete;

    ~renderpass();
    std::shared_ptr<class texture> create_attachment(uint32_t components_count = 4);
    void                           draw(const draw_params& params);
    virtual void                   commit();

    std::shared_ptr<class texture> default_attachment() const { return _default_attachment; }

  private:
    vk::Buffer upload_vertex_buffers();
};

}}} // namespace caspar::accelerator::vulkan