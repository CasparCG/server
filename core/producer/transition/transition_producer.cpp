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

#include "../../format/video_format.h"
#include "../../processor/draw_frame.h"
#include "../../processor/composite_frame.h"
#include "../../processor/transform_frame.h"
#include "../../processor/frame_processor_device.h"

#include "../../producer/frame_producer_device.h"

#include <boost/range/algorithm/copy.hpp>

namespace caspar { namespace core {	

struct transition_producer::implementation : boost::noncopyable
{
	implementation(const safe_ptr<frame_producer>& dest, const transition_info& info) : current_frame_(0), info_(info), 
		dest_producer_(dest), source_producer_(frame_producer::empty())
	{}
		
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
			[&]{dest   = receive(dest_producer_);},
			[&]{source = receive(source_producer_);}
		);

		return compose(dest, source);
	}

	safe_ptr<draw_frame> receive(safe_ptr<frame_producer>& producer)
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
			CASPAR_LOG(warning) << "Failed to receive frame. Removed producer from transition.";
		}

		if(frame == draw_frame::eof())
		{
			try
			{
				auto following = producer->get_following_producer();
				following->initialize(safe_ptr<frame_processor_device>(frame_processor_));
				following->set_leading_producer(producer);
				producer = std::move(following);
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				CASPAR_LOG(warning) << "Failed to initialize following producer.";
			}

			return receive(producer);
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

		auto my_src_frame = transform_frame(src_frame);
		auto my_dest_frame = transform_frame(dest_frame);

		my_src_frame.audio_volume(1.0-alpha);
		my_dest_frame.audio_volume(alpha);

		double dir = info_.direction == transition_direction::from_left ? 1.0 : -1.0;		
		
		if(info_.type == transition::mix)
			my_dest_frame.alpha(alpha);		
		else if(info_.type == transition::slide)			
			my_dest_frame.translate((-1.0+alpha)*dir, 0.0);			
		else if(info_.type == transition::push)
		{
			my_dest_frame.translate((-1.0+alpha)*dir, 0.0);
			my_src_frame.translate((0.0+alpha)*dir, 0.0);
		}
		else if(info_.type == transition::wipe)
		{
			my_dest_frame.translate((-1.0+alpha)*dir, 0.0);			
			my_dest_frame.texcoord((-1.0+alpha)*dir, 0.0, 0.0-(1.0-alpha)*dir, 0.0);				
		}

		return composite_frame(std::move(my_src_frame), std::move(my_dest_frame));
	}
		
	void initialize(const safe_ptr<frame_processor_device>& frame_processor)
	{
		dest_producer_->initialize(frame_processor);
		frame_processor_ = frame_processor;
	}

	std::wstring print() const
	{
		return L"transition[" + (source_producer_->print()) + L" -> " + (dest_producer_->print()) + L"]";
	}
	
	safe_ptr<frame_producer>	source_producer_;
	safe_ptr<frame_producer>	dest_producer_;
	
	unsigned short				current_frame_;
	
	const transition_info		info_;
	std::shared_ptr<frame_processor_device>	frame_processor_;
};

transition_producer::transition_producer(transition_producer&& other) : impl_(std::move(other.impl_)){}
transition_producer::transition_producer(const safe_ptr<frame_producer>& dest, const transition_info& info) : impl_(new implementation(dest, info)){}
safe_ptr<draw_frame> transition_producer::receive(){return impl_->receive();}
safe_ptr<frame_producer> transition_producer::get_following_producer() const{return impl_->get_following_producer();}
void transition_producer::set_leading_producer(const safe_ptr<frame_producer>& producer) { impl_->set_leading_producer(producer); }
void transition_producer::initialize(const safe_ptr<frame_processor_device>& frame_processor) { impl_->initialize(frame_processor);}
std::wstring transition_producer::print() const { return impl_->print();}

}}

