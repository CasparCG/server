#pragma once

#include <memory>

#include <libavutil/samplefmt.h>

namespace caspar { namespace ffmpeg {

class audio_resampler
{
public:
	audio_resampler(size_t			output_channels,		size_t			input_channels, 
					size_t			output_sample_rate,		size_t			input_sample_rate, 
					AVSampleFormat	output_sample_format,	AVSampleFormat	input_sample_format);
	
	std::vector<int8_t, tbb::cache_aligned_allocator<int8_t>> resample(std::vector<int8_t, tbb::cache_aligned_allocator<int8_t>>&& data);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}