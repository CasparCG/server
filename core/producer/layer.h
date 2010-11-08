#pragma once

#include "../producer/frame_producer.h"

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

class layer
{
	layer(const layer& other);
	layer& operator=(const layer& other);
public:
	layer();
	layer(layer&& other);
	layer& operator=(layer&& other);

	void load(const frame_producer_ptr& producer, load_option::type option = load_option::none);	
	void play();
	void pause();
	void stop();
	void clear();
		
	frame_producer_ptr active() const;
	frame_producer_ptr background() const;

	frame_ptr render_frame();
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<layer> layer_ptr;
typedef std::unique_ptr<layer> layer_uptr;

}}