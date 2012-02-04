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

#include <common/spl/memory.h>
#include <common/forward.h>

#include <core/frame/draw_frame.h>
#include <core/frame/data_frame.h>
#include <core/video_format.h>
#include <core/mixer/audio/audio_mixer.h>

#include <boost/range/iterator_range.hpp>

#include <stdint.h>
#include <vector>

FORWARD3(caspar, core, gpu, class accelerator);
FORWARD3(caspar, core, gpu, class host_buffer);

namespace caspar { namespace core {
	
class write_frame sealed : public core::draw_frame, public core::data_frame
{
	write_frame(const write_frame&);
	write_frame& operator=(const write_frame);
public:	
	explicit write_frame(const void* tag);
	explicit write_frame(const spl::shared_ptr<gpu::accelerator>& ogl, const void* tag, const struct pixel_format_desc& desc);

	write_frame(write_frame&& other);
	write_frame& operator=(write_frame&& other);

	void swap(write_frame& other);
			
	// draw_frame

	virtual void accept(struct frame_visitor& visitor) override;

	// data_frame
		
	virtual const struct pixel_format_desc& get_pixel_format_desc() const override;

	virtual const boost::iterator_range<const uint8_t*> image_data(int index) const override;
	virtual const audio_buffer& audio_data() const override;

	virtual const boost::iterator_range<uint8_t*> image_data(int index) override;
	virtual audio_buffer& audio_data() override;
	
	virtual double get_frame_rate() const override;

	virtual int width() const override;
	virtual int height() const override;
								
	virtual const void* tag() const override;
			
	// write_frames

	std::vector<spl::shared_ptr<gpu::host_buffer>> get_buffers();
private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};


}}