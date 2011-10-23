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

			CASPAR_LOG(warning) << L"Resampling." <<
									L" sample_rate:" << input_channels  <<
									L" audio_channels:" << input_channels  <<
									L" sample_fmt:" << input_sample_format;

			CASPAR_VERIFY(resampler, caspar_exception());

			resampler_.reset(resampler, audio_resample_close);
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