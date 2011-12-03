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

#include "../stdafx.h"

#include "ffmpeg_producer.h"

#include "../ffmpeg_error.h"

#include "muxer/frame_muxer.h"
#include "input/input.h"
#include "util/util.h"
#include "audio/audio_decoder.h"
#include "video/video_decoder.h"

#include <common/env.h>
#include <common/utility/assert.h>
#include <common/diagnostics/graph.h>

#include <core/video_format.h>
#include <core/producer/frame_producer.h>
#include <core/producer/frame/frame_factory.h>
#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_transform.h>

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
				
struct ffmpeg_producer : public core::frame_producer
{
	const std::wstring											filename_;
	
	const safe_ptr<diagnostics::graph>							graph_;
	boost::timer												frame_timer_;
					
	const safe_ptr<core::frame_factory>							frame_factory_;
	const core::video_format_desc								format_desc_;

	input														input_;	
	std::unique_ptr<video_decoder>								video_decoder_;
	std::unique_ptr<audio_decoder>								audio_decoder_;	
	std::unique_ptr<frame_muxer>								muxer_;

	const double												fps_;
	const int													start_;
	const bool													loop_;
	const size_t												length_;

	safe_ptr<core::basic_frame>									last_frame_;
	
	std::queue<std::pair<safe_ptr<core::basic_frame>, size_t>>	frame_buffer_;

	int64_t														frame_number_;
	int64_t														file_frame_number_;;
	
public:
	explicit ffmpeg_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filename, const std::wstring& filter, bool loop, int start, size_t length) 
		: filename_(filename)
		, frame_factory_(frame_factory)		
		, format_desc_(frame_factory->get_video_format_desc())
		, input_(graph_, filename_, loop, start, length)
		, fps_(read_fps(*input_.context(), format_desc_.fps))
		, start_(start)
		, loop_(loop)
		, length_(length)
		, last_frame_(core::basic_frame::empty())
		, frame_number_(0)
	{
		graph_->add_guide("frame-time", 0.5);
		graph_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
		graph_->set_color("underflow", diagnostics::color(0.6f, 0.3f, 0.9f));	
		diagnostics::register_graph(graph_);
		
		try
		{
			video_decoder_.reset(new video_decoder(input_.context()));
		}
		catch(averror_stream_not_found&)
		{
			CASPAR_LOG(warning) << "No video-stream found. Running without video.";	
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			CASPAR_LOG(warning) << "Failed to open video-stream. Running without video.";	
		}

		try
		{
			audio_decoder_.reset(new audio_decoder(input_.context(), frame_factory->get_video_format_desc()));
		}
		catch(averror_stream_not_found&)
		{
			CASPAR_LOG(warning) << "No audio-stream found. Running without audio.";	
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			CASPAR_LOG(warning) << "Failed to open audio-stream. Running without audio.";		
		}	

		if(!video_decoder_ && !audio_decoder_)
			BOOST_THROW_EXCEPTION(averror_stream_not_found() << msg_info("No streams found"));

		muxer_.reset(new frame_muxer(fps_, frame_factory, filter));
	}

	// frame_producer
	
	virtual safe_ptr<core::basic_frame> receive(int hints) override
	{		
		frame_timer_.restart();
		
		for(int n = 0; n < 16 && frame_buffer_.size() < 2; ++n)
			try_decode_frame(hints);
		
		graph_->update_value("frame-time", frame_timer_.elapsed()*format_desc_.fps*0.5);
				
		if(frame_buffer_.empty() && input_.eof())
			return last_frame();

		if(frame_buffer_.empty())
		{
			graph_->add_tag("underflow");	
			return core::basic_frame::late();			
		}
		
		auto frame = frame_buffer_.front(); 
		frame_buffer_.pop();
		
		++frame_number_;
		file_frame_number_ = frame.second;

		graph_->set_text(print());

		return last_frame_ = frame.first;
	}

	virtual safe_ptr<core::basic_frame> last_frame() const override
	{
		return disable_audio(last_frame_);
	}

	virtual int64_t nb_frames() const override
	{
		if(loop_)
			return std::numeric_limits<int64_t>::max();

		auto nb_frames = file_nb_frames();

		nb_frames = std::min(static_cast<int64_t>(length_), nb_frames);
		nb_frames = muxer_->calc_nb_frames(nb_frames);
		
		return nb_frames - start_;
	}

	virtual int64_t file_nb_frames() const
	{
		int64_t file_nb_frames = 0;
		file_nb_frames = std::max(file_nb_frames, video_decoder_ ? video_decoder_->nb_frames() : 0);
		file_nb_frames = std::max(file_nb_frames, audio_decoder_ ? audio_decoder_->nb_frames() : 0);
		return file_nb_frames;
	}

	virtual int64_t frame_number() const
	{
		return frame_number_;
	}

	virtual int64_t file_frame_number() const
	{
		return file_frame_number_;
	}

	virtual boost::unique_future<std::wstring> call(const std::wstring& param) override
	{
		boost::promise<std::wstring> promise;
		promise.set_value(do_call(param));
		return promise.get_future();
	}
				
	virtual std::wstring print() const override
	{
		return L"ffmpeg[" + boost::filesystem::wpath(filename_).filename() + L"|" 
						  + print_mode() + L"|" 
						  + boost::lexical_cast<std::wstring>(file_frame_number()) + L"/" + boost::lexical_cast<std::wstring>(file_nb_frames()) + L"]";
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

	std::wstring print_mode() const
	{
		return video_decoder_ ? ffmpeg::print_mode(video_decoder_->width(), video_decoder_->height(), fps_, !video_decoder_->is_progressive()) : L"";
	}
					
	std::wstring do_call(const std::wstring& param)
	{
		static const boost::wregex loop_exp(L"LOOP\\s*(?<VALUE>\\d?)", boost::regex::icase);
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
			input_.seek(boost::lexical_cast<int64_t>(what["VALUE"].str()));
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
};

safe_ptr<core::frame_producer> create_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::vector<std::wstring>& params)
{		
	static const std::vector<std::wstring> extensions = boost::assign::list_of
		(L"mpg")(L"mpeg")(L"m2v")(L"m4v")(L"mp3")(L"mp4")(L"mpga")
		(L"avi")
		(L"mov")
		(L"qt")
		(L"webm")
		(L"dv")		
		(L"f4v")(L"flv")
		(L"mkv")(L"mka")
		(L"mxf")
		(L"wmv")(L"wma")(L"wav")
		(L"rm")(L"ram")
		(L"ogg")(L"ogv")(L"oga")(L"ogx")
		(L"divx")(L"xvid");

	std::wstring filename = env::media_folder() + L"\\" + params[0];
	
	auto ext = boost::find_if(extensions, [&](const std::wstring& ex)
	{					
		return boost::filesystem::is_regular_file(boost::filesystem::wpath(filename + L"." + ex));
	});

	if(ext == extensions.end())
		return core::frame_producer::empty();

	auto path		= filename + L"." + *ext;
	auto loop		= boost::range::find(params, L"LOOP") != params.end();
	auto start		= core::get_param(L"SEEK", params, 0);
	auto length		= core::get_param(L"LENGTH", params, std::numeric_limits<size_t>::max());
	auto filter_str = core::get_param<std::wstring>(L"FILTER", params, L""); 	
		
	boost::replace_all(filter_str, L"DEINTERLACE", L"YADIF=0:-1");
	boost::replace_all(filter_str, L"DEINTERLACE_BOB", L"YADIF=1:-1");
	
	return create_producer_destroy_proxy(make_safe<ffmpeg_producer>(frame_factory, path, filter_str, loop, start, length));
}

}}