#pragma once

#include "../frame_producer.h"

#include <string>
#include <vector>

namespace caspar {
	
safe_ptr<frame_producer> create_image_scroll_producer(const std::vector<std::wstring>& params);

}