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

    [[nodiscard]] std::optional<int64_t> auto_play_delta() const override
    {
        // Return how many frames before the end to start the transition
        // Must never return 0 to avoid duplicate fields in AUTO playback

        int64_t delta;

        switch (info_.type) {
            case transition_type::cut:
                // CUT: Need full duration to show all frames
                delta = info_.duration;
                break;

            case transition_type::cutfade:
                // CUTFADE: Source is removed on first frame
                delta = 1;
                break;

            case transition_type::vfade:
                // VFADE: Source fades out by midpoint
                delta = static_cast<int64_t>(std::floor(info_.duration / 2.0));
                break;

            default:
                // Standard transitions (MIX, PUSH, SLIDE, WIPE)
                // Need full duration to complete properly and avoid duplicated fields
                delta = info_.duration;
                break;
        }

        // Ensure we never return less than 1
        return std::max(delta, int64_t(1));
    }

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
                case transition_type::fadecut:
                    return "fadecut";
                case transition_type::cutfade:
                    return "cutfade";
                case transition_type::vfade:
                    return "vfade";
                default:
                    return "n/a";
            }
        }();
        state_["transition/direction"] = [&]() -> std::string {
            switch (info_.direction) {
                case transition_direction::from_left:
                    return "from_left";
                case transition_direction::from_right:
                    return "from_right";
                case transition_direction::from_top:
                    return "from_top";
                case transition_direction::from_bottom:
                    return "from_bottom";
                default:
                    return "n/a";
            }
        }();
    }

    draw_frame receive_impl(const core::video_field field, int nb_samples) override
    {
        CASPAR_SCOPE_EXIT { update_state(); };

        update_is_ready(field);

        // If destination is not ready, always use source (don't start any transition)
        if (!dst_is_ready_) {
            return src_producer_->receive(field, nb_samples);
        }

        // If transition is complete, return destination
        if (current_frame_ >= info_.duration) {
            auto dst = dst_producer_->receive(field, nb_samples);
            if (!dst)
                dst = dst_producer_->last_frame(field);
            return dst;
        }

        // For CUT transitions, handle based on duration
        if (info_.type == transition_type::cut) {
            if (current_frame_ >= info_.duration) {
                // Cut now - return destination
                auto dst = dst_producer_->receive(field, nb_samples);
                if (!dst)
                    dst = dst_producer_->last_frame(field);
                current_frame_++; // Increment after processing
                return dst;
            } else {
                // Not time to cut yet - return source
                auto src = src_producer_->receive(field, nb_samples);
                if (!src)
                    src = src_producer_->last_frame(field);
                current_frame_++; // Increment after processing
                return src;
            }
        }

        // Compose and progress the transition
        auto result = compose(field, nb_samples);
        current_frame_++;
        return result;
    }

    [[nodiscard]] uint32_t nb_frames() const override { return dst_producer_->nb_frames(); }

    [[nodiscard]] uint32_t frame_number() const override { return dst_producer_->frame_number(); }

    [[nodiscard]] std::wstring print() const override
    {
        return L"transition[" + src_producer_->print() + L"=>" + dst_producer_->print() + L"]";
    }

    [[nodiscard]] std::wstring name() const override { return L"transition"; }

    [[nodiscard]] std::future<std::wstring> call(const std::vector<std::wstring>& params) override
    {
        return dst_producer_->call(params);
    }

    [[nodiscard]] draw_frame compose(const core::video_field field, int nb_samples) const
    {
        // Helper lambdas to get wrapped frames
        auto get_src_frame = [&]() {
            auto f = src_producer_->receive(field, nb_samples);
            if (!f)
                f = src_producer_->last_frame(field);
            return draw_frame::push(std::move(f));
        };
        auto get_dst_frame = [&]() {
            auto f = dst_producer_->receive(field, nb_samples);
            if (!f)
                f = dst_producer_->last_frame(field);
            return draw_frame::push(std::move(f));
        };

        if (info_.type == transition_type::cut) {
            return get_dst_frame();
        } else if (info_.type == transition_type::fadecut) {
            // Only use source during transition
            auto   src     = get_src_frame();
            double delta   = info_.tweener(current_frame_, 0.0, 1.0, static_cast<double>(info_.duration - 1));
            double opacity = info_.duration > 1 ? 1.0 - delta : 0.0;
            src.transform().image_transform.opacity          = opacity;
            src.transform().audio_transform.volume           = opacity;
            src.transform().audio_transform.immediate_volume = current_frame_ == 0;
            return src;
        } else if (info_.type == transition_type::vfade) {
            double delta            = info_.tweener(current_frame_, 0.0, 1.0, static_cast<double>(info_.duration - 1));
            bool   is_even_duration = (info_.duration % 2 == 0);

            if (is_even_duration) {
                double half_step = 0.5 / (info_.duration - 1);
                if (delta >= (0.5 - half_step) && delta < 0.5) {
                    auto src                                = get_src_frame();
                    src.transform().image_transform.opacity = 0.0;
                    src.transform().audio_transform.volume  = 0.0;
                    return src;
                }
                if (delta >= 0.5 && delta < (0.5 + half_step)) {
                    auto dst                                         = get_dst_frame();
                    dst.transform().image_transform.opacity          = 0.0;
                    dst.transform().audio_transform.volume           = 0.0;
                    dst.transform().audio_transform.immediate_volume = true;
                    return dst;
                }
            }
            if (!is_even_duration && delta == 0.5) {
                auto dst                                         = get_dst_frame();
                dst.transform().image_transform.opacity          = 0.0;
                dst.transform().audio_transform.volume           = 0.0;
                dst.transform().audio_transform.immediate_volume = true;
                return dst;
            } else if (delta < 0.5) {
                auto   src                              = get_src_frame();
                double fade_out                         = 1.0 - (delta * 2.0);
                src.transform().image_transform.opacity = fade_out;
                src.transform().audio_transform.volume  = fade_out;
                return src;
            } else {
                auto   dst                                       = get_dst_frame();
                double fade_in                                   = (delta - 0.5) * 2.0;
                dst.transform().image_transform.opacity          = fade_in;
                dst.transform().audio_transform.volume           = fade_in;
                dst.transform().audio_transform.immediate_volume = is_even_duration
                                                                       ? (delta < 0.5 + (1.0 / (info_.duration - 1)))
                                                                       : (delta <= 0.5 + (1.0 / (info_.duration - 1)));
                return dst;
            }
        }
        // For all other transitions, we need both frames
        auto         src_frame = get_src_frame();
        auto         dst_frame = get_dst_frame();
        const double delta     = info_.tweener(current_frame_, 0.0, 1.0, static_cast<double>(info_.duration - 1));

        // Get horizontal or vertical direction based on transition direction
        const bool is_horizontal =
            info_.direction == transition_direction::from_left || info_.direction == transition_direction::from_right;
        const double h_dir = info_.direction == transition_direction::from_left ? 1.0 : -1.0;
        const double v_dir = info_.direction == transition_direction::from_top ? 1.0 : -1.0;

        src_frame.transform().audio_transform.volume = 1.0 - delta;
        dst_frame.transform().audio_transform.volume = delta;
        if (info_.type == transition_type::cutfade) {
            double mix;
            if (current_frame_ == 0) {
                mix = 0.0;
            } else {
                mix = static_cast<double>(current_frame_) / static_cast<double>(info_.duration - 1);
            }
            if (info_.duration <= 1) {
                mix = 1.0;
            }
            dst_frame.transform().image_transform.opacity          = mix;
            dst_frame.transform().audio_transform.volume           = mix;
            dst_frame.transform().audio_transform.immediate_volume = current_frame_ == 0;
            return dst_frame;
        } else if (info_.type == transition_type::mix) {
            dst_frame.transform().image_transform.opacity = delta;
            dst_frame.transform().image_transform.is_mix  = true;
            src_frame.transform().image_transform.opacity = 1.0 - delta;
            src_frame.transform().image_transform.is_mix  = true;
        } else if (info_.type == transition_type::slide) {
            if (is_horizontal) {
                dst_frame.transform().image_transform.fill_translation[0] = (-1.0 + delta) * h_dir;
            } else {
                dst_frame.transform().image_transform.fill_translation[1] = (-1.0 + delta) * v_dir;
            }
        } else if (info_.type == transition_type::push) {
            if (is_horizontal) {
                dst_frame.transform().image_transform.fill_translation[0] = (-1.0 + delta) * h_dir;
                src_frame.transform().image_transform.fill_translation[0] = (0.0 + delta) * h_dir;
            } else {
                dst_frame.transform().image_transform.fill_translation[1] = (-1.0 + delta) * v_dir;
                src_frame.transform().image_transform.fill_translation[1] = (0.0 + delta) * v_dir;
            }
        } else if (info_.type == transition_type::wipe) {
            if (is_horizontal) {
                if (info_.direction == transition_direction::from_right) {
                    dst_frame.transform().image_transform.clip_scale[0] = delta;
                } else {
                    dst_frame.transform().image_transform.clip_translation[0] = (1.0 - delta);
                }
            } else {
                if (info_.direction == transition_direction::from_bottom) {
                    dst_frame.transform().image_transform.clip_scale[1] = delta;
                } else {
                    dst_frame.transform().image_transform.clip_translation[1] = (1.0 - delta);
                }
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
    // Using word boundaries to ensure we match complete transition names
    static const boost::wregex expr(
        LR"(.*\b(?<TRANSITION>VFADE|FADECUT|CUTFADE|CUT|PUSH|SLIDE|WIPE|MIX)\b\s+(?<DURATION>\d+)\s*(?<TWEEN>(LINEAR)|(EASE[^\s]*))?\s*(?<DIRECTION>FROMLEFT|FROMRIGHT|FROMTOP|FROMBOTTOM|LEFT|RIGHT|UP|DOWN)?.*)");
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
    else if (transition == L"FADECUT")
        transitionInfo.type = transition_type::fadecut;
    else if (transition == L"CUTFADE")
        transitionInfo.type = transition_type::cutfade;
    else if (transition == L"VFADE")
        transitionInfo.type = transition_type::vfade;

    if (direction == L"FROMLEFT" || direction == L"RIGHT")
        transitionInfo.direction = transition_direction::from_left;
    else if (direction == L"FROMRIGHT" || direction == L"LEFT")
        transitionInfo.direction = transition_direction::from_right;
    else if (direction == L"FROMTOP" || direction == L"DOWN")
        transitionInfo.direction = transition_direction::from_top;
    else if (direction == L"FROMBOTTOM" || direction == L"UP")
        transitionInfo.direction = transition_direction::from_bottom;

    return true;
}

} // namespace caspar::core
