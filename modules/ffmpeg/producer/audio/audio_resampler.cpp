/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "../../StdAfx.h"

#include "audio_resampler.h"

#include <common/exception/exceptions.h>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#include <libavcodec/avcodec.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar { namespace ffmpeg {

struct audio_resampler::implementation
{	
	std::shared_ptr<ReSampleContext> resampler_;
	
	std::vector<int8_t, tbb::cache_aligned_allocator<int8_t>> copy_buffer_;
	std::vector<int8_t, tbb::cache_aligned_allocator<int8_t>> buffer2_;

	const size_t			output_channels_;
	const AVSampleFormat	output_sample_format_;

	const size_t			input_channels_;
	const AVSampleFormat	input_sample_format_;

	implementation(size_t output_channels, size_t input_channels, size_t output_sample_rate, size_t input_sample_rate, AVSampleFormat output_sample_format, AVSampleFormat input_sample_format)
		: output_channels_(output_channels)
		, output_sample_format_(output_sample_format)
		, input_channels_(input_channels)
		, input_sample_format_(input_sample_format)
	{
		if(input_channels		!= output_channels || 
		   input_sample_rate	!= output_sample_rate ||
		   input_sample_format	!= output_sample_format)
		{	
			auto resampler = av_audio_resample_init(output_channels,		input_channels,
													output_sample_rate,		input_sample_rate,
													output_sample_format,	input_sample_format,
													16, 10, 0, 0.8);

			buffer2_.resize(AVCODEC_MAX_AUDIO_FRAME_SIZE*2);

			char sample_fmt_string[200];
			av_get_sample_fmt_string(sample_fmt_string, 200, input_sample_format);

			CASPAR_LOG(warning) << L"[audio-resampler]"		
								<< L" sample-rate: "	<< input_sample_rate 
								<< L" channels: "		<< input_channels 
								<< L" sample-fmt: "		<< widen(sample_fmt_string);

			if(resampler)
				resampler_.reset(resampler, audio_resample_close);
			else
				BOOST_THROW_EXCEPTION(caspar_exception());
		}		
	}

	std::vector<int8_t, tbb::cache_aligned_allocator<int8_t>> resample(std::vector<int8_t, tbb::cache_aligned_allocator<int8_t>>&& data)
	{
		if(resampler_ && !data.empty())
		{
			buffer2_.resize(AVCODEC_MAX_AUDIO_FRAME_SIZE*2);
			auto ret = audio_resample(resampler_.get(),
									  reinterpret_cast<short*>(buffer2_.data()), 
									  reinterpret_cast<short*>(data.data()), 
									  data.size() / (av_get_bytes_per_sample(input_sample_format_) * input_channels_)); 
			buffer2_.resize(ret * av_get_bytes_per_sample(output_sample_format_) * output_channels_);
			std::swap(data, buffer2_);
		}

		return std::move(data);
	}
};


audio_resampler::audio_resampler(size_t output_channels, size_t input_channels, size_t output_sample_rate, size_t input_sample_rate, AVSampleFormat output_sample_format, AVSampleFormat input_sample_format)
				: impl_(new implementation(output_channels, input_channels, output_sample_rate, input_sample_rate, output_sample_format, input_sample_format)){}
std::vector<int8_t, tbb::cache_aligned_allocator<int8_t>> audio_resampler::resample(std::vector<int8_t, tbb::cache_aligned_allocator<int8_t>>&& data){return impl_->resample(std::move(data));}

}}