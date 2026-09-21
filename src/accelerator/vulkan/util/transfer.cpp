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

#include "transfer.h"

#include "barrier.h"
#include "buffer.h"
#include "command_context.h"
#include "device.h"
#include "texture.h"

#include <common/future.h>
#include <common/log.h>

#include <cstring>

namespace caspar { namespace accelerator { namespace vulkan {

transfer::transfer(device& device)
    : device_(device)
    , ctx_(std::make_unique<command_context>(device.getVkDevice(), device.queue()))
{
}

transfer::~transfer() {}

std::future<std::shared_ptr<texture>>
transfer::copy_async(const array<const uint8_t>& source, int width, int height, int stride, common::bit_depth depth)
{
    std::shared_ptr<buffer> buf;

    auto tmp = source.storage<std::shared_ptr<buffer>>();
    if (tmp) {
        buf = *tmp;
    } else {
        buf = device_.create_buffer(static_cast<int>(source.size()), true);
        std::memcpy(buf->data(), source.data(), source.size());
    }

    auto tex = device_.create_texture(width, height, stride, depth);

    vk::BufferImageCopy region(0,
                               0,
                               0,
                               vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
                               vk::Offset3D(0, 0, 0),
                               vk::Extent3D(width, height, 1));

    ctx_->record_and_submit([&](vk::CommandBuffer cmd) {
        transitionImageLayout(tex->id(),
                              vk::ImageLayout::eUndefined,
                              vk::AccessFlagBits2::eNone,
                              vk::PipelineStageFlagBits2::eTopOfPipe,

                              vk::ImageLayout::eTransferDstOptimal,
                              vk::AccessFlagBits2::eTransferWrite,
                              vk::PipelineStageFlagBits2::eTransfer,
                              cmd);

        cmd.copyBufferToImage(buf->id(), tex->id(), vk::ImageLayout::eTransferDstOptimal, region);

        transitionImageLayout(tex->id(),
                              vk::ImageLayout::eTransferDstOptimal,
                              vk::AccessFlagBits2::eTransferWrite,
                              vk::PipelineStageFlagBits2::eTransfer,

                              vk::ImageLayout::eShaderReadOnlyOptimal,
                              vk::AccessFlagBits2::eShaderRead,
                              vk::PipelineStageFlagBits2::eFragmentShader,
                              cmd);
    });

    return make_ready_future(std::move(tex));
}

std::future<array<const uint8_t>> transfer::copy_async(const std::shared_ptr<texture>& source)
{
    auto buf = device_.create_buffer(source->size(), false);

    vk::CopyImageToBufferInfo2 copyInfo{};
    copyInfo.dstBuffer      = buf->id();
    copyInfo.srcImage       = source->id();
    copyInfo.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal;

    vk::BufferImageCopy2 region{};
    region.bufferOffset     = 0;
    region.imageSubresource = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
    region.imageOffset      = vk::Offset3D{0, 0, 0};
    region.imageExtent =
        vk::Extent3D{static_cast<uint32_t>(source->width()), static_cast<uint32_t>(source->height()), 1};
    copyInfo.setRegions(region);

    auto token = ctx_->record_and_submit([&](vk::CommandBuffer cmd) {
        transitionImageLayout(source->id(),
                              vk::ImageLayout::eRenderingLocalRead,
                              vk::AccessFlagBits2::eColorAttachmentWrite,
                              vk::PipelineStageFlagBits2::eColorAttachmentOutput,

                              vk::ImageLayout::eTransferSrcOptimal,
                              vk::AccessFlagBits2::eHostRead,
                              vk::PipelineStageFlagBits2::eHost,
                              cmd);
        cmd.copyImageToBuffer2(copyInfo);
    });

    return std::async(std::launch::deferred, [this, buf = std::move(buf), token]() mutable {
        if (!ctx_->wait(token)) {
            CASPAR_LOG(warning) << L"[Vulkan] Timeout waiting for readback semaphore";
        }

        auto ptr  = reinterpret_cast<uint8_t*>(buf->data());
        auto size = buf->size();
        return array<const uint8_t>(ptr, size, std::move(buf));
    });
}

}}} // namespace caspar::accelerator::vulkan
