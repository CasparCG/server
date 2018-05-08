#pragma once

#include <common/memory.h>

#include <core/fwd.h>

#include <string>
#include <vector>

namespace caspar { namespace qtquick {

spl::shared_ptr<core::frame_producer> create_producer(const core::frame_producer_dependencies& dependencies,
                                                      const std::vector<std::wstring>&         params);

}} // namespace caspar::qtquick
