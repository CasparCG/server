#pragma once

#include <core/producer/frame_producer.h>

#include <string>
#include <vector>

namespace caspar { 

safe_ptr<core::frame_producer> create_image_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::vector<std::wstring>& params);

}