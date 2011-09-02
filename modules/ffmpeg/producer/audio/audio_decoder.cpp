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

namespace caspar {
	
struct audio_decoder::implementation : boost::noncopyable
{	
	std::shared_ptr<AVCodecContext>								codec_context_;		
	const core::video_format_desc								format_desc_;
	int															index_;
	std::unique_ptr<audio_resampler>							resampler_;

	std::vector<int8_t,  tbb::cache_aligned_allocator<int8_t>>	buffer1_;

	std::queue<std::shared_ptr<AVPacket>>						packets_;

	int64_t														nb_frames_;
public:
	explicit implementation(const safe_ptr<AVFormatContext>& context, const core::video_format_desc& format_desc) 
		: format_desc_(format_desc)	
		, nb_frames_(0)
	{			   	
		try
		{
			AVCodec* dec;
			index_ = THROW_ON_ERROR2(av_find_best_stream(context.get(), AVMEDIA_TYPE_AUDIO, -1, -1, &dec, 0), "[audio_decoder]");

			THROW_ON_ERROR2(avcodec_open(context->streams[index_]->codec, dec), "[audio_decoder]");
			
			codec_context_.reset(context->streams[index_]->codec, avcodec_close);

			buffer1_.resize(AVCODEC_MAX_AUDIO_FRAME_SIZE*2);

			resampler_.reset(new audio_resampler(format_desc_.audio_channels,    codec_context_->channels,
												 format_desc_.audio_sample_rate, codec_context_->sample_rate,
												 AV_SAMPLE_FMT_S32,				 codec_context_->sample_fmt));	
		}
		catch(...)
		{
			index_ = THROW_ON_ERROR2(av_find_best_stream(context.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0), "[audio_decoder]");

			CASPAR_LOG_CURRENT_EXCEPTION();
			CASPAR_LOG(warning) << "[audio_decoder] Failed to open audio-stream. Running without audio.";			
		}
	}

	void push(const std::shared_ptr<AVPacket>& packet)
	{			
		if(packet && packet->stream_index != index_)
			return;

		packets_.push(packet);
	}	
	
	std::vector<std::shared_ptr<std::vector<int32_t>>> poll()
	{
		std::vector<std::shared_ptr<std::vector<int32_t>>> result;

		if(packets_.empty())
			return result;

		if(!codec_context_)
			return empty_poll();
		
		auto packet = packets_.front();

		if(packet)		
		{
			result.push_back(decode(*packet));
			if(packet->size == 0)					
				packets_.pop();
		}
		else			
		{	
			avcodec_flush_buffers(codec_context_.get());
			result.push_back(nullptr);
			packets_.pop();
		}		

		return result;
	}

	std::vector<std::shared_ptr<std::vector<int32_t>>> empty_poll()
	{
		auto packet = packets_.front();
		packets_.pop();

		if(!packet)			
			return boost::assign::list_of(nullptr);
		
		return boost::assign::list_of(std::make_shared<std::vector<int32_t>>(format_desc_.audio_samples_per_frame, 0));	
	}

	std::shared_ptr<std::vector<int32_t>> decode(AVPacket& pkt)
	{		
		buffer1_.resize(AVCODEC_MAX_AUDIO_FRAME_SIZE*2);
		int written_bytes = buffer1_.size() - FF_INPUT_BUFFER_PADDING_SIZE;
		
		int ret = THROW_ON_ERROR2(avcodec_decode_audio3(codec_context_.get(), reinterpret_cast<int16_t*>(buffer1_.data()), &written_bytes, &pkt), "[audio_decoder]");

		// There might be several frames in one packet.
		pkt.size -= ret;
		pkt.data += ret;
			
		buffer1_.resize(written_bytes);

		buffer1_ = resampler_->resample(std::move(buffer1_));
		
		const auto n_samples = buffer1_.size() / av_get_bytes_per_sample(AV_SAMPLE_FMT_S32);
		const auto samples = reinterpret_cast<int32_t*>(buffer1_.data());

		return std::make_shared<std::vector<int32_t>>(samples, samples + n_samples);
	}

	bool ready() const
	{
		return !packets_.empty();
	}
};

audio_decoder::audio_decoder(const safe_ptr<AVFormatContext>& context, const core::video_format_desc& format_desc) : impl_(new implementation(context, format_desc)){}
void audio_decoder::push(const std::shared_ptr<AVPacket>& packet){impl_->push(packet);}
bool audio_decoder::ready() const{return impl_->ready();}
std::vector<std::shared_ptr<std::vector<int32_t>>> audio_decoder::poll(){return impl_->poll();}
int64_t audio_decoder::nb_frames() const{return impl_->nb_frames_;}
}