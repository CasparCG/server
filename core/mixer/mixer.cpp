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

#include "read_frame.h"
#include "write_frame.h"

#include "audio/audio_mixer.h"
#include "image/image_mixer.h"

#include <common/env.h>
#include <common/concurrency/executor.h>
#include <common/exception/exceptions.h>
#include <common/gl/gl_check.h>
#include <common/utility/tweener.h>

#include <core/mixer/read_frame.h>
#include <core/mixer/write_frame.h>
#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_factory.h>
#include <core/producer/frame/frame_transform.h>
#include <core/producer/frame/pixel_format.h>

#include <core/video_format.h>

#include <boost/foreach.hpp>
#include <boost/timer.hpp>
#include <boost/property_tree/ptree.hpp>

#include <tbb/concurrent_queue.h>
#include <tbb/spin_mutex.h>

#include <unordered_map>

namespace caspar { namespace core {
		
template<typename T>
class tweened_transform
{
	T source_;
	T dest_;
	int duration_;
	int time_;
	tweener_t tweener_;
public:	
	tweened_transform()
		: duration_(0)
		, time_(0)
		, tweener_(get_tweener(L"linear")){}
	tweened_transform(const T& source, const T& dest, int duration, const std::wstring& tween = L"linear")
		: source_(source)
		, dest_(dest)
		, duration_(duration)
		, time_(0)
		, tweener_(get_tweener(tween)){}
	
	T fetch()
	{
		return time_ == duration_ ? dest_ : tween(static_cast<double>(time_), source_, dest_, static_cast<double>(duration_), tweener_);
	}

	T fetch_and_tick(int num)
	{						
		time_ = std::min(time_+num, duration_);
		return fetch();
	}
};

struct mixer::implementation : boost::noncopyable
{		
	safe_ptr<diagnostics::graph>	graph_;
	boost::timer					mix_timer_;

	safe_ptr<mixer::target_t>		target_;
	mutable tbb::spin_mutex			format_desc_mutex_;
	video_format_desc				format_desc_;
	safe_ptr<ogl_device>			ogl_;
	
	audio_mixer	audio_mixer_;
	image_mixer image_mixer_;
	
	std::unordered_map<int, tweened_transform<core::frame_transform>> transforms_;	
	std::unordered_map<int, blend_mode::type> blend_modes_;
			
	executor executor_;

public:
	implementation(const safe_ptr<diagnostics::graph>& graph, const safe_ptr<mixer::target_t>& target, const video_format_desc& format_desc, const safe_ptr<ogl_device>& ogl) 
		: graph_(graph)
		, target_(target)
		, format_desc_(format_desc)
		, ogl_(ogl)
		, image_mixer_(ogl)
		, executor_(L"mixer")
	{			
		graph_->set_color("mix-time", diagnostics::color(1.0f, 0.0f, 0.9f));
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
				
					auto frame1 = make_safe<core::basic_frame>(frame.second);
					frame1->get_frame_transform() = transforms_[frame.first].fetch_and_tick(1);

					if(format_desc_.field_mode != core::field_mode::progressive)
					{				
						auto frame2 = make_safe<core::basic_frame>(frame.second);
						frame2->get_frame_transform() = transforms_[frame.first].fetch_and_tick(1);
						frame1 = core::basic_frame::interlace(frame1, frame2, format_desc_.field_mode);
					}
									
					frame1->accept(audio_mixer_);					
					frame1->accept(image_mixer_);

					image_mixer_.end_layer();
				}

				auto image = image_mixer_(format_desc_);
				auto audio = audio_mixer_(format_desc_);
				image.wait();

				graph_->update_value("mix-time", mix_timer_.elapsed()*format_desc_.fps*0.5);

				target_->send(std::make_pair(make_safe<read_frame>(ogl_, format_desc_.size, std::move(image.get()), std::move(audio)), packet.second));					
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}	
		});		
	}
					
	safe_ptr<core::write_frame> create_frame(const void* tag, const core::pixel_format_desc& desc)
	{		
		return make_safe<write_frame>(ogl_, tag, desc);
	}
			
	void set_transform(int index, const frame_transform& transform, unsigned int mix_duration, const std::wstring& tween)
	{
		executor_.begin_invoke([=]
		{
			auto src = transforms_[index].fetch();
			auto dst = transform;
			transforms_[index] = tweened_transform<frame_transform>(src, dst, mix_duration, tween);
		}, high_priority);
	}
				
	void apply_transform(int index, const std::function<frame_transform(frame_transform)>& transform, unsigned int mix_duration, const std::wstring& tween)
	{
		executor_.begin_invoke([=]
		{
			auto src = transforms_[index].fetch();
			auto dst = transform(src);
			transforms_[index] = tweened_transform<frame_transform>(src, dst, mix_duration, tween);
		}, high_priority);
	}

	void clear_transforms(int index)
	{
		executor_.begin_invoke([=]
		{
			transforms_.erase(index);
			blend_modes_.erase(index);
		}, high_priority);
	}

	void clear_transforms()
	{
		executor_.begin_invoke([=]
		{
			transforms_.clear();
			blend_modes_.clear();
		}, high_priority);
	}
		
	void set_blend_mode(int index, blend_mode::type value)
	{
		executor_.begin_invoke([=]
		{
			blend_modes_[index] = value;
		}, high_priority);
	}
	
	void set_video_format_desc(const video_format_desc& format_desc)
	{
		executor_.begin_invoke([=]
		{
			tbb::spin_mutex::scoped_lock lock(format_desc_mutex_);
			format_desc_ = format_desc;
		});
	}

	core::video_format_desc get_video_format_desc() const // nothrow
	{
		tbb::spin_mutex::scoped_lock lock(format_desc_mutex_);
		return format_desc_;
	}

	boost::unique_future<boost::property_tree::wptree> info() const
	{
		boost::promise<boost::property_tree::wptree> info;
		info.set_value(boost::property_tree::wptree());
		return info.get_future();
	}
};
	
mixer::mixer(const safe_ptr<diagnostics::graph>& graph, const safe_ptr<target_t>& target, const video_format_desc& format_desc, const safe_ptr<ogl_device>& ogl) 
	: impl_(new implementation(graph, target, format_desc, ogl)){}
void mixer::send(const std::pair<std::map<int, safe_ptr<core::basic_frame>>, std::shared_ptr<void>>& frames){ impl_->send(frames);}
core::video_format_desc mixer::get_video_format_desc() const { return impl_->get_video_format_desc(); }
safe_ptr<core::write_frame> mixer::create_frame(const void* tag, const core::pixel_format_desc& desc){ return impl_->create_frame(tag, desc); }		
void mixer::set_frame_transform(int index, const core::frame_transform& transform, unsigned int mix_duration, const std::wstring& tween){impl_->set_transform(index, transform, mix_duration, tween);}
void mixer::apply_frame_transform(int index, const std::function<core::frame_transform(core::frame_transform)>& transform, unsigned int mix_duration, const std::wstring& tween)
{impl_->apply_transform(index, transform, mix_duration, tween);}
void mixer::clear_transforms(int index){impl_->clear_transforms(index);}
void mixer::clear_transforms(){impl_->clear_transforms();}
void mixer::set_blend_mode(int index, blend_mode::type value){impl_->set_blend_mode(index, value);}
void mixer::set_video_format_desc(const video_format_desc& format_desc){impl_->set_video_format_desc(format_desc);}
boost::unique_future<boost::property_tree::wptree> mixer::info() const{return impl_->info();}
}}