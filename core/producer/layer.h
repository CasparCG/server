#pragma once

#include <common/memory/safe_ptr.h>

#include <boost/noncopyable.hpp>

#include <tbb/atomic.h>

namespace caspar { namespace core {

class frame_producer;
class draw_frame;

class layer : boost::noncopyable
{
public:
	layer(); // nothrow
	layer(layer&& other); // nothrow
	~layer(); // nothrow
	layer& operator=(layer&& other); // nothrow

	//NOTE: swap is thread-safe on "other", NOT on "this".
	void swap(layer& other); // nothrow 
		
	void load(const safe_ptr<frame_producer>& producer, bool play_on_load = false); // nothrow
	void preview(const safe_ptr<frame_producer>& producer); // nothrow
	void play(); // nothrow
	void pause(); // nothrow
	void stop(); // nothrow
	void clear(); // nothrow

	safe_ptr<frame_producer> foreground() const; // nothrow
	safe_ptr<frame_producer> background() const; // nothrow

	safe_ptr<draw_frame> receive(); // nothrow
private:
	struct implementation;
	tbb::atomic<implementation*> impl_;
};

}}