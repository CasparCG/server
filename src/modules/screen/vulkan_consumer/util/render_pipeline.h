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
 * Author: CasparCG Team
 */

#pragma once

#include <vulkan/vulkan.hpp>

#include <cstdint>
#include <memory>

namespace caspar { namespace accelerator { namespace vulkan {
class texture;
}}} // namespace caspar::accelerator::vulkan

namespace caspar { namespace screen { namespace vulkan {

class swapchain;

/**
 * Push constants structure for screen rendering shader.
 * Must match the layout in screen.vert and screen.frag
 */
struct screen_push_constants
{
    float   pos_scale[2];  // Position scale for aspect ratio
    float   pos_offset[2]; // Position offset
    float   tex_scale[2];  // Texture coordinate scale
    float   tex_offset[2]; // Texture coordinate offset
    int32_t key_only;      // Show alpha channel only
    int32_t colour_space;  // 0=RGB, 1=datavideo_full, 2=datavideo_limited
    int32_t window_width;  // Window width for DataVideo conversion
    int32_t _pad;          // Padding
};

/**
 * Vulkan graphics pipeline for screen rendering.
 *
 * Provides:
 * - Graphics pipeline for rendering a texture to the swapchain
 * - Render pass management
 * - Framebuffer creation per swapchain image
 * - Command buffer recording for render operations
 */
class render_pipeline final
{
  public:
    /**
     * Create a render pipeline for screen presentation.
     *
     * @param device Logical device
     * @param physical_device Physical device
     * @param command_pool Command pool to allocate command buffers from
     * @param queue Queue to submit rendering to
     * @param swapchain Swapchain to render to
     */
    render_pipeline(vk::Device         device,
                    vk::PhysicalDevice physical_device,
                    vk::CommandPool    command_pool,
                    vk::Queue          queue,
                    swapchain&         swap);
    ~render_pipeline();

    render_pipeline(const render_pipeline&)            = delete;
    render_pipeline& operator=(const render_pipeline&) = delete;

    /**
     * Sample `src` and present it to the swapchain.
     *
     * `src` is already in eShaderReadOnlyOptimal on this (the present) queue:
     * the screen consumer uploads the host frame through the device transfer
     * service, which leaves the texture shader-read with a barrier whose dst
     * scope — on the one shared queue, in submission order — covers this draw.
     * So no extra wait/token/barrier is needed beyond the swapchain's own
     * image-available / render-finished semaphores. (Cross-queue / GPU-direct
     * sampling, which needs a layout transition + completion token, is not
     * implemented yet.)
     *
     * @param src Composited texture sampled by the shader (shader-read layout)
     * @param image_index Swapchain image index
     * @param frame_slot Frame-in-flight slot (selects command buffer + descriptor)
     * @param params Render parameters
     * @param wait_semaphore image-available semaphore to wait on
     * @param signal_semaphore render-finished semaphore to signal
     * @param fence Fence to signal after submission
     */
    void render(accelerator::vulkan::texture& src,
                uint32_t                      image_index,
                uint32_t                      frame_slot,
                const screen_push_constants&  params,
                vk::Semaphore                 wait_semaphore,
                vk::Semaphore                 signal_semaphore,
                vk::Fence                     fence);

    /**
     * Recreate framebuffers after swapchain recreation.
     */
    void recreate_framebuffers();

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

}}} // namespace caspar::screen::vulkan
