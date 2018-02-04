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

#include <boost/lexical_cast.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/distance.hpp>

#include <map>
#include <stack>
#include <vector>

namespace caspar { namespace core {

struct audio_item
{
    const void*          tag = nullptr;
    audio_transform      transform;
    array<const int32_t> audio_data;

    audio_item() {}

    audio_item(audio_item&& other)
        : tag(std::move(other.tag))
        , transform(std::move(other.transform))
        , audio_data(std::move(other.audio_data))
    {
    }
};

typedef std::vector<double> audio_buffer_ps;

struct audio_stream
{
    audio_transform prev_transform;
    audio_buffer_ps audio_data;
    bool            is_still = false;
};

struct audio_mixer::impl : boost::noncopyable
{
    monitor::subject                    monitor_subject_{"/audio"};
    std::stack<core::audio_transform>   transform_stack_;
    std::map<const void*, audio_stream> audio_streams_;
    std::vector<audio_item>             items_;
    std::vector<int>                    audio_cadence_;
    video_format_desc                   format_desc_;
    float                               master_volume_          = 1.0f;
    float                               previous_master_volume_ = master_volume_;
    spl::shared_ptr<diagnostics::graph> graph_;

  public:
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
        item.tag        = frame.stream_tag();
        item.transform  = transform_stack_.top();
        item.audio_data = frame.audio_data();

        if (item.transform.is_still)
            item.transform.volume = 0.0;

        items_.push_back(std::move(item));
    }

    void pop() { transform_stack_.pop(); }

    void set_master_volume(float volume) { master_volume_ = volume; }

    float get_master_volume() { return master_volume_; }

