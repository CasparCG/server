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
#include "route_producer.h"

#include <common/diagnostics/graph.h>
#include <common/param.h>
#include <common/timer.h>

#include <core/frame/draw_frame.h>
#include <core/frame/frame.h>
#include <core/frame/frame_transform.h>
#include <core/frame/frame_visitor.h>
#include <core/monitor/monitor.h>
#include <core/producer/frame_producer.h>
#include <core/video_channel.h>

#include <boost/regex.hpp>
#include <boost/signals2.hpp>

#include <tbb/concurrent_queue.h>

#include <optional>
#include <stack>
#include <unordered_map>
#include <unordered_set>

namespace caspar { namespace core {

class fix_stream_tag : public frame_visitor
{
    const void*                                                route_producer_ptr_;
    std::stack<std::pair<frame_transform, std::vector<draw_frame>>> frames_stack_;
    std::optional<const_frame>                                 upd_frame_;
    
    fix_stream_tag(const fix_stream_tag&);
    fix_stream_tag& operator=(const fix_stream_tag&);

  public:
    fix_stream_tag(void* stream_tag)
        : route_producer_ptr_(stream_tag)
    {
        frames_stack_ = std::stack<std::pair<frame_transform, std::vector<draw_frame>>>();
        frames_stack_.emplace(frame_transform{}, std::vector<draw_frame>());
    }

    void push(const frame_transform& transform) {
        frames_stack_.emplace(transform, std::vector<core::draw_frame>());
    }

    void visit(const const_frame& frame) {
        // Get original tag from the frame
        const void* source_tag = frame.stream_tag();
        
        // Calculate a unique but stable tag for this source
        // This calculation will always produce the same result for the same inputs
        intptr_t base_addr = reinterpret_cast<intptr_t>(route_producer_ptr_);
        intptr_t source_addr = reinterpret_cast<intptr_t>(source_tag);
        // Use XOR to create a unique value that combines route producer and source identities
        intptr_t unique_value = base_addr ^ source_addr ^ 0xDEADBEEF; // Constant helps avoid collisions
        const void* unique_tag = reinterpret_cast<const void*>(unique_value);
        
        // Apply the tag to the frame
        upd_frame_ = frame.with_tag(unique_tag);
    }

    void pop() {
        auto popped = frames_stack_.top();
        frames_stack_.pop();

        if (upd_frame_ != std::nullopt) {
            auto new_frame        = draw_frame(std::move(*upd_frame_));
            upd_frame_            = std::nullopt;
            new_frame.transform() = popped.first;
            frames_stack_.top().second.push_back(std::move(new_frame));
        } else {
            auto new_frame        = draw_frame(std::move(popped.second));
            new_frame.transform() = popped.first;
            frames_stack_.top().second.push_back(new_frame);
        }
    }

    draw_frame operator()(draw_frame frame) {
        frame.accept(*this);

        auto popped = frames_stack_.top();
        frames_stack_.pop();
        draw_frame result = draw_frame(std::move(popped.second));

        frames_stack_ = std::stack<std::pair<frame_transform, std::vector<draw_frame>>>();
        frames_stack_.emplace(frame_transform{}, std::vector<draw_frame>());
        return result;
    }
};

class route_producer
    : public frame_producer
    , public route_control
    , public std::enable_shared_from_this<route_producer>
{
    spl::shared_ptr<diagnostics::graph> graph_;

    tbb::concurrent_bounded_queue<std::pair<core::draw_frame, core::draw_frame>> buffer_;

    caspar::timer produce_timer_;
    caspar::timer consume_timer_;

    std::shared_ptr<route> route_;
    const video_format_desc format_desc_;

    std::optional<std::pair<core::draw_frame, core::draw_frame>> frame_;
    int                                                          source_channel_;
    int                                                          source_layer_;
    fix_stream_tag                                               tag_fix_;
    core::video_format_desc                                      source_format_;
    bool                                                         is_cross_channel_ = false;

    boost::signals2::scoped_connection connection_;

    int get_source_channel() const override { return source_channel_; }
    int get_source_layer() const override { return source_layer_; }

    // set the buffer depth to 2 for cross-channel routes, 1 otherwise
    void set_cross_channel(bool cross) override
    {
        is_cross_channel_ = cross;
        if (cross) {
            buffer_.set_capacity(2);
            source_format_ = route_->format_desc;
        } else {
            buffer_.set_capacity(1);
            source_format_ = core::video_format_desc();
        }
    }

  public:
    route_producer(std::shared_ptr<route> route, video_format_desc format_desc, int buffer, int source_channel, int source_layer)
        : route_(route)
        , format_desc_(format_desc)
        , source_channel_(source_channel)
        , source_layer_(source_layer)
        , tag_fix_(this)
    {
        graph_ = spl::make_shared<diagnostics::graph>();
        buffer_.set_capacity(buffer > 0 ? buffer : 1);

        graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.3f));
        graph_->set_color("produce-time", caspar::diagnostics::color(0.0f, 1.0f, 0.0f));
        graph_->set_color("consume-time", caspar::diagnostics::color(1.0f, 0.4f, 0.0f, 0.8f));
        graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
        graph_->set_text(print());
        diagnostics::register_graph(graph_);

