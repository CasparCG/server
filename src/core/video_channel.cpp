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

#include "common/os/thread.h"
#include "video_channel.h"

#include "video_format.h"

#include "consumer/output.h"
#include "frame/draw_frame.h"
#include "frame/frame.h"
#include "frame/frame_factory.h"
#include "mixer/mixer.h"
#include "producer/stage.h"

#include <common/diagnostics/graph.h>
#include <common/executor.h>
#include <common/timer.h>

#include <core/diagnostics/call_context.h>
#include <core/mixer/image/image_mixer.h>

#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace caspar { namespace core {

bool operator<(const route_id& a, const route_id& b) { return a.mode + (a.index << 2) < b.mode + (b.index << 2); }

struct video_channel::impl final
{
    monitor::state state_;

    const int index_;

    mutable std::mutex      format_desc_mutex_;
    core::video_format_desc format_desc_;

    const spl::shared_ptr<caspar::diagnostics::graph> graph_ = [](int index) {
        core::diagnostics::scoped_call_context save;
        core::diagnostics::call_context::for_thread().video_channel = index;
        return spl::make_shared<caspar::diagnostics::graph>();
    }(index_);

    caspar::core::output         output_;
    spl::shared_ptr<image_mixer> image_mixer_;
    caspar::core::mixer          mixer_;
    std::shared_ptr<core::stage> stage_;

    uint64_t frame_counter_ = 0;

    std::function<void(core::monitor::state)> tick_;

    std::map<route_id, std::weak_ptr<core::route>> routes_;
    std::mutex                                     routes_mutex_;

    std::atomic<bool> abort_request_{false};
    std::thread       thread_;

    std::function<void(int, const layer_frame&)> routesCb = [&](int layer, const layer_frame& layer_frame) {
        std::lock_guard<std::mutex> lock(routes_mutex_);
        for (auto& r : routes_) {
            // if this layer is the source for this route, push the frame to the route producers
            if (layer == r.first.index) {
                auto route = r.second.lock();
                if (!route)
                    continue;

                if (r.first.index == -1) {
                    route->signal(layer_frame.foreground);
                } else if (r.first.mode == route_mode::background ||
                           (r.first.mode == route_mode::next && layer_frame.has_background)) {
                    route->signal(draw_frame::pop(layer_frame.background));
                } else {
                    route->signal(draw_frame::pop(layer_frame.foreground));
                }
            }
        }
    };

  public:
    impl(int                                       index,
         const core::video_format_desc&            format_desc,
         std::unique_ptr<image_mixer>              image_mixer,
         std::function<void(core::monitor::state)> tick)
        : index_(index)
        , format_desc_(format_desc)
        , output_(graph_, format_desc, index)
        , image_mixer_(std::move(image_mixer))
        , mixer_(index, graph_, image_mixer_)
        , stage_(std::make_shared<core::stage>(index, graph_))
        , tick_(std::move(tick))
    {
        graph_->set_color("produce-time", caspar::diagnostics::color(0.0f, 1.0f, 0.0f));
        graph_->set_color("mix-time", caspar::diagnostics::color(1.0f, 0.0f, 0.9f, 0.8f));
        graph_->set_color("consume-time", caspar::diagnostics::color(1.0f, 0.4f, 0.0f, 0.8f));
        graph_->set_color("frame-time", caspar::diagnostics::color(1.0f, 0.4f, 0.4f, 0.8f));
        graph_->set_color("osc-time", caspar::diagnostics::color(0.3f, 0.4f, 0.0f, 0.8f));
        graph_->set_text(print());
        caspar::diagnostics::register_graph(graph_);

        CASPAR_LOG(info) << print() << " Successfully Initialized.";

        thread_ = std::thread([=] {
            set_thread_realtime_priority();
            set_thread_name(L"channel-" + std::to_wstring(index_));

            while (!abort_request_) {
                try {
                    core::video_format_desc format_desc;
                    {
                        std::lock_guard<std::mutex> lock(format_desc_mutex_);
                        format_desc = format_desc_;
                    }

                    frame_counter_ += 1;

                    auto nb_samples = format_desc.audio_cadence[frame_counter_ % format_desc.audio_cadence.size()];

                    caspar::timer frame_timer;

                    // Determine all layers that need a frame from the background producer
                    std::vector<int> background_routes = {};
                    {
                        std::lock_guard<std::mutex> lock(routes_mutex_);

                        for (auto& r : routes_) {
                            // Ensure pointer is still valid
                            if (!r.second.lock())
                                continue;

                            if (r.first.mode != route_mode::foreground) {
                                background_routes.push_back(r.first.index);
                            }
                        }
                    }

                    // Produce
                    caspar::timer produce_timer;
                    auto          stage_frames = (*stage_)(format_desc, nb_samples, background_routes, routesCb);
                    graph_->set_value("produce-time", produce_timer.elapsed() * format_desc.fps * 0.5);

                    // Mix
                    caspar::timer mix_timer;
                    auto          mixed_frame = mixer_(stage_frames, format_desc, nb_samples);
                    graph_->set_value("mix-time", mix_timer.elapsed() * format_desc.fps * 0.5);

                    // Consume
                    caspar::timer consume_timer;
                    output_(std::move(mixed_frame), format_desc);
                    graph_->set_value("consume-time", consume_timer.elapsed() * format_desc.fps * 0.5);

                    graph_->set_value("frame-time", frame_timer.elapsed() * format_desc.fps * 0.5);

                    monitor::state state = {};
                    state["stage"]       = stage_->state();
                    state["mixer"]       = mixer_.state();
                    state["output"]      = output_.state();
                    state["framerate"]   = {format_desc_.framerate.numerator(), format_desc_.framerate.denominator()};
                    state_               = state;

                    caspar::timer osc_timer;
                    tick_(state_);
                    graph_->set_value("osc-time", osc_timer.elapsed() * format_desc.fps * 0.5);
                } catch (...) {
                    CASPAR_LOG_CURRENT_EXCEPTION();
                }
            }
        });
    }

