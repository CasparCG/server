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
#include <boost/rational.hpp>
#include <common/assert.h>
#include <common/except.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <vector>

namespace caspar::core {

struct a53_cc_queue::impl final
{
    enum class cc_type_t : std::uint8_t
    {
        NTSC_CC_FIELD_1    = 0,
        NTSC_CC_FIELD_2    = 1,
        DTVCC_PACKET_DATA  = 2,
        DTVCC_PACKET_START = 3,
    };
    struct rate_limiter final
    {
        const std::size_t               whole_count_per_tick_;
        const boost::rational<unsigned> fractional_count_per_tick_;
        unsigned                        remainder_numerator_;
        explicit constexpr rate_limiter(boost::rational<unsigned> count_per_tick)
            : whole_count_per_tick_(boost::rational_cast<std::size_t>(count_per_tick))
            , fractional_count_per_tick_(count_per_tick.numerator() % count_per_tick.denominator(),
                                         count_per_tick.denominator())
            , remainder_numerator_(fractional_count_per_tick_.denominator() - 1U)
        {
        }
        constexpr std::size_t max_per_tick(unsigned ticks = 1) const noexcept
        {
            auto product       = fractional_count_per_tick_ * ticks;
            auto floor_product = boost::rational_cast<unsigned>(product);
            auto fract_product = product - floor_product;
            return whole_count_per_tick_ * ticks + floor_product + (fract_product != 0);
        }
        constexpr std::size_t tick(std::size_t available)
        {
            std::size_t retval = whole_count_per_tick_;

            auto next_remainder_numerator =
                static_cast<unsigned long long>(remainder_numerator_) + fractional_count_per_tick_.numerator();
            if (next_remainder_numerator >= fractional_count_per_tick_.denominator()) {
                retval++;
                next_remainder_numerator -= fractional_count_per_tick_.denominator();
            }
            if (available < retval) {
                remainder_numerator_ = fractional_count_per_tick_.denominator();
                return available;
            }
            remainder_numerator_ = next_remainder_numerator;
            return retval;
        }
    };
    static inline constexpr unsigned total_cc_data_bit_rate   = 9600;
    static inline constexpr unsigned eia_608_cc_data_bit_rate = 960;
    static inline constexpr unsigned cc_data_bits_per_pkt     = 16;

