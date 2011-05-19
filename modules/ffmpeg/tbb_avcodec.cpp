// Author Robert Nagy

#include "stdafx.h"

#include "tbb_avcodec.h"

#include <common/log/log.h>

#include <tbb/task.h>
#include <tbb/atomic.h>

extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavformat/avformat.h>
}

namespace caspar {

struct thread_context{};

int task_execute(AVCodecContext* s, const std::function<int(void* arg, int arg_size, int jobnr, int threadnr)>& func, void* arg, int* ret, int count, int size)
{	
	// jobnr global for all threads? Doesn't order matter?
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

			int r = func(arg, size, jobnr, threadnr);
			if (ret)
				ret[jobnr] = r;
		}
	});
	
    return 0;
}
	
int thread_execute(AVCodecContext* s, int (*func)(AVCodecContext *c2, void *arg2), void* arg, int* ret, int count, int size)
{
	return task_execute(s, [&](void* arg, int arg_size, int jobnr, int threadnr) -> int
	{
		return func(s, reinterpret_cast<uint8_t*>(arg) + jobnr*size);
	}, arg, ret, count, size);
}

int thread_execute2(AVCodecContext* s, int (*func)(AVCodecContext* c2, void* arg2, int, int), void* arg, int* ret, int count)
{
	return task_execute(s, [&](void* arg, int arg_size, int jobnr, int threadnr) -> int
	{
		return func(s, arg, jobnr, threadnr);
	}, arg, ret, count, 0);
}

int thread_init(AVCodecContext *s)
{
	// Only makes sense for slicing since decode is already called through task scheduler.
    if(!(s->thread_type & FF_THREAD_SLICE))	       
       return 0;    

	static const size_t MAX_THREADS = 16; // See mpegvideo.h

    s->active_thread_type = FF_THREAD_SLICE;
	s->thread_opaque	  = malloc(sizeof(thread_context));	 
    s->execute			  = thread_execute;
    s->execute2			  = thread_execute2;
    s->thread_count		  = MAX_THREADS; // We are using a taskscheduler, so use as many "threads/tasks" as possible. 

	CASPAR_LOG(info) << "Initialized ffmpeg tbb context.";

    return 0;
}

void thread_free(AVCodecContext* s)
{
	if(!s->thread_opaque)
		return;
	
	free(s->thread_opaque);	
	s->thread_opaque = nullptr;

	CASPAR_LOG(info) << "Released ffmpeg tbb context.";
}

int tbb_avcodec_open(AVCodecContext* avctx, AVCodec* codec)
{
	thread_init(avctx);
	// ff_thread_init will not be executed since thread_opaque != nullptr.
	return avcodec_open(avctx, codec); 
}

int tbb_avcodec_close(AVCodecContext* avctx)
{
	thread_free(avctx);
	// ff_thread_free will not be executed since thread_opaque == nullptr.
	return avcodec_close(avctx); 
}

}