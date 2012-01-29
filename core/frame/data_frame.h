#pragma once

#include <boost/noncopyable.hpp>
#include <boost/range.hpp>

#include <stdint.h>

#include "pixel_format.h"
#include "../video_format.h"

namespace caspar { namespace core {

struct data_frame : boost::noncopyable
{
	virtual ~data_frame()
	{
	}

	virtual const struct  pixel_format_desc& get_pixel_format_desc() const = 0;

	virtual const boost::iterator_range<const uint8_t*> image_data() const = 0;
	virtual const boost::iterator_range<const int32_t*> audio_data() const = 0;
	
	virtual const boost::iterator_range<uint8_t*> image_data() = 0;
	virtual const boost::iterator_range<int32_t*> audio_data() = 0;

	virtual double get_frame_rate() const = 0;
	virtual field_mode get_field_mode() const = 0;

	virtual int width() const = 0;
	virtual int height() const = 0;

	static safe_ptr<data_frame> empty()
	{
		struct empty_frame : public data_frame
		{
			virtual const struct  video_format_desc& get_video_format_desc() const
			{
				static video_format_desc invalid;
				return invalid;
			}
			virtual const struct  pixel_format_desc& get_pixel_format_desc() const 
			{
				static pixel_format_desc invalid;
				return invalid;
			}
			virtual const boost::iterator_range<const uint8_t*> image_data() const 
			{
				return boost::iterator_range<const uint8_t*>();
			}
			virtual const boost::iterator_range<const int32_t*> audio_data() const 
			{
				return boost::iterator_range<const int32_t*>();
			}
			const boost::iterator_range<uint8_t*> image_data()
			{
				return boost::iterator_range<uint8_t*>();
			}
			const boost::iterator_range<int32_t*> audio_data()
			{
				return boost::iterator_range<int32_t*>();
			}
			virtual double get_frame_rate() const
			{
				return 0.0;
			}
			virtual field_mode get_field_mode() const
			{
				return field_mode::empty;
			}
			virtual int width() const
			{
				return 0;
			}
			virtual int height() const
			{
				return 0;
			}
		};

		static safe_ptr<empty_frame> empty;
		return empty;
	}
};

}}