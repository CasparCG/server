#pragma once

#include "../consumer/frame_consumer.h"

#include <vector>

#include <boost/noncopyable.hpp>

namespace caspar { namespace core {

class frame_processor_device;
typedef std::shared_ptr<frame_processor_device> frame_processor_device_ptr;

class frame_consumer_device : boost::noncopyable
{
public:
	frame_consumer_device(const frame_processor_device_ptr& frame_processor, const video_format_desc& format_desc, const std::vector<frame_consumer_ptr>& consumers);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<frame_consumer_device> frame_consumer_device_ptr;

}}