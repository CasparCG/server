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
#include "../../processor/composite_frame.h"
#include "../../processor/transform_frame.h"
#include "../../processor/frame_processor_device.h"

#include "../../producer/frame_producer_device.h"

#include <boost/range/algorithm/copy.hpp>

namespace caspar { namespace core {	

struct transition_producer::implementation : boost::noncopyable
{
	implementation(const frame_producer_ptr& dest, const transition_info& info) 
		: current_frame_(0), info_(info), dest_producer_(dest), org_dest_producer_(dest)
	{
		if(!dest)
			BOOST_THROW_EXCEPTION(null_argument() << arg_name_info("dest"));
	}
		
	frame_producer_ptr get_following_producer() const
	{
		return dest_producer_;
	}
	
	void set_leading_producer(const frame_producer_ptr& producer)
	{
		org_source_producer_ = source_producer_ = producer;
	}
		
	gpu_frame_ptr receive()
	{
		if(current_frame_ == 0)
			CASPAR_LOG(info) << "Transition started.";

		gpu_frame_ptr result = [&]() -> gpu_frame_ptr
		{
			if(current_frame_++ >= info_.duration)
				return nullptr;

			gpu_frame_ptr source;
			gpu_frame_ptr dest;

			tbb::parallel_invoke
			(
				[&]{dest   = receive(dest_producer_);},
				[&]{source = receive(source_producer_);}
			);

			return compose(dest, source);
		}();

		if(!result)
			CASPAR_LOG(info) << "Transition ended.";

		return result;
	}

	gpu_frame_ptr receive(frame_producer_ptr& producer)
	{
		if(!producer)
			return nullptr;

		gpu_frame_ptr frame;
		try
		{
			frame = producer->receive();
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			producer = nullptr;
			CASPAR_LOG(warning) << "Removed producer from transition.";
		}

		if(frame == nullptr)
		{
			if(!producer || !producer->get_following_producer())
				return nullptr;

			try
			{
				auto following = producer->get_following_producer();
				following->initialize(frame_processor_);
				following->set_leading_producer(producer);
				producer = following;
			}
			catch(...)
			{
				CASPAR_LOG(warning) << "Failed to initialize following producer. Removing it.";
				producer = nullptr;
			}

			return receive(producer);
		}
		return frame;
	}
					
	gpu_frame_ptr compose(gpu_frame_ptr dest_frame, gpu_frame_ptr src_frame) 
	{	
		if(!dest_frame && !src_frame)
			return nullptr;

		if(info_.type == transition::cut)		
			return src_frame;
										
		double alpha = static_cast<double>(current_frame_)/static_cast<double>(info_.duration);
		unsigned char volume = static_cast<unsigned char>(alpha*256.0);

		auto my_src_frame = std::make_shared<transform_frame>(src_frame);
		auto my_dest_frame = std::make_shared<transform_frame>(dest_frame);

		my_src_frame->audio_volume(255-volume);
		my_dest_frame->audio_volume(volume);

		double dir = info_.direction == transition_direction::from_left ? 1.0 : -1.0;		
		
		if(info_.type == transition::mix)
			my_dest_frame->alpha(alpha);		
		else if(info_.type == transition::slide)			
			my_dest_frame->translate((-1.0+alpha)*dir, 0.0);			
		else if(info_.type == transition::push)
		{
			my_dest_frame->translate((-1.0+alpha)*dir, 0.0);
			my_src_frame->translate((0.0+alpha)*dir, 0.0);
		}
		else if(info_.type == transition::wipe)
		{
			my_dest_frame->translate((-1.0+alpha)*dir, 0.0);			
			my_dest_frame->texcoord((-1.0+alpha)*dir, 1.0, 1.0-(1.0-alpha)*dir, 0.0);				
		}
						
		return std::make_shared<composite_frame>(my_src_frame, my_dest_frame);
	}
		
	void initialize(const frame_processor_device_ptr& frame_processor)
	{
		dest_producer_->initialize(frame_processor);
		frame_processor_ = frame_processor;
	}

	std::wstring print() const
	{
		return L"transition_producer. dest: " + (org_dest_producer_ ? org_dest_producer_->print() : L"empty") + L" src: " + (org_source_producer_ ? org_source_producer_->print() : L"empty");
	}
	
	frame_producer_ptr			org_source_producer_;
	frame_producer_ptr			org_dest_producer_;

	frame_producer_ptr			source_producer_;
	frame_producer_ptr			dest_producer_;
	
	unsigned short				current_frame_;
	
	const transition_info		info_;
	frame_processor_device_ptr	frame_processor_;
};

transition_producer::transition_producer(const frame_producer_ptr& dest, const transition_info& info) : impl_(new implementation(dest, info)){}
gpu_frame_ptr transition_producer::receive(){return impl_->receive();}
frame_producer_ptr transition_producer::get_following_producer() const{return impl_->get_following_producer();}
void transition_producer::set_leading_producer(const frame_producer_ptr& producer) { impl_->set_leading_producer(producer); }
void transition_producer::initialize(const frame_processor_device_ptr& frame_processor) { impl_->initialize(frame_processor);}
std::wstring transition_producer::print() const { return impl_->print();}

}}

