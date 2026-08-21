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

#include "swapchain.h"
#include "vk_check.h"

#include <common/log.h>

#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <functional>
#include <limits>
#include <vector>

namespace caspar { namespace screen { namespace vulkan {

static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

struct swapchain::impl
{
    vk::Instance                    instance_;
    vk::PhysicalDevice              physical_device_;
    vk::Device                      device_;
    vk::Queue                       queue_;
    uint32_t                        queue_family_index_;
    std::function<void(int&, int&)> get_framebuffer_size_;
    std::function<void()>           wait_for_events_;
    bool                            vsync_ = false;

    vk::SurfaceKHR   surface_;
    vk::SwapchainKHR swapchain_;

    std::vector<vk::Image>     swapchain_images_;
    std::vector<vk::ImageView> swapchain_image_views_;
    vk::Format                 swapchain_format_ = vk::Format::eB8G8R8A8Unorm;
    vk::Extent2D               swapchain_extent_{0, 0};

    // Synchronization objects (per frame in flight)
    std::vector<vk::Semaphore> image_available_semaphores_;
    std::vector<vk::Semaphore> render_finished_semaphores_;
    std::vector<vk::Fence>     in_flight_fences_;
    size_t                     current_frame_ = 0;

    impl(vk::Instance                    instance,
         vk::PhysicalDevice              physical_device,
         vk::Device                      device,
         vk::Queue                       queue,
         uint32_t                        queue_family_index,
         vk::SurfaceKHR                  surface,
         bool                            vsync,
         std::function<void(int&, int&)> get_framebuffer_size,
         std::function<void()>           wait_for_events)
        : instance_(instance)
        , physical_device_(physical_device)
        , device_(device)
        , queue_(queue)
        , queue_family_index_(queue_family_index)
        , get_framebuffer_size_(std::move(get_framebuffer_size))
        , wait_for_events_(std::move(wait_for_events))
        , vsync_(vsync)
    {
        set_surface(surface);

        create_swapchain();
        create_image_views();
        create_sync_objects();

        CASPAR_LOG(info) << L"[vk::swapchain] Vulkan swapchain initialized (" << swapchain_extent_.width << L"x"
                         << swapchain_extent_.height << L", " << swapchain_images_.size() << L" images"
                         << (vsync_ ? L", vsync" : L"") << L")";
    }

    ~impl()
    {
        cleanup_swapchain();
        cleanup_sync_objects();

        if (surface_) {
            instance_.destroySurfaceKHR(surface_);
        }

        CASPAR_LOG(info) << L"[vk::swapchain] Vulkan swapchain destroyed";
    }

    void set_surface(vk::SurfaceKHR surface)
    {
        surface_ = surface;
        if (!physical_device_.getSurfaceSupportKHR(queue_family_index_, surface_)) {
            CASPAR_THROW_EXCEPTION(vk_exception() << msg_info("Queue family does not support presentation to surface"));
        }
    }

    vk::SurfaceFormatKHR choose_surface_format()
    {
        std::vector<vk::SurfaceFormatKHR> formats = physical_device_.getSurfaceFormatsKHR(surface_);

        // Log available formats for debugging
        CASPAR_LOG(debug) << L"[vk::swapchain] Available surface formats: " << formats.size();
        for (const auto& format : formats) {
            CASPAR_LOG(debug) << L"[vk::swapchain]   Format: " << static_cast<uint32_t>(format.format)
                              << L", ColorSpace: " << static_cast<uint32_t>(format.colorSpace);
        }

        // Prefer BGRA8 UNORM for direct color passthrough (matches frame texture format)
        for (const auto& format : formats) {
            if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                CASPAR_LOG(info) << L"[vk::swapchain] Selected format: B8G8R8A8_UNORM";
                return format;
            }
        }

        // Fall back to BGRA8 SRGB
        for (const auto& format : formats) {
            if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                CASPAR_LOG(info) << L"[vk::swapchain] Selected format: B8G8R8A8_SRGB";
                return format;
            }
        }

        // Try any BGRA8 format
        for (const auto& format : formats) {
            if (format.format == vk::Format::eB8G8R8A8Unorm || format.format == vk::Format::eB8G8R8A8Srgb) {
                CASPAR_LOG(info) << L"[vk::swapchain] Selected format: " << static_cast<uint32_t>(format.format);
                return format;
            }
        }

