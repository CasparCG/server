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
#include "../../stdafx.h"

#include "basic_frame.h"

#include "image_transform.h"
#include "audio_transform.h"
#include "pixel_format.h"
#include "../../video_format.h"

#include <boost/range/algorithm.hpp>

namespace caspar { namespace core {
																																						
struct basic_frame::implementation
{		
	std::vector<safe_ptr<basic_frame>> frames_;

	image_transform image_transform_;	
	audio_transform audio_transform_;
	
public:
	implementation(const std::vector<safe_ptr<basic_frame>>& frames) 
		: frames_(frames) {}
	implementation(std::vector<safe_ptr<basic_frame>>&& frames) 
		: frames_(std::move(frames)){}
	
	void accept(const basic_frame& self, frame_visitor& visitor)
	{
		visitor.begin(self);
		BOOST_FOREACH(auto frame, frames_)
			frame->accept(visitor);
		visitor.end();
	}	
};
	
basic_frame::basic_frame() : impl_(new implementation(std::vector<safe_ptr<basic_frame>>())){}
basic_frame::basic_frame(std::vector<safe_ptr<basic_frame>>&& frames) : impl_(new implementation(std::move(frames))){}
basic_frame::basic_frame(const basic_frame& other) : impl_(new implementation(*other.impl_)){}
basic_frame::basic_frame(const safe_ptr<basic_frame>& frame)
{
	std::vector<safe_ptr<basic_frame>> frames;
	frames.push_back(frame);
	impl_.reset(new implementation(std::move(frames)));
}
basic_frame::basic_frame(safe_ptr<basic_frame>&& frame)
{
	std::vector<safe_ptr<basic_frame>> frames;
	frames.push_back(std::move(frame));
	impl_.reset(new implementation(std::move(frames)));
}
void basic_frame::swap(basic_frame& other){impl_.swap(other.impl_);}
basic_frame& basic_frame::operator=(const basic_frame& other)
{
	basic_frame temp(other);
	temp.swap(*this);
	return *this;
}
basic_frame::basic_frame(basic_frame&& other) : impl_(std::move(other.impl_)){}
basic_frame& basic_frame::operator=(basic_frame&& other)
{
	basic_frame temp(std::move(other));
	temp.swap(*this);
	return *this;
}
void basic_frame::accept(frame_visitor& visitor){impl_->accept(*this, visitor);}

const image_transform& basic_frame::get_image_transform() const { return impl_->image_transform_;}
image_transform& basic_frame::get_image_transform() { return impl_->image_transform_;}
const audio_transform& basic_frame::get_audio_transform() const { return impl_->audio_transform_;}
audio_transform& basic_frame::get_audio_transform() { return impl_->audio_transform_;}

safe_ptr<basic_frame> basic_frame::interlace(const safe_ptr<basic_frame>& frame1, const safe_ptr<basic_frame>& frame2, video_mode::type mode)
{			
	if(frame1 == basic_frame::empty() && frame2 == basic_frame::empty())
		return basic_frame::empty();
	
	if(frame1 == basic_frame::eof() && frame2 == basic_frame::eof())
		return basic_frame::eof();
	
	if(frame1 == frame2 || mode == video_mode::progressive)
		return frame2;

	auto my_frame1 = make_safe<basic_frame>(frame1);
	auto my_frame2 = make_safe<basic_frame>(frame2);
	if(mode == video_mode::upper)
	{
		my_frame1->get_image_transform().set_mode(video_mode::upper);	
		my_frame2->get_image_transform().set_mode(video_mode::lower);	
	}											 
	else										 
	{											 
		my_frame1->get_image_transform().set_mode(video_mode::lower);	
		my_frame2->get_image_transform().set_mode(video_mode::upper);	
	}

	std::vector<safe_ptr<basic_frame>> frames;
	frames.push_back(my_frame1);
	frames.push_back(my_frame2);
	return basic_frame(std::move(frames));
}

safe_ptr<basic_frame> basic_frame::combine(const safe_ptr<basic_frame>& frame1, const safe_ptr<basic_frame>& frame2)
{
	if(frame1 == basic_frame::empty() && frame2 == basic_frame::empty())
		return basic_frame::empty();
	
	if(frame1 == basic_frame::eof() && frame2 == basic_frame::eof())
		return basic_frame::eof();
	
	std::vector<safe_ptr<basic_frame>> frames;
	frames.push_back(frame1);
	frames.push_back(frame2);
	return basic_frame(std::move(frames));
}

safe_ptr<basic_frame> basic_frame::fill_and_key(const safe_ptr<basic_frame>& fill, const safe_ptr<basic_frame>& key)
{
	if(fill == basic_frame::empty() && key == basic_frame::empty())
		return basic_frame::empty();
	
	if(fill == basic_frame::eof() && key == basic_frame::eof())
		return basic_frame::eof();

	if(key == basic_frame::empty() || key == basic_frame::eof())
		return fill;

	std::vector<safe_ptr<basic_frame>> frames;
	key->get_image_transform().set_is_key(true);
	frames.push_back(key);
	frames.push_back(fill);
	return basic_frame(std::move(frames));
}
	
}}