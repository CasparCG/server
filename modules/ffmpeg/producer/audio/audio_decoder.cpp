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

namespace caspar { namespace ffmpeg {
	
struct audio_decoder::implementation : public Concurrency::agent, boost::noncopyable
{
	audio_decoder::source_t& source_;
	audio_decoder::target_t& target_;

	std::shared_ptr<AVCodecContext>								codec_context_;		
	const core::video_format_desc								format_desc_;
	int															index_;
	std::unique_ptr<audio_resampler>							resampler_;

	std::vector<int8_t,  tbb::cache_aligned_allocator<int8_t>>	buffer1_;
	
	int64_t														nb_frames_;

public:
	explicit implementation(audio_decoder::source_t& source,
							audio_decoder::target_t& target,
							const safe_ptr<AVFormatContext>& context, 
							const core::video_format_desc& format_desc) 
		: source_(source)
		, target_(target)
		, format_desc_(format_desc)	
		, nb_frames_(0)
	{			   	
		
		AVCodec* dec;
		index_ = THROW_ON_ERROR3(av_find_best_stream(context.get(), AVMEDIA_TYPE_AUDIO, -1, -1, &dec, 0), "[audio_decoder]", ffmpeg_stream_not_found);
		CASPAR_VERIFY(index_ > -1, ffmpeg_error());

		THROW_ON_ERROR2(avcodec_open(context->streams[index_]->codec, dec), "[audio_decoder]");
		CASPAR_VERIFY(context->streams[index_]->codec, ffmpeg_error());
			
		codec_context_.reset(context->streams[index_]->codec, avcodec_close);

		buffer1_.resize(AVCODEC_MAX_AUDIO_FRAME_SIZE*2);

		resampler_.reset(new audio_resampler(format_desc_.audio_channels,    codec_context_->channels,
												format_desc_.audio_sample_rate, codec_context_->sample_rate,
												AV_SAMPLE_FMT_S32,				 codec_context_->sample_fmt));			
		CASPAR_VERIFY(resampler_, ffmpeg_error());

		start();
	}

	~implementation()
	{
		agent::wait(this);
	}	

	virtual void run()
	{
		try
		{
			while(true)
			{
				auto packet = Concurrency::receive(source_, [this](const std::shared_ptr<AVPacket>& packet)
				{
					return packet && packet->stream_index == index_;
				});

				if(packet == eof_packet(index_))				
					break;
				
				if(packet == loop_packet(index_))	
				{	
					avcodec_flush_buffers(codec_context_.get());
					Concurrency::send(target_, loop_audio());
				}	
				else if(packet->stream_index == index_)
				{
					Concurrency::send(target_, decode(*packet));		
					Concurrency::wait(0);
				}
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
		
		Concurrency::send(target_, eof_audio());

		done();
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

		buffer1_ = resampler_->resample(std::move(buffer1_));
		
		const auto n_samples = buffer1_.size() / av_get_bytes_per_sample(AV_SAMPLE_FMT_S32);
		const auto samples = reinterpret_cast<int32_t*>(buffer1_.data());

		return n_samples > 0 ? std::make_shared<core::audio_buffer>(samples, samples + n_samples) : nullptr;
	}
};

audio_decoder::audio_decoder(source_t& source,
							 target_t& target,
							 const safe_ptr<AVFormatContext>& context, 
							 const core::video_format_desc& format_desc)
	: impl_(new implementation(source, target, context, format_desc))
{
}
int64_t audio_decoder::nb_frames() const{return impl_->nb_frames_;}

}}