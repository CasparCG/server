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
#include <map>
#include <stack>
#include <vector>

namespace caspar { namespace core {

using namespace boost::container;

struct audio_item
{
    const void*          tag = nullptr;
    audio_transform      transform;
    array<const int32_t> samples;
};

using audio_buffer_ps = std::vector<double>;

struct audio_mixer::impl
{
    monitor::state                              state_;
    std::stack<core::audio_transform>           transform_stack_;
    std::vector<audio_item>                     items_;
    std::map<const void*, std::vector<int32_t>> audio_streams_;    // For audio cadence handling
    std::map<const void*, double>               previous_volumes_; // For audio transitions
    video_format_desc                           format_desc_;
    std::atomic<float>                          master_volume_{1.0f};
    spl::shared_ptr<diagnostics::graph>         graph_;
    size_t                                      max_expected_cadence_samples_{0};
    size_t                                      max_buffer_size_{0};
    bool                                        has_variable_cadence_{false};
    std::vector<int32_t>                        silence_buffer_;
    int                                         channels_{0};

    impl(spl::shared_ptr<diagnostics::graph> graph)
        : graph_(std::move(graph))
    {
        graph_->set_color("volume", diagnostics::color(1.0f, 0.8f, 0.1f));
        graph_->set_color("audio-clipping", diagnostics::color(0.3f, 0.6f, 0.3f));
        graph_->set_color("audio-buffer-overflow", diagnostics::color(0.6f, 0.3f, 0.3f));
        transform_stack_.push(core::audio_transform());
    }

    impl(const impl&)            = delete;
    impl& operator=(const impl&) = delete;

    void push(const frame_transform& transform)
    {
        transform_stack_.push(transform_stack_.top() * transform.audio_transform);
    }

    void visit(const const_frame& frame)
    {
        if (transform_stack_.top().volume < 0.002 || !frame.audio_data())
            return;

        items_.push_back(std::move(audio_item{frame.stream_tag(), transform_stack_.top(), frame.audio_data()}));
    }

    void pop() { transform_stack_.pop(); }

    void set_master_volume(float volume) { master_volume_ = volume; }

    float get_master_volume() { return master_volume_; }

