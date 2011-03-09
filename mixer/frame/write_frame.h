#pragma once

#include "../fwd.h"

#include "basic_frame.h"

#include "../gpu/host_buffer.h"

#include <core/video_format.h>

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <vector>

namespace caspar { namespace core {
	
struct pixel_format_desc;

class write_frame : public basic_frame, boost::noncopyable
{
public:	
	explicit write_frame(const pixel_format_desc& desc, std::vector<safe_ptr<host_buffer>> buffers);
	write_frame(write_frame&& other);
	write_frame& operator=(write_frame&& other);
	
	void swap(write_frame& other);
		
	boost::iterator_range<unsigned char*> image_data(size_t index = 0);	
	std::vector<short>& audio_data();
	
	boost::iterator_range<const unsigned char*> image_data(size_t index = 0) const;
	const std::vector<short>& audio_data() const;

	virtual void accept(frame_visitor& visitor);

	void tag(int tag);
	int tag() const;

	const pixel_format_desc& get_pixel_format_desc() const;
	std::vector<safe_ptr<host_buffer>>& buffers();
	
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<write_frame> write_frame_impl_ptr;


}}