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

#include "frame_side_data.h"
#include <common/except.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>

namespace caspar::core {

bool frame_side_data_include_on_duplicate_frames(frame_side_data_type t) noexcept
{
    switch (t) {
        case frame_side_data_type::a53_cc:
            return false;
    }
    CASPAR_THROW_EXCEPTION(programming_error() << msg_info("Assertion Failed: invalid frame_side_data_type value"));
}

struct mutable_frame_side_data::impl
{
    frame_side_data_type      type;
    std::vector<std::uint8_t> data;
    explicit impl(frame_side_data_type type, std::vector<std::uint8_t> data)
        : type(type)
        , data(std::move(data))
    {
    }
};

mutable_frame_side_data::mutable_frame_side_data(frame_side_data_type type, std::vector<std::uint8_t> data)
    : impl_(std::make_unique<impl>(type, std::move(data)))
{
}

mutable_frame_side_data::mutable_frame_side_data(const const_frame_side_data& side_data)
    : impl_(std::make_unique<impl>(*side_data.impl_))
{
}

mutable_frame_side_data::mutable_frame_side_data(mutable_frame_side_data&&) noexcept            = default;
mutable_frame_side_data& mutable_frame_side_data::operator=(mutable_frame_side_data&&) noexcept = default;
mutable_frame_side_data::~mutable_frame_side_data() noexcept                                    = default;

void mutable_frame_side_data::swap(mutable_frame_side_data& other) noexcept { impl_.swap(other.impl_); }

frame_side_data_type mutable_frame_side_data::type() const noexcept { return impl_->type; }

std::vector<std::uint8_t>&        mutable_frame_side_data::data() & noexcept { return impl_->data; }
const std::vector<std::uint8_t>&  mutable_frame_side_data::data() const& noexcept { return impl_->data; }
std::vector<std::uint8_t>&&       mutable_frame_side_data::data() && noexcept { return std::move(impl_->data); }
const std::vector<std::uint8_t>&& mutable_frame_side_data::data() const&& noexcept { return std::move(impl_->data); }

const_frame_side_data::const_frame_side_data(frame_side_data_type type, std::vector<std::uint8_t> data)
    : impl_(std::make_shared<impl>(type, std::move(data)))
{
}

void const_frame_side_data::swap(const_frame_side_data& other) noexcept { impl_.swap(other.impl_); }

frame_side_data_type const_frame_side_data::type() const noexcept { return impl_->type; }

const std::vector<std::uint8_t>& const_frame_side_data::data() const noexcept { return impl_->data; }

const_frame_side_data::const_frame_side_data(mutable_frame_side_data&& src)
    : impl_(std::move(src.impl_))
{
}

bool const_frame_side_data::operator==(const const_frame_side_data& other) const noexcept
{
    return impl_ == other.impl_;
}

bool const_frame_side_data::operator!=(const const_frame_side_data& other) const noexcept
{
    return impl_ != other.impl_;
}

bool const_frame_side_data::operator<(const const_frame_side_data& other) const noexcept { return impl_ < other.impl_; }

bool const_frame_side_data::operator>(const const_frame_side_data& other) const noexcept { return impl_ > other.impl_; }

const_frame_side_data::operator bool() const noexcept { return static_cast<bool>(impl_); }

struct frame_side_data_queue::impl final
{
    std::mutex lock;
    position   next_pos = 0;

    std::vector<const_frame_side_data> queue[max_frames()];
};

frame_side_data_queue::frame_side_data_queue()
    : impl_(std::make_unique<impl>())
{
}

frame_side_data_queue::~frame_side_data_queue() noexcept {}

frame_side_data_queue::position frame_side_data_queue::add_frame(std::vector<const_frame_side_data> side_data) noexcept
{
    auto lock = std::lock_guard(impl_->lock);
    auto pos  = impl_->next_pos++;

    impl_->queue[pos % max_frames()] = std::move(side_data);
    return pos;
}

std::optional<std::vector<const_frame_side_data>> frame_side_data_queue::get(position pos) const
{
    auto lock         = std::lock_guard(impl_->lock);
    auto [start, end] = valid_position_range_locked();
    if (pos >= start && pos < end) {
        return impl_->queue[pos % max_frames()];
    }
    return std::nullopt;
}

auto frame_side_data_queue::valid_position_range() const noexcept -> std::pair<position, position>
{
    auto lock = std::lock_guard(impl_->lock);
    return valid_position_range_locked();
}

auto frame_side_data_queue::valid_position_range_locked() const noexcept -> std::pair<position, position>
{
    if (impl_->next_pos > max_frames()) {
        return {impl_->next_pos - max_frames(), impl_->next_pos};
    }
    return {0, impl_->next_pos};
}
} // namespace caspar::core