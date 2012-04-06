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

#include "../stdafx.h"

#include "layer.h"

#include "frame_producer.h"

#include "../video_format.h"
#include "../frame/draw_frame.h"
#include "../frame/frame_transform.h"

#include <boost/optional.hpp>
#include <boost/thread/future.hpp>

namespace caspar { namespace core {

struct layer::impl
{				
	monitor::basic_subject				event_subject_;
	monitor::basic_subject				foreground_event_subject_;
	monitor::basic_subject				background_event_subject_;
	spl::shared_ptr<frame_producer>		foreground_;
	spl::shared_ptr<frame_producer>		background_;
	boost::optional<int32_t>			auto_play_delta_;

public:
	impl(int index) 
		: event_subject_(monitor::path("layer") % index)
		, foreground_event_subject_("")
		, background_event_subject_("background")
		, foreground_(frame_producer::empty())
		, background_(frame_producer::empty())
	{
		foreground_event_subject_.subscribe(event_subject_);
		background_event_subject_.subscribe(event_subject_);
	}

	void pause()
	{
		foreground_->paused(true);
	}
	
	void load(spl::shared_ptr<frame_producer> producer, bool preview, const boost::optional<int32_t>& auto_play_delta)
	{		
		background_->unsubscribe(background_event_subject_);
		background_ = std::move(producer);
		background_->subscribe(background_event_subject_);

		auto_play_delta_ = auto_play_delta;

		if(preview)
		{
			play();
			foreground_->paused(true);
		}

		if(auto_play_delta_ && foreground_ == frame_producer::empty())
			play();
	}
	
	void play()
	{			
		if(background_ != frame_producer::empty())
		{
			background_->leading_producer(foreground_);

			background_->unsubscribe(background_event_subject_);
			foreground_->unsubscribe(foreground_event_subject_);

			foreground_ = std::move(background_);
			background_ = std::move(frame_producer::empty());
			
			foreground_->subscribe(foreground_event_subject_);

			auto_play_delta_.reset();
		}

		foreground_->paused(false);
	}
	
	void stop()
	{
		foreground_->unsubscribe(foreground_event_subject_);

		foreground_ = std::move(frame_producer::empty());

		auto_play_delta_.reset();
	}
		
	draw_frame receive(const video_format_desc& format_desc)
	{		
		try
		{		
			auto frame = foreground_->receive();
			
			if(frame == core::draw_frame::late())
				return foreground_->last_frame();
						
			if(auto_play_delta_)
			{
				auto frames_left = static_cast<int64_t>(foreground_->nb_frames()) - foreground_->frame_number() - static_cast<int64_t>(*auto_play_delta_);
				if(frames_left < 1)
				{
					play();
					return receive(format_desc);
				}
			}

			event_subject_	<< monitor::event("time")	% monitor::duration(foreground_->frame_number()/format_desc.fps)
														% monitor::duration(static_cast<int64_t>(foreground_->nb_frames()) - static_cast<int64_t>(auto_play_delta_ ? *auto_play_delta_ : 0)/format_desc.fps)
							<< monitor::event("frame")	% static_cast<int64_t>(foreground_->frame_number())
														% static_cast<int64_t>((static_cast<int64_t>(foreground_->nb_frames()) - static_cast<int64_t>(auto_play_delta_ ? *auto_play_delta_ : 0)));

			foreground_event_subject_ << monitor::event("type") % foreground_->name();
			background_event_subject_ << monitor::event("type") % background_->name();
				
			return frame;
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			stop();
			return core::draw_frame::empty();
		}
	}
	
	boost::property_tree::wptree info() const
	{
		boost::property_tree::wptree info;
		info.add(L"auto_delta",	(auto_play_delta_ ? boost::lexical_cast<std::wstring>(*auto_play_delta_) : L"null"));
		info.add(L"frame-number", foreground_->frame_number());

		auto nb_frames = foreground_->nb_frames();

		info.add(L"nb_frames",	 nb_frames == std::numeric_limits<int64_t>::max() ? -1 : nb_frames);
		info.add(L"frames-left", nb_frames == std::numeric_limits<int64_t>::max() ? -1 : (foreground_->nb_frames() - foreground_->frame_number() - (auto_play_delta_ ? *auto_play_delta_ : 0)));
		info.add_child(L"producer", foreground_->info());
		info.add_child(L"background.producer", background_->info());
		return info;
	}
};

layer::layer(int index) : impl_(new impl(index)){}
layer::layer(layer&& other) : impl_(std::move(other.impl_)){}
layer& layer::operator=(layer&& other)
{
	other.swap(*this);
	return *this;
}
void layer::swap(layer& other)
{	
	impl_.swap(other.impl_);
}
void layer::load(spl::shared_ptr<frame_producer> frame_producer, bool preview, const boost::optional<int32_t>& auto_play_delta){return impl_->load(std::move(frame_producer), preview, auto_play_delta);}	
void layer::play(){impl_->play();}
void layer::pause(){impl_->pause();}
void layer::stop(){impl_->stop();}
draw_frame layer::receive(const video_format_desc& format_desc) {return impl_->receive(format_desc);}
spl::shared_ptr<frame_producer> layer::foreground() const { return impl_->foreground_;}
spl::shared_ptr<frame_producer> layer::background() const { return impl_->background_;}
boost::property_tree::wptree layer::info() const{return impl_->info();}
void layer::subscribe(const monitor::observable::observer_ptr& o) {impl_->event_subject_.subscribe(o);}
void layer::unsubscribe(const monitor::observable::observer_ptr& o) {impl_->event_subject_.unsubscribe(o);}
}}