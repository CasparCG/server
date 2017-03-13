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

#include "../stdafx.h"

#include "ffmpeg_producer.h"

#include "../ffmpeg.h"
#include "../ffmpeg_error.h"
#include "../ffmpeg_params.h"

#include "muxer/frame_muxer.h"
#include "input/input.h"
#include "util/util.h"
#include "audio/audio_decoder.h"
#include "video/video_decoder.h"

#include <common/env.h>
#include <common/utility/assert.h>
#include <common/diagnostics/graph.h>

#include <core/monitor/monitor.h>
#include <core/video_format.h>
#include <core/parameters/parameters.h>
#include <core/producer/frame_producer.h>
#include <core/producer/frame/frame_factory.h>
#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_transform.h>
#include <core/producer/separated/separated_producer.h>

#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>
#include <boost/timer.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/regex.hpp>

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
				
struct ffmpeg_producer : public core::frame_producer
{
	core::monitor::subject										monitor_subject_;
	const std::wstring											filename_;
	const std::wstring											path_relative_to_media_;

	FFMPEG_Resource												resource_type_;
	
	const safe_ptr<diagnostics::graph>							graph_;
	boost::timer												frame_timer_;
					
	const safe_ptr<core::frame_factory>							frame_factory_;
	const core::video_format_desc								format_desc_;

	std::shared_ptr<void>										initial_logger_disabler_;

	input														input_;	
	std::unique_ptr<video_decoder>								video_decoder_;
	std::unique_ptr<audio_decoder>								audio_decoder_;	
	std::unique_ptr<frame_muxer>								muxer_;

	const double												fps_;
	const uint32_t												start_;
	const uint32_t												length_;
	const bool													thumbnail_mode_;

	safe_ptr<core::basic_frame>									last_frame_;
	
	std::queue<std::pair<safe_ptr<core::basic_frame>, size_t>>	frame_buffer_;

