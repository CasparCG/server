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

#include "transition_producer.h"

#include "../../frame/draw_frame.h"
#include "../../frame/frame_transform.h"
#include "../../monitor/monitor.h"
#include "../frame_producer.h"

#include <common/scope_exit.h>

#include <future>
#include <utility>

namespace caspar::core {

class transition_producer : public frame_producer
{
    monitor::state state_;
    int            current_frame_ = 0;

    const transition_info info_;

    spl::shared_ptr<frame_producer> dst_producer_ = frame_producer::empty();
    spl::shared_ptr<frame_producer> src_producer_ = frame_producer::empty();
    bool                            dst_is_ready_ = false;

  public:
    transition_producer(const spl::shared_ptr<frame_producer>& dest, transition_info info)
        : info_(std::move(info))
        , dst_producer_(dest)
    {
        dst_is_ready_ = dst_producer_->is_ready();
        update_state();
    }

    // frame_producer

    void update_is_ready(const core::video_field field)
    {
        // Ensure a frame has been attempted
        dst_producer_->first_frame(field);

        dst_is_ready_ = dst_producer_->is_ready();
    }

    core::draw_frame last_frame(const core::video_field field) override
    {
        CASPAR_SCOPE_EXIT { update_state(); };

        update_is_ready(field);

        if (!dst_is_ready_) {
            return src_producer_->last_frame(field);
        }

        auto src = src_producer_->last_frame(field);
        auto dst = dst_producer_->last_frame(field);

        if (dst && current_frame_ >= info_.duration) {
            return dst;
        } else {
            return src;
        }
    }

    core::draw_frame first_frame(const core::video_field field) override { return dst_producer_->first_frame(field); }

    void leading_producer(const spl::shared_ptr<frame_producer>& producer) override { src_producer_ = producer; }

    [[nodiscard]] spl::shared_ptr<frame_producer> following_producer() const override
    {
        return current_frame_ >= info_.duration && dst_is_ready_ ? dst_producer_ : core::frame_producer::empty();
    }

    [[nodiscard]] std::optional<int64_t> auto_play_delta() const override { return info_.duration; }

    void update_state()
    {
        state_                        = dst_producer_->state();
        state_["transition/producer"] = dst_producer_->name();
        state_["transition/frame"]    = {current_frame_, info_.duration};
        state_["transition/type"]     = [&]() -> std::string {
            switch (info_.type) {
                case transition_type::mix:
                    return "mix";
                case transition_type::wipe:
                    return "wipe";
                case transition_type::slide:
                    return "slide";
                case transition_type::push:
                    return "push";
                case transition_type::cut:
                    return "cut";
                default:
                    return "n/a";
            }
        }();
    }

    draw_frame receive_impl(const core::video_field field, int nb_samples) override
    {
        CASPAR_SCOPE_EXIT { update_state(); };

        update_is_ready(field);

        if (!dst_is_ready_) {
            return src_producer_->receive(field, nb_samples);
        }

        auto dst = dst_producer_->receive(field, nb_samples);
        if (!dst) {
            dst = dst_producer_->last_frame(field);
        }

        auto src = src_producer_->receive(field, nb_samples);
        if (!src) {
            src = src_producer_->last_frame(field);
        }

        if (!dst) {
            return src;
        }

        if (current_frame_ >= info_.duration) {
            return dst;
        }

        current_frame_ += 1;

        return compose(dst, src);
    }

    [[nodiscard]] uint32_t nb_frames() const override { return dst_producer_->nb_frames(); }

    [[nodiscard]] uint32_t frame_number() const override { return dst_producer_->frame_number(); }

    [[nodiscard]] std::wstring print() const override
    {
        return L"transition[" + src_producer_->print() + L"=>" + dst_producer_->print() + L"]";
    }

    [[nodiscard]] std::wstring name() const override { return L"transition"; }

    std::future<std::wstring> call(const std::vector<std::wstring>& params) override
    {
        return dst_producer_->call(params);
    }

    [[nodiscard]] draw_frame compose(draw_frame dst_frame, draw_frame src_frame) const
    {
        if (info_.type == transition_type::cut) {
            return src_frame;
        }

        const double delta = info_.tweener(current_frame_, 0.0, 1.0, static_cast<double>(info_.duration));

        const double dir = info_.direction == transition_direction::from_left ? 1.0 : -1.0;

        src_frame.transform().audio_transform.volume = 1.0 - delta;
        dst_frame.transform().audio_transform.volume = delta;

        if (info_.type == transition_type::mix) {
            dst_frame.transform().image_transform.opacity = delta;
            dst_frame.transform().image_transform.is_mix  = true;
            src_frame.transform().image_transform.opacity = 1.0 - delta;
            src_frame.transform().image_transform.is_mix  = true;
        } else if (info_.type == transition_type::slide) {
            dst_frame.transform().image_transform.fill_translation[0] = (-1.0 + delta) * dir;
        } else if (info_.type == transition_type::push) {
            dst_frame.transform().image_transform.fill_translation[0] = (-1.0 + delta) * dir;
            src_frame.transform().image_transform.fill_translation[0] = (0.0 + delta) * dir;
        } else if (info_.type == transition_type::wipe) {
            if (info_.direction == transition_direction::from_right) {
                dst_frame.transform().image_transform.clip_scale[0] = delta;
            } else {
                dst_frame.transform().image_transform.clip_translation[0] = (1.0 - delta);
            }
        }

        return draw_frame::over(src_frame, dst_frame);
    }

    [[nodiscard]] core::monitor::state state() const override { return state_; }

    bool is_ready() override { return dst_producer_->is_ready(); }
};

spl::shared_ptr<frame_producer> create_transition_producer(const spl::shared_ptr<frame_producer>& destination,
                                                           const transition_info&                 info)
{
    return spl::make_shared<transition_producer>(destination, info);
}

bool try_match_transition(const std::wstring& message, transition_info& transitionInfo)
{
    static const boost::wregex expr(
        LR"(.*(?<TRANSITION>CUT|PUSH|SLIDE|WIPE|MIX)\s*(?<DURATION>\d+)\s*(?<TWEEN>(LINEAR)|(EASE[^\s]*))?\s*(?<DIRECTION>FROMLEFT|FROMRIGHT|LEFT|RIGHT)?.*)");
    boost::wsmatch what;
    if (!boost::regex_match(message, what, expr)) {
        return false;
    }

    transitionInfo.duration = std::stoi(what["DURATION"].str());
    if (transitionInfo.duration == 0) {
        return false;
    }

    auto transition        = what["TRANSITION"].str();
    auto direction         = what["DIRECTION"].matched ? what["DIRECTION"].str() : L"";
    auto tween             = what["TWEEN"].matched ? what["TWEEN"].str() : L"";
    transitionInfo.tweener = tween;

    if (transition == L"CUT")
        transitionInfo.type = transition_type::cut;
    else if (transition == L"MIX")
        transitionInfo.type = transition_type::mix;
    else if (transition == L"PUSH")
        transitionInfo.type = transition_type::push;
    else if (transition == L"SLIDE")
        transitionInfo.type = transition_type::slide;
    else if (transition == L"WIPE")
        transitionInfo.type = transition_type::wipe;

    if (direction == L"FROMLEFT" || direction == L"RIGHT")
        transitionInfo.direction = transition_direction::from_left;
    else if (direction == L"FROMRIGHT" || direction == L"LEFT")
        transitionInfo.direction = transition_direction::from_right;

    return true;
}

} // namespace caspar::core
