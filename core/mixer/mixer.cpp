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
#include <common/executor.h>
#include <common/diagnostics/graph.h>
#include <common/except.h>
#include <common/gl/gl_check.h>

#include <core/frame/draw_frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/frame_transform.h>
#include <core/frame/pixel_format.h>
#include <core/video_format.h>

#include <boost/foreach.hpp>
#include <boost/timer.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/range/algorithm_ext.hpp>

#include <tbb/concurrent_queue.h>
#include <tbb/spin_mutex.h>

#include <unordered_map>
#include <vector>

namespace caspar { namespace core {

struct mixer::impl : boost::noncopyable
{				
	spl::shared_ptr<diagnostics::graph> graph_;
	audio_mixer							audio_mixer_;
	spl::shared_ptr<image_mixer>		image_mixer_;
	
	std::unordered_map<int, blend_mode>	blend_modes_;
			
	executor executor_;

public:
	impl(spl::shared_ptr<diagnostics::graph> graph, spl::shared_ptr<image_mixer> image_mixer) 
		: graph_(std::move(graph))
		, audio_mixer_()
		, image_mixer_(std::move(image_mixer))
		, executor_(L"mixer")
	{			
		graph_->set_color("mix-time", diagnostics::color(1.0f, 0.0f, 0.9f, 0.8));
	}	
	
	const_frame operator()(std::map<int, draw_frame> frames, const video_format_desc& format_desc)
	{		
		boost::timer frame_timer;

		auto frame = executor_.invoke([=]() mutable -> const_frame
		{		
			try
			{	
				BOOST_FOREACH(auto& frame, frames)
				{
					auto blend_it = blend_modes_.find(frame.first);
					image_mixer_->begin_layer(blend_it != blend_modes_.end() ? blend_it->second : blend_mode::normal);
													
					frame.second.accept(audio_mixer_);					
					frame.second.accept(*image_mixer_);

					image_mixer_->end_layer();
				}
				
				auto image = (*image_mixer_)(format_desc);
				auto audio = audio_mixer_(format_desc);

				auto desc = core::pixel_format_desc(core::pixel_format::bgra);
				desc.planes.push_back(core::pixel_format_desc::plane(format_desc.width, format_desc.height, 4));
				return const_frame(std::move(image), std::move(audio), this, desc);	
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				return const_frame::empty();
			}	
		});		
				
		graph_->set_value("mix-time", frame_timer.elapsed()*format_desc.fps*0.5);

		return frame;
	}
					
	void set_blend_mode(int index, blend_mode value)
	{
		executor_.begin_invoke([=]
		{
			auto it = blend_modes_.find(index);
			if(it == blend_modes_.end())
				blend_modes_.insert(std::make_pair(index, value));
			else
				it->second = value;
		}, task_priority::high_priority);
	}
	
	boost::unique_future<boost::property_tree::wptree> info() const
	{
		boost::promise<boost::property_tree::wptree> info;
		info.set_value(boost::property_tree::wptree());
		return info.get_future();
	}
};
	
mixer::mixer(spl::shared_ptr<diagnostics::graph> graph, spl::shared_ptr<image_mixer> image_mixer) 
	: impl_(new impl(std::move(graph), std::move(image_mixer))){}
void mixer::set_blend_mode(int index, blend_mode value){impl_->set_blend_mode(index, value);}
boost::unique_future<boost::property_tree::wptree> mixer::info() const{return impl_->info();}
const_frame mixer::operator()(std::map<int, draw_frame> frames, const struct video_format_desc& format_desc){return (*impl_)(std::move(frames), format_desc);}
mutable_frame mixer::create_frame(const void* tag, const core::pixel_format_desc& desc) {return impl_->image_mixer_->create_frame(tag, desc);}
}}