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

#include "../StdAfx.h"

#include "ffmpeg_producer.h"

#include "../ffmpeg_error.h"

#include "muxer/frame_muxer.h"
#include "input/input.h"
#include "util/util.h"
#include "audio/audio_decoder.h"
#include "video/video_decoder.h"

#include <common/env.h>
#include <common/log.h>
#include <common/param.h>
#include <common/diagnostics/graph.h>
#include <common/future.h>
#include <common/timer.h>
#include <common/assert.h>

#include <core/video_format.h>
#include <core/producer/frame_producer.h>
#include <core/frame/audio_channel_layout.h>
#include <core/frame/frame_factory.h>
#include <core/frame/draw_frame.h>
#include <core/frame/frame_transform.h>
#include <core/monitor/monitor.h>
#include <core/help/help_repository.h>
#include <core/help/help_sink.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/regex.hpp>
#include <boost/thread/future.hpp>

#include <tbb/parallel_invoke.h>

#include <limits>
#include <memory>
#include <queue>

namespace caspar { namespace ffmpeg {

std::wstring get_relative_or_original(
		const std::wstring& filename,
		const boost::filesystem::path& relative_to)
{
	boost::filesystem::path file(filename);
	auto result = file.filename().wstring();

	boost::filesystem::path current_path = file;

	while (true)
	{
		current_path = current_path.parent_path();

		if (boost::filesystem::equivalent(current_path, relative_to))
			break;

		if (current_path.empty())
			return filename;

		result = current_path.filename().wstring() + L"/" + result;
	}

	return result;
}

struct ffmpeg_producer : public core::frame_producer_base
{
	spl::shared_ptr<core::monitor::subject>			monitor_subject_;
	const std::wstring								filename_;
	const std::wstring								path_relative_to_media_	= get_relative_or_original(filename_, env::media_folder());
	
	const spl::shared_ptr<diagnostics::graph>		graph_;
					
	const spl::shared_ptr<core::frame_factory>		frame_factory_;
	const core::video_format_desc					format_desc_;

	input											input_;	

	const double									fps_					= read_fps(input_.context(), format_desc_.fps);
	const uint32_t									start_;
		
	std::unique_ptr<video_decoder>					video_decoder_;
	std::unique_ptr<audio_decoder>					audio_decoder_;	
	std::unique_ptr<frame_muxer>					muxer_;
	core::constraints								constraints_;
	
	core::draw_frame								last_frame_				= core::draw_frame::empty();

	boost::optional<uint32_t>						seek_target_;
	
public:
	explicit ffmpeg_producer(
			const spl::shared_ptr<core::frame_factory>& frame_factory, 
			const core::video_format_desc& format_desc,
			const std::wstring& channel_layout_spec,
			const std::wstring& filename,
			const std::wstring& filter,
			bool loop,
			uint32_t start,
			uint32_t length)
		: filename_(filename)
		, frame_factory_(frame_factory)		
		, format_desc_(format_desc)
		, input_(graph_, filename_, loop, start, length)
		, fps_(read_fps(input_.context(), format_desc_.fps))
		, start_(start)
	{
		graph_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
		graph_->set_color("underflow", diagnostics::color(0.6f, 0.3f, 0.9f));	
		diagnostics::register_graph(graph_);
		

		try
		{
			video_decoder_.reset(new video_decoder(input_));
			video_decoder_->monitor_output().attach_parent(monitor_subject_);
			constraints_.width.set(video_decoder_->width());
			constraints_.height.set(video_decoder_->height());
			
			CASPAR_LOG(info) << print() << L" " << video_decoder_->print();
		}
		catch(averror_stream_not_found&)
		{
			//CASPAR_LOG(warning) << print() << " No video-stream found. Running without video.";	
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			CASPAR_LOG(warning) << print() << "Failed to open video-stream. Running without video.";	
		}

		auto channel_layout = core::audio_channel_layout::invalid();

		try
		{
			audio_decoder_ .reset(new audio_decoder(input_, format_desc_, channel_layout_spec));
			audio_decoder_->monitor_output().attach_parent(monitor_subject_);

			channel_layout = audio_decoder_->channel_layout();
			
			CASPAR_LOG(info) << print() << L" " << audio_decoder_->print();
		}
		catch(averror_stream_not_found&)
		{
			//CASPAR_LOG(warning) << print() << " No audio-stream found. Running without audio.";	
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			CASPAR_LOG(warning) << print() << " Failed to open audio-stream. Running without audio.";		
		}

		muxer_.reset(new frame_muxer(fps_, frame_factory, format_desc_, channel_layout, filter));
		
		decode_next_frame();

		CASPAR_LOG(info) << print() << L" Initialized";
	}

