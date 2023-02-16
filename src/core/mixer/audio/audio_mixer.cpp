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

#include "audio_mixer.h"

#include <core/frame/frame.h>
#include <core/frame/frame_transform.h>
#include <core/monitor/monitor.h>

#include <common/diagnostics/graph.h>

#include <boost/container/flat_map.hpp>
#include <boost/range/algorithm.hpp>

#include <atomic>
#include <stack>
#include <vector>

namespace caspar { namespace core {

using namespace boost::container;

struct audio_item
{
    audio_transform      transform;
    array<const int32_t> samples;
};

using audio_buffer_ps = std::vector<double>;

struct audio_mixer::impl
{
    monitor::state                      state_;
    std::stack<core::audio_transform>   transform_stack_;
    std::vector<audio_item>             items_;
    std::atomic<float>                  master_volume_{1.0f};
    spl::shared_ptr<diagnostics::graph> graph_;

    impl(const impl&)            = delete;
    impl& operator=(const impl&) = delete;

    impl(spl::shared_ptr<diagnostics::graph> graph)
        : graph_(std::move(graph))
    {
        graph_->set_color("volume", diagnostics::color(1.0f, 0.8f, 0.1f));
        graph_->set_color("audio-clipping", diagnostics::color(0.3f, 0.6f, 0.3f));
        transform_stack_.push(core::audio_transform());
    }

    void push(const frame_transform& transform)
    {
        transform_stack_.push(transform_stack_.top() * transform.audio_transform);
    }

    void visit(const const_frame& frame)
    {
        if (transform_stack_.top().volume < 0.002 || !frame.audio_data())
            return;

        audio_item item;
        item.transform = transform_stack_.top();
        item.samples   = frame.audio_data();

        items_.push_back(std::move(item));
    }

    void pop() { transform_stack_.pop(); }

    void set_master_volume(float volume) { master_volume_ = volume; }

    float get_master_volume() { return master_volume_; }

    array<const int32_t> mix(const video_format_desc& format_desc, int nb_samples)
    {
        auto channels = format_desc.audio_channels;
        auto items    = std::move(items_);
        auto result   = std::vector<int32_t>(nb_samples * channels, 0);

        auto mixed = std::vector<double>(nb_samples * channels, 0.0f);

        for (auto& item : items) {
            auto ptr  = item.samples.data();
            auto size = result.size();
            for (auto n = 0; n < size; ++n) {
                if (n < item.samples.size()) {
                    mixed[n] = static_cast<double>(ptr[n]) * item.transform.volume + mixed[n];
                } else {
                    auto offset = (item.samples.size()) - (channels - (n % channels));
                    mixed[n]    = static_cast<double>(ptr[offset]) * item.transform.volume + mixed[n];
                }
            }
        }

        auto master_volume = master_volume_.load();
        for (auto n = 0; n < mixed.size(); ++n) {
            auto sample = mixed[n] * master_volume;
            if (sample > std::numeric_limits<int32_t>::max()) {
                result[n] = std::numeric_limits<int32_t>::max();
            } else if (sample < std::numeric_limits<int32_t>::min()) {
                result[n] = std::numeric_limits<int32_t>::min();
            } else {
                result[n] = static_cast<int32_t>(sample);
            }
        }

        auto max = std::vector<int32_t>(channels, std::numeric_limits<int32_t>::min());
        for (size_t n = 0; n < result.size(); n += channels) {
            for (int ch = 0; ch < channels; ++ch) {
                max[ch] = std::max(max[ch], std::abs(result[n + ch]));
            }
        }

        if (boost::range::count_if(max, [](auto val) { return val >= std::numeric_limits<int32_t>::max(); }) > 0) {
            graph_->set_tag(diagnostics::tag_severity::WARNING, "audio-clipping");
        }

        state_["volume"] = std::move(max);

        graph_->set_value("volume",
                          static_cast<double>(*boost::max_element(max)) / std::numeric_limits<int32_t>::max());

        return std::move(result);
    }
};

audio_mixer::audio_mixer(spl::shared_ptr<diagnostics::graph> graph)
    : impl_(new impl(std::move(graph)))
{
}
void                 audio_mixer::push(const frame_transform& transform) { impl_->push(transform); }
void                 audio_mixer::visit(const const_frame& frame) { impl_->visit(frame); }
void                 audio_mixer::pop() { impl_->pop(); }
void                 audio_mixer::set_master_volume(float volume) { impl_->set_master_volume(volume); }
float                audio_mixer::get_master_volume() { return impl_->get_master_volume(); }
array<const int32_t> audio_mixer::operator()(const video_format_desc& format_desc, int nb_samples)
{
    return impl_->mix(format_desc, nb_samples);
}
core::monitor::state audio_mixer::state() const { return impl_->state_; }

}} // namespace caspar::core
