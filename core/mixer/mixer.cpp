/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
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

#include "read_frame.h"
#include "write_frame.h"

#include "audio/audio_mixer.h"
#include "image/image_mixer.h"

#include <common/env.h>
#include <common/concurrency/executor.h>
#include <common/concurrency/future_util.h>
#include <common/exception/exceptions.h>
#include <common/gl/gl_check.h>
#include <common/utility/tweener.h>
#include <common/memory/safe_ptr.h>

#include <core/mixer/audio/audio_util.h>
#include <core/mixer/read_frame.h>
#include <core/mixer/write_frame.h>
#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_factory.h>
#include <core/producer/frame/frame_transform.h>
#include <core/producer/frame/pixel_format.h>

#include <core/monitor/monitor.h>

#include <core/video_format.h>

#include <boost/foreach.hpp>
#include <boost/timer.hpp>
#include <boost/property_tree/ptree.hpp>

#include <tbb/concurrent_queue.h>
#include <tbb/spin_mutex.h>
#include <tbb/atomic.h>

#include <unordered_map>

namespace caspar { namespace core {

class layer_specific_frame_factory : public frame_factory
{
	safe_ptr<ogl_device>	ogl_;
	mutable tbb::spin_mutex	format_desc_mutex_;
	video_format_desc		format_desc_;
	tbb::atomic<bool>		mipmapping_;
public:
	layer_specific_frame_factory(const safe_ptr<ogl_device>& ogl, const video_format_desc& format_desc)
		: ogl_(ogl)
		, format_desc_(format_desc)
	{
		mipmapping_ = false;
	}

	void set_mipmapping(bool mipmapping)
	{
		mipmapping_ = mipmapping;
	}

	bool get_mipmapping() const
	{
		return mipmapping_;
	}

	void set_video_format_desc(const video_format_desc& format_desc)
	{
		tbb::spin_mutex::scoped_lock lock(format_desc_mutex_);
		format_desc_ = format_desc;
	}

	safe_ptr<core::write_frame> create_frame(
			const void* tag,
			const core::pixel_format_desc& desc,
			const channel_layout& audio_channel_layout) override
	{
		return make_safe<write_frame>(
				ogl_, tag, desc, audio_channel_layout, mipmapping_);
	}

	video_format_desc get_video_format_desc() const override
	{
		tbb::spin_mutex::scoped_lock lock(format_desc_mutex_);
		return format_desc_;
	}
};
		
struct mixer::implementation : boost::noncopyable
{		
	safe_ptr<diagnostics::graph>	graph_;
	boost::timer					mix_timer_;
	tbb::atomic<int64_t>			current_mix_time_;

	safe_ptr<mixer::target_t>		target_;
	video_format_desc				format_desc_;
	safe_ptr<ogl_device>			ogl_;
	channel_layout					audio_channel_layout_;
	bool							straighten_alpha_;
	
	audio_mixer	audio_mixer_;
	image_mixer image_mixer_;
	
	std::unordered_map<int, blend_mode>								blend_modes_;
	std::unordered_map<int, safe_ptr<layer_specific_frame_factory>> frame_factories_;
			
	executor executor_;
	safe_ptr<monitor::subject>		 monitor_subject_;

public:
	implementation(const safe_ptr<diagnostics::graph>& graph, const safe_ptr<mixer::target_t>& target, const video_format_desc& format_desc, const safe_ptr<ogl_device>& ogl, const channel_layout& audio_channel_layout) 
		: graph_(graph)
		, target_(target)
		, format_desc_(format_desc)
		, ogl_(ogl)
		, audio_channel_layout_(audio_channel_layout)
		, straighten_alpha_(false)
		, audio_mixer_(graph_)
		, image_mixer_(ogl)
		, executor_(L"mixer")
		, monitor_subject_(make_safe<monitor::subject>("/mixer"))
	{
		graph_->set_color("mix-time", diagnostics::color(1.0f, 0.0f, 0.9f, 0.8));
		current_mix_time_ = 0;
		executor_.invoke([&]
		{
			detail::set_current_aspect_ratio(
					static_cast<double>(format_desc.square_width)
							/ static_cast<double>(format_desc.square_height));
		});

		audio_mixer_.monitor_output().attach_parent(monitor_subject_);
	}
	
	void send(const std::pair<std::map<int, safe_ptr<core::basic_frame>>, std::shared_ptr<void>>& packet)
	{			
		executor_.begin_invoke([=]
		{		
			try
			{
				mix_timer_.restart();

				auto frames = packet.first;
				
				BOOST_FOREACH(auto& frame, frames)
				{
					auto blend_it = blend_modes_.find(frame.first);
					image_mixer_.begin_layer(blend_it != blend_modes_.end() ? blend_it->second : blend_mode::normal);
													
					frame.second->accept(audio_mixer_);					
					frame.second->accept(image_mixer_);

					image_mixer_.end_layer();
				}

				auto image = image_mixer_(format_desc_, straighten_alpha_);
				auto audio = audio_mixer_(format_desc_, audio_channel_layout_);
				image.wait();

				auto mix_time = mix_timer_.elapsed();
				graph_->set_value("mix-time", mix_time*format_desc_.fps*0.5);
				current_mix_time_ = static_cast<int64_t>(mix_time * 1000.0);

				target_->send(std::make_pair(make_safe<read_frame>(ogl_, format_desc_.size, std::move(image.get()), std::move(audio), audio_channel_layout_), packet.second));
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}	
		});		
	}

