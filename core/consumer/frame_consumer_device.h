#pragma once

#include "../consumer/frame_consumer.h"

#include <common/utility/safe_ptr.h>

#include <vector>

#include <boost/noncopyable.hpp>

namespace caspar { namespace core {
	
class draw_frame;
struct video_format_desc;

class frame_consumer_device : boost::noncopyable
{
public:
	explicit frame_consumer_device(const video_format_desc& format_desc, const std::vector<safe_ptr<frame_consumer>>& consumers);
	frame_consumer_device(frame_consumer_device&& other);
	void consume(safe_ptr<const read_frame>&& future_frame); // nothrow
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}