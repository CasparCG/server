#include "../../StdAfx.h"

#include "parallel_yadif.h"

extern "C" 
{
	#include <libavfilter/avfilter.h>
}

#include <boost/thread/once.hpp>

typedef struct {
    int mode;
    int parity;
    int frame_pending;
    int auto_enable;
    AVFilterBufferRef *cur;
    AVFilterBufferRef *next;
    AVFilterBufferRef *prev;
    AVFilterBufferRef *out;
    void (*filter_line)(uint8_t *dst,
                        uint8_t *prev, uint8_t *cur, uint8_t *next,
                        int w, int prefs, int mrefs, int parity, int mode);
    //const AVPixFmtDescriptor *csp;
} YADIFContext;

struct parallel_yadif_context
{
	struct arg
	{
		uint8_t *dst;
		uint8_t *prev;
		uint8_t *cur; 
		uint8_t *next; 
		int w; 
		int prefs; 
		int mrefs;
		int parity;
		int mode;
	};

	arg	args[64];
	int	index;

	parallel_yadif_context() : index(0){}
};

void (*org_yadif_filter_line)(uint8_t *dst, uint8_t *prev, uint8_t *cur, uint8_t *next, int w, int prefs, int mrefs, int parity, int mode) = 0;

void parallel_yadif_filter_line(parallel_yadif_context& ctx, uint8_t *dst, uint8_t *prev, uint8_t *cur, uint8_t *next, int w, int prefs, int mrefs, int parity, int mode)
{
	parallel_yadif_context::arg arg = {dst, prev, cur, next, w, prefs, mrefs, parity, mode};
	ctx.args[ctx.index++] = arg;
	
	if(ctx.index == 64)
	{		
		tbb::parallel_for(tbb::blocked_range<size_t>(0, ctx.index), [=](const tbb::blocked_range<size_t>& r)
		{
			for(auto n = r.begin(); n != r.end(); ++n)
				org_yadif_filter_line(ctx.args[n].dst, ctx.args[n].prev, ctx.args[n].cur, ctx.args[n].next, ctx.args[n].w, ctx.args[n].prefs, ctx.args[n].mrefs, ctx.args[n].parity, ctx.args[n].mode);
		});
		ctx.index = 0;
	}
}

#define RENAME(a) f ## a

#define ff(x) \
void RENAME(x)(uint8_t *dst, uint8_t *prev, uint8_t *cur, uint8_t *next, int w, int prefs, int mrefs, int parity, int mode) \
{\
	static parallel_yadif_context ctx;\
	parallel_yadif_filter_line(ctx, dst, prev, cur, next, w, prefs, mrefs, parity, mode);\
}

ff(0); ff(1); ff(2); ff(3); ff(4); ff(5); ff(6); ff(7); ff(8); ff(9); ff(10); ff(11); ff(12); ff(13); ff(14); ff(15); ff(16); ff(17);

void (*fs[])(uint8_t *dst, uint8_t *prev, uint8_t *cur, uint8_t *next, int w, int prefs, int mrefs, int parity, int mode) = 

{f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17};

namespace caspar {
	
tbb::concurrent_bounded_queue<int> tags;

void init_pool()
{
	for(int n = 0; n < 18; ++n)
		tags.push(n);
}

int init(AVFilterContext* ctx)
{
	static boost::once_flag flag = BOOST_ONCE_INIT;
	boost::call_once(&init_pool, flag);

	YADIFContext* yadif = (YADIFContext*)ctx->priv;
	org_yadif_filter_line = yadif->filter_line; // Data race is not a problem.

	int tag;
	if(!tags.try_pop(tag))
	{
		CASPAR_LOG(warning) << "Not enough scalable-yadif instances. Running non-scalable";
		return -1;
	}

	yadif->filter_line = fs[tag];
	return tag;
}

void uninit(int tag)
{
	if(tag != -1)
		tags.push(tag);
}

}