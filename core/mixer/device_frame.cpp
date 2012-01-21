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

#include "device_frame.h"

#include "gpu/ogl_device.h"
#include "gpu/host_buffer.h"
#include "gpu/device_buffer.h"

#include <core/producer/frame/frame_visitor.h>
#include <core/producer/frame/pixel_format.h>
#include <common/scope_guard.h>
#include <common/gl/gl_check.h>

#include <boost/lexical_cast.hpp>

namespace caspar { namespace core {
																																							
struct device_frame::impl : boost::noncopyable
{			
	std::vector<boost::shared_future<safe_ptr<device_buffer>>>	textures_;
	audio_buffer												audio_data_;
	const core::pixel_format_desc								desc_;
	const void*													tag_;
	core::field_mode											mode_;

	impl(const void* tag)
		: tag_(tag)
		, mode_(core::field_mode::empty)
	{
	}

	impl(std::vector<boost::unique_future<safe_ptr<device_buffer>>>&& textures, const void* tag, const core::pixel_format_desc& desc, field_mode type) 
		: desc_(desc)
		, tag_(tag)
		, mode_(type)
	{
		BOOST_FOREACH(auto& texture, textures)
			textures_.push_back(std::move(texture));
	}

	impl(boost::unique_future<safe_ptr<device_buffer>>&& texture, const void* tag, const core::pixel_format_desc& desc, field_mode type) 
		: desc_(desc)
		, tag_(tag)
		, mode_(type)
	{
		textures_.push_back(std::move(texture));
	}
			
	void accept(device_frame& self, core::frame_visitor& visitor)
	{
		visitor.begin(self);
		visitor.visit(self);
		visitor.end();
	}
};
	

device_frame::device_frame(const void* tag) : impl_(new impl(tag)){}
device_frame::device_frame(std::vector<future_texture>&& textures, const void* tag, const struct pixel_format_desc& desc, field_mode type)
	: impl_(new impl(std::move(textures), tag, desc, type)){}
device_frame::device_frame(future_texture&& texture, const void* tag, const struct pixel_format_desc& desc, field_mode type)
	: impl_(new impl(std::move(texture), tag, desc, type)){}
device_frame::device_frame(device_frame&& other) : impl_(std::move(other.impl_)){}
device_frame& device_frame::operator=(device_frame&& other)
{
	device_frame temp(std::move(other));
	temp.swap(*this);
	return *this;
}
void device_frame::swap(device_frame& other){impl_.swap(other.impl_);}
audio_buffer& device_frame::audio_data() { return impl_->audio_data_; }
const void* device_frame::tag() const {return impl_->tag_;}
const core::pixel_format_desc& device_frame::get_pixel_format_desc() const{return impl_->desc_;}
const std::vector<boost::shared_future<safe_ptr<device_buffer>>>& device_frame::get_textures() const{return impl_->textures_;}
core::field_mode device_frame::get_type() const{return impl_->mode_;}
void device_frame::accept(core::frame_visitor& visitor){impl_->accept(*this, visitor);}

}}