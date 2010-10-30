#pragma once

#include "../frame_producer.h"

#include <string>
#include <vector>

namespace caspar { namespace core { namespace image {
	
frame_producer_ptr create_image_scroll_producer(const std::vector<std::wstring>& params, const frame_format_desc& format_desc);

}}}