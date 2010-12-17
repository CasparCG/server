#pragma once

#include "../../common/exception/exceptions.h"
#include "../../common/gl/pixel_buffer_object.h"

#include "../format/video_format.h"
#include "../format/pixel_format.h"

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <vector>

#include "../../common/utility/safe_ptr.h"

namespace caspar { namespace core {
	
class read_frame
{
	friend class image_processor;
public:
	explicit read_frame(size_t width, size_t height);
	
	const boost::iterator_range<const unsigned char*> pixel_data() const;

	const std::vector<short>& audio_data() const;
	void audio_data(const std::vector<short>& audio_data);
	
private:
	void unmap();
	void map();

	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}