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

#include "frame_mixer_device.h"

#include "read_frame.h"
#include "write_frame.h"

#include "audio/audio_mixer.h"
#include "image/image_mixer.h"

#include "../channel_context.h"

#include <common/exception/exceptions.h>
#include <common/concurrency/executor.h>
#include <common/diagnostics/graph.h>
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
#include <boost/timer.hpp>

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

struct frame_mixer_device::implementation : boost::noncopyable
{		
	channel_context& channel_;

	safe_ptr<diagnostics::graph> diag_;
	boost::timer frame_timer_;
	boost::timer tick_timer_;

	audio_mixer	audio_mixer_;
	image_mixer image_mixer_;

	output_t output_;
	
	typedef std::unordered_map<int, tweened_transform<core::image_transform>> image_transforms;
	typedef std::unordered_map<int, tweened_transform<core::audio_transform>> audio_transforms;

	boost::fusion::map<boost::fusion::pair<core::image_transform, image_transforms>,
					boost::fusion::pair<core::audio_transform, audio_transforms>> transforms_;
	
	boost::fusion::map<boost::fusion::pair<core::image_transform, tweened_transform<core::image_transform>>,
					boost::fusion::pair<core::audio_transform, tweened_transform<core::audio_transform>>> root_transforms_;
public:
	implementation(channel_context& channel, const output_t& output) 
		: channel_(channel)
		, diag_(diagnostics::create_graph(narrow(print())))
		, image_mixer_(channel_)
		, output_(output)
	{
		diag_->add_guide("frame-time", 0.5f);	
		diag_->set_color("frame-time", diagnostics::color(1.0f, 0.0f, 0.0f));
		diag_->set_color("tick-time", diagnostics::color(0.1f, 0.7f, 0.8f));
		diag_->set_color("input-buffer", diagnostics::color(1.0f, 1.0f, 0.0f));	
	
		CASPAR_LOG(info) << print() << L" Successfully initialized.";	
	}
	
	safe_ptr<host_buffer> mix_image(std::map<int, safe_ptr<core::basic_frame>> frames)
	{		
		auto& root_image_transform = boost::fusion::at_key<core::image_transform>(root_transforms_);
		auto& image_transforms = boost::fusion::at_key<core::image_transform>(transforms_);
		
		BOOST_FOREACH(auto& frame, frames)
		{
			image_mixer_.begin_layer();
			
			if(channel_.format_desc.mode != core::video_mode::progressive)
			{
				auto frame1 = make_safe<core::basic_frame>(frame.second);
				auto frame2 = make_safe<core::basic_frame>(frame.second);
				
				frame1->get_image_transform() = root_image_transform.fetch_and_tick(1)*image_transforms[frame.first].fetch_and_tick(1);
				frame2->get_image_transform() = root_image_transform.fetch_and_tick(1)*image_transforms[frame.first].fetch_and_tick(1);

				if(frame1->get_image_transform() != frame2->get_image_transform())
					core::basic_frame::interlace(frame1, frame2, channel_.format_desc.mode)->accept(image_mixer_);
				else
					frame2->accept(image_mixer_);
			}
			else
			{
				auto frame1 = make_safe<core::basic_frame>(frame.second);
				frame1->get_image_transform() = root_image_transform.fetch_and_tick(1)*image_transforms[frame.first].fetch_and_tick(1);
				frame1->accept(image_mixer_);
			}

			image_mixer_.end_layer();
		}

		return image_mixer_.render();
	}

	std::vector<int16_t> mix_audio(const std::map<int, safe_ptr<core::basic_frame>>& frames)
	{
		auto& root_audio_transform = boost::fusion::at_key<core::audio_transform>(root_transforms_);
		auto& audio_transforms = boost::fusion::at_key<core::audio_transform>(transforms_);

		BOOST_FOREACH(auto& frame, frames)
		{
			const unsigned int num = channel_.format_desc.mode == core::video_mode::progressive ? 1 : 2;

			auto frame1 = make_safe<core::basic_frame>(frame.second);
			frame1->get_audio_transform() = root_audio_transform.fetch_and_tick(num)*audio_transforms[frame.first].fetch_and_tick(num);
			frame1->accept(audio_mixer_);
		}

		return audio_mixer_.mix();
	}
		
	void send(const std::map<int, safe_ptr<core::basic_frame>>& frames)
	{			
		channel_.execution.invoke([=]
		{		
			diag_->set_value("input-buffer", static_cast<float>(channel_.execution.size())/static_cast<float>(channel_.execution.capacity()));	
			frame_timer_.restart();

			auto image = mix_image(frames);
			auto audio = mix_audio(frames);
			
			diag_->update_value("frame-time", frame_timer_.elapsed()*channel_.format_desc.fps*0.5);

			output_(make_safe<read_frame>(std::move(image), std::move(audio)));

			diag_->update_value("tick-time", tick_timer_.elapsed()*channel_.format_desc.fps*0.5);
			tick_timer_.restart();
		});
		diag_->set_value("input-buffer", static_cast<float>(channel_.execution.size())/static_cast<float>(channel_.execution.capacity()));
	}
		
	safe_ptr<core::write_frame> create_frame(void* tag, const core::pixel_format_desc& desc)
	{		
		return image_mixer_.create_frame(tag, desc);
	}
			
	template<typename T>	
	void set_transform(const T& transform, unsigned int mix_duration, const std::wstring& tween)
	{
		channel_.execution.invoke([&]
		{
			auto& root = boost::fusion::at_key<T>(root_transforms_);

			auto src = root.fetch();
			auto dst = transform;
			root = tweened_transform<T>(src, dst, mix_duration, tween);
		});
	}
		