	// frame_producer
	
	core::draw_frame receive_impl() override
	{				
		auto frame = core::draw_frame::late();		
		
		caspar::timer frame_timer;
		
		end_seek();
				
		decode_next_frame();
		
		if(!muxer_->empty())
		{
			last_frame_ = frame = std::move(muxer_->front());
			muxer_->pop();
		}
		else
			graph_->set_tag("underflow");
									
		graph_->set_value("frame-time", frame_timer.elapsed()*format_desc_.fps*0.5);
		*monitor_subject_
				<< core::monitor::message("/profiler/time")	% frame_timer.elapsed() % (1.0/format_desc_.fps);			
		*monitor_subject_
				<< core::monitor::message("/file/frame")	% static_cast<int32_t>(file_frame_number())
															% static_cast<int32_t>(file_nb_frames())
				<< core::monitor::message("/file/fps")		% fps_
				<< core::monitor::message("/file/path")		% path_relative_to_media_
				<< core::monitor::message("/loop")			% input_.loop();
						
		return frame;
	}

	core::draw_frame last_frame() override
	{
		end_seek();
		return core::draw_frame::still(last_frame_);
	}

	core::constraints& pixel_constraints() override
	{
		return constraints_;
	}

	uint32_t nb_frames() const override
	{
		if(input_.loop())
			return std::numeric_limits<uint32_t>::max();

		uint32_t nb_frames = file_nb_frames();

		nb_frames = std::min(input_.length(), nb_frames);
		nb_frames = muxer_->calc_nb_frames(nb_frames);
		
		return nb_frames > start_ ? nb_frames - start_ : 0;
	}

	uint32_t file_nb_frames() const
	{
		uint32_t file_nb_frames = 0;
		file_nb_frames = std::max(file_nb_frames, video_decoder_ ? video_decoder_->nb_frames() : 0);
		file_nb_frames = std::max(file_nb_frames, audio_decoder_ ? audio_decoder_->nb_frames() : 0);
		return file_nb_frames;
	}

	uint32_t file_frame_number() const
	{
		return video_decoder_ ? video_decoder_->file_frame_number() : 0;
	}
		
	std::future<std::wstring> call(const std::vector<std::wstring>& params) override
	{
		static const boost::wregex loop_exp(LR"(LOOP\s*(?<VALUE>\d?)?)", boost::regex::icase);
		static const boost::wregex seek_exp(LR"(SEEK\s+(?<VALUE>\d+))", boost::regex::icase);
		static const boost::wregex length_exp(LR"(LENGTH\s+(?<VALUE>\d+)?)", boost::regex::icase);
		static const boost::wregex start_exp(LR"(START\\s+(?<VALUE>\\d+)?)", boost::regex::icase);

		auto param = boost::algorithm::join(params, L" ");
		
		std::wstring result;
			
		boost::wsmatch what;
		if(boost::regex_match(param, what, loop_exp))
		{
			auto value = what["VALUE"].str();
			if(!value.empty())
				input_.loop(boost::lexical_cast<bool>(value));
			result = boost::lexical_cast<std::wstring>(loop());
		}
		else if(boost::regex_match(param, what, seek_exp))
		{
			auto value = what["VALUE"].str();
			seek(boost::lexical_cast<uint32_t>(value));
		}
		else if(boost::regex_match(param, what, length_exp))
		{
			auto value = what["VALUE"].str();
			if(!value.empty())
				length(boost::lexical_cast<uint32_t>(value));			
			result = boost::lexical_cast<std::wstring>(length());
		}
		else if(boost::regex_match(param, what, start_exp))
		{
			auto value = what["VALUE"].str();
			if(!value.empty())
				start(boost::lexical_cast<uint32_t>(value));
			result = boost::lexical_cast<std::wstring>(start());
		}
		else
			CASPAR_THROW_EXCEPTION(invalid_argument());

		return make_ready_future(std::move(result));
	}
				
