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

#include "../frame_producer.h"
#include "../../frame/draw_frame.h"
#include "../../frame/frame_transform.h"

#include <tbb/parallel_invoke.h>

namespace caspar { namespace core {	

struct transition_producer : public frame_producer
{	
	const field_mode		mode_;
	unsigned int				current_frame_;
	
	const transition_info		info_;
	
	spl::shared_ptr<frame_producer>	dest_producer_;
	spl::shared_ptr<frame_producer>	source_producer_;

	spl::shared_ptr<draw_frame>		last_frame_;
		
	explicit transition_producer(const field_mode& mode, const spl::shared_ptr<frame_producer>& dest, const transition_info& info) 
		: mode_(mode)
		, current_frame_(0)
		, info_(info)
		, dest_producer_(dest)
		, source_producer_(frame_producer::empty())
		, last_frame_(draw_frame::empty()){}
	
	// frame_producer

	virtual spl::shared_ptr<frame_producer> get_following_producer() const override
	{
		return dest_producer_;
	}
	
	virtual void set_leading_producer(const spl::shared_ptr<frame_producer>& producer) override
	{
		source_producer_ = producer;
	}

	virtual spl::shared_ptr<draw_frame> receive(int flags) override
	{
		if(++current_frame_ >= info_.duration)
			return draw_frame::eof();
		
		auto dest = draw_frame::empty();
		auto source = draw_frame::empty();

		tbb::parallel_invoke(
		[&]
		{
			dest = dest_producer_->receive(flags);
			if(dest == core::draw_frame::late())
				dest = dest_producer_->last_frame();
		},
		[&]
		{
			source = source_producer_->receive(flags);
			if(source == core::draw_frame::late())
				source = source_producer_->last_frame();
		});

		return compose(dest, source);
	}

	virtual spl::shared_ptr<core::draw_frame> last_frame() const override
	{
		return last_frame_;
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
						
	spl::shared_ptr<draw_frame> compose(const spl::shared_ptr<draw_frame>& dest_frame, const spl::shared_ptr<draw_frame>& src_frame) 
	{	
		if(info_.type == transition_type::cut)		
			return src_frame;
										
		const double delta1 = info_.tweener(current_frame_*2-1, 0.0, 1.0, static_cast<double>(info_.duration*2));
		const double delta2 = info_.tweener(current_frame_*2,   0.0, 1.0, static_cast<double>(info_.duration*2));  

		const double dir = info_.direction == transition_direction::from_left ? 1.0 : -1.0;		
		
		// For interlaced transitions. Seperate fields into seperate frames which are transitioned accordingly.
		
		src_frame->get_frame_transform().volume = 1.0-delta2;
		auto s_frame1 = spl::make_shared<draw_frame>(src_frame);
		auto s_frame2 = spl::make_shared<draw_frame>(src_frame);
		
		dest_frame->get_frame_transform().volume = delta2;
		auto d_frame1 = spl::make_shared<draw_frame>(dest_frame);
		auto d_frame2 = spl::make_shared<draw_frame>(dest_frame);
		
		if(info_.type == transition_type::mix)
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
		if(info_.type == transition_type::slide)
		{
			d_frame1->get_frame_transform().fill_translation[0] = (-1.0+delta1)*dir;	
			d_frame2->get_frame_transform().fill_translation[0] = (-1.0+delta2)*dir;		
		}
		else if(info_.type == transition_type::push)
		{
			d_frame1->get_frame_transform().fill_translation[0] = (-1.0+delta1)*dir;
			d_frame2->get_frame_transform().fill_translation[0] = (-1.0+delta2)*dir;

			s_frame1->get_frame_transform().fill_translation[0] = (0.0+delta1)*dir;	
			s_frame2->get_frame_transform().fill_translation[0] = (0.0+delta2)*dir;		
		}
		else if(info_.type == transition_type::wipe)		
		{
			d_frame1->get_frame_transform().clip_scale[0] = delta1;	
			d_frame2->get_frame_transform().clip_scale[0] = delta2;			
		}
				
		const auto s_frame = s_frame1->get_frame_transform() == s_frame2->get_frame_transform() ? s_frame2 : draw_frame::interlace(s_frame1, s_frame2, mode_);
		const auto d_frame = d_frame1->get_frame_transform() == d_frame2->get_frame_transform() ? d_frame2 : draw_frame::interlace(d_frame1, d_frame2, mode_);
		
		last_frame_ = draw_frame::over(s_frame2, d_frame2);

		return draw_frame::over(s_frame, d_frame);
	}
};

spl::shared_ptr<frame_producer> create_transition_producer(const field_mode& mode, const spl::shared_ptr<frame_producer>& destination, const transition_info& info)
{
	return core::wrap_producer(spl::make_shared<transition_producer>(mode, destination, info));
}

}}

