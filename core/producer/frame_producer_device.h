#pragma once

#include "../processor/frame_processor_device.h"
#include "../consumer/frame_consumer.h"
#include "../format/video_format.h"

#include "layer.h"

#include <boost/thread.hpp>

#include <functional>

namespace caspar { namespace core {
		
class frame_producer_device : boost::noncopyable
{	
public:
	frame_producer_device(frame_producer_device&& other);
	frame_producer_device(const safe_ptr<frame_processor_device>& frame_processor);
	
	void load	(int render_layer, const safe_ptr<frame_producer>& producer, load_option::type option = load_option::none);	
	void pause	(int render_layer);
	void play	(int render_layer);
	void stop	(int render_layer);
	void clear	(int render_layer);
	void clear	();
	
	boost::unique_future<safe_ptr<frame_producer>> foreground(int render_layer) const;
	boost::unique_future<safe_ptr<frame_producer>> background(int render_layer) const;
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<frame_producer_device> frame_producer_device_ptr;

}}
