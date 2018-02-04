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

#include "StdAfx.h"

#include "video_channel.h"

#include "video_format.h"

#include "consumer/output.h"
#include "frame/draw_frame.h"
#include "frame/frame.h"
#include "frame/frame_factory.h"
#include "mixer/mixer.h"
#include "producer/stage.h"

#include <common/diagnostics/graph.h>
#include <common/env.h>
#include <common/executor.h>
#include <common/future.h>
#include <common/timer.h>

#include <core/diagnostics/call_context.h>
#include <core/mixer/image/image_mixer.h>

#include <boost/lexical_cast.hpp>

#include <mutex>
#include <string>
#include <unordered_map>

namespace caspar { namespace core {

struct video_channel::impl final
{
    spl::shared_ptr<monitor::subject> monitor_subject_;

    const int index_;

    mutable std::mutex      format_desc_mutex_;
    core::video_format_desc format_desc_;

    const spl::shared_ptr<caspar::diagnostics::graph> graph_ = [](int index) {
        core::diagnostics::scoped_call_context save;
        core::diagnostics::call_context::for_thread().video_channel = index;
        return spl::make_shared<caspar::diagnostics::graph>();
    }(index_);

    caspar::core::output         output_;
    std::future<void>            output_ready_for_frame_ = make_ready_future();
    spl::shared_ptr<image_mixer> image_mixer_;
    caspar::core::mixer          mixer_;
    caspar::core::stage          stage_;

    mutable std::mutex                                 tick_listeners_mutex_;
    int64_t                                            last_tick_listener_id = 0;
    std::unordered_map<int64_t, std::function<void()>> tick_listeners_;

    executor executor_{L"video_channel " + boost::lexical_cast<std::wstring>(index_)};

  public:
    impl(int index, const core::video_format_desc& format_desc, std::unique_ptr<image_mixer> image_mixer)
        : monitor_subject_(spl::make_shared<monitor::subject>("/channel/" + boost::lexical_cast<std::string>(index)))
        , index_(index)
        , format_desc_(format_desc)
        , output_(graph_, format_desc, index)
        , image_mixer_(std::move(image_mixer))
        , mixer_(index, graph_, image_mixer_)
        , stage_(index, graph_)
    {
        graph_->set_color("tick-time", caspar::diagnostics::color(0.0f, 0.6f, 0.9f));
        graph_->set_text(print());
        caspar::diagnostics::register_graph(graph_);

        output_.monitor_output().attach_parent(monitor_subject_);
        mixer_.monitor_output().attach_parent(monitor_subject_);
        stage_.monitor_output().attach_parent(monitor_subject_);

        executor_.begin_invoke([=] { tick(); });

        CASPAR_LOG(info) << print() << " Successfully Initialized.";
    }

    ~impl() { CASPAR_LOG(info) << print() << " Uninitializing."; }

    core::video_format_desc video_format_desc() const
    {
        std::lock_guard<std::mutex> lock(format_desc_mutex_);
        return format_desc_;
    }

    void video_format_desc(const core::video_format_desc& format_desc)
    {
        std::lock_guard<std::mutex> lock(format_desc_mutex_);
        format_desc_ = format_desc;
        stage_.clear();
    }

    void invoke_tick_listeners()
    {
        std::lock_guard<std::mutex> lock(tick_listeners_mutex_);

        auto listeners = tick_listeners_;
        ;

        for (auto listener : listeners) {
            try {
                listener.second();
            } catch (...) {
                CASPAR_LOG_CURRENT_EXCEPTION();
            }
        }
    }

    void tick()
    {
        try {
            invoke_tick_listeners();

            auto format_desc = video_format_desc();

            caspar::timer frame_timer;

            // Produce

            auto stage_frames = stage_(format_desc);

            // Mix

            auto mixed_frame = mixer_(std::move(stage_frames), format_desc);

            // Consume

            output_ready_for_frame_ = output_(std::move(mixed_frame), format_desc);
            output_ready_for_frame_.get();

            auto frame_time = frame_timer.elapsed() * format_desc.fps * 0.5;
            graph_->set_value("tick-time", frame_time);

            *monitor_subject_ << monitor::message("/profiler/time") % frame_timer.elapsed() %
                                     (1.0 / video_format_desc().fps)
                              << monitor::message("/format") % format_desc.name;
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
        }

        if (executor_.is_running())
            executor_.begin_invoke([=] { tick(); });
    }

    std::wstring print() const
    {
        return L"video_channel[" + boost::lexical_cast<std::wstring>(index_) + L"|" + video_format_desc().name + L"]";
    }

    int index() const { return index_; }

    std::shared_ptr<void> add_tick_listener(std::function<void()> listener)
    {
        std::lock_guard<std::mutex> lock(tick_listeners_mutex_);

        auto tick_listener_id = last_tick_listener_id++;
        tick_listeners_.insert(std::make_pair(tick_listener_id, listener));

        return std::shared_ptr<void>(nullptr, [=](void*) {
            std::lock_guard<std::mutex> lock(tick_listeners_mutex_);
            tick_listeners_.erase(tick_listener_id);
        });
    }
};

video_channel::video_channel(int                            index,
                             const core::video_format_desc& format_desc,
                             std::unique_ptr<image_mixer>   image_mixer)
    : impl_(new impl(index, format_desc, std::move(image_mixer)))
{
}
video_channel::~video_channel() {}
const stage&                   video_channel::stage() const { return impl_->stage_; }
stage&                         video_channel::stage() { return impl_->stage_; }
const mixer&                   video_channel::mixer() const { return impl_->mixer_; }
mixer&                         video_channel::mixer() { return impl_->mixer_; }
const output&                  video_channel::output() const { return impl_->output_; }
output&                        video_channel::output() { return impl_->output_; }
spl::shared_ptr<frame_factory> video_channel::frame_factory() { return impl_->image_mixer_; }
core::video_format_desc        video_channel::video_format_desc() const { return impl_->video_format_desc(); }
void                           core::video_channel::video_format_desc(const core::video_format_desc& format_desc)
{
    impl_->video_format_desc(format_desc);
}
int                   video_channel::index() const { return impl_->index(); }
monitor::subject&     video_channel::monitor_output() { return *impl_->monitor_subject_; }
std::shared_ptr<void> video_channel::add_tick_listener(std::function<void()> listener)
{
    return impl_->add_tick_listener(std::move(listener));
}

}} // namespace caspar::core
