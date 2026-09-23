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

#include "image_kernel.h"

#include "../util/barrier.h"
#include "../util/command_context.h"
#include "../util/descriptor_pool.h"
#include "../util/device.h"
#include "../util/pipeline.h"
#include "../util/renderpass.h"
#include "../util/texture.h"

#include <common/assert.h>

#include <core/frame/frame_transform.h>
#include <core/frame/pixel_format.h>

#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>

#include <array>
#include <cmath>
#include <memory>

namespace caspar::accelerator::vulkan {

float get_precision_factor(common::bit_depth depth)
{
    switch (depth) {
        case common::bit_depth::bit8:
            return 1.0f;
        case common::bit_depth::bit10:
            return 64.0f;
        case common::bit_depth::bit12:
            return 16.0f;
        case common::bit_depth::bit16:
        default:
            return 1.0f;
    }
}

bool is_above_screen(double y) { return y < 0.0; }

bool is_below_screen(double y) { return y > 1.0; }

bool is_left_of_screen(double x) { return x < 0.0; }

bool is_right_of_screen(double x) { return x > 1.0; }

bool is_outside_screen(const std::vector<core::frame_geometry::coord>& coords)
{
    auto x_coords =
        coords | boost::adaptors::transformed([](const core::frame_geometry::coord& c) { return c.vertex_x; });
    auto y_coords =
        coords | boost::adaptors::transformed([](const core::frame_geometry::coord& c) { return c.vertex_y; });

    return boost::algorithm::all_of(x_coords, &is_left_of_screen) ||
           boost::algorithm::all_of(x_coords, &is_right_of_screen) ||
           boost::algorithm::all_of(y_coords, &is_above_screen) || boost::algorithm::all_of(y_coords, &is_below_screen);
}

static const double epsilon = 0.001;

static const uint32_t frame_buffer_size = 3;

struct image_kernel::impl
{
    spl::shared_ptr<device>   vulkan_;
    common::bit_depth         depth_;
    std::shared_ptr<pipeline> pipeline_;

    // Output/intermediate render targets, recycled per (width,height). Kept in a
    // shared_ptr so a recycled texture's deleter can outlive this kernel without a
    // dangling pool (concurrent_unordered_map keeps element references stable).
    using attachment_queue_t = tbb::concurrent_bounded_queue<std::shared_ptr<texture>>;
    std::shared_ptr<tbb::concurrent_unordered_map<size_t, attachment_queue_t>> attachment_pools_;

    struct frame_data : public frame_context
    {
        image_kernel::impl* parent = nullptr;

        vk::Buffer       buffer = nullptr;
        void*            data   = nullptr;
        vk::DeviceMemory memory = nullptr;
        size_t           size   = 0;

        // Set by the last submit that used this slot; create_renderpass waits on
        // it before reusing the slot (keeps its vertex buffer alive until the GPU
        // is done). Replaces the per-frame VkFence — same completion info, off the
        // command_context timeline.
        completion_token token;

        // Per-slot descriptor sets, recycled on the same completion as the vertex
        // buffer: create_renderpass waits the token before this slot is reused, so
        // allocate() can safely reset the pool.
        descriptor_pool desc_pool_;

        explicit frame_data(image_kernel::impl* parent)
            : parent(parent)
            , desc_pool_(parent->vulkan_->getVkDevice(),
                         parent->pipeline_->descriptor_set_layout(),
                         parent->pipeline_->descriptor_pool_sizes())
        {
        }

        virtual vk::Buffer upload_vertex_data(const std::vector<float>& src)
        {
            return parent->upload_vertex_buffer(*this, (void*)src.data(), src.size() * sizeof(float));
        }
        virtual draw_data create_draw_data(const draw_params& params) { return parent->draw(params); }
        virtual std::shared_ptr<class pipeline> get_pipeline() { return parent->pipeline_; }
        virtual std::vector<vk::DescriptorSet>  allocate_descriptor_sets(uint32_t count)
        {
            return desc_pool_.allocate(count);
        }
        virtual void record_and_submit(const std::function<void(vk::CommandBuffer)>& record)
        {
            token = parent->cmd_ctx_.record_and_submit(record);
        }
        virtual std::shared_ptr<class texture>
        create_attachment(uint32_t width, uint32_t height, uint32_t components_count)
        {
            return parent->create_attachment(width, height, components_count);
        }
    };

    // Private to this kernel's (channel's) thread: command buffers are recorded
    // lock-free here; only the queue submit serializes (vulkan_queue's mutex).
    command_context cmd_ctx_;
    frame_data      frames_[frame_buffer_size];
    uint32_t        current_frame_index_ = 0;

