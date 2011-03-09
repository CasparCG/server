#pragma once

#include <common/memory/safe_ptr.h>
#include <common/utility/printer.h>

#include <boost/noncopyable.hpp>

#include <memory>
#include <numeric>

namespace caspar { namespace core {

class frame_producer;
class basic_frame;

class layer : boost::noncopyable
{
public:
	layer(int index = std::numeric_limits<int>::min(), const printer& parent_printer = nullptr); // nothrow
	layer(layer&& other); // nothrow
	layer& operator=(layer&& other); // nothrow

	void swap(layer& other); // nothrow 
		
	void load(const safe_ptr<frame_producer>& producer, bool play_on_load, bool preview); // nothrow
	void play(); // nothrow
	void pause(); // nothrow
	void stop(); // nothrow

	safe_ptr<frame_producer> foreground() const; // nothrow
	safe_ptr<frame_producer> background() const; // nothrow

	safe_ptr<basic_frame> receive(); // nothrow

	std::wstring print() const;
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}