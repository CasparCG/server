#pragma once

struct AVFilterContext;

namespace caspar {
	
std::shared_ptr<void> make_parallel_yadif(AVFilterContext* ctx);

}