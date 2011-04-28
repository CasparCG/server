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

#include "gpu_read_frame.h"

#include "../gpu/host_buffer.h"

#include <common/gl/gl_check.h>

namespace caspar { namespace mixer {
																																							
struct gpu_read_frame::implementation : boost::noncopyable
{
	safe_ptr<const host_buffer> image_data_;
	std::vector<short> audio_data_;

public:
	implementation(safe_ptr<const host_buffer>&& image_data, std::vector<short>&& audio_data) 
		: image_data_(std::move(image_data))
		, audio_data_(std::move(audio_data)){}	
};

gpu_read_frame::gpu_read_frame(safe_ptr<const host_buffer>&& image_data, std::vector<short>&& audio_data) : impl_(new implementation(std::move(image_data), std::move(audio_data))){}

const boost::iterator_range<const unsigned char*> gpu_read_frame::image_data() const
{
	if(!impl_->image_data_->data())
		return boost::iterator_range<const unsigned char*>();
	auto ptr = static_cast<const unsigned char*>(impl_->image_data_->data());
	return boost::iterator_range<const unsigned char*>(ptr, ptr + impl_->image_data_->size());
}
const boost::iterator_range<const short*> gpu_read_frame::audio_data() const
{
	return boost::iterator_range<const short*>(impl_->audio_data_.data(), impl_->audio_data_.data() + impl_->audio_data_.size());
}

}}