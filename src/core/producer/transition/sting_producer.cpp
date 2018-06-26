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

#include <common/param.h>
#include <common/scope_exit.h>

#include <tbb/parallel_invoke.h>

#include <future>

namespace caspar { namespace core {

class sting_producer : public frame_producer_base
{
    monitor::state  state_;
    uint32_t        current_frame_ = 0;
    caspar::tweener audio_tweener_{L"linear"};

    core::draw_frame dst_;
    core::draw_frame src_;
    core::draw_frame mask_;
    core::draw_frame overlay_;

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
        return dst_ && current_frame_ >= info_.duration ? dst_producer_ : core::frame_producer::empty();
    }

    draw_frame receive_impl(int nb_samples) override
    {
        CASPAR_SCOPE_EXIT
        {
            state_                     = dst_producer_->state();
            state_["transition/frame"] = {static_cast<int>(current_frame_), static_cast<int>(info_.duration)};
            state_["transition/type"]  = L"sting";
        };

        bool started_dst = current_frame_ >= info_.trigger_point;

        tbb::parallel_invoke(
            [&] {
                if (!started_dst)
                    return;

                dst_ = dst_producer_->receive(nb_samples);
                if (!dst_) {
                    dst_ = dst_producer_->last_frame();
                }
            },
            [&] {
                src_ = src_producer_->receive(nb_samples);
                if (!src_) {
                    src_ = src_producer_->last_frame();
                }
            },
            [&] {
                mask_ = mask_producer_->receive(nb_samples);
                if (!mask_) {
                    mask_ = mask_producer_->last_frame();
                }
            },
            [&] {
                overlay_ = overlay_producer_->receive(nb_samples);
                if (!overlay_) {
                    overlay_ = overlay_producer_->last_frame();
                }
            });

        if (!mask_) {
            return src_;
        }

        if (current_frame_ >= info_.duration) {
            return dst_;
        }

        current_frame_ += 1;

        return compose(dst_, src_, mask_, overlay_);
    }

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

    draw_frame
    compose(draw_frame dst_frame, draw_frame src_frame, draw_frame mask_frame, draw_frame overlay_frame) const
    {
        const double delta = audio_tweener_(current_frame_, 0.0, 1.0, static_cast<double>(info_.duration));

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

    const monitor::state& state() { return state_; }
};

spl::shared_ptr<frame_producer> create_sting_producer(const frame_producer_dependencies&     dependencies,
                                                      const spl::shared_ptr<frame_producer>& destination,
                                                      sting_info&                            info)
{
    // Any producer which exposes a fixed duration will work here, not just ffmpeg
    auto mask_producer = dependencies.producer_registry->create_producer(dependencies, info.mask_filename);
    info.duration      = mask_producer->nb_frames();

    auto overlay_producer = frame_producer::empty();
    if (info.overlay_filename != L"") {
        // This could be any producer, no requirement for it to be of fixed length
        overlay_producer = dependencies.producer_registry->create_producer(dependencies, info.overlay_filename);
    }

    return spl::make_shared<sting_producer>(destination, info, mask_producer, overlay_producer);
}

bool try_match_sting(const std::vector<std::wstring>& params, sting_info& stingInfo)
{
    auto match = std::find_if(params.begin(), params.end(), param_comparer(L"STING"));
    if (match == params.end())
        return false;

    auto start_ind = static_cast<int>(match - params.begin());

    if (params.size() <= start_ind + 1) {
        // No mask filename
        return false;
    }

    stingInfo.mask_filename = params.at(start_ind + 1);

    if (params.size() > start_ind + 2) {
        stingInfo.trigger_point = boost::lexical_cast<int>(params.at(start_ind + 2));
    }

    if (params.size() > start_ind + 3) {
        stingInfo.overlay_filename = params.at(start_ind + 3);
    }

    return true;
}

}} // namespace caspar::core
