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

#include "../../stdafx.h"

#include "transition_producer.h"

#include <core/video_format.h>

#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_transform.h>

#include <tbb/parallel_invoke.h>

#include <boost/assign.hpp>

using namespace boost::assign;

namespace caspar { namespace core {	

struct transition_producer : public frame_producer
{	
	const field_mode::type		mode_;
	unsigned int				current_frame_;
	
	const transition_info		info_;
	
	safe_ptr<frame_producer>	dest_producer_;
	safe_ptr<frame_producer>	source_producer_;

	safe_ptr<basic_frame>		last_frame_;
		
	explicit transition_producer(const field_mode::type& mode, const safe_ptr<frame_producer>& dest, const transition_info& info) 
		: mode_(mode)
		, current_frame_(0)
		, info_(info)
		, dest_producer_(dest)
		, source_producer_(frame_producer::empty())
		, last_frame_(basic_frame::empty()){}
	
	// frame_producer

	virtual safe_ptr<frame_producer> get_following_producer() const override
	{
		return dest_producer_;
	}
	
	virtual void set_leading_producer(const safe_ptr<frame_producer>& producer) override
	{
		source_producer_ = producer;
	}

	virtual boost::unique_future<std::wstring> call(const std::wstring& params) override
	{
		return dest_producer_->call(params);
	}

	virtual safe_ptr<basic_frame> receive(int hints) override
	{
		if(++current_frame_ >= info_.duration)
			return basic_frame::eof();
		
		auto dest = basic_frame::empty();
		auto source = basic_frame::empty();

		tbb::parallel_invoke(
		[&]
		{
			dest = receive_and_follow(dest_producer_, hints);
			if(dest == core::basic_frame::late())
				dest = dest_producer_->last_frame();
		},
		[&]
		{
			source = receive_and_follow(source_producer_, hints);
			if(source == core::basic_frame::late())
				source = source_producer_->last_frame();
		});

		return compose(dest, source);
	}

	virtual safe_ptr<core::basic_frame> last_frame() const override
	{
		return disable_audio(last_frame_);
	}

	virtual uint32_t nb_frames() const override
	{
		return get_following_producer()->nb_frames();
	}

	virtual std::wstring print() const override
	{
		return L"transition[" + source_producer_->print() + L"=>" + dest_producer_->print() + L"]";
	}
	
	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"transition-producer");
		info.add_child(L"source.producer",	   source_producer_->info());
		info.add_child(L"destination.producer", dest_producer_->info());
		return info;
	}

	// transition_producer
						
	safe_ptr<basic_frame> compose(const safe_ptr<basic_frame>& dest_frame, const safe_ptr<basic_frame>& src_frame) 
	{	
		if(info_.type == transition::cut)		
			return src_frame;
										
		const double delta1 = info_.tweener(current_frame_*2-1, 0.0, 1.0, info_.duration*2);
		const double delta2 = info_.tweener(current_frame_*2, 0.0, 1.0, info_.duration*2);  

		const double dir = info_.direction == transition_direction::from_left ? 1.0 : -1.0;		
		
		// For interlaced transitions. Seperate fields into seperate frames which are transitioned accordingly.
		
		auto s_frame1 = make_safe<basic_frame>(src_frame);
		auto s_frame2 = make_safe<basic_frame>(src_frame);

		s_frame1->get_frame_transform().volume = 0.0;
		s_frame2->get_frame_transform().volume = 1.0-delta2;

		auto d_frame1 = make_safe<basic_frame>(dest_frame);
		auto d_frame2 = make_safe<basic_frame>(dest_frame);
		
		d_frame1->get_frame_transform().volume = 0.0;
		d_frame2->get_frame_transform().volume = delta2;

		if(info_.type == transition::mix)
		{
			d_frame1->get_frame_transform().opacity = delta1;	
			d_frame1->get_frame_transform().is_mix = true;
			d_frame2->get_frame_transform().opacity = delta2;
			d_frame2->get_frame_transform().is_mix = true;

			s_frame1->get_frame_transform().opacity = 1.0-delta1;	
			s_frame1->get_frame_transform().is_mix = true;
			s_frame2->get_frame_transform().opacity = 1.0-delta2;	
			s_frame2->get_frame_transform().is_mix = true;
		}
		if(info_.type == transition::slide)
		{
			d_frame1->get_frame_transform().fill_translation[0] = (-1.0+delta1)*dir;	
			d_frame2->get_frame_transform().fill_translation[0] = (-1.0+delta2)*dir;		
		}
		else if(info_.type == transition::push)
		{
			d_frame1->get_frame_transform().fill_translation[0] = (-1.0+delta1)*dir;
			d_frame2->get_frame_transform().fill_translation[0] = (-1.0+delta2)*dir;

			s_frame1->get_frame_transform().fill_translation[0] = (0.0+delta1)*dir;	
			s_frame2->get_frame_transform().fill_translation[0] = (0.0+delta2)*dir;		
		}
		else if(info_.type == transition::wipe)		
		{
			d_frame1->get_frame_transform().clip_scale[0] = delta1;	
			d_frame2->get_frame_transform().clip_scale[0] = delta2;			
		}
				
		const auto s_frame = s_frame1->get_frame_transform() == s_frame2->get_frame_transform() ? s_frame2 : basic_frame::interlace(s_frame1, s_frame2, mode_);
		const auto d_frame = d_frame1->get_frame_transform() == d_frame2->get_frame_transform() ? d_frame2 : basic_frame::interlace(d_frame1, d_frame2, mode_);
		
		last_frame_ = basic_frame::combine(s_frame2, d_frame2);

		return basic_frame::combine(s_frame, d_frame);
	}
};

safe_ptr<frame_producer> create_transition_producer(const field_mode::type& mode, const safe_ptr<frame_producer>& destination, const transition_info& info)
{
	return create_producer_print_proxy(
			make_safe<transition_producer>(mode, destination, info));
}

}}

