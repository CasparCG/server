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

#include "write_frame.h"

#include "gpu/accelerator.h"
#include "gpu/host_buffer.h"
#include "gpu/device_buffer.h"

#include <common/exception/exceptions.h>
#include <core/frame/frame_visitor.h>
#include <core/frame/pixel_format.h>

#include <boost/lexical_cast.hpp>

namespace caspar { namespace core {
																																							
struct write_frame::impl : boost::noncopyable
{			
	std::shared_ptr<gpu::accelerator>				ogl_;
	std::vector<std::shared_ptr<gpu::host_buffer>>	buffers_;
	std::vector<boost::shared_future<safe_ptr<gpu::device_buffer>>>		textures_;
	audio_buffer									audio_data_;
	const core::pixel_format_desc					desc_;
	const void*										tag_;

	impl(const void* tag)
		: desc_(core::pixel_format::invalid)
		, tag_(tag)		
	{
	}

	impl(const safe_ptr<gpu::accelerator>& ogl, const void* tag, const core::pixel_format_desc& desc) 
		: ogl_(ogl)
		, desc_(desc)
		, tag_(tag)
	{
		std::transform(desc.planes.begin(), desc.planes.end(), std::back_inserter(buffers_), [&](const core::pixel_format_desc::plane& plane)
		{
			return ogl_->create_host_buffer(plane.size, gpu::host_buffer::usage::write_only);
		});
		textures_.resize(desc.planes.size());
	}
			
	void accept(write_frame& self, core::frame_visitor& visitor)
	{
		visitor.begin(self);
		visitor.visit(self);
		visitor.end();
	}

	boost::iterator_range<uint8_t*> image_data(int index)
	{
		if(index >= buffers_.size() || !buffers_[index]->data())
			BOOST_THROW_EXCEPTION(out_of_range());
		auto ptr = static_cast<uint8_t*>(buffers_[index]->data());
		return boost::iterator_range<uint8_t*>(ptr, ptr+buffers_[index]->size());
	}
	
	void commit()
	{
		for(int n = 0; n < buffers_.size(); ++n)
			commit(n);
	}

	void commit(int plane_index)
	{
		if(plane_index >= buffers_.size())
			return;
				
		auto buffer = std::move(buffers_[plane_index]); // Release buffer once done.

		if(!buffer)
			return;
				
		auto plane = desc_.planes.at(plane_index);
		textures_.at(plane_index) = ogl_->copy_async(make_safe_ptr(std::move(buffer)), plane.width, plane.height, plane.channels);
	}
};
	
write_frame::write_frame(const void* tag) : impl_(new impl(tag)){}
write_frame::write_frame(const safe_ptr<gpu::accelerator>& ogl, const void* tag, const core::pixel_format_desc& desc) 
	: impl_(new impl(ogl, tag, desc)){}
write_frame::write_frame(write_frame&& other) : impl_(std::move(other.impl_)){}
write_frame& write_frame::operator=(write_frame&& other)
{
	write_frame temp(std::move(other));
	temp.swap(*this);
	return *this;
}
void write_frame::swap(write_frame& other){impl_.swap(other.impl_);}
boost::iterator_range<uint8_t*> write_frame::image_data(int index){return impl_->image_data(index);}
audio_buffer& write_frame::audio_data() { return impl_->audio_data_; }
const void* write_frame::tag() const {return impl_->tag_;}
const core::pixel_format_desc& write_frame::get_pixel_format_desc() const{return impl_->desc_;}
const std::vector<boost::shared_future<safe_ptr<gpu::device_buffer>>>& write_frame::get_textures() const{return impl_->textures_;}
void write_frame::commit(int plane_index){impl_->commit(plane_index);}
void write_frame::commit(){impl_->commit();}
void write_frame::accept(core::frame_visitor& visitor){impl_->accept(*this, visitor);}

}}