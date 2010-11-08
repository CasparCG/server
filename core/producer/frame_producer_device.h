#pragma once

#include "../processor/frame_processor_device.h"
#include "../consumer/frame_consumer.h"
#include "../video/video_format.h"

#include "layer.h"

#include <functional>

namespace caspar { namespace core {
		
class frame_producer_device : boost::noncopyable
{	
public:
	frame_producer_device(const frame_processor_device_ptr& frame_processor);
	
	void load(int render_layer, const frame_producer_ptr& producer, load_option::type option = load_option::none);	
	void pause(int render_layer);
	void play(int render_layer);
	void stop(int render_layer);
	void clear(int render_layer);
	void clear();
	
	frame_producer_ptr active(int render_layer) const;
	frame_producer_ptr background(int render_layer) const;
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<frame_producer_device> frame_producer_device_ptr;

}}
