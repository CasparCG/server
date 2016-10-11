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

#include "frame_muxer.h"

#include "../filter/filter.h"
#include "../filter/audio_filter.h"
#include "../util/util.h"
#include "../../ffmpeg.h"

#include <core/producer/frame_producer.h>
#include <core/frame/draw_frame.h>
#include <core/frame/frame_transform.h>
#include <core/frame/pixel_format.h>
#include <core/frame/frame_factory.h>
#include <core/frame/frame.h>
#include <core/frame/audio_channel_layout.h>

#include <common/env.h>
#include <common/except.h>
#include <common/log.h>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C"
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

#include <common/assert.h>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/optional.hpp>

#include <deque>
#include <queue>
#include <vector>

using namespace caspar::core;

namespace caspar { namespace ffmpeg {

struct av_frame_format
{
	int										pix_format;
	std::array<int, AV_NUM_DATA_POINTERS>	line_sizes;
	int										width;
	int										height;

	av_frame_format(const AVFrame& frame)
		: pix_format(frame.format)
		, width(frame.width)
		, height(frame.height)
	{
		boost::copy(frame.linesize, line_sizes.begin());
	}

	bool operator==(const av_frame_format& other) const
	{
		return pix_format == other.pix_format
			&& line_sizes == other.line_sizes
			&& width == other.width
			&& height == other.height;
	}

	bool operator!=(const av_frame_format& other) const
	{
		return !(*this == other);
	}
};

std::unique_ptr<audio_filter> create_amerge_filter(std::vector<audio_input_pad> input_pads, const core::audio_channel_layout& layout)
{
	std::vector<audio_output_pad> output_pads;
	std::wstring amerge;

	output_pads.emplace_back(
			std::vector<int>			{ 48000 },
			std::vector<AVSampleFormat>	{ AVSampleFormat::AV_SAMPLE_FMT_S32 },
			std::vector<uint64_t>		{ static_cast<uint64_t>(av_get_default_channel_layout(layout.num_channels)) });

	if (input_pads.size() > 1)
	{
		for (int i = 0; i < input_pads.size(); ++i)
			amerge += L"[a:" + boost::lexical_cast<std::wstring>(i) + L"]";

		amerge += L"amerge=inputs=" + boost::lexical_cast<std::wstring>(input_pads.size());
	}

	std::wstring afilter;

	if (!amerge.empty())
	{
		afilter = amerge;
		afilter += L"[aout:0]";
	}

	return std::unique_ptr<audio_filter>(new audio_filter(input_pads, output_pads, u8(afilter)));
}

struct frame_muxer::impl : boost::noncopyable
{
	std::queue<std::queue<core::mutable_frame>>		video_streams_;
	std::queue<core::mutable_audio_buffer>			audio_streams_;
	std::queue<core::draw_frame>					frame_buffer_;
	display_mode									display_mode_				= display_mode::invalid;
	const boost::rational<int>						in_framerate_;
	const video_format_desc							format_desc_;
	const audio_channel_layout						audio_channel_layout_;

	std::vector<int>								audio_cadence_				= format_desc_.audio_cadence;

	spl::shared_ptr<core::frame_factory>			frame_factory_;
	boost::optional<av_frame_format>				previously_filtered_frame_;

	std::unique_ptr<filter>							filter_;
	const std::wstring								filter_str_;
	std::unique_ptr<audio_filter>					audio_filter_;
	const bool										multithreaded_filter_;
	bool											force_deinterlacing_		= env::properties().get(L"configuration.force-deinterlace", false);

	mutable boost::mutex							out_framerate_mutex_;
	boost::rational<int>							out_framerate_;

	impl(
			boost::rational<int> in_framerate,
			std::vector<audio_input_pad> audio_input_pads,
			const spl::shared_ptr<core::frame_factory>& frame_factory,
			const core::video_format_desc& format_desc,
			const core::audio_channel_layout& channel_layout,
			const std::wstring& filter_str,
			bool multithreaded_filter)
		: in_framerate_(in_framerate)
		, format_desc_(format_desc)
		, audio_channel_layout_(channel_layout)
		, frame_factory_(frame_factory)
		, filter_str_(filter_str)
		, multithreaded_filter_(multithreaded_filter)
	{
		video_streams_.push(std::queue<core::mutable_frame>());
		audio_streams_.push(core::mutable_audio_buffer());

		set_out_framerate(in_framerate_);

		if (!audio_input_pads.empty())
		{
			audio_filter_ = create_amerge_filter(std::move(audio_input_pads), audio_channel_layout_);
		}
	}

