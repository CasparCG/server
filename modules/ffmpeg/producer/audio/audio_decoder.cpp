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
	std::vector<int16_t, tbb::cache_aligned_allocator<int16_t>>	buffer_;		// avcodec_decode_audio3 needs 4 byte alignment
	std::vector<int16_t, tbb::cache_aligned_allocator<int16_t>>	audio_samples_;		// avcodec_decode_audio3 needs 4 byte alignment
	std::queue<std::shared_ptr<AVPacket>>						packets_;
public:
	explicit implementation(AVStream* stream, const core::video_format_desc& format_desc) 
		: format_desc_(format_desc)	
	{
		if(!stream || !stream->codec)
			return;

		auto codec = avcodec_find_decoder(stream->codec->codec_id);			
		if(!codec)
			return;
			
		int errn = avcodec_open(stream->codec, codec);
		if(errn < 0)
			return;
				
		index_ = stream->index;
		codec_context_.reset(stream->codec, avcodec_close);

		if(codec_context_ &&
		   (codec_context_->sample_rate != static_cast<int>(format_desc_.audio_sample_rate) || 
		   codec_context_->channels != static_cast<int>(format_desc_.audio_channels)))
		{	
			BOOST_THROW_EXCEPTION(
				file_read_error()  <<
				msg_info("Invalid sample-rate or number of channels.") <<
				arg_value_info(boost::lexical_cast<std::string>(codec_context_->sample_rate)) << 
				arg_name_info("codec_context"));
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
			decode(packets_.front());
			packets_.pop();

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

	void decode(const std::shared_ptr<AVPacket>& packet)
	{											
		if(!packet) // eof
		{
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
			buffer_.resize(4*format_desc_.audio_sample_rate*2+FF_INPUT_BUFFER_PADDING_SIZE/2, 0);

			int written_bytes = buffer_.size() - FF_INPUT_BUFFER_PADDING_SIZE/2;
			const int errn = avcodec_decode_audio3(codec_context_.get(), buffer_.data(), &written_bytes, packet.get());
			if(errn < 0)
			{	
				BOOST_THROW_EXCEPTION(
					invalid_operation() <<
					boost::errinfo_api_function("avcodec_decode_audio2") <<
					boost::errinfo_errno(AVUNERROR(errn)));
			}

			buffer_.resize(written_bytes/2);
			audio_samples_.insert(audio_samples_.end(), buffer_.begin(), buffer_.end());
			buffer_.clear();	
		}
	}

	bool ready() const
	{
		return !codec_context_ || !packets_.empty();
	}
};

audio_decoder::audio_decoder(AVStream* stream, const core::video_format_desc& format_desc) : impl_(new implementation(stream, format_desc)){}
void audio_decoder::push(const std::shared_ptr<AVPacket>& packet){impl_->push(packet);}
bool audio_decoder::ready() const{return impl_->ready();}
std::vector<std::vector<int16_t>> audio_decoder::poll(){return impl_->poll();}
}