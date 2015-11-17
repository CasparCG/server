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

#include <deque>
#include <queue>
#include <vector>

using namespace caspar::core;

namespace caspar { namespace ffmpeg {

bool is_frame_format_changed(const AVFrame& lhs, const AVFrame& rhs)
{
	if (lhs.format != rhs.format)
		return true;

	for (int i = 0; i < AV_NUM_DATA_POINTERS; ++i)
	{
		if (lhs.linesize[i] != rhs.linesize[i])
			return true;
	}

	return false;
}
	
struct frame_muxer::impl : boost::noncopyable
{	
	std::queue<core::mutable_frame>					video_stream_;
	core::mutable_audio_buffer						audio_stream_;
	std::queue<draw_frame>							frame_buffer_;
	display_mode									display_mode_			= display_mode::invalid;
	const double									in_fps_;
	const video_format_desc							format_desc_;
	audio_channel_layout							channel_layout_;
	
	std::vector<int>								audio_cadence_			= format_desc_.audio_cadence;
			
	spl::shared_ptr<core::frame_factory>			frame_factory_;
	std::shared_ptr<AVFrame>						previous_frame_;

	std::unique_ptr<filter>							filter_;
	const std::wstring								filter_str_;
	bool											force_deinterlacing_	= env::properties().get(L"configuration.force-deinterlace", true);
		
	impl(
			double in_fps,
			const spl::shared_ptr<core::frame_factory>& frame_factory,
			const core::video_format_desc& format_desc,
			const core::audio_channel_layout& channel_layout,
			const std::wstring& filter_str)
		: in_fps_(in_fps)
		, format_desc_(format_desc)
		, channel_layout_(channel_layout)
		, frame_factory_(frame_factory)
		, filter_str_(filter_str)
	{		
		// Note: Uses 1 step rotated cadence for 1001 modes (1602, 1602, 1601, 1602, 1601)
		// This cadence fills the audio mixer most optimally.
		boost::range::rotate(audio_cadence_, std::end(audio_cadence_)-1);
	}
	
	void push_video(const std::shared_ptr<AVFrame>& video)
	{		
		if(!video)
			return;

		if (previous_frame_ && video->data[0] && is_frame_format_changed(*previous_frame_, *video))
		{
			// Fixes bug where avfilter crashes server on some DV files (starts in YUV420p but changes to YUV411p after the first frame).
			if (ffmpeg::is_logging_quiet_for_thread())
				CASPAR_LOG(debug) << L"[frame_muxer] Frame format has changed. Resetting display mode.";
			else
				CASPAR_LOG(info) << L"[frame_muxer] Frame format has changed. Resetting display mode.";

			display_mode_ = display_mode::invalid;
		}

		if(!video->data[0])
		{
			auto empty_frame = frame_factory_->create_frame(this, core::pixel_format_desc(core::pixel_format::invalid), channel_layout_);
			video_stream_.push(std::move(empty_frame));
			display_mode_ = display_mode::simple;
		}
		else
		{
			if(!filter_ || display_mode_ == display_mode::invalid)
				update_display_mode(video);
				
			filter_->push(video);
			previous_frame_ = video;
			for (auto& av_frame : filter_->poll_all())
				video_stream_.push(make_frame(this, av_frame, format_desc_.fps, *frame_factory_, channel_layout_));
		}

		merge();
	}

	void push_audio(const std::shared_ptr<AVFrame>& audio)
	{
		if(!audio)
			return;

		if(!audio->data[0])		
		{
			if (channel_layout_ == core::audio_channel_layout::invalid())
				channel_layout_ = *core::audio_channel_layout_repository::get_default()->get_layout(L"stereo");

			boost::range::push_back(audio_stream_, core::mutable_audio_buffer(audio_cadence_.front() * channel_layout_.num_channels, 0));
		}
		else
		{
			auto ptr = reinterpret_cast<int32_t*>(audio->data[0]);
			audio_stream_.insert(audio_stream_.end(), ptr, ptr + audio->linesize[0]/sizeof(int32_t));
		}

		merge();
	}
	
	bool video_ready() const
	{
		switch(display_mode_)
		{
		case display_mode::deinterlace_bob_reinterlace:					
		case display_mode::interlace:	
		case display_mode::half:
			return video_stream_.size() >= 2;
		default:										
			return video_stream_.size() >= 1;
		}
	}
	