    array<const int32_t> mix(const video_format_desc& format_desc, int nb_samples)
    {
        if (format_desc_ != format_desc) {
            audio_streams_.clear();
            previous_volumes_.clear();

            format_desc_ = format_desc;
            channels_    = format_desc.audio_channels;

            // Calculate these values only when format changes
            max_expected_cadence_samples_ = 0;
            if (!format_desc.audio_cadence.empty()) {
                max_expected_cadence_samples_ =
                    *std::max_element(format_desc.audio_cadence.begin(), format_desc.audio_cadence.end());
            }

            // Pre-calculate max buffer size based on max cadence (2 frames worth)
            max_buffer_size_ = channels_;
            if (max_expected_cadence_samples_ > 0) {
                max_buffer_size_ *= 2 * max_expected_cadence_samples_;
            } else {
                max_buffer_size_ *= 4000; // Fallback: 2 frames × ~2000 samples
            }

            has_variable_cadence_ = format_desc.audio_cadence.size() > 1;

            if (has_variable_cadence_) {
                silence_buffer_.resize(channels_, 0);
            } else {
                silence_buffer_.clear();
            }
        }

        auto items  = std::move(items_);
        auto result = std::vector<int32_t>(size_t(nb_samples) * channels_, 0);
        auto mixed  = std::vector<double>(size_t(nb_samples) * channels_, 0.0f);

        std::map<const void*, std::vector<int32_t>> next_audio_streams;
        std::map<const void*, double>               next_volumes;

        for (auto& item : items) {
            auto ptr       = item.samples.data();
            auto item_size = item.samples.size();
            auto dst_size  = result.size();

            size_t         last_size = 0;
            const int32_t* last_ptr  = nullptr;

            if (has_variable_cadence_) {
                auto audio_stream = audio_streams_.find(item.tag);
                if (audio_stream != audio_streams_.end()) {
                    last_size = audio_stream->second.size();
                    last_ptr  = audio_stream->second.data();
                } else if (nullptr != item.tag) {
                    // Insert a sample of silence at startup
                    // Covers the startup case where there may be a cadence mismatch
                    // The sample of silence will be output before any valid audio data from the source
                    last_size = channels_;
                    last_ptr  = silence_buffer_.data();
                }
            }

            // Get previous volume for this tag, defaulting to current volume
            double previous_volume = item.transform.volume;
            auto   prev_vol_it     = previous_volumes_.find(item.tag);
            if (prev_vol_it != previous_volumes_.end()) {
                previous_volume = prev_vol_it->second;
            }

            // Store current volume for next frame
            next_volumes[item.tag] = item.transform.volume;

            // Sample collection and volume application loop
            for (auto n = 0; n < dst_size; ++n) {
                double sample_value = 0.0;

                if (last_ptr && n < last_size) {
                    sample_value = static_cast<double>(last_ptr[n]);
                } else if (n < last_size + item_size) {
                    sample_value = static_cast<double>(ptr[n - last_size]);
                } else {
                    // If we run out of samples, hold the last sample value per channel
                    int channel_pos = n % channels_;
                    int offset      = int(item_size) - (channels_ - channel_pos);
                    if (offset < 0) {
                        offset = channel_pos;
                    }
                    sample_value = static_cast<double>(ptr[offset]);
                }

                double applied_volume = item.transform.volume;

                // Apply sample-level volume ramping if not immediate volume and there's a volume change
                if (!item.transform.immediate_volume && std::abs(item.transform.volume - previous_volume) > 0.001) {
                    size_t sample_index  = n / channels_;
                    size_t total_samples = dst_size / channels_;

                    // Calculate linear interpolation position (0.0 to 1.0)
                    double position = total_samples > 1
                                          ? static_cast<double>(sample_index) / static_cast<double>(total_samples - 1)
                                          : 1.0;
                    position        = std::min(1.0, std::max(0.0, position)); // Clamp between 0 and 1

                    // Linear interpolation between previous and current volume
                    applied_volume = previous_volume + (item.transform.volume - previous_volume) * position;
                }

                mixed[n] += sample_value * applied_volume;
            }

            if (has_variable_cadence_ && item.tag) {
                if (item_size + last_size > dst_size) {
                    // Calculate remaining samples after mixing the current frame
                    auto remaining_samples = item_size + last_size - dst_size;

                    // Apply the most restrictive limit and log if needed
                    if (remaining_samples > max_buffer_size_ || remaining_samples > item_size) {
                        graph_->set_tag(diagnostics::tag_severity::WARNING, "audio-buffer-overflow");

                        // Apply the most restrictive limit
                        remaining_samples = (max_buffer_size_ < item_size) ? max_buffer_size_ : item_size;
                    }

                    std::vector<int32_t> buf(remaining_samples);
                    // Calculate the correct offset in the source buffer
                    size_t offset = (dst_size > last_size) ? (dst_size - last_size) : 0;
                    if (offset < item_size) {
                        std::memcpy(buf.data(), ptr + offset, remaining_samples * sizeof(int32_t));
                        next_audio_streams[item.tag] = std::move(buf);
                    } else {
                        next_audio_streams[item.tag] = std::vector<int32_t>();
                    }
                } else {
                    next_audio_streams[item.tag] = std::vector<int32_t>();
                }
            }
        }

        previous_volumes_ = std::move(next_volumes);
        audio_streams_    = std::move(next_audio_streams);

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

        auto max = std::vector<int32_t>(channels_, std::numeric_limits<int32_t>::min());
        for (size_t n = 0; n < result.size(); n += channels_) {
            for (int ch = 0; ch < channels_; ++ch) {
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
