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
	typedef std::vector<short, tbb::cache_aligned_allocator<short>> buffer;
	
	AVCodecContext* codec_context_;
		
	const core::video_format_desc format_desc_;

	buffer current_chunk_;

public:
	explicit implementation(AVCodecContext* codec_context, const core::video_format_desc& format_desc) 
		: codec_context_(codec_context)
		, format_desc_(format_desc)		
	{
		if(!codec_context)
		{	
			BOOST_THROW_EXCEPTION(
				null_argument() << 
				arg_name_info("codec_context"));	
		}

		if(codec_context_->sample_rate != static_cast<int>(format_desc_.audio_sample_rate) || 
		   codec_context_->channels != static_cast<int>(format_desc_.audio_channels))
		{	
			BOOST_THROW_EXCEPTION(
				file_read_error()  <<
				msg_info("Invalid sample-rate or number of channels.") <<
				arg_value_info(boost::lexical_cast<std::string>(codec_context_->sample_rate)) << 
				arg_name_info("codec_context"));
		}
	}
		
	std::vector<std::vector<short>> execute(const packet& audio_packet)
	{	
		std::vector<std::vector<short>> result;

		switch(audio_packet.type)
		{
		case flush_packet:
			avcodec_flush_buffers(codec_context_);
			break;
		case data_packet:
			auto s = current_chunk_.size();
			current_chunk_.resize(s + 4*format_desc_.audio_sample_rate*2+FF_INPUT_BUFFER_PADDING_SIZE/2);
		
			int written_bytes = (current_chunk_.size() - s)*2 - FF_INPUT_BUFFER_PADDING_SIZE;
			const int errn = avcodec_decode_audio2(codec_context_, &current_chunk_[s], &written_bytes, audio_packet.data->data(), audio_packet.data->size());
			if(errn < 0)
			{	
				BOOST_THROW_EXCEPTION(
					invalid_operation() <<
					boost::errinfo_api_function("avcodec_decode_audio2") <<
					boost::errinfo_errno(AVUNERROR(errn)));
			}

			current_chunk_.resize(s + written_bytes/2);

			const auto last = current_chunk_.end() - current_chunk_.size() % format_desc_.audio_samples_per_frame;
		
			for(auto it = current_chunk_.begin(); it != last; it += format_desc_.audio_samples_per_frame)		
				result.push_back(std::vector<short>(it, it + format_desc_.audio_samples_per_frame));		

			current_chunk_.erase(current_chunk_.begin(), last);
		}
				
		return result;
	}
};

audio_decoder::audio_decoder(AVCodecContext* codec_context, const core::video_format_desc& format_desc) : impl_(new implementation(codec_context, format_desc)){}
std::vector<std::vector<short>> audio_decoder::execute(const packet& audio_packet){return impl_->execute(audio_packet);}
}