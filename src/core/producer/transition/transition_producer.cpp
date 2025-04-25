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
        // Return how many frames before the end of the source clip to start the transition
        // to ensure the last frame of source is visible before the transition completes
        
        switch(info_.type) {
            case transition_type::cut:
                // CUT: Source is immediately replaced, so need full duration
                return info_.duration;
                
            case transition_type::cutfade:
                // CUTFADE: Source is removed on first frame, so no need for extra frames
                return 0;
                
            case transition_type::vfade:
                // VFADE: Source fades out by the midpoint
                // Need to show last source frame one frame before midpoint
                return static_cast<int64_t>(std::floor(info_.duration / 2.0)) - 1;

            default:
                // For all other transitions, need to show
                //last source frame one frame before end of transition
                return info_.duration > 0 ? info_.duration - 1 : 0;
        }
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
    }

    draw_frame receive_impl(const core::video_field field, int nb_samples) override
    {
        CASPAR_SCOPE_EXIT { update_state(); };

        update_is_ready(field);

        // If destination is not ready, always use source (don't start any transition)
        if (!dst_is_ready_) {
            return src_producer_->receive(field, nb_samples);
        }

        // Get destination frame
        auto dst = dst_producer_->receive(field, nb_samples);
        if (!dst) {
            dst = dst_producer_->last_frame(field);
        }

        // Get source frame
        auto src = src_producer_->receive(field, nb_samples);
        if (!src) {
            src = src_producer_->last_frame(field);
        }

        // If no destination frame is available, use source
        if (!dst) {
            return src;
        }

        // If transition is complete, return destination
        if (current_frame_ >= info_.duration) {
            return dst;
        }

        // For CUT transitions, immediately complete
        if (info_.type == transition_type::cut) {
            current_frame_ = info_.duration; // Force transition to complete
            return dst;
        }

        // For all other cases, compose the frame and progress the transition
        auto result = compose(dst, src);
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

    [[nodiscard]] draw_frame compose(draw_frame dst_frame0, draw_frame src_frame0) const
    {
        if (info_.type == transition_type::cut) {
            return src_frame0;
        }

        // Wrap the frames so that the transforms can be mutated
        draw_frame dst_frame = draw_frame::push(std::move(dst_frame0));
        draw_frame src_frame = draw_frame::push(std::move(src_frame0));

        // Calculate delta - will naturally be 1.0 at the final frame (when current_frame_ == info_.duration - 1)
        const double delta = info_.tweener(current_frame_, 0.0, 1.0, static_cast<double>(info_.duration - 1));
        const double dir = info_.direction == transition_direction::from_left ? 1.0 : -1.0;

        // Set default audio volumes for both frames
        src_frame.transform().audio_transform.volume = 1.0 - delta;
        dst_frame.transform().audio_transform.volume = delta;

        // Apply transition-specific transforms
        if (info_.type == transition_type::cutfade) {
            // CUTFADE: Fade in destination from black
            double mix;
            
            if (current_frame_ == 0) {
                // First frame is black
                mix = 0.0;
            } else {
                // Linear progression between frames 1 and (duration-1)
                // Will naturally reach 1.0 at final frame
                mix = static_cast<double>(current_frame_) / static_cast<double>(info_.duration - 1);
            }
            
            if (info_.duration <= 1) {
                mix = 1.0;
            }
            
            dst_frame.transform().image_transform.opacity = mix;
            dst_frame.transform().audio_transform.volume = mix;
            dst_frame.transform().audio_transform.immediate_volume = current_frame_ == 0;
            
            return dst_frame;
        } 
        else if (info_.type == transition_type::fadecut) {
            // FADECUT: Fade out source to zero opacity by the final frame,
            // then the next frame will be the unmodified destination
            
            // Calculate fade out for source - linear fade from 1.0 to 0.0
            double opacity = 0.0;
            if (info_.duration > 1) {
                // Linear fade from 1.0 to 0.0 over the entire transition
                opacity = 1.0 - delta;
            }
            
            src_frame.transform().image_transform.opacity = opacity;
            src_frame.transform().audio_transform.volume = opacity;
            src_frame.transform().audio_transform.immediate_volume = current_frame_ == 0;
            
            return src_frame;
        }
        else if (info_.type == transition_type::vfade) {
            // VFADE: Fade out source for first half, fade in destination for second half
            // At midpoint, both source and destination have zero opacity
            
            // For even-duration transitions, ensure we have black frames in the middle
            bool is_even_duration = (info_.duration % 2 == 0);
            
            // Calculate custom thresholds for even durations to create two black frames
            if (is_even_duration) {
                // For even durations, create two black frames in the middle
                // For a 10-frame transition, frames 4 and 5 would be black
                double half_step = 0.5 / (info_.duration - 1);
                
                // First middle frame (last frame of first half)
                if (delta >= (0.5 - half_step) && delta < 0.5) {
                    src_frame.transform().image_transform.opacity = 0.0;
                    src_frame.transform().audio_transform.volume = 0.0;
                    return src_frame;
                }
                
                // Second middle frame (first frame of second half)
                if (delta >= 0.5 && delta < (0.5 + half_step)) {
                    dst_frame.transform().image_transform.opacity = 0.0;
                    dst_frame.transform().audio_transform.volume = 0.0;
                    dst_frame.transform().audio_transform.immediate_volume = true;
                    return dst_frame;
                }
            }
            
            // Exactly at midpoint for odd durations (delta = 0.5)
            if (!is_even_duration && delta == 0.5) {
                // Set dst frame with zero opacity for the midpoint
                dst_frame.transform().image_transform.opacity = 0.0;
                dst_frame.transform().audio_transform.volume = 0.0;
                dst_frame.transform().audio_transform.immediate_volume = true;
                return dst_frame;
            }
            // First half: fade out source (when delta < 0.5)
            else if (delta < 0.5) {
                // Map 0.0-0.5 to 1.0-0.0 for fade out
                double fade_out = 1.0 - (delta * 2.0);
                
                src_frame.transform().image_transform.opacity = fade_out;
                src_frame.transform().audio_transform.volume = fade_out;
                return src_frame;
            }
            // Second half: fade in destination (when delta > 0.5)
            else {
                // Map 0.5-1.0 to 0.0-1.0 for fade in
                double fade_in = (delta - 0.5) * 2.0;
                
                dst_frame.transform().image_transform.opacity = fade_in;
                dst_frame.transform().audio_transform.volume = fade_in;
                dst_frame.transform().audio_transform.immediate_volume = 
                    is_even_duration ? 
                    (delta < 0.5 + (1.0 / (info_.duration - 1))) : 
                    (delta <= 0.5 + (1.0 / (info_.duration - 1)));
                
                return dst_frame;
            }
        }
        else if (info_.type == transition_type::mix) {
            dst_frame.transform().image_transform.opacity = delta;
            dst_frame.transform().image_transform.is_mix  = true;
            src_frame.transform().image_transform.opacity = 1.0 - delta;
            src_frame.transform().image_transform.is_mix  = true;
        } 
        else if (info_.type == transition_type::slide) {
            // Math naturally gives translation 0 at final frame
            dst_frame.transform().image_transform.fill_translation[0] = (-1.0 + delta) * dir;
        } 
        else if (info_.type == transition_type::push) {
            // Math naturally gives correct positions at final frame
            dst_frame.transform().image_transform.fill_translation[0] = (-1.0 + delta) * dir;
            src_frame.transform().image_transform.fill_translation[0] = (0.0 + delta) * dir;
        } 
        else if (info_.type == transition_type::wipe) {
            // Math naturally gives full reveal at final frame
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
    // Using word boundaries to ensure we match complete transition names
    static const boost::wregex expr(
        LR"(.*\b(?<TRANSITION>VFADE|FADECUT|CUTFADE|CUT|PUSH|SLIDE|WIPE|MIX)\b\s+(?<DURATION>\d+)\s*(?<TWEEN>(LINEAR)|(EASE[^\s]*))?\s*(?<DIRECTION>FROMLEFT|FROMRIGHT|LEFT|RIGHT)?.*)");
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

    return true;
}

} // namespace caspar::core