    ~impl()
    {
        CASPAR_LOG(info) << print() << " Uninitializing.";
        abort_request_ = true;
        thread_.join();
    }

    std::shared_ptr<core::route> route(int index = -1, route_mode mode = route_mode::foreground)
    {
        std::lock_guard<std::mutex> lock(routes_mutex_);

        route_id id = {};
        id.index    = index;
        id.mode     = mode;

        auto route = routes_[id].lock();
        if (!route) {
            route              = std::make_shared<core::route>();
            route->format_desc = format_desc_;
            route->name        = std::to_wstring(index_);
            if (index != -1) {
                route->name += L"/" + std::to_wstring(index);
            }
            if (mode == route_mode::background) {
                route->name += L"/background";
            } else if (mode == route_mode::next) {
                route->name += L"/next";
            }
            routes_[id] = route;
        }

        return route;
    }

    core::video_format_desc video_format_desc() const
    {
        std::lock_guard<std::mutex> lock(format_desc_mutex_);
        return format_desc_;
    }

    void video_format_desc(const core::video_format_desc& format_desc)
    {
        stage_->clear();
        {
            std::lock_guard<std::mutex> lock(format_desc_mutex_);
            format_desc_ = format_desc;
        }
        graph_->set_text(print());
    }

    std::wstring print() const
    {
        return L"video_channel[" + std::to_wstring(index_) + L"|" + video_format_desc().name + L"]";
    }

    int index() const { return index_; }
};

video_channel::video_channel(int                                       index,
                             const core::video_format_desc&            format_desc,
                             std::unique_ptr<image_mixer>              image_mixer,
                             std::function<void(core::monitor::state)> tick)
    : impl_(new impl(index, format_desc, std::move(image_mixer), std::move(tick)))
{
}
video_channel::~video_channel() {}
const std::shared_ptr<core::stage>& video_channel::stage() const { return impl_->stage_; }
std::shared_ptr<core::stage>&       video_channel::stage() { return impl_->stage_; }
const mixer&                        video_channel::mixer() const { return impl_->mixer_; }
mixer&                              video_channel::mixer() { return impl_->mixer_; }
const output&                       video_channel::output() const { return impl_->output_; }
output&                             video_channel::output() { return impl_->output_; }
spl::shared_ptr<frame_factory>      video_channel::frame_factory() { return impl_->image_mixer_; }
core::video_format_desc             video_channel::video_format_desc() const { return impl_->video_format_desc(); }
void                                core::video_channel::video_format_desc(const core::video_format_desc& format_desc)
{
    impl_->video_format_desc(format_desc);
}
int                  video_channel::index() const { return impl_->index(); }
core::monitor::state video_channel::state() const { return impl_->state_; }

std::shared_ptr<route> video_channel::route(int index, route_mode mode) { return impl_->route(index, mode); }

}} // namespace caspar::core
