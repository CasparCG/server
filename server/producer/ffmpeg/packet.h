#pragma once

#include "../../frame/audio_chunk.h"
#include "../../frame/frame.h"
#include "../../frame/format.h"

#include <tbb/scalable_allocator.h>
#include <type_traits>

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

namespace caspar{ namespace ffmpeg{

typedef std::tr1::shared_ptr<AVFrame> AVFramePtr;	
typedef std::tr1::shared_ptr<AVPacket> AVPacketPtr;

struct video_packet : boost::noncopyable
{
	video_packet(const AVPacketPtr& packet, frame_ptr&& frame, const frame_format_desc& format_desc, AVCodecContext* codec_context, AVCodec* codec) 
		:  size(packet->size), codec_context(codec_context), codec(codec), frame(std::move(frame)), format_desc(format_desc), 
			data(static_cast<uint8_t*>(scalable_aligned_malloc(packet->size, 16)))
	{
		memcpy(const_cast<uint8_t*>(data), packet->data, packet->size);
	}
		
	~video_packet()
	{
		scalable_aligned_free(const_cast<uint8_t*>(data));
	}
	
	const size_t					size;
	const uint8_t* const			data;
	AVCodecContext*	const			codec_context;
	const AVCodec* const			codec;
	const frame_ptr					frame;
	AVFramePtr						decoded_frame;
	const frame_format_desc&		format_desc;
};	
typedef std::shared_ptr<video_packet> video_packet_ptr;

struct audio_packet : boost::noncopyable
{
	audio_packet(const AVPacketPtr& packet, AVCodecContext* codec_context, AVCodec* codec, double frame_rate = 25.0) 
		: 
		size(packet->size), 
		codec_context(codec_context), 
		codec(codec),
		data(static_cast<uint8_t*>(scalable_aligned_malloc(packet->size, 16)))
	{
		memcpy(const_cast<uint8_t*>(data), packet->data, packet->size);

		size_t bytesPerSec = (codec_context->sample_rate * codec_context->channels * 2);

		audio_frame_size = bytesPerSec / 25;
		src_audio_frame_size = static_cast<size_t>(static_cast<double>(bytesPerSec) / frame_rate);

		//make sure the framesize is a multiple of the samplesize
		int sourceSizeMod = src_audio_frame_size % (codec_context->channels * 2);
		if(sourceSizeMod != 0)
			src_audio_frame_size += (codec_context->channels * 2) - sourceSizeMod;
	}
		
	~audio_packet()
	{
		scalable_aligned_free(const_cast<uint8_t*>(data));
	}
		
	size_t					src_audio_frame_size;
	size_t					audio_frame_size;

	AVCodecContext*	const	codec_context;
	const AVCodec* const	codec;
	const size_t			size;
	const uint8_t* const	data;

	std::vector<audio_chunk_ptr> audio_chunks;
};
typedef std::shared_ptr<audio_packet> audio_packet_ptr;

	}
}
