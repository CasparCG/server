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

#include "basic_frame.h"

#include "frame_transform.h"
#include "../../video_format.h"

#include <boost/foreach.hpp>

namespace caspar { namespace core {
																																						
struct basic_frame::implementation
{		
	std::vector<safe_ptr<basic_frame>> frames_;

	frame_transform frame_transform_;	
	
public:
	implementation(const std::vector<safe_ptr<basic_frame>>& frames) : frames_(frames) 
	{
	}
	implementation(std::vector<safe_ptr<basic_frame>>&& frames) : frames_(std::move(frames))
	{
	}
	implementation(safe_ptr<basic_frame>&& frame) 
	{
		frames_.push_back(std::move(frame));
	}
	implementation(const safe_ptr<basic_frame>& frame) 		
	{ 
		frames_.push_back(frame);
	}
	
	void accept(basic_frame& self, frame_visitor& visitor)
	{
		visitor.begin(self);
		BOOST_FOREACH(auto frame, frames_)
			frame->accept(visitor);
		visitor.end();
	}	
};
	
basic_frame::basic_frame() : impl_(new implementation(std::vector<safe_ptr<basic_frame>>())){}
basic_frame::basic_frame(const std::vector<safe_ptr<basic_frame>>& frames) : impl_(new implementation(frames)){}
basic_frame::basic_frame(const basic_frame& other) : impl_(new implementation(*other.impl_)){}
basic_frame::basic_frame(std::vector<safe_ptr<basic_frame>>&& frames) : impl_(new implementation(frames)){}
basic_frame::basic_frame(const safe_ptr<basic_frame>& frame) : impl_(new implementation(frame)){}
basic_frame::basic_frame(safe_ptr<basic_frame>&& frame)  : impl_(new implementation(std::move(frame))){}
basic_frame::basic_frame(basic_frame&& other) : impl_(std::move(other.impl_)){}
basic_frame& basic_frame::operator=(const basic_frame& other)
{
	basic_frame temp(other);
	temp.swap(*this);
	return *this;
}
basic_frame& basic_frame::operator=(basic_frame&& other)
{
	basic_frame temp(std::move(other));
	temp.swap(*this);
	return *this;
}
void basic_frame::swap(basic_frame& other){impl_.swap(other.impl_);}

const frame_transform& basic_frame::get_frame_transform() const { return impl_->frame_transform_;}
frame_transform& basic_frame::get_frame_transform() { return impl_->frame_transform_;}
void basic_frame::accept(frame_visitor& visitor){impl_->accept(*this, visitor);}

safe_ptr<basic_frame> basic_frame::interlace(const safe_ptr<basic_frame>& frame1, const safe_ptr<basic_frame>& frame2, field_mode::type mode)
{				
	if(frame1 == basic_frame::eof() || frame2 == basic_frame::eof())
		return basic_frame::eof();

	if(frame1 == basic_frame::empty() && frame2 == basic_frame::empty())
		return basic_frame::empty();
	
	if(frame1 == frame2 || mode == field_mode::progressive)
		return frame2;

	auto my_frame1 = make_safe<basic_frame>(frame1);
	auto my_frame2 = make_safe<basic_frame>(frame2);
	if(mode == field_mode::upper)
	{
		my_frame1->get_frame_transform().field_mode = field_mode::upper;	
		my_frame2->get_frame_transform().field_mode = field_mode::lower;	
	}									 
	else								 
	{									 
		my_frame1->get_frame_transform().field_mode = field_mode::lower;	
		my_frame2->get_frame_transform().field_mode = field_mode::upper;	
	}

	std::vector<safe_ptr<basic_frame>> frames;
	frames.push_back(my_frame1);
	frames.push_back(my_frame2);
	return make_safe<basic_frame>(std::move(frames));
}

safe_ptr<basic_frame> basic_frame::combine(const safe_ptr<basic_frame>& frame1, const safe_ptr<basic_frame>& frame2)
{	
	if(frame1 == basic_frame::eof() || frame2 == basic_frame::eof())
		return basic_frame::eof();
	
	if(frame1 == basic_frame::empty() && frame2 == basic_frame::empty())
		return basic_frame::empty();

	std::vector<safe_ptr<basic_frame>> frames;
	frames.push_back(frame1);
	frames.push_back(frame2);
	return make_safe<basic_frame>(std::move(frames));
}

safe_ptr<basic_frame> basic_frame::fill_and_key(const safe_ptr<basic_frame>& fill, const safe_ptr<basic_frame>& key)
{	
	if(fill == basic_frame::eof() || key == basic_frame::eof())
		return basic_frame::eof();

	if(fill == basic_frame::empty() || key == basic_frame::empty())
		return basic_frame::empty();

	std::vector<safe_ptr<basic_frame>> frames;
	key->get_frame_transform().is_key = true;
	frames.push_back(key);
	frames.push_back(fill);
	return make_safe<basic_frame>(std::move(frames));
}

safe_ptr<basic_frame> disable_audio(const safe_ptr<basic_frame>& frame)
{
	if(frame == basic_frame::empty())
		return frame;

	basic_frame frame2 = frame;
	frame2.get_frame_transform().volume = 0.0;
	return make_safe<basic_frame>(std::move(frame2));
}
	
}}