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

#include "gpu_write_frame.h"

#include <core/producer/frame/pixel_format.h>

#include "../image/image_mixer.h"
#include "../audio/audio_mixer.h"

#include "../gpu/host_buffer.h"

#include <common/gl/gl_check.h>

#include <boost/range/algorithm.hpp>

namespace caspar { namespace mixer {
																																							
struct gpu_write_frame::implementation : boost::noncopyable
{				
	std::vector<safe_ptr<host_buffer>> buffers_;
	std::vector<short> audio_data_;
	const core::pixel_format_desc desc_;
	int tag_;

public:
	implementation(int tag, const core::pixel_format_desc& desc, const std::vector<safe_ptr<host_buffer>>& buffers) 
		: desc_(desc)
		, buffers_(buffers)
		, tag_(tag){}
	
	void accept(gpu_write_frame& self, core::frame_visitor& visitor)
	{
		visitor.begin(self);
		visitor.visit(self);
		visitor.end();
	}

	boost::iterator_range<unsigned char*> image_data(size_t index)
	{
		if(index >= buffers_.size() || !buffers_[index]->data())
			return boost::iterator_range<const unsigned char*>();
		auto ptr = static_cast<unsigned char*>(buffers_[index]->data());
		return boost::iterator_range<unsigned char*>(ptr, ptr+buffers_[index]->size());
	}

	const boost::iterator_range<const unsigned char*> image_data(size_t index) const
	{
		if(index >= buffers_.size() || !buffers_[index]->data())
			return boost::iterator_range<const unsigned char*>();
		auto ptr = static_cast<const unsigned char*>(buffers_[index]->data());
		return boost::iterator_range<const unsigned char*>(ptr, ptr+buffers_[index]->size());
	}
};
	
gpu_write_frame::gpu_write_frame(int tag, const core::pixel_format_desc& desc, const std::vector<safe_ptr<host_buffer>>& buffers) : impl_(new implementation(tag, desc, buffers)){}
gpu_write_frame::gpu_write_frame(gpu_write_frame&& other) : impl_(std::move(other.impl_)){}
void gpu_write_frame::swap(gpu_write_frame& other){impl_.swap(other.impl_);}
gpu_write_frame& gpu_write_frame::operator=(gpu_write_frame&& other)
{
	gpu_write_frame temp(std::move(other));
	temp.swap(*this);
	return *this;
}
void gpu_write_frame::accept(core::frame_visitor& visitor){impl_->accept(*this, visitor);}
boost::iterator_range<unsigned char*> gpu_write_frame::image_data(size_t index){return impl_->image_data(index);}
std::vector<short>& gpu_write_frame::audio_data() { return impl_->audio_data_; }
const boost::iterator_range<const unsigned char*> gpu_write_frame::image_data(size_t index) const
{
	return boost::iterator_range<const unsigned char*>(impl_->image_data(index).begin(), impl_->image_data(index).end());
}
const boost::iterator_range<const short*> gpu_write_frame::audio_data() const
{
	return boost::iterator_range<const short*>(impl_->audio_data_.data(), impl_->audio_data_.data() + impl_->audio_data_.size());
}
int gpu_write_frame::tag() const {return impl_->tag_;}
const core::pixel_format_desc& gpu_write_frame::get_pixel_format_desc() const{return impl_->desc_;}
std::vector<safe_ptr<host_buffer>>& gpu_write_frame::get_plane_buffers(){return impl_->buffers_;}
}}