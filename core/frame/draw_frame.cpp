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

#include "data_frame.h"

#include "frame_transform.h"

#include <boost/foreach.hpp>

namespace caspar { namespace core {
		
struct draw_frame::impl
{		
	int													tag_;
	std::vector<draw_frame>								frames_;
	std::shared_ptr<spl::unique_ptr<const data_frame>>	data_frame_;

	core::frame_transform frame_transform_;		
public:		
	enum tags
	{
		frame_tag = 0,
		empty_tag,
		eof_tag,
		late_tag
	};

	impl(int tag = impl::empty_tag)
		: tag_(tag)
	{
	}

	impl(spl::unique_ptr<const data_frame>&& frame) 
		: tag_(frame_tag)
		, data_frame_(new spl::unique_ptr<const data_frame>(std::move(frame)))
	{
	}
	
	impl(std::vector<draw_frame> frames)
		: tag_(frame_tag)
		, frames_(std::move(frames))
	{
	}
		
	void accept(frame_visitor& visitor) const
	{
		visitor.push(frame_transform_);
		if(data_frame_)
		{
			visitor.visit(**data_frame_);
		}
		else
		{
			BOOST_FOREACH(auto frame, frames_)
				frame.accept(visitor);
		}
		visitor.pop();
	}	

	bool operator==(const impl& other)
	{
		return	tag_ == other.tag_ && 
				frames_ == other.frames_ && 
				data_frame_ == other.data_frame_;
	}
};
	
draw_frame::draw_frame() : impl_(new impl()){}
draw_frame::draw_frame(int tag) : impl_(new impl(std::move(tag))){}
draw_frame::draw_frame(const draw_frame& other) : impl_(new impl(*other.impl_)){}
draw_frame::draw_frame(draw_frame&& other) : impl_(std::move(other.impl_)){}
draw_frame::draw_frame(spl::unique_ptr<const data_frame>&& frame)  : impl_(new impl(std::move(frame))){}
draw_frame::draw_frame(std::vector<draw_frame> frames) : impl_(new impl(frames)){}
draw_frame::~draw_frame(){}
draw_frame& draw_frame::operator=(draw_frame other)
{
	other.swap(*this);
	return *this;
}
void draw_frame::swap(draw_frame& other){impl_.swap(other.impl_);}

const core::frame_transform& draw_frame::transform() const { return impl_->frame_transform_;}
core::frame_transform& draw_frame::transform() { return impl_->frame_transform_;}
void draw_frame::accept(frame_visitor& visitor) const{impl_->accept(visitor);}
bool draw_frame::operator==(const draw_frame& other)const{return *impl_ == *other.impl_;}
bool draw_frame::operator!=(const draw_frame& other)const{return !(*this == other);}

draw_frame draw_frame::interlace(draw_frame frame1, draw_frame frame2, core::field_mode mode)
{				
	if(frame1 == draw_frame::eof() || frame2 == draw_frame::eof())
		return draw_frame::eof();

	if(frame1 == draw_frame::empty() && frame2 == draw_frame::empty())
		return draw_frame::empty();
	
	if(frame1 == frame2 || mode == field_mode::progressive)
		return frame2;

	if(mode == field_mode::upper)
	{
		frame1.transform().image_transform.field_mode = field_mode::upper;	
		frame2.transform().image_transform.field_mode = field_mode::lower;	
	}									 
	else								 
	{									 
		frame1.transform().image_transform.field_mode = field_mode::lower;	
		frame2.transform().image_transform.field_mode = field_mode::upper;	
	}

	std::vector<draw_frame> frames;
	frames.push_back(std::move(frame1));
	frames.push_back(std::move(frame2));
	return draw_frame(std::move(frames));
}

draw_frame draw_frame::over(draw_frame frame1, draw_frame frame2)
{	
	if(frame1 == draw_frame::eof() || frame2 == draw_frame::eof())
		return draw_frame::eof();
	
	if(frame1 == draw_frame::empty() && frame2 == draw_frame::empty())
		return draw_frame::empty();

	std::vector<draw_frame> frames;
	frames.push_back(std::move(frame1));
	frames.push_back(std::move(frame2));
	return draw_frame(std::move(frames));
}

draw_frame draw_frame::mask(draw_frame fill, draw_frame key)
{	
	if(fill == draw_frame::eof() || key == draw_frame::eof())
		return draw_frame::eof();

	if(fill == draw_frame::empty() || key == draw_frame::empty())
		return draw_frame::empty();

	std::vector<draw_frame> frames;
	key.transform().image_transform.is_key = true;
	frames.push_back(std::move(key));
	frames.push_back(std::move(fill));
	return draw_frame(std::move(frames));
}

draw_frame draw_frame::still(draw_frame frame)
{
	frame.transform().image_transform.is_still	= true;	
	frame.transform().audio_transform.volume	= 0.0;		
	return frame;
}

const draw_frame& draw_frame::eof()
{
	static draw_frame frame(impl::eof_tag);
	return frame;
}

const draw_frame& draw_frame::empty()
{
	static draw_frame frame(impl::empty_tag);
	return frame;
}

const draw_frame& draw_frame::late()
{
	static draw_frame frame(impl::late_tag);
	return frame;
}
	

}}