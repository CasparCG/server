#pragma once

struct AVFilterContext;

namespace caspar {
	
int make_scalable_yadif(AVFilterContext* ctx);
void release_scalable_yadif(int tag);

}