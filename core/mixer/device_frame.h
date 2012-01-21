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

#pragma once

#include <common/memory/safe_ptr.h>

#include <core/producer/frame/basic_frame.h>
#include <core/video_format.h>
#include <core/mixer/audio/audio_mixer.h>

#include <boost/range/iterator_range.hpp>
#include <boost/thread/future.hpp>

#include <stdint.h>
#include <vector>

namespace caspar { namespace core {
	
class device_frame sealed : public core::basic_frame
{
public:	
	typedef boost::unique_future<safe_ptr<class device_buffer>> future_texture;

	explicit device_frame(const void* tag);
	explicit device_frame(std::vector<future_texture>&& textures, const void* tag, const struct pixel_format_desc& desc, field_mode type);
	explicit device_frame(future_texture&& texture, const void* tag, const struct pixel_format_desc& desc, field_mode type);

	device_frame(device_frame&& other);
	device_frame& operator=(device_frame&& other);
			
	// basic_frame

	virtual void accept(struct frame_visitor& visitor) override;

	// write _frame

	void swap(device_frame& other);
			
	audio_buffer& audio_data();
		
	field_mode get_type() const;
	
	const void* tag() const;

	const struct pixel_format_desc& get_pixel_format_desc() const;
	
private:
	friend class image_mixer;
	
	const std::vector<boost::shared_future<safe_ptr<class device_buffer>>>& get_textures() const;

	struct impl;
	safe_ptr<impl> impl_;
};


}}