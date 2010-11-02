#pragma once

#include "../producer/frame_producer.h"

namespace caspar { namespace core { namespace renderer {

enum load_option
{
	none,
	preview,
	auto_play
};
			
class layer
{
	layer(const layer& other);
	layer& operator=(const layer& other);
public:
	layer();
	layer(layer&& other);
	layer& operator=(layer&& other);

	void load(const frame_producer_ptr& producer, load_option option = load_option::none);	
	void play();
	void pause();
	void stop();
	void clear();
		
	frame_producer_ptr active() const;
	frame_producer_ptr background() const;

	gpu_frame_ptr get_frame();
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<layer> layer_ptr;
typedef std::unique_ptr<layer> layer_uptr;

}}}