    const boost::rational<int> frame_rate_;
    // closed captions ignores whether the frame rate is drop-frame or not
    const boost::rational<unsigned>  non_drop_frame_rate_;
    const bool                       interlaced_;
    std::mutex                       mutex_;
    rate_limiter                     eia_608_pkts_per_field_limiter_;
    std::queue<cc_data_pkt>          eia_608_pkts_;
    bool                             last_popped_eia_608_was_second_field_ = interlaced_;
    rate_limiter                     total_pkts_per_field_limiter_;
    std::queue<cc_data_pkt>          cta_708_pkts_;
    bool                             did_first_pop_ = false;
    static boost::rational<unsigned> get_non_drop_frame_rate(boost::rational<int> frame_rate)
    {
        CASPAR_ENSURE(frame_rate > 0);
        auto ceil_frame_rate = frame_rate;
        if (ceil_frame_rate.denominator() != 1) {
            ceil_frame_rate = boost::rational_cast<int>(frame_rate) + 1;
        }
        // return ceil(frame_rate) to round off the drop-frame part of the frame rate
        return boost::rational<unsigned>(ceil_frame_rate.numerator(), ceil_frame_rate.denominator());
    }
    boost::rational<unsigned> pkts_per_field(unsigned cc_data_bit_rate) const
    {
        boost::rational<unsigned> non_drop_field_rate = non_drop_frame_rate_;
        if (interlaced_) {
            non_drop_field_rate *= 2U;
        }
        return boost::rational<unsigned>(cc_data_bit_rate, cc_data_bits_per_pkt) / non_drop_field_rate;
    }
    explicit impl(boost::rational<int> frame_rate, bool interlaced)
        : frame_rate_(frame_rate)
        , interlaced_(interlaced)
        , non_drop_frame_rate_(get_non_drop_frame_rate(frame_rate))
        , eia_608_pkts_per_field_limiter_(pkts_per_field(eia_608_cc_data_bit_rate))
        , total_pkts_per_field_limiter_(pkts_per_field(total_cc_data_bit_rate))
    {
    }
    void push_field_locked(const std::vector<std::uint8_t>& field)
    {
        // intentionally ignore any remainder to avoid out-of-bounds access
        std::size_t pkt_count = field.size() / cc_data_pkt{}.size();
        for (std::size_t i = 0; i < pkt_count; i++) {
            cc_data_pkt pkt{};
            std::memcpy(pkt.data(), &field[i * cc_data_pkt{}.size()], cc_data_pkt{}.size());

            // Based on: https://en.wikipedia.org/wiki/CTA-708#Closed_Caption_Data_Packet_(cc_data_pkt)
            bool cc_valid = (pkt[0] & 0x04) != 0;
            if (!cc_valid)
                continue;
            auto cc_type = static_cast<cc_type_t>(pkt[0] & 0x03);
            switch (cc_type) {
                case cc_type_t::NTSC_CC_FIELD_1:
                case cc_type_t::NTSC_CC_FIELD_2:
                    // ignore padding, since we end up with too much when reducing frame rate
                    if (pkt[1] != 0x80 || pkt[2] != 0x80) {
                        eia_608_pkts_.push(pkt);
                    }
                    break;
                case cc_type_t::DTVCC_PACKET_DATA:
                case cc_type_t::DTVCC_PACKET_START:
                    cta_708_pkts_.push(pkt);
                    break;
            }
        }
    }
    cc_data_pkt pop_eia_608_pkt_for_field_locked()
    {
        bool second_field                     = !last_popped_eia_608_was_second_field_ && interlaced_;
        last_popped_eia_608_was_second_field_ = second_field;

        if (!eia_608_pkts_.empty()) {
            bool pop;
            auto cc_type = static_cast<cc_type_t>(eia_608_pkts_.front()[0] & 0x03);
            if (cc_type == cc_type_t::NTSC_CC_FIELD_1) {
                pop = !second_field;
            } else {
                CASPAR_ENSURE(cc_type == cc_type_t::NTSC_CC_FIELD_2);
                pop = second_field || !interlaced_;
            }
            if (pop) {
                auto pkt = eia_608_pkts_.front();
                eia_608_pkts_.pop();
                return pkt;
            }
        }
        if (second_field) {
            return {0xFD, 0x80, 0x80};
        }
        return {0xFC, 0x80, 0x80};
    }
    std::vector<std::uint8_t> pop_field_or_frame_locked(bool pop_second_frame)
    {
        auto [eia_608_pkt_count, total_pkt_count] = get_pkt_counts_for_pop_locked(pop_second_frame);
        std::vector<std::uint8_t> cc_data_pkts;
        cc_data_pkts.reserve(total_pkt_count * cc_data_pkt{}.size());

        for (std::size_t i = 0; i < eia_608_pkt_count; i++) {
            auto pkt = pop_eia_608_pkt_for_field_locked();
            cc_data_pkts.insert(cc_data_pkts.end(), pkt.begin(), pkt.end());
        }
        while (cc_data_pkts.size() / cc_data_pkt{}.size() < total_pkt_count) {
            cc_data_pkt pkt{0xFA, 0x00, 0x00};
            if (!cta_708_pkts_.empty()) {
                pkt = cta_708_pkts_.front();
                cta_708_pkts_.pop();
            }
            cc_data_pkts.insert(cc_data_pkts.end(), pkt.begin(), pkt.end());
        }
        return cc_data_pkts;
    }
    std::vector<std::uint8_t>           pop_field_locked() { return pop_field_or_frame_locked(false); }
    std::vector<std::uint8_t>           pop_frame_locked() { return pop_field_or_frame_locked(interlaced_); }
    std::pair<std::size_t, std::size_t> get_pkt_counts_for_pop_locked(bool pop_second_frame)
    {
        auto did_first_pop = did_first_pop_;
        did_first_pop_     = true;
        auto get_pkt_count = [&](std::size_t first_available_unpadded, rate_limiter& pkts_per_field_limiter) {
            std::size_t retval;
            if (did_first_pop) {
                retval = pkts_per_field_limiter.tick(~0U);
            } else {
                retval =
                    pkts_per_field_limiter.tick(first_available_unpadded < pkts_per_field_limiter.whole_count_per_tick_
                                                    ? pkts_per_field_limiter.whole_count_per_tick_
                                                    : first_available_unpadded);
            }
            if (pop_second_frame) {
                retval += pkts_per_field_limiter.tick(~0U);
            }
            return retval;
        };
        std::size_t eia_608_pkt_count = get_pkt_count(eia_608_pkts_.size(), eia_608_pkts_per_field_limiter_);
        std::size_t total_pkt_count =
            get_pkt_count(eia_608_pkt_count + cta_708_pkts_.size(), total_pkts_per_field_limiter_);
        return {eia_608_pkt_count, total_pkt_count};
    }
    std::size_t max_field_size() const noexcept
    {
        return total_pkts_per_field_limiter_.max_per_tick() * cc_data_pkt{}.size();
    }
    std::size_t max_frame_size() const noexcept
    {
        return total_pkts_per_field_limiter_.max_per_tick(interlaced_ ? 2 : 1) * cc_data_pkt{}.size();
    }
};

a53_cc_queue::a53_cc_queue(boost::rational<int> frame_rate, bool interlaced)
    : impl_(std::make_unique<impl>(frame_rate, interlaced))
{
}

a53_cc_queue::~a53_cc_queue() noexcept {}

boost::rational<int> a53_cc_queue::frame_rate() const noexcept { return impl_->frame_rate_; }
bool                 a53_cc_queue::interlaced() const noexcept { return impl_->interlaced_; }
std::size_t          a53_cc_queue::max_field_size() const noexcept { return impl_->max_field_size(); }
std::size_t          a53_cc_queue::max_frame_size() const noexcept { return impl_->max_frame_size(); }

a53_cc_queue::locked::locked(a53_cc_queue& queue)
    : queue_(queue)
    , lock_(queue.impl_->mutex_)
{
}

void a53_cc_queue::locked::push_field(const std::vector<std::uint8_t>& field)
{
    queue_.impl_->push_field_locked(field);
}
std::vector<std::uint8_t> a53_cc_queue::locked::pop_field() { return queue_.impl_->pop_field_locked(); }
std::vector<std::uint8_t> a53_cc_queue::locked::pop_frame() { return queue_.impl_->pop_frame_locked(); }

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