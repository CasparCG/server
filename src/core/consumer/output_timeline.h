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
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

namespace caspar::core {

// Timing published by the output that carries a channel's picture, so that other outputs of the
// same channel (for example an audio card) can align themselves with the picture as it actually
// leaves the machine, rather than with their own buffering.
//
// One instance is shared by every consumer of a channel through channel_info. Only one output
// publishes: the first to claim it, identified by its port index. Readers check
// video_delay_updates to tell whether anything is publishing.
struct output_timeline
{
    // Delay from the moment the most recently scheduled frame was handed to the output to the
    // moment it is displayed, in nanoseconds of the steady clock.
    std::atomic<double>        video_delay_ns{0.0};
    std::atomic<std::uint64_t> video_delay_updates{0};

    // Frames the publishing output reported as displayed late or dropped.
    std::atomic<std::uint64_t> late_frames{0};
    std::atomic<std::uint64_t> dropped_frames{0};

    // Returns true if port_index is now (or already was) the publisher.
    bool claim(int port_index)
    {
        int expected = no_publisher;
        return publisher_.compare_exchange_strong(expected, port_index) || expected == port_index;
    }

    void release(int port_index)
    {
        int expected = port_index;
        publisher_.compare_exchange_strong(expected, no_publisher);
    }

    [[nodiscard]] bool is_publisher(int port_index) const { return publisher_.load() == port_index; }

  private:
    static constexpr int no_publisher = -1;
    std::atomic<int>     publisher_{no_publisher};
};

// The steady-clock timestamp used for handover times, in nanoseconds.
inline std::int64_t output_timeline_now_ns()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

// Handover-to-display delay of one frame: the time it has already spent queued since handover,
// plus how far ahead of the card's current playback position its display slot is. display_time
// and stream_time are in the card's own time base (time_scale units per second).
inline double handover_to_display_ns(std::int64_t now_ns,
                                     std::int64_t handed_over_ns,
                                     std::int64_t display_time,
                                     std::int64_t stream_time,
                                     std::int64_t time_scale)
{
    const auto queued_ns = static_cast<double>(now_ns - handed_over_ns);
    const auto ahead_ns  = static_cast<double>(display_time - stream_time) * 1e9 / static_cast<double>(time_scale);
    return queued_ns + ahead_ns;
}

} // namespace caspar::core
