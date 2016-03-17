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

#include "../ffmpeg_pipeline.h"
#include "../ffmpeg.h"
#include "util/util.h"

#include <common/param.h>
#include <common/diagnostics/graph.h>
#include <common/future.h>

#include <core/frame/draw_frame.h>
#include <core/help/help_repository.h>
#include <core/help/help_sink.h>
#include <core/producer/media_info/media_info.h>
#include <core/producer/framerate/framerate_producer.h>

#include <future>

namespace caspar { namespace ffmpeg {

struct seek_out_of_range : virtual user_error {};

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
	ffmpeg_pipeline									pipeline_;
	const std::wstring								filename_;
	const std::wstring								path_relative_to_media_	= get_relative_or_original(filename_, env::media_folder());
	
	const spl::shared_ptr<diagnostics::graph>		graph_;
					
	const core::video_format_desc					format_desc_;

	core::constraints								constraints_;
	
	core::draw_frame								first_frame_			= core::draw_frame::empty();
	core::draw_frame								last_frame_				= core::draw_frame::empty();

	boost::optional<uint32_t>						seek_target_;
	
public:
	explicit ffmpeg_producer(
			ffmpeg_pipeline pipeline, 
			const core::video_format_desc& format_desc)
		: pipeline_(std::move(pipeline))
		, filename_(u16(pipeline_.source_filename()))
		, format_desc_(format_desc)
	{
		graph_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
		graph_->set_color("underflow", diagnostics::color(0.6f, 0.3f, 0.9f));	
		diagnostics::register_graph(graph_);

		pipeline_.graph(graph_);
		pipeline_.start();

		while ((first_frame_ = pipeline_.try_pop_frame()) == core::draw_frame::late())
			boost::this_thread::sleep_for(boost::chrono::milliseconds(1));

		constraints_.width.set(pipeline_.width());
		constraints_.height.set(pipeline_.height());

		if (is_logging_quiet_for_thread())
			CASPAR_LOG(debug) << print() << L" Initialized";
		else
			CASPAR_LOG(info) << print() << L" Initialized";
	}

	// frame_producer
	
	core::draw_frame receive_impl() override
	{				
		auto frame = core::draw_frame::late();
		
		caspar::timer frame_timer;
		
		auto decoded_frame = first_frame_;

		if (decoded_frame == core::draw_frame::empty())
			decoded_frame = pipeline_.try_pop_frame();
		else
			first_frame_ = core::draw_frame::empty();

		if (decoded_frame == core::draw_frame::empty())
			frame = core::draw_frame::still(last_frame_);
		else if (decoded_frame != core::draw_frame::late())
			last_frame_ = frame = core::draw_frame(std::move(decoded_frame));
		else if (pipeline_.started())
			graph_->set_tag(diagnostics::tag_severity::WARNING, "underflow");

		graph_->set_text(print());

		graph_->set_value("frame-time", frame_timer.elapsed()*format_desc_.fps*0.5);
		*monitor_subject_
				<< core::monitor::message("/profiler/time")	% frame_timer.elapsed() % (1.0/format_desc_.fps);			
		*monitor_subject_
				<< core::monitor::message("/file/frame")	% static_cast<int32_t>(pipeline_.last_frame())
															% static_cast<int32_t>(pipeline_.length())
				<< core::monitor::message("/file/fps")		% boost::rational_cast<double>(pipeline_.framerate())
				<< core::monitor::message("/file/path")		% path_relative_to_media_
				<< core::monitor::message("/loop")			% pipeline_.loop();

		return frame;
	}

	core::draw_frame last_frame() override
	{
		return core::draw_frame::still(last_frame_);
	}

	core::constraints& pixel_constraints() override
	{
		return constraints_;
	}

	uint32_t nb_frames() const override
	{
		if (pipeline_.loop())
			return std::numeric_limits<uint32_t>::max();

		return pipeline_.length();
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
				pipeline_.loop(boost::lexical_cast<bool>(value));
			result = boost::lexical_cast<std::wstring>(pipeline_.loop());
		}
		else if(boost::regex_match(param, what, seek_exp))
		{
			auto value = what["VALUE"].str();
			pipeline_.seek(boost::lexical_cast<uint32_t>(value));
		}
		else if(boost::regex_match(param, what, length_exp))
		{
			auto value = what["VALUE"].str();
			if(!value.empty())
				pipeline_.length(boost::lexical_cast<uint32_t>(value));			
			result = boost::lexical_cast<std::wstring>(pipeline_.length());
		}
		else if(boost::regex_match(param, what, start_exp))
		{
			auto value = what["VALUE"].str();
			if(!value.empty())
				pipeline_.start_frame(boost::lexical_cast<uint32_t>(value));
			result = boost::lexical_cast<std::wstring>(pipeline_.start_frame());
		}
		else
			CASPAR_THROW_EXCEPTION(invalid_argument());

		return make_ready_future(std::move(result));
	}
				
	std::wstring print() const override
	{
		return L"ffmpeg[" + boost::filesystem::path(filename_).filename().wstring() + L"|" 
						  + print_mode() + L"|" 
						  + boost::lexical_cast<std::wstring>(pipeline_.last_frame()) + L"/" + boost::lexical_cast<std::wstring>(pipeline_.length()) + L"]";
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
		info.add(L"width",				pipeline_.width());
		info.add(L"height",				pipeline_.height());
		info.add(L"progressive",		pipeline_.progressive());
		info.add(L"fps",				boost::rational_cast<double>(pipeline_.framerate()));
		info.add(L"loop",				pipeline_.loop());
		info.add(L"frame-number",		frame_number());
		info.add(L"nb-frames",			nb_frames());
		info.add(L"file-frame-number",	pipeline_.last_frame());
		info.add(L"file-nb-frames",		pipeline_.length());
		return info;
	}
	
