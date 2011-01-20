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
#include "../../stdafx.h"

#include "transition_producer.h"

#include "../../video_format.h"
#include "../../mixer/frame/draw_frame.h"
#include "../../mixer/frame_mixer_device.h"
#include "../../mixer/image/image_mixer.h"
#include "../../mixer/audio/audio_mixer.h"

#include <boost/range/algorithm/copy.hpp>

namespace caspar { namespace core {	

struct transition_producer::implementation : boost::noncopyable
{	
	unsigned short				current_frame_;
	
	const transition_info		info_;
	
	safe_ptr<frame_producer>	dest_producer_;
	safe_ptr<frame_producer>	source_producer_;

	std::shared_ptr<frame_factory>	frame_factory_;

	implementation(const safe_ptr<frame_producer>& dest, const transition_info& info) 
		: current_frame_(0)
		, info_(info)
		, dest_producer_(dest)
		, source_producer_(frame_producer::empty()){}
				
	void initialize(const safe_ptr<frame_factory>& frame_factory)
	{
		dest_producer_->initialize(frame_factory);
		frame_factory_ = frame_factory;
	}

	safe_ptr<frame_producer> get_following_producer() const
	{
		return dest_producer_;
	}
	
	void set_leading_producer(const safe_ptr<frame_producer>& producer)
	{
		source_producer_ = producer;
	}
		
	safe_ptr<draw_frame> receive()
	{
		if(current_frame_++ >= info_.duration)
			return draw_frame::eof();

		auto source = draw_frame::empty();
		auto dest = draw_frame::empty();

		tbb::parallel_invoke
		(
			[&]{dest   = render_sub_frame(dest_producer_);},
			[&]{source = render_sub_frame(source_producer_);}
		);

		return compose(dest, source);
	}
	
	safe_ptr<draw_frame> render_sub_frame(safe_ptr<frame_producer>& producer)
	{
		if(producer == frame_producer::empty())
			return draw_frame::eof();

		auto frame = draw_frame::eof();
		try
		{
			frame = producer->receive();
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			producer = frame_producer::empty();
			CASPAR_LOG(warning) << "Failed to receive frame. Removed producer from transition.";
		}

		if(frame == draw_frame::eof())
		{
			try
			{
				auto following = producer->get_following_producer();
				following->initialize(safe_ptr<frame_factory>(frame_factory_));
				following->set_leading_producer(producer);
				producer = std::move(following);
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				producer = frame_producer::empty();
				CASPAR_LOG(warning) << "Failed to initialize following producer.";
			}

			return render_sub_frame(producer);
		}
		return frame;
	}
					
	safe_ptr<draw_frame> compose(const safe_ptr<draw_frame>& dest_frame, const safe_ptr<draw_frame>& src_frame) 
	{	
		if(dest_frame == draw_frame::eof() && src_frame == draw_frame::eof())
			return draw_frame::eof();

		if(info_.type == transition::cut)		
			return src_frame != draw_frame::eof() ? src_frame : draw_frame::empty();
										
		double alpha = static_cast<double>(current_frame_)/static_cast<double>(info_.duration);

		auto my_src_frame = draw_frame(src_frame);
		auto my_dest_frame = draw_frame(dest_frame);

		my_src_frame.get_audio_transform().set_gain(1.0-alpha);
		my_dest_frame.get_audio_transform().set_gain(alpha);

		double dir = info_.direction == transition_direction::from_left ? 1.0 : -1.0;		
		
		if(info_.type == transition::mix)
			my_dest_frame.get_image_transform().set_opacity(alpha);		
		else if(info_.type == transition::slide)			
			my_dest_frame.get_image_transform().set_position((-1.0+alpha)*dir, 0.0);			
		else if(info_.type == transition::push)
		{
			my_dest_frame.get_image_transform().set_position((-1.0+alpha)*dir, 0.0);
			my_src_frame.get_image_transform().set_position((0.0+alpha)*dir, 0.0);
		}
		else if(info_.type == transition::wipe)
		{
			my_dest_frame.get_image_transform().set_position((-1.0+alpha)*dir, 0.0);			
			my_dest_frame.get_image_transform().set_uv((-1.0+alpha)*dir, 0.0, 0.0-(1.0-alpha)*dir, 0.0);				
		}

		return draw_frame(std::move(my_src_frame), std::move(my_dest_frame));
	}

	std::wstring print() const
	{
		return L"transition[" + (source_producer_->print()) + L" -> " + (dest_producer_->print()) + L"]";
	}
};

transition_producer::transition_producer(transition_producer&& other) : impl_(std::move(other.impl_)){}
transition_producer::transition_producer(const safe_ptr<frame_producer>& dest, const transition_info& info) : impl_(new implementation(dest, info)){}
safe_ptr<draw_frame> transition_producer::receive(){return impl_->receive();}
safe_ptr<frame_producer> transition_producer::get_following_producer() const{return impl_->get_following_producer();}
void transition_producer::set_leading_producer(const safe_ptr<frame_producer>& producer) { impl_->set_leading_producer(producer); }
void transition_producer::initialize(const safe_ptr<frame_factory>& frame_factory) { impl_->initialize(frame_factory);}
std::wstring transition_producer::print() const { return impl_->print();}

}}