	template<typename T>
	void set_transform(int index, const T& transform, unsigned int mix_duration, const std::wstring& tween)
	{
		channel_.execution.invoke([&]
		{
			auto& transforms = boost::fusion::at_key<T>(transforms_);

			auto src = transforms[index].fetch();
			auto dst = transform;
			transforms[index] = tweened_transform<T>(src, dst, mix_duration, tween);
		});
	}
		
	template<typename T>
	void apply_transform(const std::function<T(const T&)>& transform, unsigned int mix_duration, const std::wstring& tween)
	{
		return channel_.execution.invoke([&]
		{
			auto& root = boost::fusion::at_key<T>(root_transforms_);

			auto src = root.fetch();
			auto dst = transform(src);
			root = tweened_transform<T>(src, dst, mix_duration, tween);
		});
	}
		
	template<typename T>
	void apply_transform(int index, const std::function<T(T)>& transform, unsigned int mix_duration, const std::wstring& tween)
	{
		channel_.execution.invoke([&]
		{
			auto& transforms = boost::fusion::at_key<T>(transforms_);

			auto src = transforms[index].fetch();
			auto dst = transform(src);
			transforms[index] = tweened_transform<T>(src, dst, mix_duration, tween);
		});
	}

	template<typename T>
	void reset_transform(unsigned int mix_duration, const std::wstring& tween)
	{
		channel_.execution.invoke([&]
		{
			auto& transforms = boost::fusion::at_key<T>(transforms_);

			BOOST_FOREACH(auto& t, transforms)			
				 t.second = tweened_transform<T>(t.second.fetch(), T(), mix_duration, tween);			
			set_transform(T(), mix_duration, tween);
		});
	}

	template<typename T>
	void reset_transform(int index, unsigned int mix_duration, const std::wstring& tween)
	{
		channel_.execution.invoke([&]
		{		
			set_transform(T(), mix_duration, tween);
		});
	}
	
	std::wstring print() const
	{
		return L"frame_mixer_device";
	}
};
	
frame_mixer_device::frame_mixer_device(channel_context& channel, const output_t& output)
	: impl_(new implementation(channel, output)){}
void frame_mixer_device::send(const std::map<int, safe_ptr<core::basic_frame>>& frames){impl_->send(frames);}
const core::video_format_desc& frame_mixer_device::get_video_format_desc() const { return impl_->channel_.format_desc; }
safe_ptr<core::write_frame> frame_mixer_device::create_frame(void* tag, const core::pixel_format_desc& desc){ return impl_->create_frame(tag, desc); }		
safe_ptr<core::write_frame> frame_mixer_device::create_frame(void* tag, size_t width, size_t height, core::pixel_format::type pix_fmt)
{
	// Create bgra frame
	core::pixel_format_desc desc;
	desc.pix_fmt = pix_fmt;
	desc.planes.push_back( core::pixel_format_desc::plane(width, height, 4));
	return create_frame(tag, desc);
}
void frame_mixer_device::set_image_transform(const core::image_transform& transform, unsigned int mix_duration, const std::wstring& tween){impl_->set_transform<core::image_transform>(transform, mix_duration, tween);}
void frame_mixer_device::set_image_transform(int index, const core::image_transform& transform, unsigned int mix_duration, const std::wstring& tween){impl_->set_transform<core::image_transform>(index, transform, mix_duration, tween);}
void frame_mixer_device::set_audio_transform(const core::audio_transform& transform, unsigned int mix_duration, const std::wstring& tween){impl_->set_transform<core::audio_transform>(transform, mix_duration, tween);}
void frame_mixer_device::set_audio_transform(int index, const core::audio_transform& transform, unsigned int mix_duration, const std::wstring& tween){impl_->set_transform<core::audio_transform>(index, transform, mix_duration, tween);}
void frame_mixer_device::apply_image_transform(const std::function<core::image_transform(core::image_transform)>& transform, unsigned int mix_duration, const std::wstring& tween){impl_->apply_transform<core::image_transform>(transform, mix_duration, tween);}
void frame_mixer_device::apply_image_transform(int index, const std::function<core::image_transform(core::image_transform)>& transform, unsigned int mix_duration, const std::wstring& tween){impl_->apply_transform<core::image_transform>(index, transform, mix_duration, tween);}
void frame_mixer_device::apply_audio_transform(const std::function<core::audio_transform(core::audio_transform)>& transform, unsigned int mix_duration, const std::wstring& tween){impl_->apply_transform<core::audio_transform>(transform, mix_duration, tween);}
void frame_mixer_device::apply_audio_transform(int index, const std::function<core::audio_transform(core::audio_transform)>& transform, unsigned int mix_duration, const std::wstring& tween){impl_->apply_transform<core::audio_transform>(index, transform, mix_duration, tween);}
void frame_mixer_device::reset_image_transform(unsigned int mix_duration, const std::wstring& tween){impl_->reset_transform<core::image_transform>(mix_duration, tween);}
void frame_mixer_device::reset_image_transform(int index, unsigned int mix_duration, const std::wstring& tween){impl_->reset_transform<core::image_transform>(index, mix_duration, tween);}
void frame_mixer_device::reset_audio_transform(unsigned int mix_duration, const std::wstring& tween){impl_->reset_transform<core::audio_transform>(mix_duration, tween);}
void frame_mixer_device::reset_audio_transform(int index, unsigned int mix_duration, const std::wstring& tween){impl_->reset_transform<core::audio_transform>(index, mix_duration, tween);}

}}