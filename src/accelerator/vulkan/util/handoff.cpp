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

#include "handoff.h"

#include "barrier.h"
#include "vulkan_queue.h"

namespace caspar { namespace accelerator { namespace vulkan {

namespace {
queue_distance classify(const vulkan_queue& producer, const vulkan_queue& consumer)
{
    if (producer.vk_queue() == consumer.vk_queue())
        return queue_distance::same_queue;
    if (producer.family_index() == consumer.family_index())
        return queue_distance::same_family;
    return queue_distance::different_family;
}
} // namespace

handoff_token make_handoff(const vulkan_queue&     producer,
                           const vulkan_queue&     consumer,
                           vk::ImageLayout         old_layout,
                           vk::ImageLayout         new_layout,
                           vk::PipelineStageFlags2 src_stage,
                           vk::AccessFlags2        src_access,
                           vk::PipelineStageFlags2 dst_stage,
                           vk::AccessFlags2        dst_access)
{
    handoff_token token;
    token.distance   = classify(producer, consumer);
    token.src_family = producer.family_index();
    token.dst_family = consumer.family_index();
    token.old_layout = old_layout;
    token.new_layout = new_layout;
    token.src_stage  = src_stage;
    token.src_access = src_access;
    token.dst_stage  = dst_stage;
    token.dst_access = dst_access;
    return token;
}

void record_release(vk::CommandBuffer cmd, const handoff_token& token, vk::Image image)
{
    if (token.distance == queue_distance::different_family) {
        // Release half: producer's last-use src scope, empty dst scope (the
        // matching acquire on the consumer supplies the dst scope).
        transitionImageLayoutQFOT(image,
                                  token.old_layout,
                                  token.src_access,
                                  token.src_stage,
                                  token.new_layout,
                                  vk::AccessFlagBits2::eNone,
                                  vk::PipelineStageFlagBits2::eNone,
                                  token.src_family,
                                  token.dst_family,
                                  cmd);
        return;
    }

    // Distance 0/1: a plain transition. The image must reach new_layout regardless;
    // at distance 0 the producer's dst scope orders the same-queue consumer, at
    // distance 1 the consumer's timeline wait does (no ownership transfer needed).
    transitionImageLayout(image,
                          token.old_layout,
                          token.src_access,
                          token.src_stage,
                          token.new_layout,
                          token.dst_access,
                          token.dst_stage,
                          cmd);
}

void acquire_into(vk::CommandBuffer cmd, const handoff_token& token, vk::Image image)
{
    if (token.distance != queue_distance::different_family)
        return; // distance 0/1: producer transition + timeline wait suffice.

    // Acquire half: empty src scope, consumer's first-use dst scope. Mirrors the
    // release exactly (same layouts, families, range) so the pair is valid.
    transitionImageLayoutQFOT(image,
                              token.old_layout,
                              vk::AccessFlagBits2::eNone,
                              vk::PipelineStageFlagBits2::eNone,
                              token.new_layout,
                              token.dst_access,
                              token.dst_stage,
                              token.src_family,
                              token.dst_family,
                              cmd);
}

}}} // namespace caspar::accelerator::vulkan