    array<int32_t> mix(const video_format_desc& format_desc)
    {
        if (format_desc_ != format_desc) {
            audio_streams_.clear();
            audio_cadence_ = format_desc.audio_cadence;
            format_desc_   = format_desc;
        }

        std::map<const void*, audio_stream> next_audio_streams;

        for (auto& item : items_) {
            audio_buffer_ps next_audio;

            auto next_transform = item.transform;
            auto prev_transform = next_transform;

            auto tag = item.tag;

            auto it    = next_audio_streams.find(tag);
            bool found = it != next_audio_streams.end();

            if (!found) {
                it    = audio_streams_.find(tag);
                found = it != audio_streams_.end();
            }

            if (found) {
                prev_transform = it->second.prev_transform;
                next_audio     = std::move(it->second.audio_data);
            }

            // Skip it if there is no existing audio stream and item has no audio-data.
            if (!found && !item.audio_data)
                continue;

            const double prev_volume = prev_transform.volume * previous_master_volume_;
            const double next_volume = next_transform.volume * master_volume_;

            // TODO: Move volume mixing into code below, in order to support audio sample counts not corresponding to
            // frame audio samples.
            auto alpha =
                (next_volume - prev_volume) / static_cast<double>(item.audio_data.size() / format_desc_.audio_channels);

            for (size_t n = 0; n < item.audio_data.size(); ++n) {
                auto sample_multiplier = (prev_volume + (n / format_desc_.audio_channels) * alpha);
                next_audio.push_back(item.audio_data.data()[n] * sample_multiplier);
            }

            next_audio_streams[tag].prev_transform =
                std::move(next_transform); // Store all active tags, inactive tags will be removed at the end.
            next_audio_streams[tag].audio_data = std::move(next_audio);
            next_audio_streams[tag].is_still   = item.transform.is_still;
        }

        previous_master_volume_ = master_volume_;
        items_.clear();

        audio_streams_ = std::move(next_audio_streams);

        if (audio_streams_.empty())
            audio_streams_[nullptr].audio_data = audio_buffer_ps(audio_size(audio_cadence_.front()), 0.0);

        audio_buffer_ps result_ps(audio_size(audio_cadence_.front()), 0.0);
        for (auto& stream : audio_streams_ | boost::adaptors::map_values) {
            if (stream.audio_data.size() < result_ps.size()) {
                auto samples = (result_ps.size() - stream.audio_data.size()) / format_desc_.audio_channels;
                CASPAR_LOG(trace) << L"[audio_mixer] Appended " << samples << L" zero samples";
                CASPAR_LOG(trace) << L"[audio_mixer] Actual number of samples "
                                  << stream.audio_data.size() / format_desc_.audio_channels;
                CASPAR_LOG(trace) << L"[audio_mixer] Wanted number of samples "
                                  << result_ps.size() / format_desc_.audio_channels;
                stream.audio_data.resize(result_ps.size(), 0.0);
            }

            auto out =
                boost::range::transform(result_ps, stream.audio_data, std::begin(result_ps), std::plus<double>());
            stream.audio_data.erase(std::begin(stream.audio_data),
                                    std::begin(stream.audio_data) + std::distance(std::begin(result_ps), out));
        }

        boost::range::rotate(audio_cadence_, std::begin(audio_cadence_) + 1);

        auto result = std::vector<int32_t>{};
        result.reserve(result_ps.size());
        const int32_t min_amplitude = std::numeric_limits<int32_t>::min();
        const int32_t max_amplitude = std::numeric_limits<int32_t>::max();
        bool          clipping      = false;
        boost::range::transform(result_ps, std::back_inserter(result), [&](double sample) {
            if (sample > max_amplitude) {
                clipping = true;
                return max_amplitude;
            } else if (sample < min_amplitude) {
                clipping = true;
                return min_amplitude;
            } else
                return static_cast<int32_t>(sample);
        });

        if (clipping)
            graph_->set_tag(diagnostics::tag_severity::WARNING, "audio-clipping");

        const int num_channels = format_desc_.audio_channels;
        monitor_subject_ << monitor::message("/nb_channels") % num_channels;

        auto max = std::vector<int32_t>(num_channels, std::numeric_limits<int32_t>::min());

        for (size_t n = 0; n < result.size(); n += num_channels)
            for (int ch = 0; ch < num_channels; ++ch)
                max[ch] = std::max(max[ch], std::abs(result[n + ch]));

        // Makes the dBFS of silence => -dynamic range of 32bit LPCM => about -192 dBFS
        // Otherwise it would be -infinity
        static const auto MIN_PFS = 0.5f / static_cast<float>(std::numeric_limits<int32_t>::max());

        for (int i = 0; i < num_channels; ++i) {
            const auto pFS  = max[i] / static_cast<float>(std::numeric_limits<int32_t>::max());
            const auto dBFS = 20.0f * std::log10(std::max(MIN_PFS, pFS));

            auto chan_str = boost::lexical_cast<std::string>(i + 1);

            monitor_subject_ << monitor::message("/" + chan_str + "/pFS") % pFS;
            monitor_subject_ << monitor::message("/" + chan_str + "/dBFS") % dBFS;
        }

        graph_->set_value("volume",
                          static_cast<double>(*boost::max_element(max)) / std::numeric_limits<int32_t>::max());

        return array<int32_t>(std::move(result));
    }

    size_t audio_size(size_t num_samples) const { return num_samples * format_desc_.audio_channels; }
};

audio_mixer::audio_mixer(spl::shared_ptr<diagnostics::graph> graph)
    : impl_(new impl(std::move(graph)))
{
}
void           audio_mixer::push(const frame_transform& transform) { impl_->push(transform); }
void           audio_mixer::visit(const const_frame& frame) { impl_->visit(frame); }
void           audio_mixer::pop() { impl_->pop(); }
void           audio_mixer::set_master_volume(float volume) { impl_->set_master_volume(volume); }
float          audio_mixer::get_master_volume() { return impl_->get_master_volume(); }
array<int32_t> audio_mixer::operator()(const video_format_desc& format_desc) { return impl_->mix(format_desc); }
monitor::subject&           audio_mixer::monitor_output() { return impl_->monitor_subject_; }

}} // namespace caspar::core