	core::monitor::subject& monitor_output()
	{
		return *monitor_subject_;
	}

	// ffmpeg_producer

	std::wstring print_mode() const
	{
		return ffmpeg::print_mode(
				pipeline_.width(),
				pipeline_.height(),
				boost::rational_cast<double>(pipeline_.framerate()), 
				!pipeline_.progressive());
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
	sink.example(L">> PLAY 1-10 folder/clip CHANNEL_LAYOUT film", L"given the defaults in casparcg.config this will specifies that the clip has 6 audio channels of the type 5.1 and that they are in the order FL FC FR BL BR LFE regardless of what ffmpeg says.");
	sink.example(L">> PLAY 1-10 folder/clip CHANNEL_LAYOUT \"5.1:LFE FL FC FR BL BR\"", L"specifies that the clip has 6 audio channels of the type 5.1 and that they are in the specified order regardless of what ffmpeg says.");
	sink.para()->text(L"The FFmpeg producer also supports changing some of the settings via ")->code(L"CALL")->text(L":");
	sink.example(L">> CALL 1-10 LOOP 1");
	sink.example(L">> CALL 1-10 START 10");
	sink.example(L">> CALL 1-10 LENGTH 50");
	core::describe_framerate_producer(sink);
}

spl::shared_ptr<core::frame_producer> create_producer(
		const core::frame_producer_dependencies& dependencies,
		const std::vector<std::wstring>& params,
		const spl::shared_ptr<core::media_info_repository>& info_repo)
{
	auto filename = probe_stem(env::media_folder() + L"/" + params.at(0), false);

	if(filename.empty())
		return core::frame_producer::empty();
	
	auto pipeline = ffmpeg_pipeline()
			.from_file(u8(filename))
			.loop(contains_param(L"LOOP", params))
			.start_frame(get_param(L"START", params, get_param(L"SEEK", params, static_cast<uint32_t>(0))))
			.length(get_param(L"LENGTH", params, std::numeric_limits<uint32_t>::max()))
			.vfilter(u8(get_param(L"FILTER", params, L"")))
			.to_memory(dependencies.frame_factory, dependencies.format_desc);

	auto producer = create_destroy_proxy(spl::make_shared_ptr(std::make_shared<ffmpeg_producer>(
			pipeline,
			dependencies.format_desc)));

	if (pipeline.framerate() == -1) // Audio only.
		return producer;

	auto source_framerate = pipeline.framerate();
	auto target_framerate = boost::rational<int>(
			dependencies.format_desc.time_scale,
			dependencies.format_desc.duration);

	return core::create_framerate_producer(
			producer,
			source_framerate,
			target_framerate,
			dependencies.format_desc.field_mode,
			dependencies.format_desc.audio_cadence);
}

core::draw_frame create_thumbnail_frame(
		const core::frame_producer_dependencies& dependencies,
		const std::wstring& media_file,
		const spl::shared_ptr<core::media_info_repository>& info_repo)
{
	auto quiet_logging = temporary_enable_quiet_logging_for_thread(true);
	auto filename = probe_stem(env::media_folder() + L"/" + media_file, true);

	if (filename.empty())
		return core::draw_frame::empty();

	auto render_specific_frame = [&](std::int64_t frame_num)
	{
		auto pipeline = ffmpeg_pipeline()
			.from_file(u8(filename))
			.start_frame(static_cast<uint32_t>(frame_num))
			.to_memory(dependencies.frame_factory, dependencies.format_desc);
		pipeline.start();

		auto frame = core::draw_frame::empty();
		while ((frame = pipeline.try_pop_frame()) == core::draw_frame::late())
			boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
		return frame;
	};

	auto info = info_repo->get(filename);

	if (!info)
		return core::draw_frame::empty();

	auto total_frames = info->duration;
	auto grid = env::properties().get(L"configuration.thumbnails.video-grid", 2);

	if (grid < 1)
	{
		CASPAR_LOG(error) << L"configuration/thumbnails/video-grid cannot be less than 1";
		BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("configuration/thumbnails/video-grid cannot be less than 1"));
	}

	if (grid == 1)
	{
		return render_specific_frame(total_frames / 2);
	}

	auto num_snapshots = grid * grid;

	std::vector<core::draw_frame> frames;

	for (int i = 0; i < num_snapshots; ++i)
	{
		int x = i % grid;
		int y = i / grid;
		std::int64_t desired_frame;

		if (i == 0)
			desired_frame = 0; // first
		else if (i == num_snapshots - 1)
			desired_frame = total_frames - 2; // last
		else
			// evenly distributed across the file.
			desired_frame = total_frames * i / (num_snapshots - 1);

		auto frame = render_specific_frame(desired_frame);
		frame.transform().image_transform.fill_scale[0] = 1.0 / static_cast<double>(grid);
		frame.transform().image_transform.fill_scale[1] = 1.0 / static_cast<double>(grid);
		frame.transform().image_transform.fill_translation[0] = 1.0 / static_cast<double>(grid) * x;
		frame.transform().image_transform.fill_translation[1] = 1.0 / static_cast<double>(grid) * y;

		frames.push_back(frame);
	}

	return core::draw_frame(frames);
}

}}
