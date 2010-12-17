#pragma once

#include "fwd.h"

#include "draw_frame.h"

#include "../format/video_format.h"
#include "../format/pixel_format.h"

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <vector>

namespace caspar { namespace core {
		
class write_frame : public draw_frame
{
	friend class frame_processor_device;
public:	
	write_frame(const pixel_format_desc& desc);
	write_frame(write_frame&& other);
	
	void swap(write_frame& other);

	write_frame& operator=(write_frame&& other);
	
	boost::iterator_range<unsigned char*> pixel_data(size_t index = 0);
	const boost::iterator_range<const unsigned char*> pixel_data(size_t index = 0) const;
	
	std::vector<short>& audio_data();
	
private:
		
	void map();
	void unmap();
	void process_image(image_processor& processor);
	void process_audio(audio_processor& processor);

	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<write_frame> write_frame_impl_ptr;


}}