#pragma once

#include <common/memory/safe_ptr.h>

#include <vector>

struct AVFrame;

namespace caspar {

namespace core {

struct frame_factory;
class write_frame;

}
	
class deinterlacer
{
public:
	deinterlacer(const safe_ptr<core::frame_factory>& factory);

	std::vector<safe_ptr<core::write_frame>> execute(const safe_ptr<AVFrame>& frame);

private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}