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

#include <common/array.h>

#include <cstdint>
#include <memory>

namespace caspar::core {

/** inspired by FFmpeg's `AVFrameSideDataType` */
enum class frame_side_data_type
{
    /** ATSC A53 Part 4 Closed Captions */
    a53_cc,
};

class mutable_frame_side_data final
{
    friend class const_frame_side_data;

  public:
    explicit mutable_frame_side_data(frame_side_data_type type, array<std::uint8_t> data);
    mutable_frame_side_data(const mutable_frame_side_data&) = delete;
    mutable_frame_side_data(mutable_frame_side_data&&) noexcept;
    mutable_frame_side_data& operator=(const mutable_frame_side_data&) = delete;
    mutable_frame_side_data& operator=(mutable_frame_side_data&&) noexcept;
    ~mutable_frame_side_data() noexcept;

    void swap(mutable_frame_side_data& other) noexcept;

    frame_side_data_type type() const noexcept;

    array<std::uint8_t>&        data() & noexcept;
    const array<std::uint8_t>&  data() const& noexcept;
    array<std::uint8_t>&&       data() && noexcept;
    const array<std::uint8_t>&& data() const&& noexcept;

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

class const_frame_side_data final
{
    friend class mutable_frame_side_data;

  public:
    explicit const_frame_side_data(frame_side_data_type type, array<std::uint8_t> data);
    const_frame_side_data(mutable_frame_side_data&&);
    const_frame_side_data(const const_frame_side_data&) noexcept            = default;
    const_frame_side_data(const_frame_side_data&&) noexcept                 = default;
    const_frame_side_data& operator=(const const_frame_side_data&) noexcept = default;
    const_frame_side_data& operator=(const_frame_side_data&&) noexcept      = default;
    ~const_frame_side_data() noexcept                                       = default;

    void swap(const_frame_side_data& other) noexcept;

    frame_side_data_type type() const noexcept;

    const array<std::uint8_t>& data() const noexcept;

    bool operator==(const const_frame_side_data& other) const noexcept;
    bool operator!=(const const_frame_side_data& other) const noexcept;
    bool operator<(const const_frame_side_data& other) const noexcept;
    bool operator>(const const_frame_side_data& other) const noexcept;

    explicit operator bool() const noexcept;

  private:
    using impl = mutable_frame_side_data::impl;
    std::shared_ptr<impl> impl_;
};

} // namespace caspar::core
