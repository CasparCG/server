#pragma once

#include "../../common/exception/exceptions.h"
#include "../../common/gl/pixel_buffer_object.h"

#include "../format/video_format.h"
#include "../format/pixel_format.h"

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <vector>

namespace caspar { namespace core {
	
class read_frame
{
public:
	explicit read_frame(common::gl::pbo_ptr&& frame, std::vector<short>&& audio_data);
	
	const boost::iterator_range<const unsigned char*> pixel_data() const;
	const std::vector<short>& audio_data() const;

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}