	std::wstring print() const override
	{
		return L"ffmpeg[" + boost::filesystem::path(filename_).filename().wstring() + L"|" 
						  + print_mode() + L"|" 
						  + boost::lexical_cast<std::wstring>(file_frame_number()) + L"/" + boost::lexical_cast<std::wstring>(file_nb_frames()) + L"]";
	}

	std::wstring name() const override
	{
		return L"ffmpeg";
	}

	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type",				L"ffmpeg");
		info.add(L"filename",			filename_);
		info.add(L"width",				video_decoder_ ? video_decoder_->width() : 0);
		info.add(L"height",				video_decoder_ ? video_decoder_->height() : 0);
		info.add(L"progressive",		video_decoder_ ? video_decoder_->is_progressive() : 0);
		info.add(L"fps",				fps_);
		info.add(L"loop",				input_.loop());
		info.add(L"frame-number",		frame_number());
		auto nb_frames2 = nb_frames();
		info.add(L"nb-frames",			nb_frames2 == std::numeric_limits<int64_t>::max() ? -1 : nb_frames2);
		info.add(L"file-frame-number",	file_frame_number());
		info.add(L"file-nb-frames",		file_nb_frames());
		return info;
	}
	
	core::monitor::subject& monitor_output()
	{
		return *monitor_subject_;
	}

	// ffmpeg_producer
	
	void end_seek()
	{
		for(int n = 0; n < 8 && (last_frame_ == core::draw_frame::empty() || (seek_target_ && file_frame_number() != *seek_target_+2)); ++n)
		{
			decode_next_frame();
			if(!muxer_->empty())
			{
				last_frame_ = muxer_->front();
				seek_target_.reset();
			}
		}
	}

	void loop(bool value)
	{
		input_.loop(value);
	}

	bool loop() const
	{
		return input_.loop();
	}

	void length(uint32_t value)
	{
		input_.length(value);
	}

	uint32_t length()
	{
		return input_.length();
	}
	
	void start(uint32_t value)
	{
		input_.start(value);
	}

	uint32_t start()
	{
		return input_.start();
	}

	void seek(uint32_t target)
	{		
		seek_target_ = std::min(target, file_nb_frames());

		input_.seek(*seek_target_);
		muxer_->clear();
	}

	std::wstring print_mode() const
	{
		return ffmpeg::print_mode(video_decoder_ ? video_decoder_->width() : 0, 
			video_decoder_ ? video_decoder_->height() : 0, 
			fps_, 
			video_decoder_ ? !video_decoder_->is_progressive() : false);
	}
			
