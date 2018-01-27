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

#include "../StdAfx.h"

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
	spl::shared_ptr<monitor::subject>	monitor_subject_;
	spl::shared_ptr<frame_producer>		foreground_			= frame_producer::empty();
	spl::shared_ptr<frame_producer>		background_			= frame_producer::empty();;
	boost::optional<int32_t>			auto_play_delta_;
	bool								is_paused_			= false;
	int64_t								current_frame_age_	= 0;

public:
	impl(int index)
		: monitor_subject_(spl::make_shared<monitor::subject>(
				"/layer/" + boost::lexical_cast<std::string>(index)))
//		, foreground_event_subject_("")
//		, background_event_subject_("background")
	{
//		foreground_event_subject_.subscribe(event_subject_);
//		background_event_subject_.subscribe(event_subject_);
	}

	void set_foreground(spl::shared_ptr<frame_producer> producer)
	{
		foreground_->monitor_output().detach_parent();
		foreground_ = std::move(producer);
		foreground_->monitor_output().attach_parent(monitor_subject_);
	}

	void pause()
	{
		foreground_->paused(true);
		is_paused_ = true;
	}

	void resume()
	{
		foreground_->paused(false);
		is_paused_ = false;
	}

	void load(spl::shared_ptr<frame_producer> producer, bool preview, const boost::optional<int32_t>& auto_play_delta)
	{
//		background_->unsubscribe(background_event_subject_);
		background_ = std::move(producer);
//		background_->subscribe(background_event_subject_);

		auto_play_delta_ = auto_play_delta;

		if(preview)
		{
			play();
			receive(video_format::invalid);
			foreground_->paused(true);
			is_paused_ = true;
		}

		if(auto_play_delta_ && foreground_ == frame_producer::empty())
			play();
	}

	void play()
	{
		if(background_ != frame_producer::empty())
		{
			background_->leading_producer(foreground_);

			set_foreground(background_);
			background_ = std::move(frame_producer::empty());

			auto_play_delta_.reset();
		}

		foreground_->paused(false);
		is_paused_ = false;
	}

	void stop()
	{
		set_foreground(frame_producer::empty());

		auto_play_delta_.reset();
	}

	draw_frame receive(const video_format_desc& format_desc)
	{
		try
		{
			*monitor_subject_ << monitor::message("/paused") % is_paused_;

			boost::timer produce_timer;
			auto frame = foreground_->receive();
			auto produce_time = produce_timer.elapsed();

			*monitor_subject_ << monitor::message("/profiler/time") % produce_time % (1.0 / format_desc.fps);

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

			//event_subject_	<< monitor::event("time")	% monitor::duration(foreground_->frame_number()/format_desc.fps)
			//											% monitor::duration(static_cast<int64_t>(foreground_->nb_frames()) - static_cast<int64_t>(auto_play_delta_ ? *auto_play_delta_ : 0)/format_desc.fps)
			//				<< monitor::event("frame")	% static_cast<int64_t>(foreground_->frame_number())
			//											% static_cast<int64_t>((static_cast<int64_t>(foreground_->nb_frames()) - static_cast<int64_t>(auto_play_delta_ ? *auto_play_delta_ : 0)));

			//foreground_event_subject_ << monitor::event("type") % foreground_->name();
			//background_event_subject_ << monitor::event("type") % background_->name();

			current_frame_age_ = frame.get_and_record_age_millis();

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
		info.add(L"frame-age", current_frame_age_);
		info.add_child(L"foreground.producer", foreground_->info());
		info.add_child(L"background.producer", background_->info());
		return info;
	}

	boost::property_tree::wptree delay_info() const
	{
		boost::property_tree::wptree info;
		info.add(L"producer", foreground_->print());
		info.add(L"frame-age", current_frame_age_);
		return info;
	}

	void on_interaction(const interaction_event::ptr& event)
	{
		foreground_->on_interaction(event);
	}

	bool collides(double x, double y) const
	{
		return foreground_->collides(x, y);
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
void layer::resume(){impl_->resume();}
void layer::stop(){impl_->stop();}
draw_frame layer::receive(const video_format_desc& format_desc) {return impl_->receive(format_desc);}
spl::shared_ptr<frame_producer> layer::foreground() const { return impl_->foreground_;}
spl::shared_ptr<frame_producer> layer::background() const { return impl_->background_;}
boost::property_tree::wptree layer::info() const{return impl_->info();}
boost::property_tree::wptree layer::delay_info() const{return impl_->delay_info();}
monitor::subject& layer::monitor_output() {return *impl_->monitor_subject_;}
void layer::on_interaction(const interaction_event::ptr& event) { impl_->on_interaction(event); }
bool layer::collides(double x, double y) const { return impl_->collides(x, y); }
}}
