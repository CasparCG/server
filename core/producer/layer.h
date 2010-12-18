#pragma once

#include "../producer/frame_producer.h"

#include <boost/noncopyable.hpp>

namespace caspar { namespace core {

struct load_option
{
	enum type
	{
		none,
		preview,
		auto_play
	};
};

class layer : boost::noncopyable
{
public:
	layer(size_t index = -1);
	layer(layer&& other);
	layer& operator=(layer&& other);

	void load(const safe_ptr<frame_producer>& producer, load_option::type option = load_option::none);	
	void play();
	void pause();
	void stop();
	void clear();

	bool empty() const;
		
	safe_ptr<frame_producer> foreground() const;
	safe_ptr<frame_producer> background() const;

	safe_ptr<draw_frame> receive();
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<layer> layer_ptr;
typedef std::unique_ptr<layer> layer_uptr;

}}