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

#include "completion_token.h"

#include <vulkan/vulkan.hpp>

namespace caspar { namespace accelerator { namespace vulkan {

class vulkan_queue;

// How a producer queue relates to a consumer queue for an EXCLUSIVE-image
// hand-off. A pure function of the two queues, decided once by startup queue
// assignment. Each tier is a monotonic superset of the one above it.
enum class queue_distance
{
    same_queue       = 0, // same VkQueue (incl. aliased): submission order suffices
    same_family      = 1, // same family, different queue: + timeline wait
    different_family = 2, // different family: + timeline wait + QFOT release/acquire
};

// A self-describing record of one producer->consumer image hand-off: the queue
// relationship, the layout the image moves through (old -> new), and each side's
// pipeline scope. record_release() (producer) and acquire_into() (consumer) both
// read it, so the release and acquire halves of a QFOT cannot drift. `completion`
// is the producer's timeline signal, filled in *after* the producer submits — the
// consumer feeds it into its own submit's wait_tokens (see
// command_context::record_and_submit's wait overload).
//
// A default / same_queue token is inert: bool() is false, record_release() still
// performs the plain layout transition (the image must reach new_layout either
// way), and acquire_into() does nothing.
struct handoff_token
{
    completion_token        completion;
    queue_distance          distance   = queue_distance::same_queue;
    uint32_t                src_family = vk::QueueFamilyIgnored;
    uint32_t                dst_family = vk::QueueFamilyIgnored;
    vk::ImageLayout         old_layout = vk::ImageLayout::eUndefined;
    vk::ImageLayout         new_layout = vk::ImageLayout::eUndefined;
    vk::PipelineStageFlags2 src_stage  = vk::PipelineStageFlagBits2::eNone; // producer's last use
    vk::AccessFlags2        src_access = vk::AccessFlagBits2::eNone;
    vk::PipelineStageFlags2 dst_stage  = vk::PipelineStageFlagBits2::eNone; // consumer's first use
    vk::AccessFlags2        dst_access = vk::AccessFlagBits2::eNone;

    // True when the hand-off needs cross-queue work (a timeline wait, and at
    // distance 2 the ownership barriers). False at distance 0.
    explicit operator bool() const { return distance != queue_distance::same_queue; }
};

// Classify producer vs consumer and capture the exchange's layouts + scopes into a
// token (with an empty `completion`, to be filled after the producer submits). The
// producer's last-use scope is (src_stage, src_access); the consumer's first-use
// scope is (dst_stage, dst_access); the image moves old_layout -> new_layout.
handoff_token make_handoff(const vulkan_queue&     producer,
                           const vulkan_queue&     consumer,
                           vk::ImageLayout         old_layout,
                           vk::ImageLayout         new_layout,
                           vk::PipelineStageFlags2 src_stage,
                           vk::AccessFlags2        src_access,
                           vk::PipelineStageFlags2 dst_stage,
                           vk::AccessFlags2        dst_access);

// Producer half. Record `image`'s final layout transition into `cmd`. Always
// performs old_layout -> new_layout; at distance 2 it is a queue-family RELEASE
// (ownership src_family -> dst_family, empty dst scope). Call this as the
// producer's last barrier on `image`, in place of a plain transitionImageLayout.
void record_release(vk::CommandBuffer cmd, const handoff_token& token, vk::Image image);

// Consumer half. At distance 2 record the matching QFOT ACQUIRE of `image` into
// `cmd` before the consumer's first use; at distance 0/1 record nothing (the
// producer's transition already moved the layout and — at distance 1 — the
// timeline wait orders it). The caller must separately add `token.completion` to
// the consumer submit's wait_tokens.
void acquire_into(vk::CommandBuffer cmd, const handoff_token& token, vk::Image image);

}}} // namespace caspar::accelerator::vulkan
