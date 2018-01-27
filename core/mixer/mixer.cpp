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

#include <common/env.h>
#include <common/diagnostics/graph.h>
#include <common/except.h>

#include <core/frame/draw_frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/frame_transform.h>
#include <core/frame/pixel_format.h>
#include <core/frame/audio_channel_layout.h>
#include <core/video_format.h>

#include <boost/timer.hpp>

#include <atomic>
#include <memory>
#include <mutex>

namespace caspar { namespace core {

struct mixer::impl : boost::noncopyable
{
	int									channel_index_;
	spl::shared_ptr<diagnostics::graph>	graph_;
	spl::shared_ptr<monitor::subject>	monitor_subject_	= spl::make_shared<monitor::subject>("/mixer");
	audio_mixer							audio_mixer_		{ graph_ };
	spl::shared_ptr<image_mixer>		image_mixer_;

	std::atomic<bool>     				straighten_alpha_	= false;

	boost::asio::thread_pool            context_;
			
public:
	impl(int channel_index, spl::shared_ptr<diagnostics::graph> graph, spl::shared_ptr<image_mixer> image_mixer) 
		: channel_index_(channel_index)
		, graph_(std::move(graph))
		, image_mixer_(std::move(image_mixer))
		, context_(0)
	{			
		graph_->set_color("mix-time", diagnostics::color(1.0f, 0.0f, 0.9f, 0.8f));
		audio_mixer_.monitor_output().attach_parent(monitor_subject_);
	}
	
	const_frame operator()(std::map<int, draw_frame> frames, const video_format_desc& format_desc, const core::audio_channel_layout& channel_layout)
	{		
		boost::timer frame_timer;

		auto frame = [=]() mutable -> const_frame
		{		
			try
			{
				detail::set_current_aspect_ratio(
						static_cast<double>(format_desc.square_width)
						/ static_cast<double>(format_desc.square_height));

				for (auto& frame : frames)
				{
					frame.second.accept(audio_mixer_);
					frame.second.transform().image_transform.layer_depth = 1;
					frame.second.accept(*image_mixer_);
				}
				
				auto image = (*image_mixer_)(format_desc, straighten_alpha_);
				auto audio = audio_mixer_(format_desc, channel_layout);

				auto desc = core::pixel_format_desc(core::pixel_format::bgra);
				desc.planes.push_back(core::pixel_format_desc::plane(format_desc.width, format_desc.height, 4));
				return const_frame(std::move(image), std::move(audio), this, desc, channel_layout);
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				return const_frame::empty();
			}	
		}();

		auto mix_time = frame_timer.elapsed();
		graph_->set_value("mix-time", mix_time * format_desc.fps * 0.5);

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

	void set_straight_alpha_output(bool value)
	{
		straighten_alpha_ = value;
	}

	bool get_straight_alpha_output()
	{
		return straighten_alpha_;
	}
};
	
mixer::mixer(int channel_index, spl::shared_ptr<diagnostics::graph> graph, spl::shared_ptr<image_mixer> image_mixer) 
	: impl_(new impl(channel_index, std::move(graph), std::move(image_mixer))){}
void mixer::set_master_volume(float volume) { impl_->set_master_volume(volume); }
float mixer::get_master_volume() { return impl_->get_master_volume(); }
void mixer::set_straight_alpha_output(bool value) { impl_->set_straight_alpha_output(value); }
bool mixer::get_straight_alpha_output() { return impl_->get_straight_alpha_output(); }
const_frame mixer::operator()(std::map<int, draw_frame> frames, const video_format_desc& format_desc, const core::audio_channel_layout& channel_layout){ return (*impl_)(std::move(frames), format_desc, channel_layout); }
mutable_frame mixer::create_frame(const void* tag, const core::pixel_format_desc& desc, const core::audio_channel_layout& channel_layout) {return impl_->image_mixer_->create_frame(tag, desc, channel_layout);}
monitor::subject& mixer::monitor_output() { return *impl_->monitor_subject_; }
}}
