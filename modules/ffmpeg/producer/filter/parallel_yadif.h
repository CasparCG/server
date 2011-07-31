#pragma once

struct AVFilterContext;

namespace caspar {
	
int init_parallel_yadif(AVFilterContext* ctx);
void uninit_parallel_yadif(int tag);

}