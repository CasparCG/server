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

#include "frame_pool.h"

#include <utility>

namespace caspar { namespace accelerator { namespace ogl {

frame_pool::frame_pool(std::shared_ptr<frame_factory_gl> frame_factory, const void* tag, core::pixel_format_desc desc)
    : frame_factory_(std::move(frame_factory))
    , tag_(tag)
    , desc_(std::move(desc))
{
}

std::pair<core::mutable_frame, std::any&> frame_pool::create_frame()
{
    // TODO - ensure width and height match. If not then empty the pool?

    // TODO - is there risk of order issues with the atomics?

    std::shared_ptr<pooled_buffer> frame;

    // Find a frame which is not in use
    for (const std::shared_ptr<pooled_buffer>& candidate : pool_) {
        if (!candidate->in_use) {
            frame = candidate;
            break;
        }
    }

    // Add a new buffer to the pool
    if (!frame) {
        frame = std::make_shared<pooled_buffer>();

        for (const core::pixel_format_desc::plane& plane : desc_.planes) {
            frame->buffers.push_back(frame_factory_->create_buffer(plane.size));
        }

        pool_.push_back(frame);
    }

    frame->in_use = true;

    // Make copies of the buffers
    std::vector<caspar::array<uint8_t>> buffers;
    for (const caspar::array<std::uint8_t>& buffer : frame->buffers) {
        buffers.push_back(buffer.clone());
    }

    auto drop_hook = std::shared_ptr<void>(nullptr, [frame](void*) {
        // Mark as no longer in use, once the buffers are freed
        frame->in_use = false;
    });

    auto new_frame = frame_factory_->import_buffers(tag_, desc_, std::move(buffers), std::move(drop_hook));

    return std::make_pair(std::move(new_frame), std::ref(frame->data));
}

void frame_pool::for_each(const std::function<void(std::any& data)>& fn)
{
    for (std::shared_ptr<pooled_buffer>& candidate : pool_) {
        fn(candidate->data);
    }
}

}}} // namespace caspar::accelerator::ogl