// Author Robert Nagy

#include "stdafx.h"

#include "tbb_avcodec.h"

#include <common/log/log.h>
#include <common/env.h>

#include <tbb/task.h>
#include <tbb/atomic.h>

#include <regex>
#include <boost/algorithm/string.hpp>

extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavformat/avformat.h>
}

namespace caspar {
		
int thread_execute(AVCodecContext* s, int (*func)(AVCodecContext *c2, void *arg2), void* arg, int* ret, int count, int size)
{
	tbb::parallel_for(tbb::blocked_range<size_t>(0, count), [&](const tbb::blocked_range<size_t>& r)
	{
		for(size_t n = r.begin(); n != r.end(); ++n)		
		{
			int r = func(s, reinterpret_cast<uint8_t*>(arg) + n*size);
			if(ret)
				ret[n] = r;
		}
	});

	return 0;
}

int thread_execute2(AVCodecContext* s, int (*func)(AVCodecContext* c2, void* arg2, int, int), void* arg, int* ret, int count)
{	
    tbb::atomic<int> counter;
	counter = 0;
		
	// Execute s->thread_count number of tasks in parallel.
	tbb::parallel_for(0, s->thread_count, 1, [&](int threadnr) 
	{
		while(true)
		{
			int jobnr = counter++;
			if(jobnr >= count)
				break;

			int r = func(s, arg, jobnr, threadnr);
			if (ret)
				ret[jobnr] = r;
		}
	});

	return 0;
}

void thread_init(AVCodecContext* s)
{
	static const size_t MAX_THREADS = 16; // See mpegvideo.h
	static int dummy_opaque;

    s->active_thread_type = FF_THREAD_SLICE;
	s->thread_opaque	  = &dummy_opaque; 
    s->execute			  = thread_execute;
    s->execute2			  = thread_execute2;
    s->thread_count		  = MAX_THREADS; // We are using a task-scheduler, so use as many "threads/tasks" as possible. 

	CASPAR_LOG(info) << "Initialized ffmpeg tbb context.";
}

void thread_free(AVCodecContext* s)
{
	s->thread_opaque = nullptr;

	CASPAR_LOG(info) << "Released ffmpeg tbb context.";
}

int tbb_avcodec_open(AVCodecContext* avctx, AVCodec* codec)
{
	avctx->thread_count = 1;
	// Some codecs don't like to have multiple multithreaded decoding instances. Only enable for those we know work.
	if(codec->id == CODEC_ID_MPEG2VIDEO && 
	  (codec->capabilities & CODEC_CAP_SLICE_THREADS) && 
	  (avctx->thread_type & FF_THREAD_SLICE))
	{
		thread_init(avctx);
	}	
	// ff_thread_init will not be executed since thread_opaque != nullptr || thread_count == 1.
	return avcodec_open(avctx, codec); 
}

int tbb_avcodec_close(AVCodecContext* avctx)
{
	thread_free(avctx);
	// ff_thread_free will not be executed since thread_opaque == nullptr.
	return avcodec_close(avctx); 
}

}