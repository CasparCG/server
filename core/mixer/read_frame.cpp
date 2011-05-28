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

#include "read_frame.h"

#include "gpu/host_buffer.h"	
#include "gpu/ogl_device.h"

namespace caspar { namespace core {
																																							
struct read_frame::implementation : boost::noncopyable
{
	boost::unique_future<safe_ptr<host_buffer>> image_data_;
	std::vector<int16_t> audio_data_;

public:
	implementation(boost::unique_future<safe_ptr<host_buffer>>&& image_data, std::vector<int16_t>&& audio_data) 
		: image_data_(std::move(image_data))
		, audio_data_(std::move(audio_data)){}	

	const boost::iterator_range<const uint8_t*> image_data()
	{
		try
		{
			if(!image_data_.get()->data())
			{
				ogl_device::invoke([&]
				{
					image_data_.get()->map();
				}, high_priority);
			}

			if(!image_data_.get()->data())
				return boost::iterator_range<const uint8_t*>();
			auto ptr = static_cast<const uint8_t*>(image_data_.get()->data());
			return boost::iterator_range<const uint8_t*>(ptr, ptr + image_data_.get()->size());
		}
		catch(...) // image_data_ future might store exception.
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			return boost::iterator_range<const uint8_t*>();
		}
	}
	const boost::iterator_range<const int16_t*> audio_data() const
	{
		return boost::iterator_range<const int16_t*>(audio_data_.data(), audio_data_.data() + audio_data_.size());
	}
};

read_frame::read_frame(boost::unique_future<safe_ptr<host_buffer>>&& image_data, std::vector<int16_t>&& audio_data) 
	: impl_(new implementation(std::move(image_data), std::move(audio_data))){}
read_frame::read_frame(safe_ptr<host_buffer>&& image_data, std::vector<int16_t>&& audio_data) 
{
	boost::promise<safe_ptr<host_buffer>> p;
	p.set_value(std::move(image_data));
	impl_.reset(new implementation(std::move(p.get_future()), std::move(audio_data)));
}

const boost::iterator_range<const uint8_t*> read_frame::image_data() const{return impl_->image_data();}
const boost::iterator_range<const int16_t*> read_frame::audio_data() const{return impl_->audio_data();}

}}