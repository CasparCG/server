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

#include <deque>
#include <functional>
#include <memory>
#include <mutex>

#include <vulkan/vulkan.hpp>

namespace caspar { namespace accelerator { namespace vulkan {

class vulkan_queue;

// A command pool + timeline semaphore + a deque of recycled one-time command
// buffers, bound to one vulkan_queue: record a one-time command buffer, submit
// it on the queue signalling the timeline, and return a completion_token. The
// timeline drives both buffer reclamation (reuse a buffer once the GPU passed
// its value) and the readback wait().
//
// Internally synchronized for the record path: one context is shared by every
// channel's transfer ops, so record_and_submit holds a mutex guarding the pool,
// the inflight ring and the timeline value. wait() is deliberately left
// lock-free — it only reads the semaphore via a self-contained completion_token,
// touching no mutable state, so a host readback can block without serializing
// other submitters. The owner must ensure this context's work is complete before
// destruction (the dtor does not wait) — call wait_idle() first.
class command_context final
{
  public:
    command_context(vk::Device device, std::shared_ptr<vulkan_queue> queue);
    ~command_context();

    command_context(const command_context&)            = delete;
    command_context& operator=(const command_context&) = delete;

    // Reuse-or-allocate a one-time command buffer, begin/record(fn)/end, and
    // submit it on the bound queue signalling this context's timeline.
    completion_token record_and_submit(const std::function<void(vk::CommandBuffer)>& record);

    // As above, but the submit first waits on the given completion tokens. Tokens
    // on THIS context's own timeline are dropped — same-queue ordering
    // (queue_distance::same_queue) is already covered by submission order plus the
    // producer's barriers; cross-queue tokens (a different timeline semaphore) become
    // timeline waits on the submit. This is the consumer half of a cross-queue
    // handoff(): the producer's completion_token flows in here so its work is
    // visible before the commands recorded by `record` run.
    completion_token record_and_submit(const std::function<void(vk::CommandBuffer)>& record,
                                       vk::ArrayProxy<const completion_token>        wait_tokens);

    // As above, and additionally signal timelines that are NOT this context's own
    // when the submit completes. The one caller today is the hardware-decode
    // import: FFmpeg hands out a frame together with its own timeline semaphore
    // and expects every user to wait it and signal it back at an incremented
    // value, which is how FFmpeg knows the surface is free to recycle. Passing
    // the foreign token in `signal_tokens` (semaphore + the value to signal) puts
    // that release on the same submit that read the surface, so nothing has to
    // block the CPU to keep the pool safe. Tokens on this context's own timeline
    // are dropped — that value is already signalled by the submit itself.
    completion_token record_and_submit(const std::function<void(vk::CommandBuffer)>& record,
                                       vk::ArrayProxy<const completion_token>        wait_tokens,
                                       vk::ArrayProxy<const completion_token>        signal_tokens);

    // Block until the token's value is reached (or timeout). True on success;
    // an empty token is already complete.
    bool wait(const completion_token& token, uint64_t timeout_ns = 1'000'000'000) const;

    // Block until every submission made on this context has completed, so the owner can destroy it
    // safely (the dtor itself does not wait — see class note). Waits only this context's own timeline,
    // not the whole device, so it is safe to call while other queues stay busy. No-op if nothing was
    // ever submitted.
    bool wait_idle(uint64_t timeout_ns = UINT64_MAX);

    // The completion of the most recent submit on this context ({timeline, last signalled value}; an
    // empty-valued token if nothing was ever submitted). A snapshot for polling drain progress
    // without blocking (vkGetSemaphoreCounterValue) — e.g. deferred command_context destruction.
    completion_token current_completion();

    // The queue this context records and submits on, so a producer can build a
    // hand-off (make_handoff(*ctx->queue(), render_queue, ...)) without a separate
    // lookup.
    const std::shared_ptr<vulkan_queue>& queue() const { return queue_; }

  private:
    struct inflight_command_buffer
    {
        vk::CommandBuffer cmd;
        uint64_t          value;
    };

    // Reuse the oldest retired command buffer, or allocate a fresh one.
    vk::CommandBuffer acquire_command_buffer();

    vk::Device                          device_;
    std::shared_ptr<vulkan_queue>       queue_;
    vk::CommandPool                     pool_;
    vk::Semaphore                       timeline_;
    uint64_t                            value_ = 0;
    std::deque<inflight_command_buffer> inflight_;
    std::mutex                          mutex_;
};

}}} // namespace caspar::accelerator::vulkan
