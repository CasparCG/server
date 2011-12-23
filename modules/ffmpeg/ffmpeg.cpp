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

#include "StdAfx.h"

#include "consumer/ffmpeg_consumer.h"
#include "producer/ffmpeg_producer.h"

#include <common/log/log.h>

#include <core/consumer/frame_consumer.h>
#include <core/producer/frame_producer.h>

#include <tbb/recursive_mutex.h>

#if defined(_MSC_VER)
#pragma warning (disable : 4244)
#pragma warning (disable : 4603)
#pragma warning (disable : 4996)
#endif

extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libavutil/avutil.h>
	#include <libavfilter/avfilter.h>
}

namespace caspar { namespace ffmpeg {
	
int ffmpeg_lock_callback(void **mutex, enum AVLockOp op) 
{ 
	if(!mutex)
		return 0;

	auto my_mutex = reinterpret_cast<tbb::recursive_mutex*>(*mutex);
	
	switch(op) 
	{ 
		case AV_LOCK_CREATE: 
		{ 
			*mutex = new tbb::recursive_mutex(); 
			break; 
		} 
		case AV_LOCK_OBTAIN: 
		{ 
			if(my_mutex)
				my_mutex->lock(); 
			break; 
		} 
		case AV_LOCK_RELEASE: 
		{ 
			if(my_mutex)
				my_mutex->unlock(); 
			break; 
		} 
		case AV_LOCK_DESTROY: 
		{ 
			delete my_mutex;
			*mutex = nullptr;
			break; 
		} 
	} 
	return 0; 
} 

static void sanitize(uint8_t *line)
{
    while(*line)
	{
        if(*line < 0x08 || (*line > 0x0D && *line < 0x20))
            *line='?';
        line++;
    }
}

void log_callback(void* ptr, int level, const char* fmt, va_list vl)
{
    static int print_prefix=1;
    static int count;
    static char prev[1024];
    char line[8192];
    static int is_atty;
    AVClass* avc= ptr ? *(AVClass**)ptr : NULL;
    if(level > av_log_get_level())
        return;
    line[0]=0;
	
#undef fprintf
    if(print_prefix && avc) 
	{
        if (avc->parent_log_context_offset) 
		{
            AVClass** parent= *(AVClass***)(((uint8_t*)ptr) + avc->parent_log_context_offset);
            if(parent && *parent)
                std::sprintf(line, "[%s @ %p] ", (*parent)->item_name(parent), parent);            
        }
        std::sprintf(line + strlen(line), "[%s @ %p] ", avc->item_name(ptr), ptr);
    }

    std::vsprintf(line + strlen(line), fmt, vl);

    print_prefix = strlen(line) && line[strlen(line)-1] == '\n';
	
    //if(print_prefix && !strcmp(line, prev)){
    //    count++;
    //    if(is_atty==1)
    //        fprintf(stderr, "    Last message repeated %d times\r", count);
    //    return;
    //}
    //if(count>0){
    //    fprintf(stderr, "    Last message repeated %d times\n", count);
    //    count=0;
    //}
    strcpy(prev, line);
    sanitize((uint8_t*)line);

	int len = strlen(line);
	if(len > 0)
		line[len-1] = 0;
	
	if(level == AV_LOG_DEBUG)
		CASPAR_LOG(debug) << L"[ffmpeg] " << line;
	else if(level == AV_LOG_INFO)
		CASPAR_LOG(info) << L"[ffmpeg] " << line;
	else if(level == AV_LOG_WARNING)
		CASPAR_LOG(warning) << L"[ffmpeg] " << line;
	else if(level == AV_LOG_ERROR)
		CASPAR_LOG(error) << L"[ffmpeg] " << line;
	else if(level == AV_LOG_FATAL)
		CASPAR_LOG(fatal) << L"[ffmpeg] " << line;
	else
		CASPAR_LOG(trace) << L"[ffmpeg] " << line;

    //colored_fputs(av_clip(level>>3, 0, 6), line);
}

//static int query_yadif_formats(AVFilterContext *ctx)
//{
//    static const int pix_fmts[] = {
//        PIX_FMT_YUV444P,
//        PIX_FMT_YUV422P,
//        PIX_FMT_YUV420P,
//        PIX_FMT_YUV410P,
//        PIX_FMT_YUV411P,
//        PIX_FMT_GRAY8,
//        PIX_FMT_YUVJ444P,
//        PIX_FMT_YUVJ422P,
//        PIX_FMT_YUVJ420P,
//        AV_NE( PIX_FMT_GRAY16BE, PIX_FMT_GRAY16LE ),
//        PIX_FMT_YUV440P,
//        PIX_FMT_YUVJ440P,
//        AV_NE( PIX_FMT_YUV444P16BE, PIX_FMT_YUV444P16LE ),
//        AV_NE( PIX_FMT_YUV422P16BE, PIX_FMT_YUV422P16LE ),
//        AV_NE( PIX_FMT_YUV420P16BE, PIX_FMT_YUV420P16LE ),
//        PIX_FMT_YUVA420P,
//        PIX_FMT_NONE
//    };
//    avfilter_set_common_pixel_formats(ctx, avfilter_make_format_list(pix_fmts));
//
//    return 0;
//}
//
//#pragma warning (push)
//#pragma warning (disable : 4706)
//void fix_yadif_filter_format_query()
//{
//	AVFilter** filter = nullptr;
//    while((filter = av_filter_next(filter)) && *filter)
//	{
//		if(strstr((*filter)->name, "yadif") != 0)
//			(*filter)->query_formats = query_yadif_formats;
//	}
//}
//#pragma warning (pop)

void init()
{
	av_lockmgr_register(ffmpeg_lock_callback);
	av_log_set_callback(log_callback);

    avfilter_register_all();
	//fix_yadif_filter_format_query();
	av_register_all();
    avformat_network_init();
	avcodec_init();
    avcodec_register_all();
	
	core::register_consumer_factory([](const std::vector<std::wstring>& params){return create_consumer(params);});
	core::register_producer_factory(create_producer);
}

void uninit()
{
	avfilter_uninit();
    avformat_network_deinit();
	av_lockmgr_register(nullptr);
}

std::wstring make_version(unsigned int ver)
{
	std::wstringstream str;
	str << ((ver >> 16) & 0xFF) << L"." << ((ver >> 8) & 0xFF) << L"." << ((ver >> 0) & 0xFF);
	return str.str();
}

std::wstring get_avcodec_version()
{
	return make_version(avcodec_version());
}

std::wstring get_avformat_version()
{
	return make_version(avformat_version());
}

std::wstring get_avutil_version()
{
	return make_version(avutil_version());
}

std::wstring get_avfilter_version()
{
	return make_version(avfilter_version());
}

std::wstring get_swscale_version()
{
	return make_version(swscale_version());
}

}}