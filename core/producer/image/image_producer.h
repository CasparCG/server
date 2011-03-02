#pragma once

#include "../frame_producer.h"

#include <string>
#include <vector>

namespace caspar { namespace core { namespace image {

safe_ptr<frame_producer> create_image_producer(const std::vector<std::wstring>& params);

std::wstring get_image_version();

}}}