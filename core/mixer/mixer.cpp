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

#include "../video_channel_context.h"

#include <common/exception/exceptions.h>
#include <common/concurrency/executor.h>
#include <common/utility/tweener.h>

#include <core/mixer/read_frame.h>
#include <core/mixer/write_frame.h>
#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_factory.h>
#include <core/producer/frame/pixel_format.h>
#include <core/producer/frame/audio_transform.h>
#include <core/producer/frame/image_transform.h>

#include <core/video_format.h>

#include <boost/fusion/container/map.hpp>
#include <boost/fusion/include/at_key.hpp>
#include <boost/foreach.hpp>

#include <tbb/parallel_invoke.h>

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
	video_channel_context& channel_;
	
	audio_mixer	audio_mixer_;
	image_mixer image_mixer_;
	
	typedef std::unordered_map<int, tweened_transform<core::image_transform>> image_transforms;
	typedef std::unordered_map<int, tweened_transform<core::audio_transform>> audio_transforms;

	boost::fusion::map<boost::fusion::pair<core::image_transform, image_transforms>,
					boost::fusion::pair<core::audio_transform, audio_transforms>> transforms_;
	
public:
	implementation(video_channel_context& video_channel) 
		: channel_(video_channel)
		, audio_mixer_(channel_.get_format_desc())
		, image_mixer_(channel_)
	{	
		CASPAR_LOG(info) << print() << L" Successfully initialized.";	
	}
			
	safe_ptr<read_frame> execute(const std::map<int, safe_ptr<core::basic_frame>>& frames)
	{			
		try
		{
			decltype(mix_image(frames)) image;
			decltype(mix_audio(frames)) audio;

			tbb::parallel_invoke(
					[&]{image = mix_image(frames);}, 
					[&]{audio = mix_audio(frames);});
			
			return make_safe<read_frame>(channel_.ogl(), channel_.get_format_desc().size, std::move(image), std::move(audio));
		}
		catch(...)
		{
			channel_.ogl().gc().wait();
			image_mixer_ = image_mixer(channel_);
			audio_mixer_ = audio_mixer(channel_.get_format_desc());
			channel_.ogl().gc().wait();

			CASPAR_LOG_CURRENT_EXCEPTION();
			return make_safe<read_frame>();
		}
	}
			
	safe_ptr<core::write_frame> create_frame(const void* tag, const core::pixel_format_desc& desc)
	{		
		return image_mixer_.create_frame(tag, desc);
	}

	void reset_transforms()
	{
		channel_.execution().invoke([&]
		{
			boost::fusion::at_key<image_transform>(transforms_).clear();
			boost::fusion::at_key<audio_transform>(transforms_).clear();
		});
	}
		
	template<typename T>
	void set_transform(int index, const T& transform, unsigned int mix_duration, const std::wstring& tween)
	{
		channel_.execution().invoke([&]
		{
			auto& transforms = boost::fusion::at_key<T>(transforms_);

			auto src = transforms[index].fetch();
			auto dst = transform;
			transforms[index] = tweened_transform<T>(src, dst, mix_duration, tween);
		});
	}
				
	template<typename T>
	void apply_transform(int index, const std::function<T(T)>& transform, unsigned int mix_duration, const std::wstring& tween)
	{
		channel_.execution().invoke([&]
		{
			auto& transforms = boost::fusion::at_key<T>(transforms_);

			auto src = transforms[index].fetch();
			auto dst = transform(src);
			transforms[index] = tweened_transform<T>(src, dst, mix_duration, tween);
		});
	}
		
	std::wstring print() const
	{
		return L"mixer";
	}

private:
		
	boost::unique_future<safe_ptr<host_buffer>> mix_image(std::map<int, safe_ptr<core::basic_frame>> frames)
	{		
		auto& image_transforms = boost::fusion::at_key<core::image_transform>(transforms_);
		
		BOOST_FOREACH(auto& frame, frames)
		{
			image_mixer_.begin_layer();

			auto frame1 = make_safe<core::basic_frame>(frame.second);
			frame1->get_image_transform() = image_transforms[frame.first].fetch_and_tick(1);
						
			if(channel_.get_format_desc().mode != core::video_mode::progressive)
			{
				auto frame2 = make_safe<core::basic_frame>(frame.second);
				frame2->get_image_transform() = image_transforms[frame.first].fetch_and_tick(1);
				if(frame1->get_image_transform() != frame2->get_image_transform())
					frame1 = core::basic_frame::interlace(frame1, frame2, channel_.get_format_desc().mode);
			}

			frame1->accept(image_mixer_);

			image_mixer_.end_layer();
		}

		return image_mixer_.render();
	}

	std::vector<int16_t> mix_audio(const std::map<int, safe_ptr<core::basic_frame>>& frames)
	{
		auto& audio_transforms = boost::fusion::at_key<core::audio_transform>(transforms_);

		BOOST_FOREACH(auto& frame, frames)
		{
			const unsigned int num = channel_.get_format_desc().mode == core::video_mode::progressive ? 1 : 2;

			auto frame1 = make_safe<core::basic_frame>(frame.second);
			frame1->get_audio_transform() = audio_transforms[frame.first].fetch_and_tick(num);
			frame1->accept(audio_mixer_);
		}

		return audio_mixer_.mix();
	}
};
	
mixer::mixer(video_channel_context& video_channel) : impl_(new implementation(video_channel)){}
safe_ptr<core::read_frame> mixer::execute(const std::map<int, safe_ptr<core::basic_frame>>& frames){ return impl_->execute(frames);}
core::video_format_desc mixer::get_video_format_desc() const { return impl_->channel_.get_format_desc(); }
safe_ptr<core::write_frame> mixer::create_frame(const void* tag, const core::pixel_format_desc& desc){ return impl_->create_frame(tag, desc); }		
safe_ptr<core::write_frame> mixer::create_frame(const void* tag, size_t width, size_t height, core::pixel_format::type pix_fmt)
{
	// Create bgra frame
	core::pixel_format_desc desc;
	desc.pix_fmt = pix_fmt;
	desc.planes.push_back( core::pixel_format_desc::plane(width, height, 4));
	return create_frame(tag, desc);
}
void mixer::reset_transforms(){impl_->reset_transforms();}
void mixer::set_image_transform(int index, const core::image_transform& transform, unsigned int mix_duration, const std::wstring& tween){impl_->set_transform<core::image_transform>(index, transform, mix_duration, tween);}
void mixer::set_audio_transform(int index, const core::audio_transform& transform, unsigned int mix_duration, const std::wstring& tween){impl_->set_transform<core::audio_transform>(index, transform, mix_duration, tween);}
void mixer::apply_image_transform(int index, const std::function<core::image_transform(core::image_transform)>& transform, unsigned int mix_duration, const std::wstring& tween){impl_->apply_transform<core::image_transform>(index, transform, mix_duration, tween);}
void mixer::apply_audio_transform(int index, const std::function<core::audio_transform(core::audio_transform)>& transform, unsigned int mix_duration, const std::wstring& tween){impl_->apply_transform<core::audio_transform>(index, transform, mix_duration, tween);}

}}