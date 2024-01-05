/*
 * Copyright (c) 2018 Norsk rikskringkasting AS
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
 * Author: Julian Waller, julian@superfly.tv
 */

#include "../../StdAfx.h"

#include "sting_producer.h"

#include "../../frame/draw_frame.h"
#include "../../frame/frame_transform.h"
#include "../../monitor/monitor.h"
#include "../frame_producer.h"

#include <common/scope_exit.h>

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

class sting_producer : public frame_producer
{
    monitor::state  state_;
    uint32_t        current_frame_ = 0;
    caspar::tweener audio_tweener_{L"linear"};

    frame_pair dst_;
    frame_pair src_;
    frame_pair mask_;
    frame_pair overlay_;

    const sting_info info_;

    spl::shared_ptr<frame_producer> dst_producer_     = frame_producer::empty();
    spl::shared_ptr<frame_producer> src_producer_     = frame_producer::empty();
    spl::shared_ptr<frame_producer> mask_producer_    = frame_producer::empty();
    spl::shared_ptr<frame_producer> overlay_producer_ = frame_producer::empty();

  public:
    sting_producer(const spl::shared_ptr<frame_producer>& dest,
                   const sting_info&                      info,
                   const spl::shared_ptr<frame_producer>& mask,
                   const spl::shared_ptr<frame_producer>& overlay)
        : info_(info)
        , dst_producer_(dest)
        , mask_producer_(mask)
        , overlay_producer_(overlay)
    {
    }

    // frame_producer

    void leading_producer(const spl::shared_ptr<frame_producer>& producer) override { src_producer_ = producer; }

    spl::shared_ptr<frame_producer> following_producer() const override
    {
        auto duration = target_duration();
        return duration && current_frame_ >= *duration ? dst_producer_ : core::frame_producer::empty();
    }

    std::optional<int64_t> auto_play_delta() const override
    {
        auto duration = static_cast<int64_t>(mask_producer_->nb_frames());
        // ffmpeg will return -1 when media is still loading, so we need to cast duration first
        if (duration > -1) {
            return std::optional<int64_t>(duration);
        }
        return {};
    }

    std::optional<uint32_t> target_duration() const
    {
        auto autoplay = auto_play_delta();
        if (!autoplay) {
            return {};
        }

        auto autoplay2 = static_cast<uint32_t>(*autoplay);
        if (info_.audio_fade_duration < UINT32_MAX) {
            return std::max(autoplay2, info_.audio_fade_duration + info_.audio_fade_start);
        }
        return autoplay2;
    }

    draw_frame receive_impl(const core::video_field field, int nb_samples) override
    {
        auto duration = target_duration();

        CASPAR_SCOPE_EXIT
        {
            state_                    = dst_producer_->state();
            state_["transition/type"] = std::string("sting");

            if (duration)
                state_["transition/frame"] = {static_cast<int>(current_frame_), static_cast<int>(*duration)};
        };

        if (duration && current_frame_ >= *duration) {
            return dst_producer_->receive(field, nb_samples);
        }

        auto src = src_.get(field);
        if (!src) {
            src = src_producer_->receive(field, nb_samples);
            src_.set(field, src);
            if (!src) {
                src = src_producer_->last_frame(field);
            }
        }

        bool started_dst = current_frame_ >= info_.trigger_point;
        auto dst         = dst_.get(field);
        if (!dst && started_dst) {
            dst = dst_producer_->receive(field, nb_samples);
            dst_.set(field, dst);
            if (!dst) {
                dst = dst_producer_->last_frame(field);
            }

            if (!dst) { // Not ready yet
                src_.set(field, draw_frame{});
                return src;
            }
        }

        auto mask = mask_.get(field);
        if (!mask) {
            mask = mask_producer_->receive(field, nb_samples);
            mask_.set(field, mask);
        }
        bool expecting_overlay = overlay_producer_ != core::frame_producer::empty();
        auto overlay           = overlay_.get(field);
        if (expecting_overlay && !overlay) {
            overlay = overlay_producer_->receive(field, nb_samples);
            overlay_.set(field, overlay);
        }

        // Not started, and mask or overlay is not ready
        bool mask_and_overlay_valid = mask && (!expecting_overlay || overlay);
        if (current_frame_ == 0 && !mask_and_overlay_valid) {
            src_.set(field, draw_frame{});
            return src;
        }

        // Ensure mask and overlay are in perfect sync
        if (!mask_and_overlay_valid) {
            // If one is behind, then fetch the last_frame of both
            mask    = mask_producer_->last_frame(field);
            overlay = overlay_producer_->last_frame(field);
        }

        auto res = compose(dst, src, mask, overlay);

        dst_.set(field, draw_frame{});
        src_.set(field, draw_frame{});

        if (mask_and_overlay_valid) {
            mask_.set(field, draw_frame{});
            overlay_.set(field, draw_frame{});

            current_frame_ += 1;
        }

        return res;
    }

