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

#include "../stdafx.h"

#include "layer.h"

#include "frame_producer.h"

#include "frame/basic_frame.h"


namespace caspar { namespace core {
	
struct layer::implementation
{				
	safe_ptr<frame_producer>	foreground_;
	safe_ptr<frame_producer>	background_;
	bool						is_paused_;
	int							auto_play_delta_;
	int64_t						frame_number_;
public:
	implementation() 
		: foreground_(frame_producer::empty())
		, background_(frame_producer::empty())
		, is_paused_(false)
		, auto_play_delta_(-1){}
	
	void pause(){is_paused_ = true;}
	void resume(){is_paused_ = false;}

	void load(const safe_ptr<frame_producer>& producer, bool preview, int auto_play_delta)
	{		
		background_	= producer;

		if(auto_play_delta > -1)
		{
			if(producer->nb_frames() > 0)
				auto_play_delta_ = auto_play_delta;
			else
				CASPAR_LOG(warning) << producer->print() << L" Does not support auto-play.";
		}

		if(preview) // Play the first frame and pause.
		{			
			play();
			receive();
			pause();
		}

		if(auto_play_delta >= 0 && foreground_ == frame_producer::empty())
			play();
	}
	
	void play()
	{			
		if(background_ != frame_producer::empty())
		{
			background_->set_leading_producer(foreground_);
			foreground_ = background_;
			frame_number_ = 0;
			auto_play_delta_ = -1;
			background_ = frame_producer::empty();
		}
		resume();
	}
	
	void stop()
	{
		foreground_ = frame_producer::empty();
	}
		
	safe_ptr<basic_frame> receive()
	{		
		if(is_paused_)
			return foreground_->last_frame();
		
		const auto frames_left = foreground_->nb_frames() - (++frame_number_) - auto_play_delta_;

		auto frame = receive_and_follow(foreground_, frame_producer::NO_HINT);
		if(frame == core::basic_frame::late())
			return foreground_->last_frame();
		
		if(auto_play_delta_ == 0)
		{
			if(frame == core::basic_frame::eof())
			{
				CASPAR_ASSERT(frames_left == 0);

				CASPAR_LOG(info) << L"Automatically playing next clip with " << auto_play_delta_ << " frames offset.";
				
				play();
				frame = receive();
			}
		}
		else if(auto_play_delta_ > 0)
		{
			if(frames_left <= 0 || frame == core::basic_frame::eof())
			{
				CASPAR_VERIFY(frame != core::basic_frame::eof() && "Received early EOF. Media duration metadata incorrect.");

				CASPAR_LOG(info) << L"Automatically playing next clip with " << auto_play_delta_ << " frames offset.";
				
				play();
				frame = receive();
			}
		}
				
		return frame;
	}

	bool empty() const
	{
		return background_ == core::frame_producer::empty() && foreground_ == core::frame_producer::empty();
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
safe_ptr<basic_frame> layer::receive() {return impl_->receive();}
safe_ptr<frame_producer> layer::foreground() const { return impl_->foreground_;}
safe_ptr<frame_producer> layer::background() const { return impl_->background_;}
bool layer::empty() const {return impl_->empty();}
}}