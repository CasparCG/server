#pragma once

#include <common/memory/safe_ptr.h>

#include <boost/noncopyable.hpp>
#include <boost/range.hpp>

#include <tbb/cache_aligned_allocator.h>

#include <stdint.h>

#include "../video_format.h"

namespace caspar { namespace core {
	
typedef std::vector<int32_t, tbb::cache_aligned_allocator<int32_t>> audio_buffer;

struct data_frame
{
	data_frame(){}
	virtual ~data_frame(){}

	virtual const struct pixel_format_desc& get_pixel_format_desc() const = 0;

	virtual const boost::iterator_range<const uint8_t*> image_data(int index = 0) const = 0;
	virtual const audio_buffer& audio_data() const = 0;
	
	virtual const boost::iterator_range<uint8_t*> image_data(int index) = 0;
	virtual audio_buffer& audio_data() = 0;

	virtual double get_frame_rate() const = 0;

	virtual int width() const = 0;
	virtual int height() const = 0;

	virtual const void* tag() const = 0;

	static safe_ptr<data_frame> empty();
private:
	data_frame(const data_frame&);
	data_frame& operator=(const data_frame&);
};

}}