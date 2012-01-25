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
#include <common/forward.h>

#include <core/producer/frame/basic_frame.h>
#include <core/video_format.h>
#include <core/mixer/audio/audio_mixer.h>

#include <boost/range/iterator_range.hpp>

#include <stdint.h>
#include <vector>

FORWARD1(boost, template<typename> class shared_future);
FORWARD3(caspar, core, gpu, class accelerator);
FORWARD3(caspar, core, gpu, class device_buffer);

namespace caspar { namespace core {
	
class write_frame sealed : public core::basic_frame
{
public:	
	explicit write_frame(const void* tag);
	explicit write_frame(const safe_ptr<gpu::accelerator>& ogl, const void* tag, const struct pixel_format_desc& desc, const field_mode& mode = field_mode::progressive);

	write_frame(write_frame&& other);
	write_frame& operator=(write_frame&& other);
			
	// basic_frame

	virtual void accept(struct frame_visitor& visitor) override;

	// write _frame

	void swap(write_frame& other);
			
	boost::iterator_range<uint8_t*> image_data(int plane_index = 0);	
	audio_buffer& audio_data();
	
	void commit(int plane_index);
	void commit();
	
	field_mode get_field_mode() const;
	
	const void* tag() const;

	const struct pixel_format_desc& get_pixel_format_desc() const;
	
	const std::vector<boost::shared_future<safe_ptr<gpu::device_buffer>>>& get_textures() const;
private:
	struct impl;
	safe_ptr<impl> impl_;
};


}}