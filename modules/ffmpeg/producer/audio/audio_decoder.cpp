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
#include "../../stdafx.h"

#include "audio_decoder.h"

#include "audio_resampler.h"

#include "../util.h"
#include "../../ffmpeg_error.h"

#include <core/video_format.h>

#include <tbb/cache_aligned_allocator.h>

#include <queue>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

using namespace Concurrency;

namespace caspar { namespace ffmpeg {
	
struct audio_decoder::implementation : boost::noncopyable
{	
	audio_decoder::source_t&									source_;
	int															index_;
	const safe_ptr<AVCodecContext>								codec_context_;		
	const core::video_format_desc								format_desc_;
	audio_resampler												resampler_;

	std::vector<int8_t,  tbb::cache_aligned_allocator<int8_t>>	buffer1_;

	std::queue<safe_ptr<AVPacket>>								packets_;
public:
	explicit implementation(audio_decoder::source_t& source, const safe_ptr<AVFormatContext>& context, const core::video_format_desc& format_desc) 
		: source_(source)
		, codec_context_(open_codec(*context, AVMEDIA_TYPE_AUDIO, index_))
		, format_desc_(format_desc)	
		, buffer1_(AVCODEC_MAX_AUDIO_FRAME_SIZE*2)
		, resampler_(format_desc_.audio_channels,    codec_context_->channels,
												 format_desc_.audio_sample_rate, codec_context_->sample_rate,
												 AV_SAMPLE_FMT_S32,				 codec_context_->sample_fmt)
	{			   
	}
		
	std::shared_ptr<core::audio_buffer> poll()
	{
		auto packet = create_packet();
		
		if(packets_.empty())
		{
			if(!try_receive(source_, packet) || packet->stream_index != index_)
				return nullptr;
			else
				packets_.push(packet);
		}
		
		packet = packets_.front();
										
		std::shared_ptr<core::audio_buffer> audio;
		if(packet == loop_packet())			
		{	
			avcodec_flush_buffers(codec_context_.get());		
			audio = loop_audio();
		}	
		else
			audio = decode(*packet);
		
		if(packet->size == 0)					
			packets_.pop();

		return audio;
	}
	
	std::shared_ptr<core::audio_buffer> decode(AVPacket& pkt)
	{		
		buffer1_.resize(AVCODEC_MAX_AUDIO_FRAME_SIZE*2);
		int written_bytes = buffer1_.size() - FF_INPUT_BUFFER_PADDING_SIZE;
		
		int ret = THROW_ON_ERROR2(avcodec_decode_audio3(codec_context_.get(), reinterpret_cast<int16_t*>(buffer1_.data()), &written_bytes, &pkt), "[audio_decoder]");

		// There might be several frames in one packet.
		pkt.size -= ret;
		pkt.data += ret;
			
		buffer1_.resize(written_bytes);

		buffer1_ = resampler_.resample(std::move(buffer1_));
		
		const auto n_samples = buffer1_.size() / av_get_bytes_per_sample(AV_SAMPLE_FMT_S32);
		const auto samples = reinterpret_cast<int32_t*>(buffer1_.data());

		return std::make_shared<core::audio_buffer>(samples, samples + n_samples);
	}
};

audio_decoder::audio_decoder(audio_decoder::source_t& source, const safe_ptr<AVFormatContext>& context, const core::video_format_desc& format_desc) : impl_(new implementation(source, context, format_desc)){}
std::shared_ptr<core::audio_buffer> audio_decoder::poll(){return impl_->poll();}

}}