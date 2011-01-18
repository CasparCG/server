#pragma once

#include "frame_producer.h"

#include <common/memory/safe_ptr.h>

#include <boost/noncopyable.hpp>
#include <boost/thread/future.hpp>

#include <functional>

namespace caspar { namespace core {

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \class	frame_producer_device
///
/// \brief
/// 		
///                |**********| <-   empty frame   <- |***********| <-   frame format  <- |**********|
///   PROTOCOL ->  | PRODUCER |                       |   MIXER	  |                       | CONSUMER |  -> DISPLAY DEVICE
///                |**********| -> rendered frames -> |***********| -> formatted frame -> |**********|
///   
////////////////////////////////////////////////////////////////////////////////////////////////////

class frame_producer_device : boost::noncopyable
{
public:
	typedef std::function<void(const std::vector<safe_ptr<draw_frame>>&)> output_func;

	explicit frame_producer_device(const video_format_desc& format_desc, const safe_ptr<frame_factory>& factory, const output_func& output);
	frame_producer_device(frame_producer_device&& other);
		
	void load(int index, const safe_ptr<frame_producer>& producer, bool play_on_load = false);
	void preview(int index, const safe_ptr<frame_producer>& producer);
	void pause(int index);
	void play(int index);
	void stop(int index);
	void clear(int index);
	void clear();	
	boost::unique_future<safe_ptr<frame_producer>> foreground(int index) const;
	boost::unique_future<safe_ptr<frame_producer>> background(int index) const;
	const video_format_desc& get_video_format_desc() const;

private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}