	int64_t														frame_number_;
	uint32_t													file_frame_number_;
		
public:
	explicit ffmpeg_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filename, FFMPEG_Resource resource_type, const std::wstring& filter, bool loop, uint32_t start, uint32_t length, bool thumbnail_mode, const ffmpeg_producer_params& vid_params, bool alpha = false)
		: filename_(filename)
		, path_relative_to_media_(get_relative_or_original(filename, env::media_folder()))
		, resource_type_(resource_type)
		, frame_factory_(frame_factory)		
		, format_desc_(frame_factory->get_video_format_desc())
		, initial_logger_disabler_(temporary_disable_logging_for_thread(thumbnail_mode))
		, input_(graph_, filename_, resource_type, loop, start, length, thumbnail_mode, vid_params)
		, fps_(read_fps(*input_.context(), format_desc_.fps))
		, start_(start)
		, length_(length)
		, thumbnail_mode_(thumbnail_mode)
		, last_frame_(core::basic_frame::empty())
		, frame_number_(0)
	{
		graph_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
		graph_->set_color("underflow", diagnostics::color(0.6f, 0.3f, 0.9f));	
		diagnostics::register_graph(graph_);
	
		try
		{
			video_decoder_.reset(new video_decoder(input_.context(), alpha ? 1 : -1));
			if (!thumbnail_mode_)
				CASPAR_LOG(info) << print() << L" " << video_decoder_->print();
		}
		catch(averror_stream_not_found&)
		{
			//CASPAR_LOG(warning) << print() << " No video-stream found. Running without video.";	
		}
		catch(...)
		{
			if (!thumbnail_mode_)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				CASPAR_LOG(warning) << print() << "Failed to open video-stream. Running without video.";	
			}
		}

		core::channel_layout audio_channel_layout = core::default_channel_layout_repository().get_by_name(L"STEREO");

		if (!thumbnail_mode_ && !alpha)
		{
			try
			{
				audio_decoder_.reset(new audio_decoder(input_.context(), frame_factory->get_video_format_desc(), L""));
				audio_channel_layout = audio_decoder_->channel_layout();
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
		}

		if(!video_decoder_ && !audio_decoder_)
			BOOST_THROW_EXCEPTION(averror_stream_not_found() << msg_info("No streams found"));

		muxer_.reset(new frame_muxer(fps_, frame_factory, thumbnail_mode_, audio_channel_layout, filter));
	}

	// frame_producer
	
	virtual safe_ptr<core::basic_frame> receive(int hints) override
	{
		return render_frame(hints).first;
	}

	virtual safe_ptr<core::basic_frame> last_frame() const override
	{
		return disable_audio(last_frame_);
	}

	std::pair<safe_ptr<core::basic_frame>, uint32_t> render_frame(int hints)
	{		
		frame_timer_.restart();
		auto disable_logging = temporary_disable_logging_for_thread(thumbnail_mode_);
				
		for(int n = 0; n < 16 && frame_buffer_.size() < 2; ++n)
			try_decode_frame(hints);
		
		graph_->set_value("frame-time", frame_timer_.elapsed()*format_desc_.fps*0.5);

		if (frame_buffer_.empty())
		{
			if (input_.eof())
			{
				send_osc();
				return std::make_pair(last_frame(), -1);
			}
			else if (resource_type_ == FFMPEG_FILE)
			{
				graph_->set_tag("underflow");  
				send_osc();
				return std::make_pair(core::basic_frame::late(), -1);     
			}
			else
			{
				send_osc();
				return std::make_pair(last_frame(), -1);
			}
		}
		
		auto frame = frame_buffer_.front(); 
		frame_buffer_.pop();
		
		++frame_number_;
		file_frame_number_ = frame.second;

		graph_->set_text(print());

		last_frame_ = frame.first;

		send_osc();

		return frame;
	}

	void send_osc()
	{
		monitor_subject_	<< core::monitor::message("/profiler/time")		% frame_timer_.elapsed() % (1.0/format_desc_.fps);			
								
		monitor_subject_	<< core::monitor::message("/file/time")			% (file_frame_number()/fps_) 
																			% (file_nb_frames()/fps_)
							<< core::monitor::message("/file/frame")			% static_cast<int32_t>(file_frame_number())
																			% static_cast<int32_t>(file_nb_frames())
							<< core::monitor::message("/file/fps")			% fps_
							<< core::monitor::message("/file/path")			% path_relative_to_media_
							<< core::monitor::message("/loop")				% input_.loop();
	}
	
	safe_ptr<core::basic_frame> render_specific_frame(uint32_t file_position, int hints)
	{
		// Some trial and error and undeterministic stuff here
		static const int NUM_RETRIES = 32;
		
		if (file_position > 0) // Assume frames are requested in sequential order,
			                   // therefore no seeking should be necessary for the first frame.
		{
			input_.seek(file_position > 1 ? file_position - 2: file_position).get();
			boost::this_thread::sleep(boost::posix_time::milliseconds(40));
		}

		for (int i = 0; i < NUM_RETRIES; ++i)
		{
			boost::this_thread::sleep(boost::posix_time::milliseconds(40));
		
			auto frame = render_frame(hints);

			if (frame.second == std::numeric_limits<uint32_t>::max())
			{
				// Retry
				continue;
			}
			else if (frame.second == file_position + 1 || frame.second == file_position)
				return frame.first;
			else if (frame.second > file_position + 1)
			{
				CASPAR_LOG(trace) << print() << L" " << frame.second << L" received, wanted " << file_position + 1;
				int64_t adjusted_seek = file_position - (frame.second - file_position + 1);

				if (adjusted_seek > 1 && file_position > 0)
				{
					CASPAR_LOG(trace) << print() << L" adjusting to " << adjusted_seek;
					input_.seek(static_cast<uint32_t>(adjusted_seek) - 1).get();
					boost::this_thread::sleep(boost::posix_time::milliseconds(40));
				}
				else
					return frame.first;
			}
		}

		CASPAR_LOG(trace) << print() << " Giving up finding frame at " << file_position;
		return core::basic_frame::empty();
	}

	virtual safe_ptr<core::basic_frame> create_thumbnail_frame() override
	{
		auto disable_logging = temporary_disable_logging_for_thread(thumbnail_mode_);

		auto total_frames = nb_frames();
		auto grid = env::properties().get(L"configuration.thumbnails.video-grid", 2);

		if (grid < 1)
		{
			CASPAR_LOG(error) << L"configuration/thumbnails/video-grid cannot be less than 1";
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("configuration/thumbnails/video-grid cannot be less than 1"));
		}

		if (grid == 1)
		{
			return render_specific_frame(total_frames / 2, 0/*DEINTERLACE_HINT*/);
		}

		auto num_snapshots = grid * grid;

		std::vector<safe_ptr<core::basic_frame>> frames;

		for (int i = 0; i < num_snapshots; ++i)
		{
			int x = i % grid;
			int y = i / grid;
			int desired_frame;
			
			if (i == 0)
				desired_frame = 0; // first
			else if (i == num_snapshots - 1)
				desired_frame = total_frames - 1; // last
			else
				// evenly distributed across the file.
				desired_frame = total_frames * i / (num_snapshots - 1);

			auto frame = render_specific_frame(desired_frame, 0/*DEINTERLACE_HINT*/);
			frame->get_frame_transform().fill_scale[0] = 1.0 / static_cast<double>(grid);
			frame->get_frame_transform().fill_scale[1] = 1.0 / static_cast<double>(grid);
			frame->get_frame_transform().fill_translation[0] = 1.0 / static_cast<double>(grid) * x;
			frame->get_frame_transform().fill_translation[1] = 1.0 / static_cast<double>(grid) * y;

			frames.push_back(frame);
		}

		return make_safe<core::basic_frame>(frames);
	}
	
	uint32_t file_frame_number() const
	{
		return video_decoder_ ? video_decoder_->file_frame_number() : 0;
	}

	virtual uint32_t nb_frames() const override
	{
		//if(input_.loop())
		if(resource_type_ == FFMPEG_DEVICE || resource_type_ == FFMPEG_STREAM || input_.loop()) 
			return std::numeric_limits<uint32_t>::max();

		uint32_t nb_frames = file_nb_frames();

		nb_frames = std::min(length_, nb_frames - start_);
		nb_frames = muxer_->calc_nb_frames(nb_frames);
		
		return nb_frames;
	}

	uint32_t file_nb_frames() const
	{
		uint32_t file_nb_frames = 0;
		file_nb_frames = std::max(file_nb_frames, video_decoder_ ? video_decoder_->nb_frames() : 0);
		file_nb_frames = std::max(file_nb_frames, audio_decoder_ ? audio_decoder_->nb_frames() : 0);
		return file_nb_frames;
	}
	
	virtual boost::unique_future<std::wstring> call(const std::wstring& param) override
	{
		boost::promise<std::wstring> promise;
		promise.set_value(do_call(param));
		return promise.get_future();
	}
				
	virtual std::wstring print() const override
	{
		return L"ffmpeg[" + boost::filesystem::path(filename_).filename().wstring() + L"|" 
						  + print_mode() + L"|" 
						  + boost::lexical_cast<std::wstring>(file_frame_number_) + L"/" + boost::lexical_cast<std::wstring>(file_nb_frames()) + L"]";
	}

	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type",				L"ffmpeg-producer");
		info.add(L"filename",			filename_);
		info.add(L"width",				video_decoder_ ? video_decoder_->width() : 0);
		info.add(L"height",				video_decoder_ ? video_decoder_->height() : 0);
		info.add(L"progressive",		video_decoder_ ? video_decoder_->is_progressive() : false);
		info.add(L"fps",				fps_);
		info.add(L"loop",				input_.loop());
		info.add(L"frame-number",		frame_number_);
		auto nb_frames2 = nb_frames();
		info.add(L"nb-frames",			nb_frames2 == std::numeric_limits<int64_t>::max() ? -1 : nb_frames2);
		info.add(L"file-frame-number",	file_frame_number_);
		info.add(L"file-nb-frames",		file_nb_frames());
		return info;
	}

	// ffmpeg_producer

	bool has_alpha()
	{
		return av_find_best_stream(input_.context().get(), AVMEDIA_TYPE_VIDEO, 1, -1, nullptr, 0) != AVERROR_STREAM_NOT_FOUND;
	}

	std::wstring print_mode() const
	{
		return video_decoder_ ? ffmpeg::print_mode(video_decoder_->width(), video_decoder_->height(), fps_, !video_decoder_->is_progressive()) : L"";
	}
					
	std::wstring do_call(const std::wstring& param)
	{
		static const boost::wregex loop_exp(L"LOOP\\s*(?<VALUE>\\d?)?", boost::regex::icase);
		static const boost::wregex seek_exp(L"SEEK\\s+(?<VALUE>\\d+)", boost::regex::icase);
		
		boost::wsmatch what;
		if(boost::regex_match(param, what, loop_exp))
		{
			if(!what["VALUE"].str().empty())
				input_.loop(boost::lexical_cast<bool>(what["VALUE"].str()));
			return boost::lexical_cast<std::wstring>(input_.loop());
		}
		if(boost::regex_match(param, what, seek_exp))
		{
			input_.seek(boost::lexical_cast<uint32_t>(what["VALUE"].str()));
			return L"";
		}

		BOOST_THROW_EXCEPTION(invalid_argument());
	}

	void try_decode_frame(int hints)
	{
		std::shared_ptr<AVPacket> pkt;

		for(int n = 0; n < 32 && ((video_decoder_ && !video_decoder_->ready()) || (audio_decoder_ && !audio_decoder_->ready())) && input_.try_pop(pkt); ++n)
		{
			if(video_decoder_)
				video_decoder_->push(pkt);
			if(audio_decoder_)
				audio_decoder_->push(pkt);
		}
		
		std::shared_ptr<AVFrame>			video;
		std::shared_ptr<core::audio_buffer> audio;

		tbb::parallel_invoke(
		[&]
		{
			if(!muxer_->video_ready() && video_decoder_)	
				video = video_decoder_->poll();	
		},
		[&]
		{		
			if(!muxer_->audio_ready() && audio_decoder_)
				audio = audio_decoder_->poll();
		});
		
		muxer_->push(video, hints);
		muxer_->push(audio);

		if(!audio_decoder_)
		{
			if(video == flush_video())
				muxer_->push(flush_audio());
			else if(!muxer_->audio_ready())
				muxer_->push(empty_audio());
		}

		if(!video_decoder_)
		{
			if(audio == flush_audio())
				muxer_->push(flush_video(), 0);
			else if(!muxer_->video_ready())
				muxer_->push(empty_video(), 0);
		}
		
		size_t file_frame_number = 0;
		file_frame_number = std::max(file_frame_number, video_decoder_ ? video_decoder_->file_frame_number() : 0);
		//file_frame_number = std::max(file_frame_number, audio_decoder_ ? audio_decoder_->file_frame_number() : 0);

		for(auto frame = muxer_->poll(); frame; frame = muxer_->poll())
			frame_buffer_.push(std::make_pair(make_safe_ptr(frame), file_frame_number));
	}

	core::monitor::subject& monitor_output()
	{
		return monitor_subject_;
	}
};

