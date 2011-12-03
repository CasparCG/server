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