	void push(const std::shared_ptr<AVFrame>& video_frame)
	{
		if (!video_frame)
			return;

		av_frame_format current_frame_format(*video_frame);

		if (previously_filtered_frame_ && video_frame->data[0] && *previously_filtered_frame_ != current_frame_format)
		{
			// Fixes bug where avfilter crashes server on some DV files (starts in YUV420p but changes to YUV411p after the first frame).
			if (ffmpeg::is_logging_quiet_for_thread())
				CASPAR_LOG(debug) << L"[frame_muxer] Frame format has changed. Resetting display mode.";
			else
				CASPAR_LOG(info) << L"[frame_muxer] Frame format has changed. Resetting display mode.";

			display_mode_ = display_mode::invalid;
			filter_.reset();
			previously_filtered_frame_ = boost::none;
		}

		if (video_frame == flush_video())
		{
			video_streams_.push(std::queue<core::mutable_frame>());
		}
		else if (video_frame == empty_video())
		{
			video_streams_.back().push(frame_factory_->create_frame(this, core::pixel_format::invalid, audio_channel_layout_));
			display_mode_ = display_mode::simple;
		}
		else
		{
			if (!filter_ || display_mode_ == display_mode::invalid)
				update_display_mode(video_frame);

			if (filter_)
			{
				filter_->push(video_frame);
				previously_filtered_frame_ = current_frame_format;

				for (auto& av_frame : filter_->poll_all())
					video_streams_.back().push(make_frame(this, av_frame, *frame_factory_, audio_channel_layout_));
			}
		}

		if (video_streams_.back().size() > 32)
			CASPAR_THROW_EXCEPTION(invalid_operation() << source_info("frame_muxer") << msg_info("video-stream overflow. This can be caused by incorrect frame-rate. Check clip meta-data."));
	}

	void push(const std::vector<std::shared_ptr<core::mutable_audio_buffer>>& audio_samples_per_stream)
	{
		if (audio_samples_per_stream.empty())
			return;

		bool is_flush = boost::count_if(
				audio_samples_per_stream,
				[](std::shared_ptr<core::mutable_audio_buffer> a) { return a == flush_audio(); }) > 0;

		if (is_flush)
		{
			audio_streams_.push(core::mutable_audio_buffer());
		}
		else if (audio_samples_per_stream.at(0) == empty_audio())
		{
			boost::range::push_back(audio_streams_.back(), core::mutable_audio_buffer(audio_cadence_.front() * audio_channel_layout_.num_channels, 0));
		}
		else
		{
			for (int i = 0; i < audio_samples_per_stream.size(); ++i)
			{
				auto range = boost::make_iterator_range_n(
						audio_samples_per_stream.at(i)->data(),
						audio_samples_per_stream.at(i)->size());

				audio_filter_->push(i, range);
			}

			for (auto frame : audio_filter_->poll_all(0))
			{
				auto audio = boost::make_iterator_range_n(
						reinterpret_cast<std::int32_t*>(frame->extended_data[0]),
						frame->nb_samples * frame->channels);

				boost::range::push_back(audio_streams_.back(), audio);
			}
		}

		if (audio_streams_.back().size() > 32 * audio_cadence_.front() * audio_channel_layout_.num_channels)
			CASPAR_THROW_EXCEPTION(invalid_operation() << source_info("frame_muxer") << msg_info("audio-stream overflow. This can be caused by incorrect frame-rate. Check clip meta-data."));
	}

	bool video_ready() const
	{
		return video_streams_.size() > 1 || (video_streams_.size() >= audio_streams_.size() && video_ready2());
	}

	bool audio_ready() const
	{
		return audio_streams_.size() > 1 || (audio_streams_.size() >= video_streams_.size() && audio_ready2());
	}

	bool video_ready2() const
	{
		return video_streams_.front().size() >= 1;
	}

	bool audio_ready2() const
	{
		return audio_streams_.front().size() >= audio_cadence_.front() * audio_channel_layout_.num_channels;
	}

	core::draw_frame poll()
	{
		if (!frame_buffer_.empty())
		{
			auto frame = frame_buffer_.front();
			frame_buffer_.pop();
			return frame;
		}

		if (video_streams_.size() > 1 && audio_streams_.size() > 1 && (!video_ready2() || !audio_ready2()))
		{
			if (!video_streams_.front().empty() || !audio_streams_.front().empty())
				CASPAR_LOG(trace) << "Truncating: " << video_streams_.front().size() << L" video-frames, " << audio_streams_.front().size() << L" audio-samples.";

			video_streams_.pop();
			audio_streams_.pop();
		}

		if (!video_ready2() || !audio_ready2() || display_mode_ == display_mode::invalid)
			return core::draw_frame::empty();

		auto frame			= pop_video();
		frame.audio_data()	= pop_audio();

		frame_buffer_.push(core::draw_frame(std::move(frame)));

		return poll();
	}

	core::mutable_frame pop_video()
	{
		auto frame = std::move(video_streams_.front().front());
		video_streams_.front().pop();
		return frame;
	}

	core::mutable_audio_buffer pop_audio()
	{
		CASPAR_VERIFY(audio_streams_.front().size() >= audio_cadence_.front() * audio_channel_layout_.num_channels);

		auto begin	= audio_streams_.front().begin();
		auto end	= begin + (audio_cadence_.front() * audio_channel_layout_.num_channels);

		core::mutable_audio_buffer samples(begin, end);
		audio_streams_.front().erase(begin, end);

		boost::range::rotate(audio_cadence_, std::begin(audio_cadence_) + 1);

		return samples;
	}