        CASPAR_LOG(debug) << print() << L" Initialized";
    }

    void connect_slot()
    {
        auto weak_self = weak_from_this();
        connection_ =
            route_->signal.connect([weak_self](const core::draw_frame& frame1, const core::draw_frame& frame2) {
                if (auto self = weak_self.lock()) {
                    auto frame1b = frame1;
                    if (!frame1b) {
                        // We got a frame, so ensure it is a real frame (otherwise the layer gets confused)
                        frame1b = core::draw_frame::push(frame1);
                    }

                    // Update the tag in the frame to allow the audio mixer to distinguish between the source frame and the routed frame
                    frame1b = self->tag_fix_(frame1b);

                    auto frame2b = frame2;
                    if (!frame2b) {
                        // Ensure that any interlaced channel will repeat frames instead of showing black.
                        frame2b = frame1b;
                    } else {
                        // For interlaced formats, ensure field B gets the proper tag as well
                        frame2b = self->tag_fix_(frame2b);
                    }

                    if (!self->buffer_.try_push(std::make_pair(frame1b, frame2b))) {
                        self->graph_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
                    }
                    self->graph_->set_value("produce-time",
                                            self->produce_timer_.elapsed() * self->route_->format_desc.fps * 0.5);
                    self->produce_timer_.restart();
                }
            });
    }

    draw_frame last_frame(const core::video_field field) override
    {
        if (!frame_) {
            std::pair<core::draw_frame, core::draw_frame> frame;
            if (buffer_.try_pop(frame)) {
                frame_ = frame;
            }
        }

        if (!frame_) {
            return core::draw_frame{};
        }

        if (field == core::video_field::b) {
            return core::draw_frame::still(frame_->second);
        } else {
            return core::draw_frame::still(frame_->first);
        }
    }

    core::video_field next_field_ = core::video_field::a;
    draw_frame receive_impl(core::video_field field, int nb_samples) override
    {
        // If going i -> p, alternate between the fields
        // Note: this doesn't fix the audio if going 50i -> 25p
        if (field == core::video_field::progressive && source_format_.field_count != 1 && format_desc_.fps >= source_format_.fps) {
            field = next_field_;
            next_field_ = (next_field_ == core::video_field::a) ? core::video_field::b : core::video_field::a;
        }

        if (field == core::video_field::a || field == core::video_field::progressive) {
            std::pair<core::draw_frame, core::draw_frame> frame;
            if (!buffer_.try_pop(frame)) {
                graph_->set_tag(diagnostics::tag_severity::WARNING, "late-frame");
            } else {
                frame_ = frame;
            }
        }

        graph_->set_value("consume-time", consume_timer_.elapsed() * route_->format_desc.fps * 0.5);
        consume_timer_.restart();

        if (!frame_) {
            return core::draw_frame{};
        }

        if (field == core::video_field::b) {
            return frame_->second;
        } else {
            return frame_->first;
        }
    }

    bool is_ready() override { return true; }

    std::wstring print() const override { return L"route[" + route_->name + L"]"; }

    std::wstring name() const override { return L"route"; }

    core::monitor::state state() const override
    {
        core::monitor::state state;
        state["route/channel"] = source_channel_;

        if (source_layer_ > -1) {
            state["route/layer"] = source_layer_;
        }

        return state;
    }
};

spl::shared_ptr<core::frame_producer> create_route_producer(const core::frame_producer_dependencies& dependencies,
                                                            const std::vector<std::wstring>&         params)
{
    static boost::wregex expr(L"route://(?<CHANNEL>\\d+)(-(?<LAYER>\\d+))?", boost::regex::icase);
    boost::wsmatch       what;

    if (params.empty() || !boost::regex_match(params.at(0), what, expr)) {
        return core::frame_producer::empty();
    }

    auto channel = std::stoi(what["CHANNEL"].str());
    auto layer   = what["LAYER"].matched ? std::stoi(what["LAYER"].str()) : -1;

    auto mode = core::route_mode::foreground;
    if (layer >= 0) {
        if (contains_param(L"BACKGROUND", params))
            mode = core::route_mode::background;
        else if (contains_param(L"NEXT", params))
            mode = core::route_mode::next;
    }

    auto channel_it =
        std::find_if(dependencies.channels.begin(),
                     dependencies.channels.end(),
                     [=](const spl::shared_ptr<core::video_channel>& ch) { return ch->index() == channel; });

    if (channel_it == dependencies.channels.end()) {
        CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"No channel with id " + std::to_wstring(channel)));
    }

    auto buffer = get_param(L"BUFFER", params, 0);
    auto rp     = spl::make_shared<route_producer>((*channel_it)->route(layer, mode), dependencies.format_desc, buffer, channel, layer);
    rp->connect_slot();
    return rp;
}

}} // namespace caspar::core
