#pragma once

#include "../frame/frame_fwd.h"
#include "../consumer/frame_consumer.h"

#include <vector>

namespace caspar { namespace core { namespace renderer {

class display_device
{
public:
	display_device(const frame_format_desc& format_desc, const std::vector<frame_consumer_ptr>& consumers);
	void display(const gpu_frame_ptr& frame);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<display_device> display_device_ptr;

}}}