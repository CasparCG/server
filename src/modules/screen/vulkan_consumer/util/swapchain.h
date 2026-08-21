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
#include <functional>
#include <memory>
#include <vector>

namespace caspar { namespace screen { namespace vulkan {

class texture;

/**
 * Vulkan swapchain tied to a platform surface: creation, image acquisition,
 * presentation, resize handling and vsync control.
 */
class swapchain final
{
  public:
    /**
     * Create a swapchain for the given Vulkan surface.
     *
     * @param instance Vulkan instance
     * @param physical_device Physical device
     * @param device Logical device
     * @param queue Presentation queue
     * @param queue_family_index Queue family index for presentation
     * @param surface Pre-created Vulkan surface (owned by caller, destroyed with instance)
     * @param vsync Enable vertical sync
     * @param get_framebuffer_size Callback returning the current drawable size in pixels
     */
    swapchain(vk::Instance                    instance,
              vk::PhysicalDevice              physical_device,
              vk::Device                      device,
              vk::Queue                       queue,
              uint32_t                        queue_family_index,
              vk::SurfaceKHR                  surface,
              bool                            vsync,
              std::function<void(int&, int&)> get_framebuffer_size,
              std::function<void()>           wait_for_events = {});
    ~swapchain();

    swapchain(const swapchain&)            = delete;
    swapchain& operator=(const swapchain&) = delete;

    /**
     * Acquire the next swapchain image.
     *
     * @return Image index, or UINT32_MAX if swapchain needs recreation
     */
    uint32_t acquire_next_image();

    /**
     * Present the current swapchain image.
     *
     * @param image_index Index returned from acquire_next_image
     * @return true if successful, false if swapchain needs recreation
     */
    bool present(uint32_t image_index);

    /**
     * Get the swapchain image view at the given index.
     *
     * @param index Image index
     * @return Image view
     */
    vk::ImageView get_image_view(uint32_t index) const;

    /**
     * Get the swapchain image at the given index.
     *
     * @param index Image index
     * @return Image
     */
    vk::Image get_image(uint32_t index) const;

    /**
     * Get the number of swapchain images.
     */
    uint32_t image_count() const;

    /**
     * Get the swapchain extent (width, height).
     */
    void get_extent(uint32_t& width, uint32_t& height) const;

    /**
     * Get the swapchain format.
     */
    vk::Format format() const;

    /**
     * Recreate the swapchain (e.g., after window resize).
     */
    void recreate();

    /**
     * Get the image available semaphore for the current frame.
     *
     * @return Image available semaphore
     */
    vk::Semaphore image_available_semaphore() const;

    /**
     * Get the render finished semaphore for a specific swapchain image.
     * Must be indexed by the acquired image index, not by the frame slot,
     * so that the presentation engine cannot reuse a semaphore while an
     * un-re-acquired image still holds a reference to it.
     *
     * @param image_index Index returned from acquire_next_image
     * @return Render finished semaphore
     */
    vk::Semaphore render_finished_semaphore(uint32_t image_index) const;

    /**
     * Get the in-flight fence for the current frame.
     *
     * @return In-flight fence
     */
    vk::Fence in_flight_fence() const;

    /**
     * Wait for the current frame's fence to signal.
     */
    void wait_for_fence();

    /**
     * Reset the current frame's fence.
     */
    void reset_fence();

    /**
     * Advance to the next frame for synchronization objects.
     */
    void next_frame();

    /**
     * Get the current frame slot index (0..max_frames_in_flight-1).
     * Use this to select per-frame resources such as command buffers.
     */
    uint32_t frame_slot() const;

    /**
     * Get the maximum number of frames that can be in flight simultaneously.
     */
    uint32_t max_frames_in_flight() const;

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

}}} // namespace caspar::screen::vulkan
