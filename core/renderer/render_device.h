#pragma once

#include "../producer/frame_producer.h"
#include "../consumer/frame_consumer.h"

#include "layer.h"

namespace caspar{
	
class Monitor;

namespace renderer{
	
class render_device : boost::noncopyable
{	
public:
	render_device(const frame_format_desc& format_desc, unsigned int index, const std::vector<frame_consumer_ptr>& consumers);
	
	void load(int exLayer, const frame_producer_ptr& pProducer, load_option option = load_option::none);	
	void pause(int exLayer);
	void play(int exLayer);
	void stop(int exLayer);
	void clear(int exLayer);
	void clear();

	frame_producer_ptr active(int exLayer) const;
	frame_producer_ptr background(int exLayer) const;

	const frame_format_desc& frame_format_desc() const;		
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<render_device> render_device_ptr;
typedef std::unique_ptr<render_device> render_device_uptr;

}}
