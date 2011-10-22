/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#include "../StdAfx.h"

#include "mixer.h"

#include "read_frame.h"
#include "write_frame.h"

#include "audio/audio_mixer.h"
#include "image/image_mixer.h"

#include <common/exception/exceptions.h>
#include <common/concurrency/executor.h>
#include <common/utility/tweener.h>
#include <common/env.h>
#include <common/gl/gl_check.h>

#include <core/mixer/read_frame.h>
#include <core/mixer/write_frame.h>
#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_factory.h>
#include <core/producer/frame/pixel_format.h>
#include <core/producer/frame/frame_transform.h>

#include <core/video_format.h>

#include <boost/fusion/container/map.hpp>
#include <boost/fusion/include/at_key.hpp>
#include <boost/foreach.hpp>

#include <unordered_map>

using namespace Concurrency;

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
	const video_format_desc format_desc_;
	ogl_device&				ogl_;
	
	audio_mixer	audio_mixer_;
	image_mixer image_mixer_;
	
	std::unordered_map<int, tweened_transform<core::frame_transform>> transforms_;	
	std::unordered_map<int, blend_mode::type> blend_modes_;
		
	critical_section			mutex_;
	Concurrency::transformer<safe_ptr<message<std::map<int, safe_ptr<basic_frame>>>>, 
							 safe_ptr<message<safe_ptr<core::read_frame>>>> mixer_;
public:
	implementation(mixer::source_t& source, mixer::target_t& target, const video_format_desc& format_desc, ogl_device& ogl) 
		: format_desc_(format_desc)
		, ogl_(ogl)
		, audio_mixer_(format_desc)
		, image_mixer_(ogl, format_desc)
		, mixer_(std::bind(&implementation::mix, this, std::placeholders::_1), &target)
	{	
		CASPAR_LOG(info) << print() << L" Successfully initialized.";	
		source.link_target(&mixer_);
	}
		
	safe_ptr<message<safe_ptr<core::read_frame>>> mix(const safe_ptr<message<std::map<int, safe_ptr<basic_frame>>>>& msg)
	{		
		auto frames = msg->value();

		{
			critical_section::scoped_lock lock(mutex_);

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
		}

		auto image = image_mixer_.render();
		auto audio = audio_mixer_.mix();
			
		{
			scoped_oversubcription_token oversubscribe;
			image.wait();
		}

		auto frame = make_safe<read_frame>(ogl_, format_desc_.size, std::move(image.get()), std::move(audio));

		return msg->transfer<safe_ptr<core::read_frame>>(std::move(frame));	
	}
						
	safe_ptr<core::write_frame> create_frame(const void* tag, const core::pixel_format_desc& desc)
	{		
		return image_mixer_.create_frame(tag, desc);
	}

	boost::unique_future<safe_ptr<core::write_frame>> create_frame2(const void* tag, const core::pixel_format_desc& desc)
	{		
		return image_mixer_.create_frame2(tag, desc);
	}
		
	void set_transform(int index, const frame_transform& transform, unsigned int mix_duration, const std::wstring& tween)
	{
		critical_section::scoped_lock lock(mutex_);

		auto src = transforms_[index].fetch();
		auto dst = transform;
		transforms_[index] = tweened_transform<frame_transform>(src, dst, mix_duration, tween);
	}
				
	void apply_transform(int index, const std::function<frame_transform(frame_transform)>& transform, unsigned int mix_duration, const std::wstring& tween)
	{
		critical_section::scoped_lock lock(mutex_);

		auto src = transforms_[index].fetch();
		auto dst = transform(src);
		transforms_[index] = tweened_transform<frame_transform>(src, dst, mix_duration, tween);
	}

	void clear_transforms()
	{
		critical_section::scoped_lock lock(mutex_);

		transforms_.clear();
		blend_modes_.clear();
	}
		
	void set_blend_mode(int index, blend_mode::type value)
	{
		critical_section::scoped_lock lock(mutex_);

		blend_modes_[index] = value;
	}

	std::wstring print() const
	{
		return L"mixer";
	}
};
	
mixer::mixer(mixer::source_t& source, mixer::target_t& target, const video_format_desc& format_desc, ogl_device& ogl) : impl_(new implementation(source, target, format_desc, ogl)){}
core::video_format_desc mixer::get_video_format_desc() const { return impl_->format_desc_; }
safe_ptr<core::write_frame> mixer::create_frame(const void* tag, const core::pixel_format_desc& desc){ return impl_->create_frame(tag, desc); }		
boost::unique_future<safe_ptr<write_frame>> mixer::create_frame2(const void* video_stream_tag, const pixel_format_desc& desc){ return impl_->create_frame2(video_stream_tag, desc); }			
void mixer::set_frame_transform(int index, const core::frame_transform& transform, unsigned int mix_duration, const std::wstring& tween){impl_->set_transform(index, transform, mix_duration, tween);}
void mixer::apply_frame_transform(int index, const std::function<core::frame_transform(core::frame_transform)>& transform, unsigned int mix_duration, const std::wstring& tween){impl_->apply_transform(index, transform, mix_duration, tween);}
void mixer::clear_transforms(){impl_->clear_transforms();}
void mixer::set_blend_mode(int index, blend_mode::type value){impl_->set_blend_mode(index, value);}
}}