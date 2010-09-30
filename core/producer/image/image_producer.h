#pragma once

#include "../frame_producer.h"

#include <string>
#include <vector>

namespace caspar { namespace image {

frame_producer_ptr create_image_producer(const std::vector<std::wstring>& params, const frame_format_desc& format_desc);

}}