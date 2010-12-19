#pragma once

#include "producer/frame_producer_device.h"
#include "consumer/frame_consumer_device.h"
#include "processor/frame_processor_device.h"

#include <boost/noncopyable.hpp>

#include <memory>

namespace caspar { namespace core {

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \class	channel
///
/// \brief
/// 		
///                |**********| <-   empty frame   <- |***********| <-   frame format  <- |**********|
///   PROTOCOL ->  | PRODUCER |                       | PROCESSOR |                       | CONSUMER |  -> DISPLAY DEVICE
///                |**********| -> rendered frames -> |***********| -> formatted frame -> |**********|
///   
////////////////////////////////////////////////////////////////////////////////////////////////////
class channel : boost::noncopyable
{
public:
	channel(channel&& other);
	channel(const safe_ptr<frame_producer_device>& producer_device, const safe_ptr<frame_processor_device>& processor_device, const safe_ptr<frame_consumer_device>& consumer_device);
	
	void load(int render_layer, const safe_ptr<frame_producer>& producer, bool autoplay = false);
	void preview(int render_layer, const safe_ptr<frame_producer>& producer);
	void pause(int render_layer);
	void play(int render_layer);
	void stop(int render_layer);
	void clear(int render_layer);
	void clear();	
	boost::unique_future<safe_ptr<frame_producer>> foreground(int render_layer) const;
	boost::unique_future<safe_ptr<frame_producer>> background(int render_layer) const;
	const video_format_desc& get_video_format_desc() const;
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}