#include "../../StdAfx.h"

#include "scalable_yadif.h"

extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavutil/avutil.h>
	#include <libavutil/imgutils.h>
	#include <libavfilter/avfilter.h>
	#include <libavfilter/avcodec.h>
	#include <libavfilter/avfiltergraph.h>
	#include <libavfilter/vsink_buffer.h>
	#include <libavfilter/vsrc_buffer.h>
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
    const AVPixFmtDescriptor *csp;
} YADIFContext;

struct scalable_yadif_context
{
	std::vector<std::function<void()>> calls;
	int end_prefs;

	scalable_yadif_context() : end_prefs(std::numeric_limits<int>::max()){}
};

void (*org_yadif_filter_line)(uint8_t *dst, uint8_t *prev, uint8_t *cur, uint8_t *next, int w, int prefs, int mrefs, int parity, int mode) = 0;

void scalable_yadif_filter_line(scalable_yadif_context& ctx, uint8_t *dst, uint8_t *prev, uint8_t *cur, uint8_t *next, int w, int prefs, int mrefs, int parity, int mode)
{
	if(ctx.end_prefs == std::numeric_limits<int>::max())
		ctx.end_prefs = -prefs;

	ctx.calls.push_back([=]
	{
		org_yadif_filter_line(dst, prev, cur, next, w, prefs, mrefs, parity, mode);
	});

	if(prefs == ctx.end_prefs)
	{		
		tbb::parallel_for(tbb::blocked_range<size_t>(0, ctx.calls.size()), [=](const tbb::blocked_range<size_t>& r)
		{
			for(auto n = r.begin(); n != r.end(); ++n)
				ctx.calls[n]();
		});
		ctx.calls     = std::vector<std::function<void()>>();
		ctx.end_prefs = std::numeric_limits<int>::max();
	}
}

#define RENAME(a) f ## a

#define ff(x) \
void RENAME(x)(uint8_t *dst, uint8_t *prev, uint8_t *cur, uint8_t *next, int w, int prefs, int mrefs, int parity, int mode) \
{\
	static scalable_yadif_context ctx;\
	scalable_yadif_filter_line(ctx, dst, prev, cur, next, w, prefs, mrefs, parity, mode);\
}

ff(0); ff(1); ff(2); ff(3); ff(4); ff(5); ff(6); ff(7); ff(8); ff(9); ff(10); ff(11); ff(12); ff(13); ff(14); ff(15); ff(16); ff(17);

void (*fs[])(uint8_t *dst, uint8_t *prev, uint8_t *cur, uint8_t *next, int w, int prefs, int mrefs, int parity, int mode) = 

{f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17};

namespace caspar {
	
tbb::concurrent_bounded_queue<int> tags;

void init()
{
	for(int n = 0; n < 18; ++n)
		tags.push(n);
}

int make_scalable_yadif(AVFilterContext* ctx)
{
	static boost::once_flag flag = BOOST_ONCE_INIT;
	boost::call_once(&init, flag);

	YADIFContext* yadif = (YADIFContext*)ctx->priv;
	org_yadif_filter_line = yadif->filter_line; // Data race is not a problem.

	int tag;
	if(!tags.try_pop(tag))
	{
		CASPAR_LOG(warning) << "Not enough scalable-yadif instances. Running non-scalable";
		return -1;
	}

	yadif->filter_line = fs[tag]; // hmm, will only work for one concurrent instance...
	return tag;
}

void release_scalable_yadif(int tag)
{
	if(tag != -1)
		tags.push(tag);
}

}