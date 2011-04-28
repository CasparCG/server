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

#include "../producer/frame/basic_frame.h"
#include "../producer/frame/audio_transform.h"

namespace caspar { namespace core {
	
struct layer::implementation
{				
	safe_ptr<frame_producer>	foreground_;
	safe_ptr<frame_producer>	background_;
	safe_ptr<basic_frame>		last_frame_;
	bool						is_paused_;
public:
	implementation() 
		: foreground_(frame_producer::empty())
		, background_(frame_producer::empty())
		, last_frame_(basic_frame::empty())
		, is_paused_(false){}
	
	void pause(){is_paused_ = true;}
	void resume(){is_paused_ = false;}

	void load(const safe_ptr<frame_producer>& producer, bool preview)
	{		
		background_ = producer;

		if(preview)
		{
			play();
			receive();
			pause();
		}
	}
	
	void play()
	{			
		if(background_ != frame_producer::empty())
		{
			background_->set_leading_producer(foreground_);
			foreground_ = background_;
			background_ = frame_producer::empty();
		}
		resume();
	}
	
	void stop()
	{
		pause();
		last_frame_ = basic_frame::empty();
		foreground_ = frame_producer::empty();
	}
		
	safe_ptr<basic_frame> receive()
	{		
		if(is_paused_)		
			last_frame_->get_audio_transform().set_has_audio(false);		
		else
			last_frame_ = receive_and_follow(foreground_);

		return last_frame_;
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
	layer tmp(other);
	tmp.swap(*this);
	return *this;
}
void layer::swap(layer& other)
{	
	impl_.swap(other.impl_);
}
void layer::load(const safe_ptr<frame_producer>& frame_producer, bool preview){return impl_->load(frame_producer, preview);}	
void layer::play(){impl_->play();}
void layer::pause(){impl_->pause();}
void layer::stop(){impl_->stop();}
safe_ptr<basic_frame> layer::receive() {return impl_->receive();}
safe_ptr<frame_producer> layer::foreground() const { return impl_->foreground_;}
safe_ptr<frame_producer> layer::background() const { return impl_->background_;}
}}