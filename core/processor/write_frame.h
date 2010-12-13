#pragma once

#include "fwd.h"

#include "draw_frame.h"

#include "../../common/exception/exceptions.h"
#include "../../common/gl/pixel_buffer_object.h"

#include "../format/video_format.h"
#include "../format/pixel_format.h"

#include <boost/noncopyable.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <array>
#include <vector>

namespace caspar { namespace core {
		
class write_frame : public draw_frame
{
public:	
	write_frame(const pixel_format_desc& desc);
	write_frame(write_frame&& other);
	
	void swap(write_frame& other);

	write_frame& operator=(write_frame&& other);

	void reset();

	boost::iterator_range<unsigned char*> pixel_data(size_t index = 0);
	const boost::iterator_range<const unsigned char*> pixel_data(size_t index = 0) const;
	
	std::vector<short>& audio_data();
	virtual const std::vector<short>& audio_data() const;
	
private:
		
	void begin_write();
	void end_write();
	void draw(frame_shader& shader);

	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<write_frame> write_frame_impl_ptr;


}}