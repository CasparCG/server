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

#include <common/array.h>
#include <common/bit_depth.h>

#include <future>
#include <memory>

namespace caspar { namespace accelerator { namespace vulkan {

class device;
class texture;
class command_context;

// The shared host<->device copy service: one instance per device, used by every
// channel's image_mixer for plane uploads (h->d) and the final frame readback
// (d->h). It records onto its own command_context — whose record mutex serializes
// the many channel threads hitting it — and submits on the device's shared queue;
// allocation of the staging buffers and destination textures is delegated back to
// the device's pools. The CPU blocks only on readback (.get()), never on upload.
class transfer final
{
  public:
    explicit transfer(device& device);
    ~transfer();

    transfer(const transfer&)            = delete;
    transfer& operator=(const transfer&) = delete;

    // Upload (h->d). Records + submits synchronously on the caller's thread and
    // returns a ready future: no host readback, so no CPU wait — GPU consumers of
    // the texture are ordered by the memory barriers, not by the CPU.
    std::future<std::shared_ptr<texture>>
    copy_async(const array<const uint8_t>& source, int width, int height, int stride, common::bit_depth depth);

    // Readback (d->h). Records + submits synchronously; the returned deferred
    // future blocks at .get() on the completion wait — the one legitimate CPU
    // wait, because the caller is consuming bytes.
    std::future<array<const uint8_t>> copy_async(const std::shared_ptr<texture>& source);

  private:
    device&                          device_;
    std::unique_ptr<command_context> ctx_;
};

}}} // namespace caspar::accelerator::vulkan
