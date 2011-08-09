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
	std::shared_ptr<ReSampleContext>							resampler_;

	std::vector<int8_t,  tbb::cache_aligned_allocator<int8_t>>	buffer1_;
	std::vector<int8_t,  tbb::cache_aligned_allocator<int8_t>>	buffer2_;
	std::vector<int16_t, tbb::cache_aligned_allocator<int16_t>>	audio_samples_;	
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

			//nb_frames_ = context->streams[index_]->nb_frames;
			//if(nb_frames_ == 0)
			//	nb_frames_ = context->streams[index_]->duration * context->streams[index_]->time_base.den;

			if(codec_context_->sample_rate	!= static_cast<int>(format_desc_.audio_sample_rate) || 
			   codec_context_->channels		!= static_cast<int>(format_desc_.audio_channels) ||
			   codec_context_->sample_fmt	!= AV_SAMPLE_FMT_S16)
			{	
				auto resampler = av_audio_resample_init(format_desc_.audio_channels,    codec_context_->channels,
														format_desc_.audio_sample_rate, codec_context_->sample_rate,
														AV_SAMPLE_FMT_S16,				codec_context_->sample_fmt,
														16, 10, 0, 0.8);

				buffer2_.resize(AVCODEC_MAX_AUDIO_FRAME_SIZE*2);

				CASPAR_LOG(warning) << L" Invalid audio format. Resampling.";

				if(resampler)
					resampler_.reset(resampler, audio_resample_close);
				else
					codec_context_ = nullptr;
			}		
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
	
	std::vector<std::shared_ptr<std::vector<int16_t>>> poll()
	{
		if(!codec_context_)
			return empty_poll();

		std::vector<std::shared_ptr<std::vector<int16_t>>> result;

		while(!packets_.empty() && result.empty())
		{						
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
		}

		return result;
	}

	std::vector<std::shared_ptr<std::vector<int16_t>>> empty_poll()
	{
		if(packets_.empty())
			return std::vector<std::shared_ptr<std::vector<int16_t>>>();
		
		auto packet = packets_.front();
		packets_.pop();

		if(!packet)			
			return boost::assign::list_of(nullptr);
		
		return boost::assign::list_of(std::make_shared<std::vector<int16_t>>(format_desc_.audio_samples_per_frame, 0));	
	}

	std::shared_ptr<std::vector<int16_t>> decode(AVPacket& pkt)
	{		
		int written_bytes = buffer1_.size() - FF_INPUT_BUFFER_PADDING_SIZE;

		int ret = THROW_ON_ERROR2(avcodec_decode_audio3(codec_context_.get(), reinterpret_cast<int16_t*>(buffer1_.data()), &written_bytes, &pkt), "[audio_decoder]");

		// There might be several frames in one packet.
		pkt.size -= ret;
		pkt.data += ret;
			
		if(resampler_)
		{
			auto ret = audio_resample(resampler_.get(),
										reinterpret_cast<short*>(buffer2_.data()), 
										reinterpret_cast<short*>(buffer1_.data()), 
										written_bytes / (av_get_bytes_per_sample(codec_context_->sample_fmt) * codec_context_->channels)); 
			written_bytes = ret * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) * format_desc_.audio_channels;
			std::swap(buffer1_, buffer2_);
		}

		const auto n_samples = written_bytes / av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
		const auto samples = reinterpret_cast<int16_t*>(buffer1_.data());

		return std::make_shared<std::vector<int16_t>>(samples, samples + n_samples);
	}

	bool ready() const
	{
		return !codec_context_ || packets_.size() > 2;
	}
};

audio_decoder::audio_decoder(const safe_ptr<AVFormatContext>& context, const core::video_format_desc& format_desc) : impl_(new implementation(context, format_desc)){}
void audio_decoder::push(const std::shared_ptr<AVPacket>& packet){impl_->push(packet);}
bool audio_decoder::ready() const{return impl_->ready();}
std::vector<std::shared_ptr<std::vector<int16_t>>> audio_decoder::poll(){return impl_->poll();}
int64_t audio_decoder::nb_frames() const{return impl_->nb_frames_;}
}