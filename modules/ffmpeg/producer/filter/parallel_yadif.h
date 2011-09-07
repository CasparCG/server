#pragma once

#include <memory>

struct AVFilterContext;

namespace caspar { namespace ffmpeg {
	
std::shared_ptr<void> make_parallel_yadif(AVFilterContext* ctx);

}}