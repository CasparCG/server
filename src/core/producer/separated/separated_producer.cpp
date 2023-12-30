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
 * Author: Robert Nagy, ronag89@gmail.com
 */

#include "../../StdAfx.h"

#include "separated_producer.h"

#include <common/scope_exit.h>

#include <core/frame/draw_frame.h>
#include <core/monitor/monitor.h>
#include <core/producer/frame_producer.h>

#include <future>

namespace caspar { namespace core {

struct frame_pair
{
    draw_frame field1;
    draw_frame field2;

    draw_frame get(video_field field)
    {
        if (field == video_field::b)
            return field2;
        else
            return field1;
    }

    void set(video_field field, const draw_frame& frame)
    {
        if (field == video_field::b)
            field2 = frame;
        else
            field1 = frame;
    }
};

class separated_producer : public frame_producer
{
    monitor::state state_;

    spl::shared_ptr<frame_producer> fill_producer_;
    spl::shared_ptr<frame_producer> key_producer_;
    frame_pair                      fill_;
    frame_pair                      key_;

  public:
    explicit separated_producer(const spl::shared_ptr<frame_producer>& fill, const spl::shared_ptr<frame_producer>& key)
        : fill_producer_(fill)
        , key_producer_(key)
    {
        CASPAR_LOG(debug) << print() << L" Initialized";
    }

    // frame_producer

    draw_frame last_frame(const core::video_field field) override
    {
        return draw_frame::mask(fill_producer_->last_frame(field), key_producer_->last_frame(field));
    }
    draw_frame first_frame(const core::video_field field) override
    {
        return draw_frame::mask(fill_producer_->first_frame(field), key_producer_->first_frame(field));
    }

    draw_frame receive_impl(const core::video_field field, int nb_samples) override
    {
        CASPAR_SCOPE_EXIT
        {
            state_          = fill_producer_->state();
            state_["keyer"] = key_producer_->state();
        };

        auto fill = fill_.get(field);
        auto key  = key_.get(field);

        if (!fill) {
            fill = fill_producer_->receive(field, nb_samples);
        }

        if (!key) {
            key = key_producer_->receive(field, nb_samples);
        }

        if (!fill || !key) {
            fill_.set(field, fill);
            key_.set(field, key);
            return core::draw_frame{};
        }

        auto frame = draw_frame::mask(fill, key);

        fill_.set(field, draw_frame{});
        key_.set(field, draw_frame{});

        return frame;
    }

    uint32_t frame_number() const override { return fill_producer_->frame_number(); }

    uint32_t nb_frames() const override { return std::min(fill_producer_->nb_frames(), key_producer_->nb_frames()); }

    std::wstring print() const override
    {
        return L"separated[fill:" + fill_producer_->print() + L"|key[" + key_producer_->print() + L"]]";
    }

    std::future<std::wstring> call(const std::vector<std::wstring>& params) override
    {
        key_producer_->call(params);
        return fill_producer_->call(params);
    }

    std::wstring name() const override { return L"separated"; }

    core::monitor::state state() const override { return state_; }

    bool is_ready() override { return key_producer_->is_ready() && fill_producer_->is_ready(); }
};

spl::shared_ptr<frame_producer> create_separated_producer(const spl::shared_ptr<frame_producer>& fill,
                                                          const spl::shared_ptr<frame_producer>& key)
{
    return spl::make_shared<separated_producer>(fill, key);
}

}} // namespace caspar::core
