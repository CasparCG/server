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
#include "frame/basic_frame.h"

#include <boost/property_tree/ptree.hpp>

namespace caspar { namespace core {
	
struct layer::implementation
{				
	safe_ptr<frame_producer>	foreground_;
	safe_ptr<frame_producer>	background_;
	int64_t						frame_number_;
	int32_t						auto_play_delta_;
	bool						is_paused_;

public:
	implementation() 
		: foreground_(frame_producer::empty())
		, background_(frame_producer::empty())
		, frame_number_(0)
		, auto_play_delta_(-1)
		, is_paused_(false)
	{
	}
	
	void pause()
	{
		is_paused_ = true;
	}

	void resume()
	{
		is_paused_ = false;
	}

	void load(const safe_ptr<frame_producer>& producer, bool preview, int auto_play_delta)
	{		
		background_		 = producer;
		auto_play_delta_ = auto_play_delta;

		if(auto_play_delta_ > -1 && foreground_ == frame_producer::empty())
			play();

		if(preview) // Play the first frame and pause.
		{			
			play();
			receive(frame_producer::NO_HINT);
			pause();
		}
	}
	
	void play()
	{			
		if(background_ != frame_producer::empty())
		{
			background_->set_leading_producer(foreground_);
			
			foreground_			= background_;
			background_			= frame_producer::empty();
			frame_number_		= 0;
			auto_play_delta_	= -1;	
		}

		is_paused_			= false;
	}
	
	void stop()
	{
		foreground_			= frame_producer::empty();
		background_			= background_;
		frame_number_		= 0;
		auto_play_delta_	= -1;

		is_paused_			= true;
	}
		
	safe_ptr<basic_frame> receive(int hints)
	{		
		try
		{
			if(is_paused_)
			{
				if(foreground_->last_frame() == basic_frame::empty())
					foreground_->receive(frame_producer::NO_HINT);

				return disable_audio(foreground_->last_frame());
			}

			auto frame = receive_and_follow(foreground_, hints);
			if(frame == core::basic_frame::late())
				return foreground_->last_frame();

			auto frames_left = static_cast<int64_t>(foreground_->nb_frames()) - static_cast<int64_t>(++frame_number_) - static_cast<int64_t>(auto_play_delta_);
			if(auto_play_delta_ > -1 && frames_left < 1)
			{
				play();
				return receive(hints);
			}
				
			return frame;
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			stop();
			return core::basic_frame::empty();
		}
	}

	boost::unique_future<std::wstring> call(bool foreground, const std::wstring& param)
	{
		return (foreground ? foreground_ : background_)->call(param);
	}

	bool empty() const
	{
		return background_ == core::frame_producer::empty() && foreground_ == core::frame_producer::empty();
	}

	boost::property_tree::wptree info() const
	{
		boost::property_tree::wptree info;
		info.add(L"status",		is_paused_ ? L"paused" : (foreground_ == frame_producer::empty() ? L"stopped" : L"playing"));
		info.add(L"auto_delta",	auto_play_delta_);
		info.add(L"frame-number", frame_number_);

		auto nb_frames = foreground_->nb_frames();

		info.add(L"nb_frames",	 nb_frames == std::numeric_limits<int64_t>::max() ? -1 : nb_frames);
		info.add(L"frames-left", nb_frames == std::numeric_limits<int64_t>::max() ? -1 : (foreground_->nb_frames() - frame_number_ - auto_play_delta_));
		info.add_child(L"foreground.producer", foreground_->info());
		info.add_child(L"background.producer", background_->info());
		return info;
	}
};

layer::layer() : impl_(new implementation()){}
layer::layer(layer&& other) : impl_(std::move(other.impl_)){}
layer& layer::operator=(layer&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}
layer::layer(const layer& other) : impl_(new implementation(*other.impl_)){}
layer& layer::operator=(const layer& other)
{
	layer temp(other);
	temp.swap(*this);
	return *this;
}
void layer::swap(layer& other)
{	
	impl_.swap(other.impl_);
}
void layer::load(const safe_ptr<frame_producer>& frame_producer, bool preview, int auto_play_delta){return impl_->load(frame_producer, preview, auto_play_delta);}	
void layer::play(){impl_->play();}
void layer::pause(){impl_->pause();}
void layer::stop(){impl_->stop();}
bool layer::is_paused() const{return impl_->is_paused_;}
int64_t layer::frame_number() const{return impl_->frame_number_;}
safe_ptr<basic_frame> layer::receive(int hints) {return impl_->receive(hints);}
safe_ptr<frame_producer> layer::foreground() const { return impl_->foreground_;}
safe_ptr<frame_producer> layer::background() const { return impl_->background_;}
bool layer::empty() const {return impl_->empty();}
boost::unique_future<std::wstring> layer::call(bool foreground, const std::wstring& param){return impl_->call(foreground, param);}
boost::property_tree::wptree layer::info() const{return impl_->info();}
}}