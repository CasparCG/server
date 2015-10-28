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
* Author: Helge Norberg, helge.norberg@svt.se
*/

#include "StdAfx.h"

#include "ffmpeg.h"

#include <core/frame/audio_channel_layout.h>

#include <common/except.h>
#include <common/assert.h>

#include "producer/filter/audio_filter.h"
#include "producer/util/util.h"

#include <asmlib.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/format.hpp>

#include <cstdint>
#include <sstream>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C"
{
#include <libavfilter/avfilter.h>
#include <libavutil/channel_layout.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar { namespace core {

std::wstring generate_pan_filter_str(
		const audio_channel_layout& input,
		const audio_channel_layout& output,
		boost::optional<std::wstring> mix_config)
{
	std::wstringstream result;

	result << L"pan=" << (boost::wformat(L"0x%|1$x|") % ffmpeg::create_channel_layout_bitmask(output.num_channels)).str();

	if (!mix_config)
	{
		if (input.type == output.type && !input.channel_order.empty() && !input.channel_order.empty())
		{	// No config needed because the layouts are of the same type. Generate mix config string.
			std::vector<std::wstring> mappings;

			for (auto& input_name : input.channel_order)
				mappings.push_back(input_name + L"=" + input_name);

			mix_config = boost::join(mappings, L"|");
		}
		else
		{	// Fallback to passthru c0=c0| c1=c1 | ...
			for (int i = 0; i < output.num_channels; ++i)
				result << L"|c" << i << L"=c" << i;

			CASPAR_LOG(trace) << "[audio_channel_remapper] Passthru " << input.num_channels << " channels into " << output.num_channels;

			return result.str();
		}
	}

	CASPAR_LOG(trace) << L"[audio_channel_remapper] Using mix config: " << *mix_config;

	// Split on | to find the output sections
	std::vector<std::wstring> output_sections;
	boost::split(output_sections, *mix_config, boost::is_any_of(L"|"), boost::algorithm::token_compress_off);

	for (auto& output_section : output_sections)
	{
		bool normalize_ratios = boost::contains(output_section, L"<");
		std::wstring mix_char = normalize_ratios ? L"<" : L"=";

		// Split on either = or < to get the output name and mix spec
		std::vector<std::wstring> output_and_spec;
		boost::split(output_and_spec, output_section, boost::is_any_of(mix_char), boost::algorithm::token_compress_off);
		auto& mix_spec = output_and_spec.at(1);

		// Replace each occurance of each channel name with c<index>
		for (int i = 0; i < input.channel_order.size(); ++i)
			boost::replace_all(mix_spec, input.channel_order.at(i), L"c" + boost::lexical_cast<std::wstring>(i));

		auto output_name = boost::trim_copy(output_and_spec.at(0));
		auto actual_output_indexes = output.indexes_of(output_name);

		for (auto actual_output_index : actual_output_indexes)
		{
			result << L"|c" << actual_output_index << L" " << mix_char;
			result << mix_spec;
		}
	}

	return result.str();
}

struct audio_channel_remapper::impl
{
	const audio_channel_layout				input_layout_;
	const audio_channel_layout				output_layout_;
	const bool								the_same_layouts_	= input_layout_ == output_layout_;
	std::unique_ptr<ffmpeg::audio_filter>	filter_;

	impl(
			audio_channel_layout input_layout,
			audio_channel_layout output_layout,
			spl::shared_ptr<audio_mix_config_repository> mix_repo)
		: input_layout_(std::move(input_layout))
		, output_layout_(std::move(output_layout))
	{
		if (input_layout_ == audio_channel_layout::invalid())
			CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info(L"Input audio channel layout is invalid"));

		if (output_layout_ == audio_channel_layout::invalid())
			CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info(L"Output audio channel layout is invalid"));

		CASPAR_LOG(trace) << L"[audio_channel_remapper] Input:  " << input_layout_.print();
		CASPAR_LOG(trace) << L"[audio_channel_remapper] Output: " << output_layout_.print();

		if (!the_same_layouts_)
		{
			auto mix_config = mix_repo->get_config(input_layout_.type, output_layout_.type);
			auto pan_filter = u8(generate_pan_filter_str(input_layout_, output_layout_, mix_config));

			CASPAR_LOG(trace) << "[audio_channel_remapper] Using audio filter: " << pan_filter;
			auto logging_disabled = ffmpeg::temporary_disable_logging_for_thread(true);
			filter_.reset(new ffmpeg::audio_filter(
					boost::rational<int>(1, 1),
					48000,
					AV_SAMPLE_FMT_S32,
					ffmpeg::create_channel_layout_bitmask(input_layout_.num_channels),
					{ 48000 },
					{ AV_SAMPLE_FMT_S32 },
					{ ffmpeg::create_channel_layout_bitmask(output_layout_.num_channels) },
					pan_filter));
		}
		else
			CASPAR_LOG(trace) << "[audio_channel_remapper] No remapping/mixing needed because the input and output layout is equal.";
	}

	audio_buffer mix_and_rearrange(audio_buffer input)
	{
		CASPAR_ENSURE(input.size() % input_layout_.num_channels == 0);

		if (the_same_layouts_)
			return std::move(input);

		auto num_samples			=	input.size() / input_layout_.num_channels;
		auto expected_output_size	=	num_samples * output_layout_.num_channels;
		auto input_frame			=	std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* p)
										{
											if (p)
												av_frame_free(&p);
										});

		input_frame->channels		=	input_layout_.num_channels;
		input_frame->channel_layout	=	ffmpeg::create_channel_layout_bitmask(input_layout_.num_channels);
		input_frame->sample_rate	=	48000;
		input_frame->nb_samples		=	static_cast<int>(num_samples);
		input_frame->format			=	AV_SAMPLE_FMT_S32;
		input_frame->pts			=	0;

		av_samples_fill_arrays(
				input_frame->extended_data,
				input_frame->linesize,
				reinterpret_cast<const std::uint8_t*>(input.data()),
				input_frame->channels,
				input_frame->nb_samples,
				static_cast<AVSampleFormat>(input_frame->format),
				16);

		filter_->push(input_frame);

		auto frames = filter_->poll_all();

		CASPAR_ENSURE(frames.size() == 1); // Expect 1:1 from pan filter

		auto& frame = frames.front();
		auto output_size = frame->channels * frame->nb_samples;

		CASPAR_ENSURE(output_size == expected_output_size);

		return audio_buffer(
				reinterpret_cast<std::int32_t*>(frame->extended_data[0]),
				output_size,
				true,
				std::move(frame));
	}
};

audio_channel_remapper::audio_channel_remapper(
		audio_channel_layout input_layout,
		audio_channel_layout output_layout,
		spl::shared_ptr<audio_mix_config_repository> mix_repo)
	: impl_(new impl(std::move(input_layout), std::move(output_layout), std::move(mix_repo)))
{
}

audio_buffer audio_channel_remapper::mix_and_rearrange(audio_buffer input)
{
	return impl_->mix_and_rearrange(std::move(input));
}

}}
