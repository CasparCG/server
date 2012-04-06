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

#include <common/assert.h>
#include <common/except.h>
#include <common/log.h>
#include <common/env.h>

#include <tbb/atomic.h>
#include <tbb/parallel_for.h>
#include <tbb/tbb_thread.h>

#include <boost/foreach.hpp>

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
		
static const int MAX_THREADS = 16; // See mpegvideo.h

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
	// TODO: Micro-optimize...

	std::array<std::vector<int>, 16> jobs;
	
	for(int n = 0; n < count; ++n)	
		jobs[(n*MAX_THREADS) / count].push_back(n);	
	
	tbb::parallel_for(0, MAX_THREADS, [&](int n)    
    {   
		BOOST_FOREACH(auto k, jobs[n])
		{
			int r = func(s, arg, k, n);
			if(ret) 
				ret[k]= r;
		}
    });   

	return 0; 
}

void thread_init(AVCodecContext* s)
{
	static int dummy_opaque;

    s->active_thread_type = FF_THREAD_SLICE;
	s->thread_opaque	  = &dummy_opaque; 
    s->execute			  = thread_execute;
    s->execute2			  = thread_execute2;
    s->thread_count		  = MAX_THREADS; // We are using a task-scheduler, so use as many "threads/tasks" as possible. 
}

void thread_free(AVCodecContext* s)
{
	if(!s->thread_opaque)
		return;

	s->thread_opaque = nullptr;
}

int tbb_avcodec_open(AVCodecContext* avctx, AVCodec* codec)
{
	if(codec->capabilities & CODEC_CAP_EXPERIMENTAL)
		CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info("Experimental codecs are not supported."));

	avctx->thread_count = 1;

	if(codec->capabilities & CODEC_CAP_SLICE_THREADS) 	
		thread_init(avctx);
	
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