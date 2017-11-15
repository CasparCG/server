/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
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

#include <common/memory.h>

#include <core/fwd.h>

#include <boost/rational.hpp>
#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <string>
#include <vector>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C"
{
#include <libavutil/samplefmt.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

struct AVFrame;
struct AVFilterLink;

namespace caspar { namespace ffmpeg {

struct audio_input_pad
{
	boost::rational<int>	time_base;
	int						sample_rate;
	AVSampleFormat			sample_fmt;
	std::uint64_t			audio_channel_layout;

	audio_input_pad(
			boost::rational<int> time_base,
			int sample_rate,
			AVSampleFormat sample_fmt,
			std::uint64_t audio_channel_layout)
		: time_base(std::move(time_base))
		, sample_rate(sample_rate)
		, sample_fmt(sample_fmt)
		, audio_channel_layout(audio_channel_layout)
	{
	}
};

struct audio_output_pad
{
	std::vector<int>			sample_rates;
	std::vector<AVSampleFormat>	sample_fmts;
	std::vector<std::uint64_t>	audio_channel_layouts;

	audio_output_pad(
			std::vector<int> sample_rates,
			std::vector<AVSampleFormat> sample_fmts,
			std::vector<std::uint64_t> audio_channel_layouts)
		: sample_rates(std::move(sample_rates))
		, sample_fmts(std::move(sample_fmts))
		, audio_channel_layouts(std::move(audio_channel_layouts))
	{
	}
};

class audio_filter : boost::noncopyable
{
public:
	audio_filter(
			std::vector<audio_input_pad> input_pads,
			std::vector<audio_output_pad> output_pads,
			const std::string& filtergraph);
	audio_filter(audio_filter&& other);
	audio_filter& operator=(audio_filter&& other);

	void set_guaranteed_output_num_samples_per_frame(int output_pad_id, int num_samples);
	void push(int input_pad_id, const std::shared_ptr<AVFrame>& frame);
	void push(int input_pad_id, const boost::iterator_range<const int32_t*>& frame_samples);
	std::shared_ptr<AVFrame> poll(int output_pad_id);
	std::vector<spl::shared_ptr<AVFrame>> poll_all(int output_pad_id);

	std::wstring filter_str() const;
	int get_num_output_pads() const;
	const AVFilterLink& get_output_pad_info(int output_pad_id) const;
private:
	struct implementation;
	spl::shared_ptr<implementation> impl_;
};

audio_input_pad create_input_pad(const core::video_format_desc& in_format, int num_channels);

}}
