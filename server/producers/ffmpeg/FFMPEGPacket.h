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
 
#ifndef _FFMPEG_PACKET_H_
#define _FFMPEG_PACKET_H_

#include "FFMPEGException.h"

#include "../../audio/audiomanager.h"
#include "../../utils/Noncopyable.hpp"

#include <tbb/pipeline.h>
#include <tbb/cache_aligned_allocator.h>

#include <vector>
#include <memory>
#include <type_traits>

#include <boost/noncopyable.hpp>

extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <ffmpeg/avformat.h>
//	#include <libavformat/avformat.h>
}

namespace caspar
{
	namespace ffmpeg
	{
				
typedef std::tr1::shared_ptr<AVFrame> AVFramePtr;	
typedef std::tr1::shared_ptr<AVPacket> AVPacketPtr;
typedef std::tr1::shared_ptr<unsigned char> BytePtr;

struct av_frame_allocator
{
	static AVFrame* construct()
	{ return avcodec_alloc_frame(); }
	static void destroy(AVFrame* const block)
	{ av_free(block); }
};

///=================================================================================================
/// <summary>	Values that represent different packet types used in ffmpeg pipeline. </summary>
///
/// <remarks>	Rona 2010-06-19. </remarks>
///=================================================================================================
enum FFMPEGPacketType
{
	VideoPacket = 1,
	AudioPacket = 2,
	VideoFrame = 4,
	UnkownPacket = 8,
	Any = VideoPacket | AudioPacket | VideoFrame | UnkownPacket
};

///=================================================================================================
/// <summary>	Base class for packets used in ffmpeg pipeline. </summary>
///
/// <remarks>	Rona 2010-06-19. </remarks>
///=================================================================================================
struct FFMPEGPacket : boost::noncopyable
{	
	enum  { packet_type = Any };

	virtual ~FFMPEGPacket()
	{
		if(data != nullptr)
			allocator_.deallocate(data, size);
	}

	AVCodecContext* codecContext;
	AVCodec*	codec;
	size_t		size;
	uint8_t*	data;
	const FFMPEGPacketType type;
	static tbb::cache_aligned_allocator<uint8_t> allocator_;

protected:
	FFMPEGPacket(FFMPEGPacketType type) : type(type), data(nullptr) {}
	FFMPEGPacket(const AVPacketPtr& packet, FFMPEGPacketType type, AVCodecContext* codecContext, AVCodec* codec) 
		: 
		size(packet->size), 
		type(type), 
		codecContext(codecContext), codec(codec)
	{
		data = allocator_.allocate(packet->size);
		memcpy(data, packet->data, packet->size);
	}
};
typedef std::shared_ptr<FFMPEGPacket> FFMPEGPacketPtr;

///=================================================================================================
/// <summary>	Used to avoid returning nullptr from input filter and stopping the producer for for packets of unknown type. </summary>
///
/// <remarks>	Rona 2010-06-19. </remarks>
///=================================================================================================
struct FFMPEGUnknownPacket : public FFMPEGPacket
{
	enum  { packet_type = UnkownPacket };

	FFMPEGUnknownPacket() : FFMPEGPacket(UnkownPacket){}
};

///=================================================================================================
/// <summary>	FFMPEG video packet. </summary>
///
/// <remarks>	Rona 2010-06-19. </remarks>
///=================================================================================================
struct FFMPEGVideoPacket : public FFMPEGPacket
{
	enum  { packet_type = VideoPacket };

	FFMPEGVideoPacket(const AVPacketPtr& packet, const FramePtr& pFrame, const FrameFormatDescription& frameFormat, AVCodecContext* codecContext, AVCodec* codec, bool hasAudio) 
		: 
		FFMPEGPacket(packet, VideoPacket, codecContext, codec), pFrame(pFrame), frameFormat(frameFormat), hasAudio(hasAudio)
	{
	}
	
	bool					hasAudio;
	FrameFormatDescription  frameFormat;
	FramePtr				pFrame;
	AVFramePtr				pDecodedFrame;
};	

///=================================================================================================
/// <summary>	FFMPEG audio packet. </summary>
///
/// <remarks>	Rona 2010-06-19. </remarks>
///=================================================================================================
struct FFMPEGAudioPacket : public FFMPEGPacket
{
	enum  { packet_type = AudioPacket };

	FFMPEGAudioPacket(const AVPacketPtr& packet, AVCodecContext* codecContext, AVCodec* codec, double videoFrameRate = 25.0) 
		: 
		FFMPEGPacket(packet, AudioPacket, codecContext, codec)
	{
		int bytesPerSec = (codecContext->sample_rate * codecContext->channels * 2);

		if(bytesPerSec / 25 == 0)
			throw ffmpeg_error(ffmpeg_make_error_code(ffmpeg_errc::invalid_audio_frame_size));		

		audioFrameSize = bytesPerSec / 25;
		sourceAudioFrameSize = static_cast<int>(static_cast<double>(bytesPerSec) / videoFrameRate);

		//make sure the framesize is a multiple of the samplesize
		int sourceSizeMod = sourceAudioFrameSize % (codecContext->channels * 2);
		if(sourceSizeMod != 0)
			sourceAudioFrameSize += (codecContext->channels * 2) - sourceSizeMod;

		if(audioFrameSize == 0)
			throw ffmpeg_error(ffmpeg_make_error_code(ffmpeg_errc::invalid_audio_frame_size));
	}
		
	std::vector<audio::AudioDataChunkPtr> audioDataChunks;
	int sourceAudioFrameSize;
	int audioFrameSize;
};

///=================================================================================================
/// <summary>	Used to allow deallocation of unused memory in finished video and audio packets. </summary>
///
/// <remarks>	Rona 2010-06-19. </remarks>
///=================================================================================================
struct FFMPEGVideoFrame : public FFMPEGPacket
{
	enum  { packet_type = VideoFrame };

	FFMPEGVideoFrame() : FFMPEGPacket(VideoFrame){}
	FFMPEGVideoFrame(FramePtr pFrame) : FFMPEGPacket(VideoFrame), pFrame(pFrame){}
	FFMPEGVideoFrame(const FFMPEGVideoPacket* pPacket) : FFMPEGPacket(VideoFrame), pFrame(pPacket->pFrame), hasAudio(pPacket->hasAudio){}
	FramePtr pFrame;
	bool hasAudio;
};
typedef std::shared_ptr<FFMPEGVideoFrame> FFMPEGVideoFramePtr;

///=================================================================================================
/// <summary>	Ffmpeg filter implementation. </summary>
///
/// <remarks>	Rona, 2010-06-20. </remarks>
///=================================================================================================
template <typename PacketType>
class FFMPEGFilterImpl
{
protected:
	~FFMPEGFilterImpl(){}
public:
	void* operator()(void* item)
	{				
		if(item == nullptr)
			return nullptr;

		FFMPEGPacket* pPacket = static_cast<FFMPEGPacket*>(item);

		if((pPacket->type & PacketType::packet_type) == 0)
			return item;

		PacketType* pLocalPacket = static_cast<PacketType*>(pPacket); 

		if(!is_valid(pLocalPacket))
			return pLocalPacket;

		return process(pLocalPacket);
	}

	virtual void finalize(void* item)
	{
		delete static_cast<FFMPEGPacket*>(item);
	}
	
private:
	virtual void* process(PacketType* pPacket) = 0;
	virtual bool is_valid(PacketType* pPacket)
	{
		return pPacket != nullptr;
	}
};

	}
}

#endif