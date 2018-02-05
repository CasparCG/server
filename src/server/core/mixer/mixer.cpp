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

#include "../StdAfx.h"

#include "mixer.h"

#include "../frame/frame.h"

#include "audio/audio_mixer.h"
#include "image/image_mixer.h"

#include <common/diagnostics/graph.h>
#include <common/env.h>
#include <common/except.h>
#include <common/executor.h>
#include <common/future.h>
#include <common/timer.h>

#include <core/frame/draw_frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/frame_transform.h>
#include <core/frame/pixel_format.h>
#include <core/video_format.h>

#include <boost/lexical_cast.hpp>

#include <tbb/concurrent_queue.h>

#include <atomic>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace caspar { namespace core {

struct mixer::impl : boost::noncopyable
{
    int                                  channel_index_;
    spl::shared_ptr<diagnostics::graph>  graph_;
    spl::shared_ptr<monitor::subject>    monitor_subject_ = spl::make_shared<monitor::subject>("/mixer");
    audio_mixer                          audio_mixer_{graph_};
    spl::shared_ptr<image_mixer>         image_mixer_;
    std::queue<std::future<const_frame>> buffer_;

  public:
    impl(int channel_index, spl::shared_ptr<diagnostics::graph> graph, spl::shared_ptr<image_mixer> image_mixer)
        : channel_index_(channel_index)
        , graph_(std::move(graph))
        , image_mixer_(std::move(image_mixer))
    {
        audio_mixer_.monitor_output().attach_parent(monitor_subject_);
    }

    const_frame operator()(std::map<int, draw_frame> frames, const video_format_desc& format_desc)
    {
        for (auto& frame : frames) {
            frame.second.accept(audio_mixer_);
            frame.second.transform().image_transform.layer_depth = 1;
            frame.second.accept(*image_mixer_);
        }

        auto image = (*image_mixer_)(format_desc);
        auto audio = audio_mixer_(format_desc);

        buffer_.push(std::async(std::launch::deferred, [image = std::move(image), audio = std::move(audio), graph = graph_, format_desc, tag = this]() mutable
        {
            auto desc = pixel_format_desc(pixel_format::bgra);
            desc.planes.push_back(pixel_format_desc::plane(format_desc.width, format_desc.height, 4));
            std::vector<array<const uint8_t>> image_data;
            image_data.emplace_back(std::move(image.get()));
            return const_frame(tag, std::move(image_data), std::move(audio), desc);
        }));

        if (buffer_.size() < 3) {
            return const_frame{};
        }

        auto frame = std::move(buffer_.front().get());
        buffer_.pop();
        return frame;
    }

    void set_master_volume(float volume)
    {
        audio_mixer_.set_master_volume(volume);
    }

    float get_master_volume()
    {
        return audio_mixer_.get_master_volume();
    }
};

mixer::mixer(int channel_index, spl::shared_ptr<diagnostics::graph> graph, spl::shared_ptr<image_mixer> image_mixer)
    : impl_(new impl(channel_index, std::move(graph), std::move(image_mixer)))
{
}
void        mixer::set_master_volume(float volume) { impl_->set_master_volume(volume); }
float       mixer::get_master_volume() { return impl_->get_master_volume(); }
const_frame mixer::operator()(std::map<int, draw_frame> frames, const video_format_desc& format_desc)
{
    return (*impl_)(std::move(frames), format_desc);
}
mutable_frame mixer::create_frame(const void* tag, const pixel_format_desc& desc)
{
    return impl_->image_mixer_->create_frame(tag, desc);
}
monitor::subject& mixer::monitor_output() { return *impl_->monitor_subject_; }
}} // namespace caspar::core
