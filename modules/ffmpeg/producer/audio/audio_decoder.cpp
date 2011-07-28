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

#include <tbb/task_group.h>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
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
public:
	explicit implementation(const std::shared_ptr<AVFormatContext>& context, const core::video_format_desc& format_desc) 
		: format_desc_(format_desc)	
	{			   
		AVCodec* dec;
		index_ = av_find_best_stream(context.get(), AVMEDIA_TYPE_AUDIO, -1, -1, &dec, 0);

		int errn = avcodec_open(context->streams[index_]->codec, dec);
		if(errn < 0)
			return;
				
		codec_context_.reset(context->streams[index_]->codec, avcodec_close);

		if(codec_context_ &&
		   (codec_context_->sample_rate != static_cast<int>(format_desc_.audio_sample_rate) || 
		    codec_context_->channels	!= static_cast<int>(format_desc_.audio_channels)) ||
			codec_context_->sample_fmt	!= AV_SAMPLE_FMT_S16)
		{	
			auto resampler = av_audio_resample_init(format_desc_.audio_channels,    codec_context_->channels,
													format_desc_.audio_sample_rate, codec_context_->sample_rate,
													AV_SAMPLE_FMT_S16,				codec_context_->sample_fmt,
													16, 10, 0, 0.8);

			CASPAR_LOG(warning) << L" Invalid audio format.";

			if(resampler)
				resampler_.reset(resampler, audio_resample_close);
			else
				codec_context_ = nullptr;
		}		
	}

	void push(const std::shared_ptr<AVPacket>& packet)
	{			
		if(!codec_context_)
			return;

		if(packet && packet->stream_index != index_)
			return;

		packets_.push(packet);
	}	
	
	std::vector<std::vector<int16_t>> poll()
	{
		std::vector<std::vector<int16_t>> result;

		if(!codec_context_)
			result.push_back(std::vector<int16_t>(format_desc_.audio_samples_per_frame, 0));
		else if(!packets_.empty())
		{
			decode();

			while(audio_samples_.size() > format_desc_.audio_samples_per_frame)
			{
				const auto begin = audio_samples_.begin();
				const auto end   = audio_samples_.begin() + format_desc_.audio_samples_per_frame;

				result.push_back(std::vector<int16_t>(begin, end));
				audio_samples_.erase(begin, end);
			}
		}

		return result;
	}

	void decode()
	{			
		if(packets_.empty())
			return;

		if(!packets_.front()) // eof
		{
			packets_.pop();

			auto truncate = audio_samples_.size() % format_desc_.audio_samples_per_frame;
			if(truncate > 0)
			{
				audio_samples_.resize(audio_samples_.size() - truncate); 
				CASPAR_LOG(info) << L"Truncating " << truncate << L" audio-samples."; 
			}
			avcodec_flush_buffers(codec_context_.get());
		}
		else
		{
			buffer1_.resize(AVCODEC_MAX_AUDIO_FRAME_SIZE*2, 0);
			int written_bytes = buffer1_.size() - FF_INPUT_BUFFER_PADDING_SIZE;

			const int ret = avcodec_decode_audio3(codec_context_.get(), reinterpret_cast<int16_t*>(buffer1_.data()), &written_bytes, packets_.front().get());
			if(ret < 0)
			{	
				BOOST_THROW_EXCEPTION(
					invalid_operation() <<
					boost::errinfo_api_function("avcodec_decode_audio2") <<
					boost::errinfo_errno(AVUNERROR(ret)));
			}

			packets_.front()->size -= ret;
			packets_.front()->data += ret;

			if(packets_.front()->size <= 0)
				packets_.pop();

			buffer1_.resize(written_bytes);

			if(resampler_)
			{
				buffer2_.resize(AVCODEC_MAX_AUDIO_FRAME_SIZE*2, 0);
				auto ret = audio_resample(resampler_.get(),
										  reinterpret_cast<short*>(buffer2_.data()), 
										  reinterpret_cast<short*>(buffer1_.data()), 
										  buffer1_.size() / (av_get_bytes_per_sample(codec_context_->sample_fmt) * codec_context_->channels)); 
				buffer2_.resize(ret * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) * format_desc_.audio_channels);
				std::swap(buffer1_, buffer2_);
			}

			const auto n_samples = buffer1_.size() / av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
			const auto samples = reinterpret_cast<int16_t*>(buffer1_.data());

			audio_samples_.insert(audio_samples_.end(), samples, samples + n_samples);	
		}
	}

	bool ready() const
	{
		return !codec_context_ || !packets_.empty();
	}
};

audio_decoder::audio_decoder(const std::shared_ptr<AVFormatContext>& context, const core::video_format_desc& format_desc) : impl_(new implementation(context, format_desc)){}
void audio_decoder::push(const std::shared_ptr<AVPacket>& packet){impl_->push(packet);}
bool audio_decoder::ready() const{return impl_->ready();}
std::vector<std::vector<int16_t>> audio_decoder::poll(){return impl_->poll();}
}