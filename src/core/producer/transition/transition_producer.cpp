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

#include <tbb/parallel_invoke.h>

#include <future>

namespace caspar { namespace core {

class transition_producer : public frame_producer_base
{
    monitor::state                    state_;
    int                               current_frame_ = 0;

    const transition_info info_;

    spl::shared_ptr<frame_producer> dest_producer_;
    spl::shared_ptr<frame_producer> source_producer_ = frame_producer::empty();

    bool paused_ = false;

  public:
    transition_producer(const spl::shared_ptr<frame_producer>& dest,
                        const transition_info&                 info)
        : info_(info)
        , dest_producer_(dest)
    {
    }

    // frame_producer

    void leading_producer(const spl::shared_ptr<frame_producer>& producer) override { source_producer_ = producer; }

    spl::shared_ptr<frame_producer> following_producer() const override
    {
        return current_frame_ >= info_.duration ? dest_producer_ : core::frame_producer::empty();
    }

    draw_frame receive_impl() override
    {
        CASPAR_SCOPE_EXIT
        {
            state_.clear();
            state_.insert_or_assign(dest_producer_->state());

            state_["transition/frame"] = { current_frame_, info_.duration };
            state_["transition/type"] = [&]() -> std::string
            {
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
        };

        auto dest   = draw_frame{};
        auto source = draw_frame{};

        tbb::parallel_invoke(
            [&] {
                dest = dest_producer_->receive();
                if (!dest) {
                    dest = dest_producer_->last_frame();
                }
            },
            [&] {
                source = source_producer_->receive();
                if (!source) {
                    source = source_producer_->last_frame();
                }
            });

        if (!dest) {
            return source;
        }

        if (current_frame_ >= info_.duration) {
            return dest;
        }

        current_frame_ += 1;

        return compose(std::move(dest), std::move(source));
    }

    uint32_t nb_frames() const override { return dest_producer_->nb_frames(); }

    uint32_t frame_number() const override { return dest_producer_->frame_number(); }

    std::wstring print() const override
    {
        return L"transition[" + source_producer_->print() + L"=>" + dest_producer_->print() + L"]";
    }

    std::wstring name() const override { return L"transition"; }

    std::future<std::wstring> call(const std::vector<std::wstring>& params) override
    {
        return dest_producer_->call(params);
    }

    draw_frame compose(draw_frame dst_frame, draw_frame src_frame) const
    {
        if (info_.type == transition_type::cut) {
            return src_frame;
        }

        const double delta = info_.tweener(current_frame_ , 0.0, 1.0, static_cast<double>(info_.duration));

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
            dst_frame.transform().image_transform.clip_scale[0] = delta;
        }

        return draw_frame::over(src_frame, dst_frame);
    }

    const monitor::state& state() { return state_; }
};

spl::shared_ptr<frame_producer> create_transition_producer(const spl::shared_ptr<frame_producer>& destination,
                                                           const transition_info&                 info)
{
    return spl::make_shared<transition_producer>(destination, info);
}

}} // namespace caspar::core