	bool audio_ready() const
	{
		switch(display_mode_)
		{
		case display_mode::duplicate:					
			return audio_stream_.size() >= static_cast<size_t>(audio_cadence_[0] + audio_cadence_[1 % audio_cadence_.size()]) * channel_layout_.num_channels;
		default:										
			return audio_stream_.size() >= static_cast<size_t>(audio_cadence_.front()) * channel_layout_.num_channels;
		}
	}

	bool empty() const
	{
		return frame_buffer_.empty();
	}

	core::draw_frame front() const
	{
		return frame_buffer_.front();
	}

	void pop()
	{
		frame_buffer_.pop();
	}
		
	void merge()
	{
		while(video_ready() && audio_ready() && display_mode_ != display_mode::invalid)
		{				
			auto frame1			= pop_video();
			frame1.audio_data()	= pop_audio();

			switch(display_mode_)
			{
			case display_mode::simple:						
			case display_mode::deinterlace_bob:				
			case display_mode::deinterlace:	
				{
					frame_buffer_.push(core::draw_frame(std::move(frame1)));
					break;
				}
			case display_mode::interlace:					
			case display_mode::deinterlace_bob_reinterlace:	
				{				
					auto frame2 = pop_video();

					frame_buffer_.push(core::draw_frame::interlace(
						core::draw_frame(std::move(frame1)),
						core::draw_frame(std::move(frame2)),
						format_desc_.field_mode));	
					break;
				}
			case display_mode::duplicate:	
				{
					//boost::range::push_back(frame1.audio_data(), pop_audio());

					auto second_audio_frame = core::mutable_frame(
							std::vector<array<std::uint8_t>>(),
							pop_audio(),
							frame1.stream_tag(),
							core::pixel_format_desc(),
							channel_layout_);
					auto first_frame = core::draw_frame(std::move(frame1));
					auto muted_first_frame = core::draw_frame(first_frame);
					muted_first_frame.transform().audio_transform.volume = 0;
					auto second_frame = core::draw_frame({ core::draw_frame(std::move(second_audio_frame)), muted_first_frame });

					// Same video but different audio.
					frame_buffer_.push(first_frame);
					frame_buffer_.push(second_frame);
					break;
				}
			case display_mode::half:	
				{				
					pop_video(); // Throw away

					frame_buffer_.push(core::draw_frame(std::move(frame1)));
					break;
				}
			default:
				CASPAR_THROW_EXCEPTION(invalid_operation());
			}
		}
	}
	
	core::mutable_frame pop_video()
	{
		auto frame = std::move(video_stream_.front());
		video_stream_.pop();		
		return std::move(frame);
	}

	core::mutable_audio_buffer pop_audio()
	{
		if (audio_stream_.size() < audio_cadence_.front() * channel_layout_.num_channels)
			CASPAR_THROW_EXCEPTION(out_of_range());

		auto begin = audio_stream_.begin();
		auto end   = begin + audio_cadence_.front() * channel_layout_.num_channels;

		core::mutable_audio_buffer samples(begin, end);
		audio_stream_.erase(begin, end);
		
		boost::range::rotate(audio_cadence_, std::begin(audio_cadence_)+1);

		return samples;
	}
				