        // Just use the first available format
        CASPAR_LOG(warning) << L"[vk::swapchain] Using first available format: "
                            << static_cast<uint32_t>(formats[0].format);
        return formats[0];
    }

    vk::PresentModeKHR choose_present_mode() { return vk::PresentModeKHR::eFifo; }

    vk::Extent2D choose_extent(const vk::SurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }

        int width = 0, height = 0;
        get_framebuffer_size_(width, height);

        vk::Extent2D extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        extent.width  = std::clamp(extent.width,  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return extent;
    }

    void create_swapchain()
    {
        vk::SurfaceCapabilitiesKHR capabilities = physical_device_.getSurfaceCapabilitiesKHR(surface_);

        auto surfaceFormat = choose_surface_format();
        auto presentMode   = choose_present_mode();
        auto extent        = choose_extent(capabilities);

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR createInfo{};
        createInfo.surface          = surface_;
        createInfo.minImageCount    = imageCount;
        createInfo.imageFormat      = surfaceFormat.format;
        createInfo.imageColorSpace  = surfaceFormat.colorSpace;
        createInfo.imageExtent      = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.preTransform     = capabilities.currentTransform;
        createInfo.compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        createInfo.presentMode      = presentMode;
        createInfo.clipped          = VK_TRUE;
        createInfo.oldSwapchain     = nullptr;

        swapchain_ = device_.createSwapchainKHR(createInfo);

        // Get swapchain images
        swapchain_images_ = device_.getSwapchainImagesKHR(swapchain_);

        swapchain_format_ = surfaceFormat.format;
        swapchain_extent_ = extent;
    }

    void create_image_views()
    {
        swapchain_image_views_.resize(swapchain_images_.size());

        for (size_t i = 0; i < swapchain_images_.size(); ++i) {
            vk::ImageViewCreateInfo createInfo{};
            createInfo.image                           = swapchain_images_[i];
            createInfo.viewType                        = vk::ImageViewType::e2D;
            createInfo.format                          = swapchain_format_;
            createInfo.components.r                    = vk::ComponentSwizzle::eIdentity;
            createInfo.components.g                    = vk::ComponentSwizzle::eIdentity;
            createInfo.components.b                    = vk::ComponentSwizzle::eIdentity;
            createInfo.components.a                    = vk::ComponentSwizzle::eIdentity;
            createInfo.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
            createInfo.subresourceRange.baseMipLevel   = 0;
            createInfo.subresourceRange.levelCount     = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount     = 1;

            swapchain_image_views_[i] = device_.createImageView(createInfo);
        }
    }

    void create_sync_objects()
    {
        image_available_semaphores_.resize(MAX_FRAMES_IN_FLIGHT);
        // One render-finished semaphore per swapchain image: the presentation
        // engine may hold onto it until the image is re-acquired, so we must
        // not reuse a semaphore for a different image before that happens.
        render_finished_semaphores_.resize(swapchain_images_.size());
        in_flight_fences_.resize(MAX_FRAMES_IN_FLIGHT);

        vk::SemaphoreCreateInfo semaphoreInfo{};

        vk::FenceCreateInfo fenceInfo{};
        fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            image_available_semaphores_[i] = device_.createSemaphore(semaphoreInfo);
            in_flight_fences_[i]           = device_.createFence(fenceInfo);
        }
        for (size_t i = 0; i < swapchain_images_.size(); ++i) {
            render_finished_semaphores_[i] = device_.createSemaphore(semaphoreInfo);
        }
    }

    void cleanup_swapchain()
    {
        for (auto imageView : swapchain_image_views_) {
            device_.destroyImageView(imageView);
        }
        swapchain_image_views_.clear();

        if (swapchain_) {
            device_.destroySwapchainKHR(swapchain_);
            swapchain_ = nullptr;
        }
    }

    void cleanup_sync_objects()
    {
        for (auto semaphore : image_available_semaphores_) {
            device_.destroySemaphore(semaphore);
        }
        for (auto semaphore : render_finished_semaphores_) {
            device_.destroySemaphore(semaphore);
        }
        for (auto fence : in_flight_fences_) {
            device_.destroyFence(fence);
        }
        image_available_semaphores_.clear();
        render_finished_semaphores_.clear();
        in_flight_fences_.clear();
    }

