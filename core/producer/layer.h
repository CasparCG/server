#pragma once

#include <common/memory/safe_ptr.h>

#include <boost/noncopyable.hpp>

#include <memory>
#include <numeric>

namespace caspar { namespace core {

class frame_producer;
class basic_frame;

class layer //: boost::noncopyable
{
public:
	layer(); // nothrow
	layer(layer&& other); // nothrow
	layer& operator=(layer&& other); // nothrow
	layer(const layer&);
	layer& operator=(const layer&);

	void swap(layer& other); // nothrow 
		
	void load(const safe_ptr<frame_producer>& producer, bool preview); // nothrow
	void play(); // nothrow
	void pause(); // nothrow
	void stop(); // nothrow

	safe_ptr<frame_producer> foreground() const; // nothrow
	safe_ptr<frame_producer> background() const; // nothrow

	safe_ptr<basic_frame> receive(); // nothrow
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}