	void update_display_mode(const std::shared_ptr<AVFrame>& frame)
	{
		std::wstring filter_str = filter_str_;

		display_mode_ = display_mode::simple;

		auto mode = get_mode(*frame);
		if(mode == core::field_mode::progressive && frame->height < 720 && in_fps_ < 50.0) // SD frames are interlaced. Probably incorrect meta-data. Fix it.
			mode = core::field_mode::upper;

		auto fps  = in_fps_;

		if(filter::is_deinterlacing(filter_str_))
			mode = core::field_mode::progressive;

		if(filter::is_double_rate(filter_str_))
			fps *= 2;
			
		display_mode_ = get_display_mode(mode, fps, format_desc_.field_mode, format_desc_.fps);
			
		if((frame->height != 480 || format_desc_.height != 486) && // don't deinterlace for NTSC DV
				display_mode_ == display_mode::simple && mode != core::field_mode::progressive && format_desc_.field_mode != core::field_mode::progressive && 
				frame->height != format_desc_.height)
		{
			display_mode_ = display_mode::deinterlace_bob_reinterlace; // The frame will most likely be scaled, we need to deinterlace->reinterlace	
		}

		// ALWAYS de-interlace, until we have GPU de-interlacing.
		if(force_deinterlacing_ && frame->interlaced_frame && display_mode_ != display_mode::deinterlace_bob && display_mode_ != display_mode::deinterlace)
			display_mode_ = display_mode::deinterlace_bob_reinterlace;
		
		if(display_mode_ == display_mode::deinterlace)
			filter_str = append_filter(filter_str, L"YADIF=0:-1");
		else if(display_mode_ == display_mode::deinterlace_bob || display_mode_ == display_mode::deinterlace_bob_reinterlace)
			filter_str = append_filter(filter_str, L"YADIF=1:-1");

		if(display_mode_ == display_mode::invalid)
		{
			if (ffmpeg::is_logging_quiet_for_thread())
				CASPAR_LOG(debug) << L"[frame_muxer] Auto-transcode: Failed to detect display-mode.";
			else
				CASPAR_LOG(warning) << L"[frame_muxer] Auto-transcode: Failed to detect display-mode.";

			display_mode_ = display_mode::simple;
		}

		if(frame->height == 480) // NTSC DV
		{
			auto pad_str = L"PAD=" + boost::lexical_cast<std::wstring>(frame->width) + L":486:0:2:black";
			filter_str = append_filter(filter_str, pad_str);
		}

		filter_.reset (new filter(
			frame->width,
			frame->height,
			boost::rational<int>(1000000, static_cast<int>(in_fps_ * 1000000)),
			boost::rational<int>(static_cast<int>(in_fps_ * 1000000), 1000000),
			boost::rational<int>(frame->sample_aspect_ratio.num, frame->sample_aspect_ratio.den),
			static_cast<AVPixelFormat>(frame->format),
			std::vector<AVPixelFormat>(),
			u8(filter_str)));

		if (ffmpeg::is_logging_quiet_for_thread())
			CASPAR_LOG(debug) << L"[frame_muxer] " << display_mode_ << L" " << print_mode(frame->width, frame->height, in_fps_, frame->interlaced_frame > 0);
		else
			CASPAR_LOG(info) << L"[frame_muxer] " << display_mode_ << L" " << print_mode(frame->width, frame->height, in_fps_, frame->interlaced_frame > 0);
	}
	
	uint32_t calc_nb_frames(uint32_t nb_frames) const
	{
		uint64_t nb_frames2 = nb_frames;
		
		if(filter_ && filter_->is_double_rate()) // Take into account transformations in filter.
			nb_frames2 *= 2;

		switch(display_mode_) // Take into account transformation in run.
		{
		case display_mode::deinterlace_bob_reinterlace:
		case display_mode::interlace:	
		case display_mode::half:
			nb_frames2 /= 2;
			break;
		case display_mode::duplicate:
			nb_frames2 *= 2;
			break;
		}

		return static_cast<uint32_t>(nb_frames2);
	}

	void clear()
	{
		while(!video_stream_.empty())
			video_stream_.pop();	

		audio_stream_.clear();

		while(!frame_buffer_.empty())
			frame_buffer_.pop();
		
		filter_.reset();
	}
};

frame_muxer::frame_muxer(
		double in_fps,
		const spl::shared_ptr<core::frame_factory>& frame_factory,
		const core::video_format_desc& format_desc,
		const core::audio_channel_layout& channel_layout,
		const std::wstring& filter)
	: impl_(new impl(in_fps, frame_factory, format_desc, channel_layout, filter)){}
void frame_muxer::push_video(const std::shared_ptr<AVFrame>& frame){impl_->push_video(frame);}
void frame_muxer::push_audio(const std::shared_ptr<AVFrame>& frame){impl_->push_audio(frame);}
bool frame_muxer::empty() const{return impl_->empty();}
core::draw_frame frame_muxer::front() const{return impl_->front();}
void frame_muxer::pop(){return impl_->pop();}
void frame_muxer::clear(){impl_->clear();}
uint32_t frame_muxer::calc_nb_frames(uint32_t nb_frames) const {return impl_->calc_nb_frames(nb_frames);}
bool frame_muxer::video_ready() const{return impl_->video_ready();}
bool frame_muxer::audio_ready() const{return impl_->audio_ready();}

}}