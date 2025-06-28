/*
 * Copyright (c) 2025 Jacob Lifshay <programmerjake@gmail.com>
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

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

#include <boost/rational.hpp>

namespace caspar::core {

/** inspired by FFmpeg's `AVFrameSideDataType` */
enum class frame_side_data_type
{
    /** ATSC A53 Part 4 Closed Captions */
    a53_cc,
};

bool frame_side_data_include_on_duplicate_frames(frame_side_data_type t) noexcept;

class a53_cc_queue final
{
  public:
    // name from https://en.wikipedia.org/wiki/CTA-708#Closed_Caption_Data_Packet_(cc_data_pkt)
    typedef std::array<std::uint8_t, 3> cc_data_pkt;
    static_assert(sizeof(cc_data_pkt) == cc_data_pkt{}.size(), "cc_data_pkt must not have any padding");

    // pass in the frame rate, not to be confused with field rate
    explicit a53_cc_queue(boost::rational<int> frame_rate, bool interlaced);
    a53_cc_queue(const a53_cc_queue&)            = delete;
    a53_cc_queue(a53_cc_queue&&)                 = delete;
    a53_cc_queue& operator=(const a53_cc_queue&) = delete;
    a53_cc_queue& operator=(a53_cc_queue&&)      = delete;
    ~a53_cc_queue() noexcept;

    boost::rational<int> frame_rate() const noexcept;
    bool                 interlaced() const noexcept;
    std::size_t          max_field_size() const noexcept;
    std::size_t          max_frame_size() const noexcept;

    class locked final
    {
      public:
        explicit locked(a53_cc_queue& queue);

        void push_field(const std::vector<std::uint8_t>& field);
        /// get a single field worth of closed captions
        std::vector<std::uint8_t> pop_field();
        /// get a whole frame worth of closed captions. if interlacing is on, this gets two fields worth.
        std::vector<std::uint8_t> pop_frame();

        a53_cc_queue& queue() { return queue_; }

      private:
        a53_cc_queue&                queue_;
        std::unique_lock<std::mutex> lock_;
    };

    locked lock() { return locked(*this); }

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

class const_frame_side_data;

class mutable_frame_side_data final
{
    friend class const_frame_side_data;

  public:
    explicit mutable_frame_side_data(frame_side_data_type type, std::vector<std::uint8_t> data);
    mutable_frame_side_data(const const_frame_side_data&);
    mutable_frame_side_data(const mutable_frame_side_data&) = delete;
    mutable_frame_side_data(mutable_frame_side_data&&) noexcept;
    mutable_frame_side_data& operator=(const mutable_frame_side_data&) = delete;
    mutable_frame_side_data& operator=(mutable_frame_side_data&&) noexcept;
    ~mutable_frame_side_data() noexcept;

    void swap(mutable_frame_side_data& other) noexcept;

    frame_side_data_type type() const noexcept;

    std::vector<std::uint8_t>&        data() & noexcept;
    const std::vector<std::uint8_t>&  data() const& noexcept;
    std::vector<std::uint8_t>&&       data() && noexcept;
    const std::vector<std::uint8_t>&& data() const&& noexcept;

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

class const_frame_side_data final
{
    friend class mutable_frame_side_data;

  public:
    explicit const_frame_side_data(frame_side_data_type type, std::vector<std::uint8_t> data);
    const_frame_side_data(mutable_frame_side_data&&);
    const_frame_side_data(const const_frame_side_data&) noexcept            = default;
    const_frame_side_data(const_frame_side_data&&) noexcept                 = default;
    const_frame_side_data& operator=(const const_frame_side_data&) noexcept = default;
    const_frame_side_data& operator=(const_frame_side_data&&) noexcept      = default;
    ~const_frame_side_data() noexcept                                       = default;

    void swap(const_frame_side_data& other) noexcept;

    frame_side_data_type type() const noexcept;

    const std::vector<std::uint8_t>& data() const noexcept;

    bool operator==(const const_frame_side_data& other) const noexcept;
    bool operator!=(const const_frame_side_data& other) const noexcept;
    bool operator<(const const_frame_side_data& other) const noexcept;
    bool operator>(const const_frame_side_data& other) const noexcept;

    explicit operator bool() const noexcept;

  private:
    using impl = mutable_frame_side_data::impl;
    std::shared_ptr<impl> impl_;
};

/// a thread-safe queue
class frame_side_data_queue final
{
  public:
    typedef std::uint64_t position;

    frame_side_data_queue();
    frame_side_data_queue(const frame_side_data_queue&)            = delete;
    frame_side_data_queue(frame_side_data_queue&&)                 = delete;
    frame_side_data_queue& operator=(const frame_side_data_queue&) = delete;
    frame_side_data_queue& operator=(frame_side_data_queue&&)      = delete;
    ~frame_side_data_queue() noexcept;

    static constexpr std::size_t max_frames() noexcept { return 1 << 9; }

    /// Adds a new frame worth of side-data, incrementing the end position of `valid_position_range`.
    /// If that would make `valid_position_range` contain more than `max_frames` positions, then the oldest one is
    /// removed by incrementing the start position of `valid_position_range`.
    position add_frame(std::vector<const_frame_side_data> side_data) noexcept;

    std::optional<std::vector<const_frame_side_data>> get(position pos) const;

    /// position values in `.first <= pos < .second` are valid
    std::pair<position, position> valid_position_range() const noexcept;

  private:
    /// position values in `.first <= pos < .second` are valid
    std::pair<position, position> valid_position_range_locked() const noexcept;

    struct impl;
    std::unique_ptr<impl> impl_;
};

struct frame_side_data_in_queue final
{
    frame_side_data_queue::position        pos;
    std::shared_ptr<frame_side_data_queue> queue;

    std::optional<std::vector<const_frame_side_data>> get() const { return queue ? queue->get(pos) : std::nullopt; }
};

} // namespace caspar::core
