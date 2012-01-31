#include "../StdAfx.h"

#include "data_frame.h"

#include "pixel_format.h"

namespace caspar { namespace core {

data_frame::~data_frame()
{
}

safe_ptr<data_frame> data_frame::empty()
{
	struct empty_frame : public data_frame
	{
		virtual const struct pixel_format_desc& get_pixel_format_desc() const override
		{
			static pixel_format_desc invalid(pixel_format::invalid);
			return invalid;
		}
		virtual const boost::iterator_range<const uint8_t*> image_data() const override
		{
			return boost::iterator_range<const uint8_t*>();
		}
		virtual const boost::iterator_range<const int32_t*> audio_data() const override
		{
			return boost::iterator_range<const int32_t*>();
		}
		const boost::iterator_range<uint8_t*> image_data() override
		{
			return boost::iterator_range<uint8_t*>();
		}
		const boost::iterator_range<int32_t*> audio_data() override
		{
			return boost::iterator_range<int32_t*>();
		}
		virtual double get_frame_rate() const override
		{
			return 0.0;
		}
		virtual int width() const override
		{
			return 0;
		}
		virtual int height() const override
		{
			return 0;
		}
	};

	static safe_ptr<empty_frame> empty;
	return empty;
}

}}