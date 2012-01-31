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

#include "../../frame/data_frame.h"

#include <common/memory/safe_ptr.h>
#include <common/forward.h>
#include <common/exception/exceptions.h>

#include <core/mixer/audio/audio_mixer.h>

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <stdint.h>
#include <memory>
#include <vector>

FORWARD1(boost, template<typename> class unique_future);
FORWARD3(caspar, core, gpu, class host_buffer);

namespace caspar { namespace core {
	
class read_frame sealed : public data_frame
{
public:
	read_frame(const void* tag, boost::unique_future<safe_ptr<gpu::host_buffer>>&& image_data, audio_buffer&& audio_data, const struct video_format_desc& format_desc);
	
	// data_frame

	virtual const struct  pixel_format_desc& get_pixel_format_desc() const override;

	virtual const boost::iterator_range<const uint8_t*> image_data(int index) const override;
	virtual const audio_buffer& audio_data() const override;
	
	virtual double	   get_frame_rate() const override;

	virtual int width() const override;
	virtual int height() const override;

	virtual const boost::iterator_range<uint8_t*> image_data(int index) override
	{
		BOOST_THROW_EXCEPTION(invalid_operation());
	}
	virtual audio_buffer& audio_data() override
	{
		BOOST_THROW_EXCEPTION(invalid_operation());
	}
		
	virtual const void* tag() const override;	
private:
	struct impl;
	std::shared_ptr<impl> impl_;
};

}}