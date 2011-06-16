/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#include "StdAfx.h"

#include "consumer/ffmpeg_consumer.h"
#include "producer/ffmpeg_producer.h"

#include <core/consumer/frame_consumer.h>
#include <core/producer/frame_producer.h>

#include <tbb/mutex.h>

#if defined(_MSC_VER)
#pragma warning (disable : 4244)
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

namespace caspar {
	
int ffmpeg_lock_callback(void **mutex, enum AVLockOp op) 
{ 
	static tbb::mutex				container_mutex;
	static std::vector<tbb::mutex>	container; 

	if(!mutex)
		return 0;

	auto my_mutex = reinterpret_cast<tbb::mutex*>(*mutex);
	
	switch(op) 
	{ 
		case AV_LOCK_CREATE: 
		{ 
			tbb::mutex::scoped_lock lock(container_mutex);
			container.push_back(tbb::mutex());
			*mutex = &container.back(); 
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
			tbb::mutex::scoped_lock lock(container_mutex);
			container.erase(std::remove_if(container.begin(), container.end(), [&](const tbb::mutex& m)
			{
				return &m == my_mutex;
			}), container.end());
			break; 
		} 
	} 
	return 0; 
} 

void init_ffmpeg()
{
	av_register_all();
	avcodec_init();
	av_lockmgr_register(ffmpeg_lock_callback);
	
	core::register_consumer_factory([](const std::vector<std::wstring>& params){return create_ffmpeg_consumer(params);});
	core::register_producer_factory(create_ffmpeg_producer);
}

void uninit_ffmpeg()
{
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

}