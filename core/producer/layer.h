#pragma once

#include <common/memory/safe_ptr.h>

#include <boost/noncopyable.hpp>

namespace caspar { namespace core {

class frame_producer;
class draw_frame;

class layer : boost::noncopyable
{
public:
	layer(int index = -1); // nothrow
	layer(layer&& other); // nothrow
	layer& operator=(layer&& other); // nothrow
	
	void load(const safe_ptr<frame_producer>& producer, bool autoplay = false); // nothrow
	void preview(const safe_ptr<frame_producer>& producer); // nothrow
	void play(); // nothrow
	void pause(); // nothrow
	void stop(); // nothrow
	void clear(); // nothrow

	bool empty() const; // nothrow
		
	safe_ptr<frame_producer> foreground() const; // nothrow
	safe_ptr<frame_producer> background() const; // nothrow

	safe_ptr<draw_frame> receive(); // nothrow
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}