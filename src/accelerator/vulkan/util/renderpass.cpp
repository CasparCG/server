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

#include "renderpass.h"
#include "../image/image_kernel.h"
#include "device.h"
#include "pipeline.h"
#include "texture.h"

namespace caspar { namespace accelerator { namespace vulkan {

vk::Buffer renderpass::upload_vertex_buffers()
{
    uint32_t total_coords = 0;
    for (auto& layer : layers_) {
        layer.vertex_buffer_offset = total_coords * 6 * sizeof(float);
        total_coords += static_cast<uint32_t>(layer.coords.size());
    }
    std::vector<float> fl(total_coords * 6);

    size_t idx = 0;
    for (auto& layer : layers_) {
        for (auto& c : layer.coords) {
            fl[idx * 6 + 0] = static_cast<float>(c.vertex_x);
            fl[idx * 6 + 1] = static_cast<float>(c.vertex_y);
            fl[idx * 6 + 2] = static_cast<float>(c.texture_x);
            fl[idx * 6 + 3] = static_cast<float>(c.texture_y);
            fl[idx * 6 + 4] = static_cast<float>(c.texture_r);
            fl[idx * 6 + 5] = static_cast<float>(c.texture_q);
            ++idx;
        }
    }

    return _ctx->upload_vertex_data(fl);
}

renderpass::renderpass(frame_context* ctx, uint32_t width, uint32_t height)
    : _ctx(ctx)
    , _pipeline(ctx->get_pipeline())
    , _width(width)
    , _height(height)
    , _default_attachment(ctx->create_attachment(width, height, 4))
{
}

renderpass::~renderpass() {}

std::shared_ptr<texture> renderpass::create_attachment(uint32_t components_count)
{
    return _ctx->create_attachment(_width, _height, components_count);
}

void renderpass::draw(const draw_params& params)
{
    auto attachment         = params.background;
    auto [coords, uniforms] = _ctx->create_draw_data(params);

    if (coords.empty()) {
        return;
    }

    std::array<vk::ImageView, 7> textures = {params.textures[0]->view(),
                                             params.textures[0]->view(),
                                             params.textures[0]->view(),
                                             params.textures[0]->view(),
                                             attachment->view(),
                                             params.textures[0]->view(),
                                             params.textures[0]->view()};

    for (int n = 0; n < params.textures.size(); ++n) {
        textures[n] = params.textures[n]->view();
    }
    if (params.local_key) {
        textures[5] = params.local_key->view();
    }
    if (params.layer_key) {
        textures[6] = params.layer_key->view();
    }

    layers_.push_back({
        attachment,
        params.local_key,
        params.layer_key,
        std::move(textures),
        std::move(coords),
        uniforms,
    });
}

void renderpass::commit()
{
    auto vertex_buffer = upload_vertex_buffers();

    auto cmd_buffer = _ctx->get_command_buffer();
    cmd_buffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vk::ClearValue clearColor{vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f})};

    // Viewport and scissor
    vk::Viewport viewport{0.0f, 0.0f, static_cast<float>(_width), static_cast<float>(_height), 0.0f, 1.0f};

    vk::Extent2D extent = {_width, _height};
    vk::Rect2D   scissor{{0, 0}, extent};

    if (layers_.empty()) {
        // No layers, just clear the default attachment
        vk::RenderingAttachmentInfo attachment_info{};
        attachment_info.imageView   = _default_attachment->view();
        attachment_info.imageLayout = vk::ImageLayout::eRenderingLocalRead;
        attachment_info.loadOp      = vk::AttachmentLoadOp::eClear;
        attachment_info.storeOp     = vk::AttachmentStoreOp::eStore;

        vk::RenderingInfo rendering_info{};
        rendering_info.renderArea = scissor;
        rendering_info.layerCount = 1;
        rendering_info.setColorAttachments(attachment_info);

        cmd_buffer.beginRendering(rendering_info);
        cmd_buffer.setViewport(0, viewport);
        cmd_buffer.setScissor(0, scissor);
    } else {
        // create a renderpass for each layer
        bool                     default_cleared = false;
        std::shared_ptr<texture> previous_attachment;
        for (auto& layer : layers_) {
            if (layer.attachment != previous_attachment) {
                // We need to start a new render pass

                if (previous_attachment) {
                    // If this is not the first pass, end the previous render pass
                    cmd_buffer.endRendering();

                    if (previous_attachment != _default_attachment) {
                        // If we're done with a non-default attachment, we need to transition it to a shader read layout
                        vk::ImageMemoryBarrier2 memoryBarrier{};
                        auto range = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
                        memoryBarrier.subresourceRange = range;
                        memoryBarrier.srcStageMask     = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
                        memoryBarrier.srcAccessMask    = vk::AccessFlagBits2::eColorAttachmentWrite;
                        memoryBarrier.dstStageMask     = vk::PipelineStageFlagBits2::eFragmentShader;
                        memoryBarrier.dstAccessMask    = vk::AccessFlagBits2::eInputAttachmentRead;
                        memoryBarrier.oldLayout        = vk::ImageLayout::eRenderingLocalRead;
                        memoryBarrier.newLayout        = vk::ImageLayout::eShaderReadOnlyOptimal;
                        memoryBarrier.image            = previous_attachment->id();

                        vk::DependencyInfo dependencyInfo{};
                        dependencyInfo.setImageMemoryBarriers(memoryBarrier);
                        cmd_buffer.pipelineBarrier2(dependencyInfo);
                    }
                }

                // We only want to clear the default attachment once
                bool do_clear = (layer.attachment != _default_attachment) || !default_cleared;

                vk::RenderingAttachmentInfo attachment_info{};
                attachment_info.imageView   = layer.attachment->view();
                attachment_info.imageLayout = vk::ImageLayout::eRenderingLocalRead;
                attachment_info.loadOp      = do_clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
                attachment_info.storeOp     = vk::AttachmentStoreOp::eStore;

                if (layer.attachment == _default_attachment) {
                    default_cleared = true;
                }

                previous_attachment = layer.attachment;

                vk::RenderingInfo rendering_info{};
                rendering_info.renderArea = scissor;
                rendering_info.layerCount = 1;
                rendering_info.setColorAttachments(attachment_info);

                cmd_buffer.beginRendering(rendering_info);
                cmd_buffer.setViewport(0, viewport);
                cmd_buffer.setScissor(0, scissor);
            } else {
                // We are continuing in the same render pass, so we need a barrier to ensure the attachment is ready
                vk::MemoryBarrier2 memoryBarrier{};
                memoryBarrier.srcStageMask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
                memoryBarrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
                memoryBarrier.dstStageMask  = vk::PipelineStageFlagBits2::eFragmentShader;
                memoryBarrier.dstAccessMask = vk::AccessFlagBits2::eInputAttachmentRead;

                vk::DependencyInfo dependencyInfo{};
                dependencyInfo.dependencyFlags    = vk::DependencyFlagBits::eByRegion;
                dependencyInfo.memoryBarrierCount = 1;
                dependencyInfo.pMemoryBarriers    = &memoryBarrier;
                cmd_buffer.pipelineBarrier2(dependencyInfo);
            }

            _pipeline->draw(cmd_buffer,
                            vertex_buffer,
                            static_cast<uint32_t>(layer.coords.size()),
                            layer.vertex_buffer_offset,
                            layer.uniforms,
                            layer.textures);
        }
    }

    cmd_buffer.endRendering();
    cmd_buffer.end();

    _ctx->submit();
}

}}} // namespace caspar::accelerator::vulkan