	void decode_next_frame()
	{
		for(int n = 0; n < 32 && muxer_->empty(); ++n)
		{
			if(!muxer_->video_ready())
				muxer_->push_video(video_decoder_ ? (*video_decoder_)() : create_frame());
			if(!muxer_->audio_ready())
				muxer_->push_audio(audio_decoder_ ? (*audio_decoder_)() : create_frame());
		}

		graph_->set_text(print());
	}
};

void describe_producer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"A producer for playing media files supported by FFmpeg.");
	sink.syntax(L"[clip:string] {[loop:LOOP]} {START,SEEK [start:int]} {LENGTH [start:int]} {FILTER [filter:string]} {CHANNEL_LAYOUT [channel_layout:string]}");
	sink.para()
		->text(L"The FFmpeg Producer can play all media that FFmpeg can play, which includes many ")
		->text(L"QuickTime video codec such as Animation, PNG, PhotoJPEG, MotionJPEG, as well as ")
		->text(L"H.264, FLV, WMV and several audio codecs as well as uncompressed audio.");
	sink.definitions()
		->item(L"clip", L"The file without the file extension to play. It should reside under the media folder.")
		->item(L"loop", L"Will cause the media file to loop between start and start + length")
		->item(L"start", L"Optionally sets the start frame. 0 by default. If loop is specified this will be the frame where it starts over again.")
		->item(L"length", L"Optionally sets the length of the clip. If not specified the clip will be played to the end. If loop is specified the file will jump to start position once this number of frames has been played.")
		->item(L"filter", L"If specified, will be used as an FFmpeg video filter.")
		->item(L"channel_layout",
				L"Optionally override the automatically deduced audio channel layout. "
				L"Either a named layout as specified in casparcg.config or in the format [type:string]:[channel_order:string] for a custom layout.");
	sink.para()->text(L"Examples:");
	sink.example(L">> PLAY 1-10 folder/clip", L"to play all frames in a clip and stop at the last frame.");
	sink.example(L">> PLAY 1-10 folder/clip LOOP", L"to loop a clip between the first frame and the last frame.");
	sink.example(L">> PLAY 1-10 folder/clip LOOP START 10", L"to loop a clip between frame 10 and the last frame.");
	sink.example(L">> PLAY 1-10 folder/clip LOOP START 10 LENGTH 50", L"to loop a clip between frame 10 and frame 60.");
	sink.example(L">> PLAY 1-10 folder/clip START 10 LENGTH 50", L"to play frames 10-60 in a clip and stop.");
	sink.example(L">> PLAY 1-10 folder/clip FILTER yadif=1,-1", L"to deinterlace the video.");
	sink.example(L">> PLAY 1-10 folder/clip CHANNEL_LAYOUT dolbydigital", L"given the defaults in casparcg.config this will specifies that the clip has 6 audio channels of the type 5.1 and that they are in the order FL FC FR BL BR LFE regardless of what ffmpeg says.");
	sink.example(L">> PLAY 1-10 folder/clip CHANNEL_LAYOUT \"5.1:LFE FL FC FR BL BR\"", L"specifies that the clip has 6 audio channels of the type 5.1 and that they are in the specified order regardless of what ffmpeg says.");
	sink.para()->text(L"The FFmpeg producer also supports changing some of the settings via CALL:");
	sink.example(L">> CALL 1-10 LOOP 1");
	sink.example(L">> CALL 1-10 START 10");
	sink.example(L">> CALL 1-10 LENGTH 50");
}

spl::shared_ptr<core::frame_producer> create_producer(const core::frame_producer_dependencies& dependencies, const std::vector<std::wstring>& params)
{		
	auto filename = probe_stem(env::media_folder() + L"/" + params.at(0));

	if(filename.empty())
		return core::frame_producer::empty();
	
	bool loop			= contains_param(L"LOOP", params);
	auto start			= get_param(L"START", params, get_param(L"SEEK", params, static_cast<uint32_t>(0)));
	auto length			= get_param(L"LENGTH", params, std::numeric_limits<uint32_t>::max());
	auto filter_str		= get_param(L"FILTER", params, L"");
	auto channel_layout	= get_param(L"CHANNEL_LAYOUT", params, L"");

	return create_destroy_proxy(spl::make_shared_ptr(std::make_shared<ffmpeg_producer>(
			dependencies.frame_factory,
			dependencies.format_desc,
			channel_layout,
			filename,
			filter_str,
			loop,
			start,
			length)));
}

}}