	safe_ptr<layer_specific_frame_factory> get_frame_factory(int layer_index)
	{
		return executor_.invoke([=]() -> safe_ptr<layer_specific_frame_factory>
		{
			auto found = frame_factories_.find(layer_index);

			if (found == frame_factories_.end())
			{
				auto factory = make_safe<layer_specific_frame_factory>(ogl_, format_desc_);

				frame_factories_.insert(std::make_pair(layer_index, factory));

				return factory;
			}

			return found->second;
		});
	}

	blend_mode::type get_blend_mode(int index)
	{
		return executor_.invoke([=]
		{
			return blend_modes_[index].mode;
		});
	}
				
	void set_blend_mode(int index, blend_mode::type value)
	{
		executor_.begin_invoke([=]
		{
			blend_modes_[index].mode = value;
		}, high_priority);
	}

	void clear_blend_mode(int index)
	{
		executor_.begin_invoke([=]
		{
			blend_modes_.erase(index);
		}, high_priority);
	}

	void clear_blend_modes()
	{
		executor_.begin_invoke([=]
		{
			blend_modes_.clear();
		}, high_priority);
	}

	chroma get_chroma(int index)
	{
		return executor_.invoke([=]
		{
			return blend_modes_[index].chroma;
		});
	}

    void set_chroma(int index, const chroma & value)
    {
        executor_.begin_invoke([=]
        {
            blend_modes_[index].chroma = value;
        }, high_priority);
    }

	bool get_mipmap(int index)
	{
		return get_frame_factory(index)->get_mipmapping();
	}

	void set_mipmap(int index, bool mipmap)
	{
		get_frame_factory(index)->set_mipmapping(mipmap);
	}

	void clear_mipmap(int index)
	{
		executor_.begin_invoke([=]
		{
			frame_factories_.erase(index);
		}, high_priority);
	}

	void clear_mipmap()
	{
		executor_.begin_invoke([=]
		{
			frame_factories_.clear();
		}, high_priority);
	}

	void set_straight_alpha_output(bool value)
	{
        executor_.begin_invoke([=]
        {
			straighten_alpha_ = value;
        }, high_priority);
	}

	bool get_straight_alpha_output()
	{
		return executor_.invoke([=]
		{
			return straighten_alpha_;
		});
	}

	float get_master_volume()
	{
		return executor_.invoke([=]
		{
			return audio_mixer_.get_master_volume();
		});
	}

	void set_master_volume(float volume)
	{
		executor_.begin_invoke([=]
		{
			audio_mixer_.set_master_volume(volume);
		}, high_priority);
	}
	
	void set_video_format_desc(const video_format_desc& format_desc)
	{
		executor_.begin_invoke([=]
		{
			format_desc_ = format_desc;
			detail::set_current_aspect_ratio(
					static_cast<double>(format_desc.square_width)
							/ static_cast<double>(format_desc.square_height));

			BOOST_FOREACH(auto& factory, frame_factories_)
				factory.second->set_video_format_desc(format_desc);
		});
	}

	boost::unique_future<boost::property_tree::wptree> info() const
	{
		boost::property_tree::wptree info;
		info.add(L"mix-time", current_mix_time_);

		return wrap_as_future(std::move(info));
	}

	boost::unique_future<boost::property_tree::wptree> delay_info() const
	{
		boost::property_tree::wptree info;
		info.put_value(current_mix_time_);

		return wrap_as_future(std::move(info));
	}
};
	
mixer::mixer(const safe_ptr<diagnostics::graph>& graph, const safe_ptr<target_t>& target, const video_format_desc& format_desc, const safe_ptr<ogl_device>& ogl, const channel_layout& audio_channel_layout) 
	: impl_(new implementation(graph, target, format_desc, ogl, audio_channel_layout)){}
void mixer::send(const std::pair<std::map<int, safe_ptr<core::basic_frame>>, std::shared_ptr<void>>& frames){ impl_->send(frames);}
safe_ptr<frame_factory> mixer::get_frame_factory(int layer_index) { return impl_->get_frame_factory(layer_index); }
blend_mode::type mixer::get_blend_mode(int index) { return impl_->get_blend_mode(index); }
void mixer::set_blend_mode(int index, blend_mode::type value){impl_->set_blend_mode(index, value);}
chroma mixer::get_chroma(int index) { return impl_->get_chroma(index); }
void mixer::set_chroma(int index, const chroma & value){impl_->set_chroma(index, value);}
void mixer::clear_blend_mode(int index) { impl_->clear_blend_mode(index); }
void mixer::clear_blend_modes() { impl_->clear_blend_modes(); }
bool mixer::get_mipmap(int index) { return impl_->get_mipmap(index); }
void mixer::set_mipmap(int index, bool mipmap) { impl_->set_mipmap(index, mipmap); }
void mixer::clear_mipmap(int index) { impl_->clear_mipmap(index); }
void mixer::clear_mipmap() { impl_->clear_mipmap(); }
void mixer::set_straight_alpha_output(bool value) { impl_->set_straight_alpha_output(value); }
bool mixer::get_straight_alpha_output() { return impl_->get_straight_alpha_output(); }
float mixer::get_master_volume() { return impl_->get_master_volume(); }
void mixer::set_master_volume(float volume) { impl_->set_master_volume(volume); }
void mixer::set_video_format_desc(const video_format_desc& format_desc){impl_->set_video_format_desc(format_desc);}
boost::unique_future<boost::property_tree::wptree> mixer::info() const{return impl_->info();}
boost::unique_future<boost::property_tree::wptree> mixer::delay_info() const{return impl_->delay_info();}
monitor::subject& mixer::monitor_output(){return *impl_->monitor_subject_;}
}}