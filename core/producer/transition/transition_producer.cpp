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

#include "../../frame/frame_format.h"
#include "../../frame/gpu_frame.h"
#include "../../frame/composite_gpu_frame.h"
#include "../../frame/frame_factory.h"

#include "../../../common/utility/memory.h"
#include "../../renderer/render_device.h"

#include <boost/range/algorithm/copy.hpp>

namespace caspar { namespace core {	

struct transition_producer::implementation : boost::noncopyable
{
	implementation(const frame_producer_ptr& dest, const transition_info& info, const frame_format_desc& format_desc) 
		: current_frame_(0), info_(info), format_desc_(format_desc), dest_(dest)
	{
		if(!dest)
			BOOST_THROW_EXCEPTION(null_argument() << arg_name_info("dest"));
	}
		
	frame_producer_ptr get_following_producer() const
	{
		return dest_;
	}
	
	void set_leading_producer(const frame_producer_ptr& producer)
	{
		source_ = producer;
	}
		
	gpu_frame_ptr get_frame()
	{
		return ++current_frame_ >= info_.duration ? nullptr : compose(get_producer_frame(dest_), get_producer_frame(source_));
	}

	gpu_frame_ptr get_producer_frame(frame_producer_ptr& producer)
	{
		if(producer == nullptr)
		{	
			auto frame = factory_->create_frame(format_desc_);
			common::clear(frame->data(), frame->size());
			return frame;
		}

		gpu_frame_ptr frame;
		try
		{
			frame = producer->get_frame();
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			producer = nullptr;
			CASPAR_LOG(warning) << "Removed renderer from transition.";
		}

		if(frame == nullptr && producer != nullptr && producer->get_following_producer() != nullptr)
		{
			auto following = producer->get_following_producer();
			following->initialize(factory_);
			following->set_leading_producer(producer);
			producer = following;
			return get_producer_frame(producer);
		}
		return frame;
	}
			
	gpu_frame_ptr compose(const gpu_frame_ptr& dest_frame, const gpu_frame_ptr& src_frame) 
	{	
		if(!src_frame)
			return dest_frame;

		if(info_.type == transition_type::cut || !dest_frame)		
			return src_frame;
		
		int volume = static_cast<int>(static_cast<double>(current_frame_)/static_cast<double>(info_.duration)*256.0);
				
		for(size_t n = 0; n < dest_frame->audio_data().size(); ++n)
			dest_frame->audio_data()[n] = static_cast<short>((static_cast<int>(dest_frame->audio_data()[n])*volume)>>8);

		for(size_t n = 0; n < src_frame->audio_data().size(); ++n)
			src_frame->audio_data()[n] = static_cast<short>((static_cast<int>(src_frame->audio_data()[n])*(256-volume))>>8);
				
		float alpha = static_cast<float>(current_frame_)/static_cast<float>(info_.duration);
		auto composite = std::make_shared<composite_gpu_frame>(format_desc_.width, format_desc_.height);
		composite->add(src_frame);
		composite->add(dest_frame);
		if(info_.type == transition_type::mix)
		{
			src_frame->alpha(1.0f-alpha);
			dest_frame->alpha(alpha);
		}
		else if(info_.type == transition_type::slide)
		{
			if(info_.direction == transition_direction::from_left)			
				dest_frame->translate(-1.0f+alpha, 0.0f);			
			else if(info_.direction == transition_direction::from_right)
				dest_frame->translate(1.0f-alpha, 0.0f);			
		}
		else if(info_.type == transition_type::push)
		{
			if(info_.direction == transition_direction::from_left)		
			{
				dest_frame->translate(-1.0f+alpha, 0.0f);
				src_frame->translate(0.0f+alpha, 0.0f);
			}
			else if(info_.direction == transition_direction::from_right)
			{
				dest_frame->translate(1.0f-alpha, 0.0f);
				src_frame->translate(0.0f-alpha, 0.0f);
			}
		}
		return composite;
	}
		
	void initialize(const frame_factory_ptr& factory)
	{
		dest_->initialize(factory);
		factory_ = factory;
	}

	const frame_format_desc format_desc_;

	frame_producer_ptr		source_;
	frame_producer_ptr		dest_;
	
	unsigned short			current_frame_;
	
	const transition_info	info_;
	frame_factory_ptr		factory_;
};

transition_producer::transition_producer(const frame_producer_ptr& dest, const transition_info& info, const frame_format_desc& format_desc) 
	: impl_(new implementation(dest, info, format_desc)){}
gpu_frame_ptr transition_producer::get_frame(){return impl_->get_frame();}
frame_producer_ptr transition_producer::get_following_producer() const{return impl_->get_following_producer();}
void transition_producer::set_leading_producer(const frame_producer_ptr& producer) { impl_->set_leading_producer(producer); }
const frame_format_desc& transition_producer::get_frame_format_desc() const { return impl_->format_desc_; } 
void transition_producer::initialize(const frame_factory_ptr& factory) { impl_->initialize(factory);}

}}

