#pragma once

#include "../../common/exception/exceptions.h"

#include "../format/video_format.h"
#include "../format/pixel_format.h"

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <vector>

namespace caspar { namespace core {

class read_frame_impl : boost::noncopyable
{
public:	
	read_frame_impl(size_t width, size_t height);

	const boost::iterator_range<const unsigned char*> pixel_data() const;
	const std::vector<short>& audio_data() const;
	std::vector<short>& audio_data();
	
	void begin_read();
	void end_read();

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<read_frame_impl> read_frame_impl_ptr;

class read_frame
{
public:
	explicit read_frame(read_frame_impl_ptr&& frame);
	
	const boost::iterator_range<const unsigned char*> pixel_data() const;
	const std::vector<short>& audio_data() const;

private:
	read_frame_impl_ptr impl_;
};

}}