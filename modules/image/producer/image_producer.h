#pragma once

#include <core/producer/frame_producer.h>

#include <string>
#include <vector>

namespace caspar { 

safe_ptr<core::frame_producer> create_image_producer(const std::vector<std::wstring>& params);

}