    explicit impl(const spl::shared_ptr<device>& vulkan, common::bit_depth depth)
        : vulkan_(vulkan)
        , depth_(depth)
        , pipeline_(std::make_shared<pipeline>(vulkan->getVkDevice(),
                                               depth == common::bit_depth::bit8 ? vk::Format::eR8G8B8A8Unorm
                                                                                : vk::Format::eR16G16B16A16Unorm))
        , attachment_pools_(std::make_shared<tbb::concurrent_unordered_map<size_t, attachment_queue_t>>())
        , cmd_ctx_(vulkan->getVkDevice(), vulkan->queue())
        , frames_{frame_data{this}, frame_data{this}, frame_data{this}}
    {
    }

    ~impl()
    {
        auto vk_device = vulkan_->getVkDevice();

        // command_context's dtor requires the device idle (it does not waitIdle);
        // the in-flight slots may still reference these vertex buffers.
        vk_device.waitIdle();

        for (auto& frame : frames_) {
            if (frame.buffer) {
                vk_device.unmapMemory(frame.memory);
                vk_device.destroyBuffer(frame.buffer);
                vk_device.freeMemory(frame.memory);
            }
        }
    }

    spl::shared_ptr<renderpass> create_renderpass(uint32_t width, uint32_t height)
    {
        auto& ctx = frames_[(++current_frame_index_) % frame_buffer_size];

        // Wait until the previous use of this slot has completed on the GPU before
        // reusing its vertex buffer (bounds in-flight frames to frame_buffer_size).
        if (ctx.token && !cmd_ctx_.wait(ctx.token)) {
            CASPAR_LOG(warning) << L"[Vulkan image_kernel] Timeout waiting for frame completion";
        }

        return spl::make_shared<renderpass>(&ctx, width, height);
    }

    uint32_t findDedicatedMemoryType(uint32_t typeMask, vk::MemoryPropertyFlags properties)
    {
        auto memProperties = vulkan_->getMemoryProperties();
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
            if ((typeMask & (1 << i)) && ((memProperties.memoryTypes[i].propertyFlags & properties) == properties)) {
                return i;
            }
        }
        throw std::runtime_error("[Vulkan image_kernel] Failed to find suitable memory type");
    }

    vk::Buffer upload_vertex_buffer(frame_data& vb, void* data, size_t size)
    {
        if (vb.size < size) {
            auto vk_device = vulkan_->getVkDevice();

            if (vb.buffer) {
                vk_device.unmapMemory(vb.memory);
                vk_device.destroyBuffer(vb.buffer);
                vk_device.freeMemory(vb.memory);
            }

            // staging buffer
            vk::BufferCreateInfo stagingInfo{};
            stagingInfo.size        = size;
            stagingInfo.usage       = vk::BufferUsageFlagBits::eVertexBuffer;
            stagingInfo.sharingMode = vk::SharingMode::eExclusive;

            vb.buffer = vk_device.createBuffer(stagingInfo);

            auto stagingMemReq = vk_device.getBufferMemoryRequirements(vb.buffer);

            vk::MemoryAllocateInfo stagingAlloc{};
            stagingAlloc.allocationSize  = stagingMemReq.size;
            stagingAlloc.memoryTypeIndex = findDedicatedMemoryType(stagingMemReq.memoryTypeBits,
                                                                   vk::MemoryPropertyFlagBits::eHostVisible |
                                                                       vk::MemoryPropertyFlagBits::eHostCoherent);

            vb.memory = vk_device.allocateMemory(stagingAlloc);
            vk_device.bindBufferMemory(vb.buffer, vb.memory, 0);

            vb.data = vk_device.mapMemory(vb.memory, 0, size);
            vb.size = size;
        }
        memcpy(vb.data, data, size);

        return vb.buffer;
    }