safe_ptr<core::frame_producer> create_producer(
		const safe_ptr<core::frame_factory>& frame_factory,
		const core::parameters& params)
{		
	static const std::vector<std::wstring> invalid_exts = boost::assign::list_of(L".png")(L".tga")(L".bmp")(L".jpg")(L".jpeg")(L".gif")(L".tiff")(L".tif")(L".jp2")(L".jpx")(L".j2k")(L".j2c")(L".swf")(L".ct");

	// Infer the resource type from the resource_name
	auto resource_type = FFMPEG_FILE;
	auto tokens = core::parameters::protocol_split(params.at_original(0));
	auto filename = params.at_original(0);
	if (!tokens[0].empty())
	{
		if (tokens[0] == L"dshow")
		{
			// Camera
			resource_type = FFMPEG_DEVICE;
			filename = tokens[1];
		} else
		{
			// Stream
			resource_type = FFMPEG_STREAM;
			filename = params.at_original(0);
		}
	} else
	{
		// File
		resource_type = FFMPEG_FILE;
		filename = env::media_folder() + L"\\" + tokens[1];
		if(!boost::filesystem::exists(filename))
			filename = probe_stem(filename);

		//TODO fix these?
		//vid_params->loop       = params.has(L"LOOP");
		//vid_params->start     = params.get(L"SEEK", static_cast<uint32_t>(0));
	}

	if(filename.empty())
		return core::frame_producer::empty();
	
	auto loop		= params.has(L"LOOP");
	auto start		= params.get(L"SEEK", static_cast<uint32_t>(0));
	auto length		= params.get(L"LENGTH", std::numeric_limits<uint32_t>::max());
	auto filter_str = params.get(L"FILTER", L""); 	
	auto custom_channel_order	= params.get(L"CHANNEL_LAYOUT", L"");

	boost::replace_all(filter_str, L"DEINTERLACE_BOB", L"YADIF=1:-1");
	boost::replace_all(filter_str, L"DEINTERLACE", L"YADIF=0:-1");
	
	ffmpeg_producer_params vid_params;
	bool haveFFMPEGStartIndicator = false;
	for (size_t i = 0; i < params.size() - 1; ++i)
	{
		if (!haveFFMPEGStartIndicator && params[i] == L"--")
		{
			haveFFMPEGStartIndicator = true;
			continue;
		}
		if (haveFFMPEGStartIndicator)
		{
			auto name = narrow(params.at_original(i++)).substr(1);
			auto value = narrow(params.at_original(i));
			vid_params.options.push_back(option(name, value));
		}
	}
		
	auto producer = make_safe<ffmpeg_producer>(frame_factory, filename, FFMPEG_FILE, filter_str, loop, start, length, false, vid_params);
	auto key_producer = core::frame_producer::empty();

	try // to find a key file.
	{
		if (producer->has_alpha())
			key_producer = make_safe<ffmpeg_producer>(frame_factory, filename, FFMPEG_FILE, filter_str, loop, start, length, false, vid_params, true);
	}
	catch (...) {}

	if (producer != core::frame_producer::empty() && key_producer != core::frame_producer::empty())
		return core::create_separated_producer(producer, key_producer);

	
	return create_producer_destroy_proxy(producer);
}

safe_ptr<core::frame_producer> create_thumbnail_producer(
		const safe_ptr<core::frame_factory>& frame_factory,
		const core::parameters& params)
{		
	static const std::vector<std::wstring> invalid_exts = boost::assign::list_of
			(L".png")(L".tga")(L".bmp")(L".jpg")(L".jpeg")(L".gif")(L".tiff")(L".tif")(L".jp2")(L".jpx")(L".j2k")(L".j2c")(L".swf")(L".ct")
			(L".wav")(L".mp3"); // audio shall not have thumbnails
	auto filename = probe_stem(env::media_folder() + L"\\" + params.at_original(0), invalid_exts);

	if(filename.empty())
		return core::frame_producer::empty();
	
	auto loop		= false;
	auto start		= 0;
	auto length		= std::numeric_limits<uint32_t>::max();
	auto filter_str = L"";

	ffmpeg_producer_params vid_params;

	return make_safe<ffmpeg_producer>(frame_factory, filename, FFMPEG_FILE, filter_str, loop, start, length, true, vid_params);
}

}}