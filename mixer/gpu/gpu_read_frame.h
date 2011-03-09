#pragma once

#include "../gpu/host_buffer.h"	

#include <core/consumer/frame/read_frame.h>

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <vector>

#include <common/memory/safe_ptr.h>

namespace caspar { namespace core {
	
class gpu_read_frame : public core::read_frame
{
public:
	gpu_read_frame(safe_ptr<const host_buffer>&& image_data, std::vector<short>&& audio_data);

	const boost::iterator_range<const unsigned char*> image_data() const;
	const boost::iterator_range<const short*> audio_data() const;
	
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}