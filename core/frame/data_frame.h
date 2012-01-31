#pragma once

#include <common/memory/safe_ptr.h>

#include <boost/noncopyable.hpp>
#include <boost/range.hpp>

#include <stdint.h>

#include "../video_format.h"

namespace caspar { namespace core {

struct pixel_format_desc;

struct data_frame : boost::noncopyable
{
	virtual ~data_frame();

	virtual const struct pixel_format_desc& get_pixel_format_desc() const = 0;

	virtual const boost::iterator_range<const uint8_t*> image_data() const = 0;
	virtual const boost::iterator_range<const int32_t*> audio_data() const = 0;
	
	virtual const boost::iterator_range<uint8_t*> image_data() = 0;
	virtual const boost::iterator_range<int32_t*> audio_data() = 0;

	virtual double get_frame_rate() const = 0;

	virtual int width() const = 0;
	virtual int height() const = 0;

	static safe_ptr<data_frame> empty();
};

}}