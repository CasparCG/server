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

#include "ffmpeg.h"

#include "producer/ffmpeg_producer.h"
#include "consumer/ffmpeg_consumer.h"

#include <common/log.h>

#include <core/frame/draw_frame.h>
#include <core/consumer/frame_consumer.h>
#include <core/producer/frame_producer.h>

#include <functional>
#include <mutex>

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
	#include <libavdevice/avdevice.h>
}

namespace caspar { namespace ffmpeg {
int ffmpeg_lock_callback(void **mutex, enum AVLockOp op)
{
	if(!mutex)
		return 0;

	auto my_mutex = reinterpret_cast<std::recursive_mutex*>(*mutex);

	switch(op)
	{
		case AV_LOCK_CREATE:
		{
			*mutex = new std::recursive_mutex();
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
	static thread_local bool print_prefix_tss = true;

	char line[1024];
	AVClass* avc= ptr ? *(AVClass**)ptr : NULL;
	if (level > AV_LOG_DEBUG)
		return;
	line[0]=0;

#undef fprintf
	if(print_prefix_tss && avc)
	{
		if (avc->parent_log_context_offset)
		{
			AVClass** parent= *(AVClass***)(((uint8_t*)ptr) + avc->parent_log_context_offset);
			if(parent && *parent)
				std::snprintf(line, sizeof(line), "[%s @ %p] ", (*parent)->item_name(parent), parent);
		}
		std::snprintf(line + strlen(line), sizeof(line) - strlen(line), "[%s @ %p] ", avc->item_name(ptr), ptr);
	}

	std::vsnprintf(line + strlen(line), sizeof(line) - strlen(line), fmt, vl);

    print_prefix_tss = strlen(line) && line[strlen(line)-1] == '\n';

	sanitize((uint8_t*)line);

	try
	{
		if (level == AV_LOG_VERBOSE)
			CASPAR_LOG(debug) << L"[ffmpeg] " << line;
		else if (level == AV_LOG_INFO)
			CASPAR_LOG(info) << L"[ffmpeg] " << line;
		else if (level == AV_LOG_WARNING)
			CASPAR_LOG(warning) << L"[ffmpeg] " << line;
		else if (level == AV_LOG_ERROR)
			CASPAR_LOG(error) << L"[ffmpeg] " << line;
		else if (level == AV_LOG_FATAL)
			CASPAR_LOG(fatal) << L"[ffmpeg] " << line;
		else
			CASPAR_LOG(trace) << L"[ffmpeg] " << line;
	}
	catch (...)
	{
	}
}

void log_for_thread(void* ptr, int level, const char* fmt, va_list vl)
{
	log_callback(ptr, level, fmt, vl);
}

void init(core::module_dependencies dependencies)
{
	av_lockmgr_register(ffmpeg_lock_callback);
	av_log_set_callback(log_for_thread);

	avfilter_register_all();
	av_register_all();
	avformat_network_init();
	avcodec_register_all();
	avdevice_register_all();

	dependencies.consumer_registry->register_consumer_factory(L"FFmpeg Consumer", create_consumer);
	dependencies.consumer_registry->register_preconfigured_consumer_factory(L"ffmpeg", create_preconfigured_consumer);

	dependencies.producer_registry->register_producer_factory(L"FFmpeg Producer", create_producer);
}

void uninit()
{
	// avfilter_uninit();
	avformat_network_deinit();
	av_lockmgr_register(nullptr);
}
}}
