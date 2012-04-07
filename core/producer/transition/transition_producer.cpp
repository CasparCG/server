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
#include "../../monitor/monitor.h"

#include <tbb/parallel_invoke.h>

namespace caspar { namespace core {	

class transition_producer : public frame_producer_base
{	
	monitor::basic_subject				event_subject_;
	const field_mode					mode_;
	int									current_frame_;
	
	const transition_info				info_;
		
	spl::shared_ptr<frame_producer>		dest_producer_;
	spl::shared_ptr<frame_producer>		source_producer_;

	bool								paused_;
		
public:
	explicit transition_producer(const field_mode& mode, const spl::shared_ptr<frame_producer>& dest, const transition_info& info) 
		: mode_(mode)
		, current_frame_(0)
		, info_(info)
		, dest_producer_(dest)
		, source_producer_(frame_producer::empty())
		, paused_(false)
	{
		dest->subscribe(event_subject_);

		CASPAR_LOG(info) << print() << L" Initialized";
	}
	
	// frame_producer
		
	void leading_producer(const spl::shared_ptr<frame_producer>& producer) override
	{
		source_producer_ = producer;
	}

	draw_frame receive_impl() override
	{
		if(current_frame_ >= info_.duration)
		{
			source_producer_ = core::frame_producer::empty();
			return dest_producer_->receive();		
		}

		current_frame_ += 1;

		auto dest = draw_frame::empty();
		auto source = draw_frame::empty();

		tbb::parallel_invoke(
		[&]
		{
			dest = dest_producer_->receive();
			if(dest == core::draw_frame::late())
				dest = dest_producer_->last_frame();
		},
		[&]
		{
			source = source_producer_->receive();
			if(source == core::draw_frame::late())
				source = source_producer_->last_frame();
		});			
						
		event_subject_	<< monitor::event("transition/frame") % current_frame_ % info_.duration
						<< monitor::event("transition/type") % [&]() -> std::string
																{
																	switch(info_.type.value())
																	{
																	case transition_type::mix:		return "mix";
																	case transition_type::wipe:		return "wipe";
																	case transition_type::slide:	return "slide";
																	case transition_type::push:		return "push";
																	case transition_type::cut:		return "cut";
																	default:						return "n/a";
																	}
																}();

		return compose(dest, source);
	}

	draw_frame last_frame() override
	{
		if(current_frame_ >= info_.duration)
			return dest_producer_->last_frame();

		return frame_producer_base::last_frame();
	}
			
	uint32_t nb_frames() const override
	{
		return dest_producer_->nb_frames();
	}

	std::wstring print() const override
	{
		return L"transition[" + source_producer_->print() + L"=>" + dest_producer_->print() + L"]";
	}

	std::wstring name() const override
	{
		return L"transition";
	}
	
	boost::property_tree::wptree info() const override
	{
		return dest_producer_->info();
	}
	
	boost::unique_future<std::wstring> call(const std::wstring& str) override
	{
		return dest_producer_->call(str);
	}

	// transition_producer
						
	draw_frame compose(draw_frame dest_frame, draw_frame src_frame) const
	{	
		if(info_.type == transition_type::cut)		
			return src_frame;
										
		const double delta1 = info_.tweener(current_frame_*2-1, 0.0, 1.0, static_cast<double>(info_.duration*2));
		const double delta2 = info_.tweener(current_frame_*2,   0.0, 1.0, static_cast<double>(info_.duration*2));  

		const double dir = info_.direction == transition_direction::from_left ? 1.0 : -1.0;		
		
		// For interlaced transitions. Seperate fields into seperate frames which are transitioned accordingly.
		
		src_frame.transform().audio_transform.volume = 1.0-delta2;
		auto s_frame1 = src_frame;
		auto s_frame2 = src_frame;
		
		dest_frame.transform().audio_transform.volume = delta2;
		auto d_frame1 = dest_frame;
		auto d_frame2 = dest_frame;
		
		if(info_.type == transition_type::mix)
		{
			d_frame1.transform().image_transform.opacity = delta1;	
			d_frame1.transform().image_transform.is_mix = true;
			d_frame2.transform().image_transform.opacity = delta2;
			d_frame2.transform().image_transform.is_mix = true;

			s_frame1.transform().image_transform.opacity = 1.0-delta1;	
			s_frame1.transform().image_transform.is_mix = true;
			s_frame2.transform().image_transform.opacity = 1.0-delta2;	
			s_frame2.transform().image_transform.is_mix = true;
		}
		if(info_.type == transition_type::slide)
		{
			d_frame1.transform().image_transform.fill_translation[0] = (-1.0+delta1)*dir;	
			d_frame2.transform().image_transform.fill_translation[0] = (-1.0+delta2)*dir;		
		}
		else if(info_.type == transition_type::push)
		{
			d_frame1.transform().image_transform.fill_translation[0] = (-1.0+delta1)*dir;
			d_frame2.transform().image_transform.fill_translation[0] = (-1.0+delta2)*dir;

			s_frame1.transform().image_transform.fill_translation[0] = (0.0+delta1)*dir;	
			s_frame2.transform().image_transform.fill_translation[0] = (0.0+delta2)*dir;		
		}
		else if(info_.type == transition_type::wipe)		
		{
			d_frame1.transform().image_transform.clip_scale[0] = delta1;	
			d_frame2.transform().image_transform.clip_scale[0] = delta2;			
		}
				
		const auto s_frame = s_frame1.transform() == s_frame2.transform() ? s_frame2 : draw_frame::interlace(s_frame1, s_frame2, mode_);
		const auto d_frame = d_frame1.transform() == d_frame2.transform() ? d_frame2 : draw_frame::interlace(d_frame1, d_frame2, mode_);
		
		return draw_frame::over(s_frame, d_frame);
	}

	void subscribe(const monitor::observable::observer_ptr& o) override															
	{
		event_subject_.subscribe(o);
	}

	void unsubscribe(const monitor::observable::observer_ptr& o) override		
	{
		event_subject_.unsubscribe(o);
	}
};

spl::shared_ptr<frame_producer> create_transition_producer(const field_mode& mode, const spl::shared_ptr<frame_producer>& destination, const transition_info& info)
{
	return spl::make_shared<transition_producer>(mode, destination, info);
}

}}

