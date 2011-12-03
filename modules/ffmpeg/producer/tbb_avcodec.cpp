/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "../stdafx.h"

#include "tbb_avcodec.h"

#include <common/log/log.h>
#include <common/env.h>
#include <common/utility/assert.h>

#include <tbb/task.h>
#include <tbb/atomic.h>
#include <tbb/parallel_for.h>
#include <tbb/tbb_thread.h>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavformat/avformat.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar {
		
int thread_execute(AVCodecContext* s, int (*func)(AVCodecContext *c2, void *arg2), void* arg, int* ret, int count, int size)
{
	tbb::parallel_for(0, count, 1, [&](int i)
	{
        int r = func(s, (char*)arg + i*size);
        if(ret) 
			ret[i] = r;
    });

	return 0;
}

int thread_execute2(AVCodecContext* s, int (*func)(AVCodecContext* c2, void* arg2, int, int), void* arg, int* ret, int count)
{	
	tbb::atomic<int> counter;   
    counter = 0;   

	CASPAR_VERIFY(tbb::tbb_thread::hardware_concurrency() < 16);
	// Note: this will probably only work when tbb::task_scheduler_init::num_threads() < 16.
    tbb::parallel_for(tbb::blocked_range<int>(0, count, 2), [&](const tbb::blocked_range<int> &r)    
    {   
        int threadnr = counter++;   
        for(int jobnr = r.begin(); jobnr != r.end(); ++jobnr)
        {   
            int r = func(s, arg, jobnr, threadnr);   
            if (ret)   
                ret[jobnr] = r;   
        }
        --counter;
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
	if(!s->thread_opaque)
		return;

	s->thread_opaque = nullptr;
	
	CASPAR_LOG(info) << "Released ffmpeg tbb context.";
}

int tbb_avcodec_open(AVCodecContext* avctx, AVCodec* codec)
{
	CodecID supported_codecs[] = {CODEC_ID_MPEG2VIDEO, CODEC_ID_PRORES, CODEC_ID_FFV1};

	avctx->thread_count = 1;
	// Some codecs don't like to have multiple multithreaded decoding instances. Only enable for those we know work.
	if(std::find(std::begin(supported_codecs), std::end(supported_codecs), codec->id) != std::end(supported_codecs) && 
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