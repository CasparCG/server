#pragma once

#include "fwd.h"

#include "host_buffer.h"

#include "draw_frame.h"

#include "../format/video_format.h"
#include "../format/pixel_format.h"

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <vector>

namespace caspar { namespace core {
		
class write_frame : public draw_frame, boost::noncopyable
{
public:	
	explicit write_frame(const pixel_format_desc& desc, std::vector<safe_ptr<host_buffer>> buffers);
	write_frame(write_frame&& other);
	write_frame& operator=(write_frame&& other);
	
	void swap(write_frame& other);
		
	boost::iterator_range<unsigned char*> image_data(size_t index = 0);	
	std::vector<short>& audio_data();

	virtual void process_image(image_processor& processor);
	virtual void process_audio(audio_processor& processor);
	
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<write_frame> write_frame_impl_ptr;


}}