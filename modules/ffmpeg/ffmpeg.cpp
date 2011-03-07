#include "StdAfx.h"

#include "consumer/ffmpeg_consumer.h"
#include "producer/ffmpeg_producer.h"

#include <core/consumer/frame_consumer.h>
#include <core/producer/frame_producer.h>

#if defined(_MSC_VER)
#pragma warning (disable : 4244)
#endif

extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libavcodec/avcodec.h>
}

namespace caspar {

void init_ffmpeg()
{
	av_register_all();
	avcodec_init();

	core::register_consumer_factory(create_ffmpeg_consumer);
	core::register_producer_factory(create_ffmpeg_producer);
}

std::wstring get_avcodec_version()
{
	std::wstringstream str;
	str << ((avcodec_version() >> 16) & 0xFF) << L"." << ((avcodec_version() >> 8) & 0xFF) << L"." << ((avcodec_version() >> 0) & 0xFF);
	return str.str();
}

std::wstring get_avformat_version()
{
	std::wstringstream str;
	str << ((avformat_version() >> 16) & 0xFF) << L"." << ((avformat_version() >> 8) & 0xFF) << L"." << ((avformat_version() >> 0) & 0xFF);
	return str.str();
}

std::wstring get_swscale_version()
{
	std::wstringstream str;
	str << ((swscale_version() >> 16) & 0xFF) << L"." << ((swscale_version() >> 8) & 0xFF) << L"." << ((swscale_version() >> 0) & 0xFF);
	return str.str();
}

}