    void recreate()
    {
        int width = 0, height = 0;
        get_framebuffer_size_(width, height);
        while (width == 0 || height == 0) {
            if (wait_for_events_)
                wait_for_events_();
            get_framebuffer_size_(width, height);
        }

        device_.waitIdle();

        cleanup_swapchain();
        create_swapchain();
        create_image_views();

        CASPAR_LOG(info) << L"[vk::swapchain] Swapchain recreated (" << swapchain_extent_.width << L"x"
                         << swapchain_extent_.height << L")";
    }

    uint32_t acquire_next_image()
    {
        try {
            auto rv = device_.acquireNextImageKHR(
                swapchain_, std::numeric_limits<uint64_t>::max(), image_available_semaphores_[current_frame_], nullptr);
            return rv.value;
        } catch (const vk::OutOfDateKHRError&) {
            return std::numeric_limits<uint32_t>::max();
        }
    }

    bool present(uint32_t imageIndex)
    {
        vk::PresentInfoKHR presentInfo{};
        presentInfo.setWaitSemaphores(render_finished_semaphores_[imageIndex]);
        presentInfo.setSwapchains(swapchain_);
        presentInfo.setImageIndices(imageIndex);

        try {
            vk::Result result = queue_.presentKHR(presentInfo);
            // eSuboptimalKHR is reported as a non-success success-code; trigger recreation.
            return result == vk::Result::eSuccess;
        } catch (const vk::OutOfDateKHRError&) {
            return false;
        }
    }

    void wait_for_fence()
    {
        [[maybe_unused]] vk::Result result =
            device_.waitForFences(in_flight_fences_[current_frame_], VK_TRUE, std::numeric_limits<uint64_t>::max());
    }

    void reset_fence() { device_.resetFences(in_flight_fences_[current_frame_]); }

    void next_frame() { current_frame_ = (current_frame_ + 1) % MAX_FRAMES_IN_FLIGHT; }
};

swapchain::swapchain(vk::Instance                    instance,
                     vk::PhysicalDevice              physical_device,
                     vk::Device                      device,
                     vk::Queue                       queue,
                     uint32_t                        queue_family_index,
                     vk::SurfaceKHR                  surface,
                     bool                            vsync,
                     std::function<void(int&, int&)> get_framebuffer_size,
                     std::function<void()>           wait_for_events)
    : impl_(std::make_unique<impl>(instance,
                                   physical_device,
                                   device,
                                   queue,
                                   queue_family_index,
                                   surface,
                                   vsync,
                                   std::move(get_framebuffer_size),
                                   std::move(wait_for_events)))
{
}

swapchain::~swapchain() = default;

uint32_t swapchain::acquire_next_image() { return impl_->acquire_next_image(); }

bool swapchain::present(uint32_t image_index) { return impl_->present(image_index); }

vk::ImageView swapchain::get_image_view(uint32_t index) const { return impl_->swapchain_image_views_[index]; }

vk::Image swapchain::get_image(uint32_t index) const { return impl_->swapchain_images_[index]; }

uint32_t swapchain::image_count() const { return static_cast<uint32_t>(impl_->swapchain_images_.size()); }

void swapchain::get_extent(uint32_t& width, uint32_t& height) const
{
    width  = impl_->swapchain_extent_.width;
    height = impl_->swapchain_extent_.height;
}

vk::Format swapchain::format() const { return impl_->swapchain_format_; }

void swapchain::recreate() { impl_->recreate(); }

vk::Semaphore swapchain::image_available_semaphore() const
{
    return impl_->image_available_semaphores_[impl_->current_frame_];
}

vk::Semaphore swapchain::render_finished_semaphore(uint32_t image_index) const
{
    return impl_->render_finished_semaphores_[image_index];
}

uint32_t swapchain::frame_slot() const { return static_cast<uint32_t>(impl_->current_frame_); }

uint32_t swapchain::max_frames_in_flight() const { return MAX_FRAMES_IN_FLIGHT; }

vk::Fence swapchain::in_flight_fence() const { return impl_->in_flight_fences_[impl_->current_frame_]; }

void swapchain::wait_for_fence() { impl_->wait_for_fence(); }

void swapchain::reset_fence() { impl_->reset_fence(); }

void swapchain::next_frame() { impl_->next_frame(); }

}}} // namespace caspar::screen::vulkan
