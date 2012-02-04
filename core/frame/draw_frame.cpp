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

#include "draw_frame.h"

#include "frame_transform.h"

#include <boost/foreach.hpp>

namespace caspar { namespace core {
																																						
struct draw_frame::impl
{		
	std::vector<spl::shared_ptr<draw_frame>> frames_;

	frame_transform frame_transform_;		
public:
	impl()
	{
	}

	impl(std::vector<spl::shared_ptr<draw_frame>> frames) : frames_(std::move(frames))
	{
	}

	impl(spl::shared_ptr<draw_frame>&& frame) 
	{
		frames_.push_back(std::move(frame));
	}

	impl(const spl::shared_ptr<draw_frame>& frame) 		
	{ 
		frames_.push_back(frame);
	}
	
	void accept(draw_frame& self, frame_visitor& visitor)
	{
		visitor.begin(self);
		BOOST_FOREACH(auto frame, frames_)
			frame->accept(visitor);
		visitor.end();
	}	
};
	
draw_frame::draw_frame() : impl_(new impl()){}
draw_frame::draw_frame(const draw_frame& other) : impl_(new impl(*other.impl_)){}
draw_frame::draw_frame(draw_frame&& other) : impl_(std::move(other.impl_)){}
draw_frame::draw_frame(std::vector<spl::shared_ptr<draw_frame>> frames) : impl_(new impl(frames)){}
draw_frame::draw_frame(spl::shared_ptr<draw_frame> frame)  : impl_(new impl(std::move(frame))){}
draw_frame& draw_frame::operator=(draw_frame other)
{
	other.swap(*this);
	return *this;
}
void draw_frame::swap(draw_frame& other){impl_.swap(other.impl_);}

const frame_transform& draw_frame::get_frame_transform() const { return impl_->frame_transform_;}
frame_transform& draw_frame::get_frame_transform() { return impl_->frame_transform_;}
void draw_frame::accept(frame_visitor& visitor){impl_->accept(*this, visitor);}

spl::shared_ptr<draw_frame> draw_frame::interlace(const spl::shared_ptr<draw_frame>& frame1, const spl::shared_ptr<draw_frame>& frame2, field_mode mode)
{				
	if(frame1 == draw_frame::eof() || frame2 == draw_frame::eof())
		return draw_frame::eof();

	if(frame1 == draw_frame::empty() && frame2 == draw_frame::empty())
		return draw_frame::empty();
	
	if(frame1 == frame2 || mode == field_mode::progressive)
		return frame2;

	auto my_frame1 = spl::make_shared<draw_frame>(frame1);
	auto my_frame2 = spl::make_shared<draw_frame>(frame2);
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

	std::vector<spl::shared_ptr<draw_frame>> frames;
	frames.push_back(my_frame1);
	frames.push_back(my_frame2);
	return spl::make_shared<draw_frame>(std::move(frames));
}

spl::shared_ptr<draw_frame> draw_frame::over(const spl::shared_ptr<draw_frame>& frame1, const spl::shared_ptr<draw_frame>& frame2)
{	
	if(frame1 == draw_frame::eof() || frame2 == draw_frame::eof())
		return draw_frame::eof();
	
	if(frame1 == draw_frame::empty() && frame2 == draw_frame::empty())
		return draw_frame::empty();

	std::vector<spl::shared_ptr<draw_frame>> frames;
	frames.push_back(frame1);
	frames.push_back(frame2);
	return spl::make_shared<draw_frame>(std::move(frames));
}

spl::shared_ptr<draw_frame> draw_frame::mask(const spl::shared_ptr<draw_frame>& fill, const spl::shared_ptr<draw_frame>& key)
{	
	if(fill == draw_frame::eof() || key == draw_frame::eof())
		return draw_frame::eof();

	if(fill == draw_frame::empty() || key == draw_frame::empty())
		return draw_frame::empty();

	std::vector<spl::shared_ptr<draw_frame>> frames;
	key->get_frame_transform().is_key = true;
	frames.push_back(key);
	frames.push_back(fill);
	return spl::make_shared<draw_frame>(std::move(frames));
}
	
spl::shared_ptr<draw_frame> draw_frame::silence(const spl::shared_ptr<draw_frame>& frame)
{
	auto frame2 = spl::make_shared<draw_frame>(frame);
	frame2->get_frame_transform().volume = 0.0;
	return frame2;
}

const spl::shared_ptr<draw_frame>& draw_frame::eof()
{
	static spl::shared_ptr<draw_frame> frame = spl::make_shared<draw_frame>();
	return frame;
}

const spl::shared_ptr<draw_frame>& draw_frame::empty()
{
	static spl::shared_ptr<draw_frame> frame = spl::make_shared<draw_frame>();
	return frame;
}

const spl::shared_ptr<draw_frame>& draw_frame::late()
{
	static spl::shared_ptr<draw_frame> frame = spl::make_shared<draw_frame>();
	return frame;
}
	

}}