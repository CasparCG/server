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

#include "write_frame.h"

#include <common/except.h>
#include <core/frame/frame_visitor.h>
#include <core/frame/pixel_format.h>

#include <boost/lexical_cast.hpp>

namespace caspar { namespace accelerator { namespace cpu {
			
struct write_frame::impl : boost::noncopyable
{			
	std::vector<spl::shared_ptr<host_buffer>>	buffers_;
	core::audio_buffer							audio_data_;
	const core::pixel_format_desc				desc_;
	const void*									tag_;

	impl(const void* tag)
		: desc_(core::pixel_format::invalid)
		, tag_(tag)		
	{
	}

	impl(const void* tag, const core::pixel_format_desc& desc) 
		: desc_(desc)
		, tag_(tag)
	{
		std::transform(desc.planes.begin(), desc.planes.end(), std::back_inserter(buffers_), [&](const core::pixel_format_desc::plane& plane)
		{
			return spl::make_shared<host_buffer>(plane.size);
		});
	}
			
	void accept(write_frame& self, core::frame_visitor& visitor)
	{
		visitor.push(self.get_frame_transform());
		visitor.visit(self);
		visitor.pop();
	}

	boost::iterator_range<uint8_t*> image_data(int index)
	{
		if(index >= buffers_.size() || !buffers_[index]->data())
			BOOST_THROW_EXCEPTION(out_of_range());
		auto ptr = buffers_[index]->data();
		return boost::iterator_range<uint8_t*>(ptr, ptr+buffers_[index]->size());
	}
};
	
write_frame::write_frame(const void* tag) : impl_(new impl(tag)){}
write_frame::write_frame(const void* tag, const core::pixel_format_desc& desc) 
	: impl_(new impl(tag, desc)){}
write_frame::write_frame(write_frame&& other) : impl_(std::move(other.impl_)){}
write_frame& write_frame::operator=(write_frame&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}
void write_frame::swap(write_frame& other){impl_.swap(other.impl_);}
void write_frame::accept(core::frame_visitor& visitor){impl_->accept(*this, visitor);}
const core::pixel_format_desc& write_frame::get_pixel_format_desc() const{return impl_->desc_;}
const boost::iterator_range<const uint8_t*> write_frame::image_data(int index) const{return impl_->image_data(index);}
const core::audio_buffer& write_frame::audio_data() const{return impl_->audio_data_;}
const boost::iterator_range<uint8_t*> write_frame::image_data(int index){return impl_->image_data(index);}
core::audio_buffer& write_frame::audio_data(){return impl_->audio_data_;}
double write_frame::get_frame_rate() const{return 0.0;} // TODO: what's this?
int write_frame::width() const{return impl_->desc_.planes.at(0).width;}
int write_frame::height() const{return impl_->desc_.planes.at(0).height;}						
const void* write_frame::tag() const{return impl_->tag_;}	
std::vector<spl::shared_ptr<host_buffer>> write_frame::get_buffers(){return impl_->buffers_;}

}}}