	uint32_t calc_nb_frames(uint32_t nb_frames) const
	{
		uint64_t nb_frames2 = nb_frames;

		if(filter_ && filter_->is_double_rate()) // Take into account transformations in filter.
			nb_frames2 *= 2;

		return static_cast<uint32_t>(nb_frames2);
	}

	boost::rational<int> out_framerate() const
	{
		boost::lock_guard<boost::mutex> lock(out_framerate_mutex_);

		return out_framerate_;
	}
private:
	void update_display_mode(const std::shared_ptr<AVFrame>& frame)
	{
 		std::wstring filter_str = filter_str_;

		display_mode_ = display_mode::simple;

		auto mode = get_mode(*frame);

		if (filter::is_deinterlacing(filter_str_))
		{
			display_mode_ = display_mode::simple;
		}
		else if (mode != core::field_mode::progressive)
		{
			if (force_deinterlacing_)
			{
				display_mode_ = display_mode::deinterlace_bob;
			}
			else
			{
				bool output_also_interlaced = format_desc_.field_mode != core::field_mode::progressive;
				bool interlaced_output_compatible =
						output_also_interlaced
						&& (
								(frame->height == 480 && format_desc_.height == 486) // don't deinterlace for NTSC DV
								|| frame->height == format_desc_.height
						)
						&& in_framerate_ == format_desc_.framerate;

				display_mode_ = interlaced_output_compatible ? display_mode::simple : display_mode::deinterlace_bob;
			}
		}

		if (display_mode_ == display_mode::deinterlace_bob)
			filter_str = append_filter(filter_str, L"YADIF=1:-1");

		auto out_framerate = in_framerate_;

		if (filter::is_double_rate(filter_str))
			out_framerate *= 2;

		if (frame->height == 480) // NTSC DV
		{
			auto pad_str = L"PAD=" + boost::lexical_cast<std::wstring>(frame->width) + L":486:0:2:black";
			filter_str = append_filter(filter_str, pad_str);
		}

		filter_.reset (new filter(
				frame->width,
				frame->height,
				1 / in_framerate_,
				in_framerate_,
				boost::rational<int>(frame->sample_aspect_ratio.num, frame->sample_aspect_ratio.den),
				static_cast<AVPixelFormat>(frame->format),
				std::vector<AVPixelFormat>(),
				u8(filter_str)));

		set_out_framerate(out_framerate);

		auto in_fps = static_cast<double>(in_framerate_.numerator()) / static_cast<double>(in_framerate_.denominator());

		if (ffmpeg::is_logging_quiet_for_thread())
			CASPAR_LOG(debug) << L"[frame_muxer] " << display_mode_ << L" " << print_mode(frame->width, frame->height, in_fps, frame->interlaced_frame > 0);
		else
			CASPAR_LOG(info) << L"[frame_muxer] " << display_mode_ << L" " << print_mode(frame->width, frame->height, in_fps, frame->interlaced_frame > 0);
	}

	void merge()
	{
		while (video_ready() && audio_ready() && display_mode_ != display_mode::invalid)
		{
			auto frame1 = pop_video();
			frame1.audio_data() = pop_audio();

			frame_buffer_.push(core::draw_frame(std::move(frame1)));
		}
	}

	void set_out_framerate(boost::rational<int> out_framerate)
	{
		boost::lock_guard<boost::mutex> lock(out_framerate_mutex_);

		bool changed = out_framerate != out_framerate_;
		out_framerate_ = std::move(out_framerate);

		if (changed)
			update_audio_cadence();
	}

	void update_audio_cadence()
	{
		audio_cadence_ = find_audio_cadence(out_framerate_);

		// Note: Uses 1 step rotated cadence for 1001 modes (1602, 1602, 1601, 1602, 1601)
		// This cadence fills the audio mixer most optimally.
		boost::range::rotate(audio_cadence_, std::end(audio_cadence_) - 1);
	}
};

frame_muxer::frame_muxer(
		boost::rational<int> in_framerate,
		std::vector<audio_input_pad> audio_input_pads,
		const spl::shared_ptr<core::frame_factory>& frame_factory,
		const core::video_format_desc& format_desc,
		const core::audio_channel_layout& channel_layout,
		const std::wstring& filter,
		bool multithreaded_filter)
	: impl_(new impl(std::move(in_framerate), std::move(audio_input_pads), frame_factory, format_desc, channel_layout, filter, multithreaded_filter)){}
void frame_muxer::push(const std::shared_ptr<AVFrame>& video){impl_->push(video);}
void frame_muxer::push(const std::vector<std::shared_ptr<core::mutable_audio_buffer>>& audio_samples_per_stream){impl_->push(audio_samples_per_stream);}
core::draw_frame frame_muxer::poll(){return impl_->poll();}
uint32_t frame_muxer::calc_nb_frames(uint32_t nb_frames) const {return impl_->calc_nb_frames(nb_frames);}
bool frame_muxer::video_ready() const{return impl_->video_ready();}
bool frame_muxer::audio_ready() const{return impl_->audio_ready();}
boost::rational<int> frame_muxer::out_framerate() const { return impl_->out_framerate(); }
}}