    std::shared_ptr<texture> create_attachment(uint32_t width, uint32_t height, uint32_t components_count)
    {
        CASPAR_VERIFY(width > 0 && height > 0);

        auto format = depth_ == common::bit_depth::bit8 ? vk::Format::eR8G8B8A8Unorm : vk::Format::eR16G16B16A16Unorm;
        auto vk_device = vulkan_->getVkDevice();

        auto pool   = &(*attachment_pools_)[(width << 16 & 0xFFFF0000) | (height & 0x0000FFFF)];
        auto extent = vk::Extent3D{width, height, 1};

        std::shared_ptr<texture> tex;
        if (!pool->try_pop(tex)) {
            vk::ImageCreateInfo imageInfo{};
            imageInfo.imageType     = vk::ImageType::e2D;
            imageInfo.format        = format;
            imageInfo.extent        = extent;
            imageInfo.mipLevels     = 1;
            imageInfo.arrayLayers   = 1;
            imageInfo.initialLayout = vk::ImageLayout::eUndefined;
            imageInfo.samples       = vk::SampleCountFlagBits::e1;
            imageInfo.tiling        = vk::ImageTiling::eOptimal;
            imageInfo.usage         = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eInputAttachment |
                              vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst |
                              vk::ImageUsageFlagBits::eSampled;
            imageInfo.sharingMode = vk::SharingMode::eExclusive;
            auto image            = vk_device.createImage(imageInfo);

            auto memReq = vk_device.getImageMemoryRequirements(image);

            vk::MemoryAllocateInfo allocInfo{};
            allocInfo.allocationSize = memReq.size;
            allocInfo.memoryTypeIndex =
                findDedicatedMemoryType(memReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

            auto imageMemory = vk_device.allocateMemory(allocInfo);
            vk_device.bindImageMemory(image, imageMemory, 0);
            auto range = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

            vk::ImageViewCreateInfo createInfo(
                {}, image, vk::ImageViewType::e2D, format, vk::ComponentMapping(), range);

            auto imageView = vk_device.createImageView(createInfo);

            tex = std::make_shared<texture>(
                width, height, components_count, depth_, image, imageMemory, imageView, vk_device);
        }

        cmd_ctx_.record_and_submit([&](vk::CommandBuffer cmd) {
            transitionImageLayout(
                tex->id(),
                vk::ImageLayout::eUndefined,
                vk::AccessFlagBits2::eNone,
                vk::PipelineStageFlagBits2::eTopOfPipe,
                vk::ImageLayout::eRenderingLocalRead,
                vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eInputAttachmentRead,
                vk::PipelineStageFlagBits2::eColorAttachmentOutput | vk::PipelineStageFlagBits2::eFragmentShader,
                cmd);
        });

        tex->set_depth(depth_);

        auto ptr = tex.get();
        return std::shared_ptr<texture>(
            ptr, [tex = std::move(tex), pool, pools = attachment_pools_](texture*) mutable { pool->push(tex); });
    }

    std::pair<std::vector<core::frame_geometry::coord>, uniform_block> draw(const draw_params& params)
    {
        CASPAR_ASSERT(params.pix_desc.planes.size() == params.textures.size());

        if (params.textures.empty() || !params.background) {
            return {};
        }

        if (params.transforms.image_transform.opacity < epsilon) {
            return {};
        }

        if (params.geometry.data().empty()) {
            return {};
        }

        auto coords     = params.geometry.data();
        auto transforms = params.transforms;

        auto const first_plane = params.pix_desc.planes.at(0);
        if (params.geometry.mode() != core::frame_geometry::scale_mode::stretch && first_plane.width > 0 &&
            first_plane.height > 0) {
            auto width_scale  = static_cast<double>(params.target_width) / static_cast<double>(first_plane.width);
            auto height_scale = static_cast<double>(params.target_height) / static_cast<double>(first_plane.height);

            core::image_transform transform;
            double                target_scale;
            switch (params.geometry.mode()) {
                case core::frame_geometry::scale_mode::fit:
                    target_scale = std::min(width_scale, height_scale);

                    transform.fill_scale[0] *= target_scale / width_scale;
                    transform.fill_scale[1] *= target_scale / height_scale;
                    break;

                case core::frame_geometry::scale_mode::fill:
                    target_scale = std::max(width_scale, height_scale);
                    transform.fill_scale[0] *= target_scale / width_scale;
                    transform.fill_scale[1] *= target_scale / height_scale;
                    break;

                case core::frame_geometry::scale_mode::original:
                    transform.fill_scale[0] /= width_scale;
                    transform.fill_scale[1] /= height_scale;
                    break;

                case core::frame_geometry::scale_mode::hfill:
                    transform.fill_scale[1] *= width_scale / height_scale;
                    break;

                case core::frame_geometry::scale_mode::vfill:
                    transform.fill_scale[0] *= height_scale / width_scale;
                    break;

                default:;
            }

            transforms = transforms.combine_transform(transform, params.aspect_ratio);
        }

        coords = transforms.transform_coords(coords);

        // Skip drawing if all the coordinates will be outside the screen.
        if (coords.size() < 3 || is_outside_screen(coords)) {
            return {};
        }

        uniform_block uniforms;

        for (int n = 0; n < params.textures.size(); ++n) {
            uniforms.precision_factor[n] = get_precision_factor(params.textures[n]->depth());
        }

        const auto color_space     = core::decode_color_space(params.pix_desc);
        uniforms.color_space_index = static_cast<uint32_t>(color_space);

        if (params.pix_desc.is_straight_alpha) {
            uniforms.flags |= static_cast<uint32_t>(shader_flags::is_straight_alpha);
        }

        if (static_cast<bool>(params.local_key)) {
            uniforms.flags |= static_cast<uint32_t>(shader_flags::has_local_key);
        }
        if (static_cast<bool>(params.layer_key)) {
            uniforms.flags |= static_cast<uint32_t>(shader_flags::has_layer_key);
        }
        uniforms.pixel_format = static_cast<uint32_t>(params.pix_desc.format);

        uniforms.opacity =
            transforms.image_transform.is_key ? 1.0f : static_cast<float>(transforms.image_transform.opacity);

        if (transforms.image_transform.chroma.enable) {
            uniforms.flags |= static_cast<uint32_t>(shader_flags::chroma);

            if (transforms.image_transform.chroma.show_mask)
                uniforms.flags |= static_cast<uint32_t>(shader_flags::chroma_show_mask);

            uniforms.chroma_target_hue     = static_cast<float>(transforms.image_transform.chroma.target_hue) / 360.0f;
            uniforms.chroma_hue_width      = static_cast<float>(transforms.image_transform.chroma.hue_width);
            uniforms.chroma_min_saturation = static_cast<float>(transforms.image_transform.chroma.min_saturation);
            uniforms.chroma_min_brightness = static_cast<float>(transforms.image_transform.chroma.min_brightness);
            uniforms.chroma_softness       = 1.0f + static_cast<float>(transforms.image_transform.chroma.softness);
            uniforms.chroma_spill_suppress =
                static_cast<float>(transforms.image_transform.chroma.spill_suppress) / 360.0f;
            uniforms.chroma_spill_suppress_saturation =
                static_cast<float>(transforms.image_transform.chroma.spill_suppress_saturation);
        }

        // Setup blend_func
        auto blend_mode = params.blend_mode;
        if (transforms.image_transform.is_key) {
            blend_mode = core::blend_mode::normal;
        }

        uniforms.blend_mode = static_cast<uint32_t>(blend_mode);
        uniforms.keyer      = static_cast<uint32_t>(params.keyer);

        if (transforms.image_transform.invert) {
            uniforms.flags |= static_cast<uint32_t>(shader_flags::invert);
        }

        if (transforms.image_transform.levels.min_input > epsilon ||
            transforms.image_transform.levels.max_input < 1.0 - epsilon ||
            transforms.image_transform.levels.min_output > epsilon ||
            transforms.image_transform.levels.max_output < 1.0 - epsilon ||
            std::abs(transforms.image_transform.levels.gamma - 1.0) > epsilon) {
            uniforms.flags |= static_cast<uint32_t>(shader_flags::levels);
            uniforms.min_input  = static_cast<float>(transforms.image_transform.levels.min_input);
            uniforms.max_input  = static_cast<float>(transforms.image_transform.levels.max_input);
            uniforms.min_output = static_cast<float>(transforms.image_transform.levels.min_output);
            uniforms.max_output = static_cast<float>(transforms.image_transform.levels.max_output);
            uniforms.gamma      = static_cast<float>(transforms.image_transform.levels.gamma);
        }

        if (std::abs(transforms.image_transform.brightness - 1.0) > epsilon ||
            std::abs(transforms.image_transform.saturation - 1.0) > epsilon ||
            std::abs(transforms.image_transform.contrast - 1.0) > epsilon) {
            uniforms.flags |= static_cast<uint32_t>(shader_flags::csb);

            uniforms.brt = static_cast<float>(transforms.image_transform.brightness);
            uniforms.sat = static_cast<float>(transforms.image_transform.saturation);
            uniforms.con = static_cast<float>(transforms.image_transform.contrast);
        }

        return {std::move(coords), uniforms};
    }
};

image_kernel::image_kernel(const spl::shared_ptr<device>& device, common::bit_depth depth)
    : impl_(new impl(device, depth))
{
}
image_kernel::~image_kernel() {}

spl::shared_ptr<renderpass> image_kernel::create_renderpass(uint32_t width, uint32_t height)
{
    return impl_->create_renderpass(width, height);
}

} // namespace caspar::accelerator::vulkan