    core::draw_frame first_frame(const core::video_field field) override { return dst_producer_->first_frame(field); }

    uint32_t nb_frames() const override { return dst_producer_->nb_frames(); }

    uint32_t frame_number() const override { return dst_producer_->frame_number(); }

    std::wstring print() const override
    {
        return L"transition[" + src_producer_->print() + L"=>" + dst_producer_->print() + L"]";
    }

    std::wstring name() const override { return L"transition"; }

    std::future<std::wstring> call(const std::vector<std::wstring>& params) override
    {
        return dst_producer_->call(params);
    }

    double get_audio_delta() const
    {
        if (current_frame_ < info_.audio_fade_start) {
            return 0;
        }

        auto total_duration = target_duration();
        if (!total_duration) {
            return 0;
        }

        uint32_t frame_number = current_frame_ - info_.audio_fade_start;
        uint32_t duration     = std::min(*total_duration - info_.audio_fade_start, info_.audio_fade_duration);
        if (frame_number > duration) {
            return 1.0;
        }

        return audio_tweener_(frame_number, 0.0, 1.0, static_cast<double>(duration));
    }

    draw_frame
    compose(draw_frame dst_frame, draw_frame src_frame, draw_frame mask_frame, draw_frame overlay_frame) const
    {
        const double delta                           = get_audio_delta();
        src_frame.transform().audio_transform.volume = 1.0 - delta;
        dst_frame.transform().audio_transform.volume = delta;

        draw_frame mask_frame2                         = mask_frame;
        mask_frame.transform().image_transform.is_key  = true;
        mask_frame2.transform().image_transform.is_key = true;
        mask_frame2.transform().image_transform.invert = true;

        std::vector<draw_frame> frames;
        frames.push_back(std::move(mask_frame2));
        frames.push_back(std::move(src_frame));
        frames.push_back(std::move(mask_frame));
        frames.push_back(std::move(dst_frame));
        if (overlay_frame != draw_frame::empty())
            frames.push_back(std::move(overlay_frame));

        return draw_frame(std::move(frames));
    }

    monitor::state state() const override { return state_; }

    bool is_ready() override { return dst_producer_->is_ready(); }
};

spl::shared_ptr<frame_producer> create_sting_producer(const frame_producer_dependencies&     dependencies,
                                                      const spl::shared_ptr<frame_producer>& destination,
                                                      sting_info&                            info)
{
    // Any producer which exposes a fixed duration will work here, not just ffmpeg
    auto mask_producer = dependencies.producer_registry->create_producer(dependencies, info.mask_filename);

    auto overlay_producer = frame_producer::empty();
    if (!info.overlay_filename.empty()) {
        // This could be any producer, no requirement for it to be of fixed length
        overlay_producer = dependencies.producer_registry->create_producer(dependencies, info.overlay_filename);
    }

    return spl::make_shared<sting_producer>(destination, info, mask_producer, overlay_producer);
}

}} // namespace caspar::core
