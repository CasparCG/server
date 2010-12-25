#pragma once

#include "../consumer/frame_consumer.h"

#include <common/utility/safe_ptr.h>

#include <vector>

#include <boost/noncopyable.hpp>

#include <tbb/concurrent_queue.h>

#include <boost/thread/future.hpp>

namespace caspar { namespace core {
	
class frame_processor_device;
class draw_frame;

class frame_consumer_device : boost::noncopyable
{
public:
	frame_consumer_device(frame_consumer_device&& other);
	frame_consumer_device(const video_format_desc& format_desc, const std::vector<safe_ptr<frame_consumer>>& consumers);
	void consume(safe_ptr<const read_frame>&& future_frame);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}