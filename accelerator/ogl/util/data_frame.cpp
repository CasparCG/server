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

#include "data_frame.h"

#include "device.h"
#include "host_buffer.h"
#include "device_buffer.h"

#include <common/except.h>
#include <core/frame/frame_visitor.h>
#include <core/frame/pixel_format.h>

#include <boost/lexical_cast.hpp>

namespace caspar { namespace accelerator { namespace ogl {
																																							
struct data_frame::impl : boost::noncopyable
{			
	std::shared_ptr<device>						ogl_;
	std::vector<spl::shared_ptr<ogl::host_buffer>>	buffers_;
	core::audio_buffer								audio_data_;
	const core::pixel_format_desc					desc_;
	const void*										tag_;
	double											frame_rate_;
	core::field_mode								field_mode_;

	impl(const void* tag)
		: desc_(core::pixel_format::invalid)
		, tag_(tag)		
		, field_mode_(core::field_mode::empty)
	{
	}

	impl(const spl::shared_ptr<ogl::device>& ogl, const void* tag, const core::pixel_format_desc& desc, double frame_rate, core::field_mode field_mode) 
		: ogl_(ogl)
		, desc_(desc)
		, tag_(tag)
		, frame_rate_(frame_rate)
		, field_mode_(field_mode)
	{
		std::transform(desc.planes.begin(), desc.planes.end(), std::back_inserter(buffers_), [&](const core::pixel_format_desc::plane& plane)
		{
			return ogl_->create_host_buffer(plane.size, ogl::host_buffer::usage::write_only);
		});
	}
			
	boost::iterator_range<uint8_t*> image_data(int index)
	{
		if(index >= buffers_.size() || !buffers_[index]->data())
			BOOST_THROW_EXCEPTION(out_of_range());
		auto ptr = static_cast<uint8_t*>(buffers_[index]->data());
		return boost::iterator_range<uint8_t*>(ptr, ptr+buffers_[index]->size());
	}
};
	
data_frame::data_frame(const void* tag) : impl_(new impl(tag)){}
data_frame::data_frame(const spl::shared_ptr<ogl::device>& ogl, const void* tag, const core::pixel_format_desc& desc, double frame_rate, core::field_mode field_mode) 
	: impl_(new impl(ogl, tag, desc, frame_rate, field_mode)){}
data_frame::data_frame(data_frame&& other) : impl_(std::move(other.impl_)){}
data_frame& data_frame::operator=(data_frame&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}
void data_frame::swap(data_frame& other){impl_.swap(other.impl_);}
const core::pixel_format_desc& data_frame::pixel_format_desc() const{return impl_->desc_;}
const boost::iterator_range<const uint8_t*> data_frame::image_data(int index) const{return impl_->image_data(index);}
const core::audio_buffer& data_frame::audio_data() const{return impl_->audio_data_;}
const boost::iterator_range<uint8_t*> data_frame::image_data(int index){return impl_->image_data(index);}
core::audio_buffer& data_frame::audio_data(){return impl_->audio_data_;}
double data_frame::frame_rate() const{return impl_->frame_rate_;}
core::field_mode data_frame::field_mode() const{return impl_->field_mode_;}
int data_frame::width() const{return impl_->desc_.planes.at(0).width;}
int data_frame::height() const{return impl_->desc_.planes.at(0).height;}						
const void* data_frame::tag() const{return impl_->tag_;}	
std::vector<spl::shared_ptr<ogl::host_buffer>> data_frame::buffers() const{return impl_->buffers_;}

}}}