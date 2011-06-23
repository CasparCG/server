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
	input&							input_;
	AVCodecContext&					codec_context_;		
	const core::video_format_desc	format_desc_;

	std::vector<int16_t, tbb::cache_aligned_allocator<int16_t>>			current_chunk_;	

	size_t							frame_number_;
	bool							wait_for_eof_;

	std::vector<int16_t, tbb::cache_aligned_allocator<int16_t>> buffer_;
public:
	explicit implementation(input& input, const core::video_format_desc& format_desc) 
		: input_(input)
		, codec_context_(*input_.get_audio_codec_context())
		, format_desc_(format_desc)	
		, frame_number_(0)
		, wait_for_eof_(false)
		, buffer_(4*format_desc_.audio_sample_rate*2+FF_INPUT_BUFFER_PADDING_SIZE/2, 0)
	{
		if(codec_context_.sample_rate != static_cast<int>(format_desc_.audio_sample_rate) || 
		   codec_context_.channels != static_cast<int>(format_desc_.audio_channels))
		{	
			BOOST_THROW_EXCEPTION(
				file_read_error()  <<
				msg_info("Invalid sample-rate or number of channels.") <<
				arg_value_info(boost::lexical_cast<std::string>(codec_context_.sample_rate)) << 
				arg_name_info("codec_context"));
		}
	}
		
	std::deque<std::pair<int, std::vector<short>>> receive()
	{
		std::deque<std::pair<int, std::vector<short>>> result;
		
		std::shared_ptr<AVPacket> pkt;
		for(int n = 0; n < 32 && result.empty() && input_.try_pop_audio_packet(pkt); ++n)	
			result = decode(pkt);

		return result;
	}

	std::deque<std::pair<int, std::vector<int16_t>>> decode(const std::shared_ptr<AVPacket>& audio_packet)
	{			
		std::deque<std::pair<int, std::vector<int16_t>>> result;

		if(!audio_packet) // eof
		{	
			avcodec_flush_buffers(&codec_context_);
			current_chunk_.clear();
			frame_number_ = 0;
			wait_for_eof_ = false;
			return result;
		}

		if(wait_for_eof_)
			return result;
						
		int written_bytes = buffer_.size()-FF_INPUT_BUFFER_PADDING_SIZE/2;
		const int errn = avcodec_decode_audio3(&codec_context_, buffer_.data(), &written_bytes, audio_packet.get());
		if(errn < 0)
		{	
			BOOST_THROW_EXCEPTION(
				invalid_operation() <<
				boost::errinfo_api_function("avcodec_decode_audio2") <<
				boost::errinfo_errno(AVUNERROR(errn)));
		}

		current_chunk_.insert(current_chunk_.begin(), buffer_.begin(), buffer_.begin() + written_bytes/2);

		const auto last = current_chunk_.end() - current_chunk_.size() % format_desc_.audio_samples_per_frame;
		
		for(auto it = current_chunk_.begin(); it != last; it += format_desc_.audio_samples_per_frame)		
			result.push_back(std::make_pair(frame_number_++, std::vector<int16_t>(it, it + format_desc_.audio_samples_per_frame)));		

		current_chunk_.erase(current_chunk_.begin(), last);

		return result;
	}

	void restart()
	{
		wait_for_eof_ = true;
	}
};

audio_decoder::audio_decoder(input& input, const core::video_format_desc& format_desc) : impl_(new implementation(input, format_desc)){}
std::deque<std::pair<int, std::vector<int16_t>>> audio_decoder::receive(){return impl_->receive();}
void audio_decoder::restart(){impl_->restart();}
}