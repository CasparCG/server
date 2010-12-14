#pragma once

#include "../consumer/frame_consumer.h"

#include <vector>

#include <boost/noncopyable.hpp>

namespace caspar { namespace core {

class frame_processor_device;

class frame_consumer_device : boost::noncopyable
{
public:
	frame_consumer_device(frame_consumer_device&& other);
	frame_consumer_device(const safe_ptr<frame_processor_device>& frame_processor, const video_format_desc& format_desc, const std::vector<safe_ptr<frame_consumer>>& consumers);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}