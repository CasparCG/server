#include "../StdAfx.h"

#include "data_frame.h"

#include "pixel_format.h"

namespace caspar { namespace core {
	
spl::shared_ptr<data_frame> data_frame::empty()
{
	struct empty_frame : public data_frame
	{
		empty_frame(){}
		virtual const struct pixel_format_desc& get_pixel_format_desc() const override
		{
			static pixel_format_desc invalid(pixel_format::invalid);
			return invalid;
		}
		virtual const boost::iterator_range<const uint8_t*> image_data(int) const override
		{
			return boost::iterator_range<const uint8_t*>();
		}
		virtual audio_buffer& audio_data() const override
		{
			static audio_buffer buffer;
			return buffer;
		}
		const boost::iterator_range<uint8_t*> image_data(int) override
		{
			return boost::iterator_range<uint8_t*>();
		}
		audio_buffer& audio_data() override
		{
			static audio_buffer buffer;
			return buffer;
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
		virtual const void* tag() const override
		{
			return 0;
		}
	};

	static spl::shared_ptr<empty_frame> empty;
	return empty;
}

}}