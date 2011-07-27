/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#include "../stdafx.h"

#include "ffmpeg_producer.h"

#include "input.h"
#include "audio/audio_decoder.h"
#include "video/video_decoder.h"

#include <common/utility/assert.h>
#include <common/utility/timer.h>
#include <common/diagnostics/graph.h>

#include <core/producer/frame/basic_frame.h>
#include <core/mixer/write_frame.h>
#include <core/producer/frame/audio_transform.h>
#include <core/video_format.h>
#include <core/producer/color/color_producer.h>

#include <common/env.h>

#include <boost/timer.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>

#include <tbb/task_group.h>

#include <deque>
#include <vector>

namespace caspar {

struct display_mode
{
	enum type
	{
		simple,
		duplicate,
		half,
		interlace,
		deinterlace,
		deinterlace_half,
		count,
		invalid
	};

	static std::wstring print(display_mode::type value)
	{
		switch(value)
		{
			case simple:
				return L"simple";
			case duplicate:
				return L"duplicate";
			case half:
				return L"half";
			case interlace:
				return L"interlace";
			case deinterlace:
				return L"deinterlace";
			case deinterlace_half:
				return L"deinterlace_half";
			default:
				return L"invalid";
		}
	}
};

display_mode::type get_display_mode(const core::video_mode::type in_mode, double in_fps, const core::video_mode::type out_mode, double out_fps)
{		
	if(in_mode == core::video_mode::invalid || out_mode == core::video_mode::invalid)
		return display_mode::invalid;

	static const auto epsilon = 2.0;

	if(std::abs(in_fps - out_fps) < epsilon)
	{
		if(in_mode != core::video_mode::progressive && out_mode == core::video_mode::progressive)
			return display_mode::deinterlace_half;
		//else if(in_mode == core::video_mode::progressive && out_mode != core::video_mode::progressive)
		//	simple(); // interlace_duplicate();
		else
			return display_mode::simple;
	}
	else if(std::abs(in_fps/2.0 - out_fps) < epsilon)
	{
		if(in_mode != core::video_mode::progressive)
			return display_mode::invalid;

		if(out_mode != core::video_mode::progressive)
			return display_mode::interlace;
		else
			return display_mode::half;
	}
	else if(std::abs(in_fps - out_fps/2.0) < epsilon)
	{
		if(out_mode != core::video_mode::progressive)
			return display_mode::invalid;

		if(in_mode != core::video_mode::progressive)
			return display_mode::deinterlace;
		else
			return display_mode::duplicate;
	}

	return display_mode::invalid;
}
		
struct ffmpeg_producer : public core::frame_producer
{
	const std::wstring								filename_;
	
	const safe_ptr<diagnostics::graph>				graph_;
	boost::timer									frame_timer_;
					
	const safe_ptr<core::frame_factory>				frame_factory_;
	const core::video_format_desc					format_desc_;

	input											input_;	
	video_decoder									video_decoder_;
	audio_decoder									audio_decoder_;

	std::queue<safe_ptr<core::basic_frame>>			frame_buffer_;
	std::queue<safe_ptr<core::basic_frame>>			output_buffer_;

	const bool										auto_convert_;
	display_mode::type								display_mode_;

	std::deque<safe_ptr<core::write_frame>>			video_frames_;
	std::deque<std::vector<int16_t>>				audio_chunks_;

	tbb::task_group									tasks_;

public:
	explicit ffmpeg_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filename, const std::wstring& filter_str, bool loop, int start, int length) 
		: filename_(filename)
		, graph_(diagnostics::create_graph(narrow(print())))
		, frame_factory_(frame_factory)		
		, format_desc_(frame_factory->get_video_format_desc())
		, input_(safe_ptr<diagnostics::graph>(graph_), filename_, loop, start, length)
		, video_decoder_(input_.stream(AVMEDIA_TYPE_VIDEO), frame_factory)
		, audio_decoder_(input_.stream(AVMEDIA_TYPE_AUDIO), frame_factory->get_video_format_desc())
		, auto_convert_(env::properties().get("configuration.ffmpeg.auto-mode", false))
		, display_mode_(display_mode::invalid)
	{
		graph_->add_guide("frame-time", 0.5);
		graph_->set_color("frame-time", diagnostics::color(1.0f, 0.0f, 0.0f));
		graph_->set_color("underflow", diagnostics::color(0.6f, 0.3f, 0.9f));		
		
		for(int n = 0; n < 128 && frame_buffer_.size() < 4; ++n)
			decode_frame();
	}

	~ffmpeg_producer()
	{
		tasks_.cancel();
		tasks_.wait();
	}

	virtual safe_ptr<core::basic_frame> receive()
	{
		if(output_buffer_.empty())
		{	
			tasks_.wait();

			output_buffer_ = std::move(frame_buffer_);

			tasks_.run([=]
			{
				frame_timer_.restart();

				for(int n = 0; n < 64 && frame_buffer_.empty(); ++n)
					decode_frame();

				graph_->update_value("frame-time", static_cast<float>(frame_timer_.elapsed()*format_desc_.fps*0.5));
			});
		}
		
		auto frame = core::basic_frame::late();

		if(output_buffer_.empty())
		{
			if(input_.eof())
				frame = core::basic_frame::eof();
			else
				graph_->add_tag("underflow");			
		}
		else
		{
			frame = output_buffer_.front();
			output_buffer_.pop();
		}
		
		return frame;
	}

	void decode_frame()
	{
		for(int n = 0; n < 32 && ((video_frames_.size() < 2 && !video_decoder_.ready()) ||	(audio_chunks_.size() < 2 && !audio_decoder_.ready())); ++n) 
		{
			std::shared_ptr<AVPacket> pkt;
			if(input_.try_pop(pkt))
			{
				video_decoder_.push(pkt);
				audio_decoder_.push(pkt);
			}
		}
		
		tbb::parallel_invoke(
		[=]
		{
			if(video_frames_.size() < 2)
				boost::range::push_back(video_frames_, video_decoder_.poll());
		},
		[=]
		{
			if(audio_chunks_.size() < 2)
				boost::range::push_back(audio_chunks_, audio_decoder_.poll());
		});

		if(video_frames_.empty() || audio_chunks_.empty())
			return;

		if(auto_convert_)
			auto_convert();
		else
			simple();
	}

	void auto_convert()
	{
		auto current_display_mode = get_display_mode(video_decoder_.mode(), video_decoder_.fps(),  format_desc_.mode,  format_desc_.fps);		
		if(current_display_mode != display_mode_)
		{
			display_mode_ = current_display_mode;
			CASPAR_LOG(info) << print() << " display_mode: " << display_mode::print(display_mode_) << 
				L" in: " << core::video_mode::print(video_decoder_.mode()) << L" " << video_decoder_.fps() << " fps" <<
				L" out: " << core::video_mode::print(format_desc_.mode) << L" " << format_desc_.fps << " fps";
		}

		switch(display_mode_)
		{
		case display_mode::simple:
			return simple();
		case display_mode::duplicate:
			return duplicate();
		case display_mode::half:
			return half();
		case display_mode::interlace:
			return interlace();
		case display_mode::deinterlace:
			return deinterlace();
		case display_mode::deinterlace_half:
			return deinterlace_half();
		default:
			BOOST_THROW_EXCEPTION(invalid_operation());
		}
	}

	void simple()
	{
		CASPAR_ASSERT(!video_frames_.empty());
		CASPAR_ASSERT(!audio_chunks_.empty());

		auto frame1 = video_frames_.front();
		video_frames_.pop_front();

		frame1->audio_data() = audio_chunks_.front();
		audio_chunks_.pop_front();

		frame_buffer_.push(frame1);
	}

	void duplicate()
	{		
		CASPAR_ASSERT(!video_frames_.empty());
		CASPAR_ASSERT(!audio_chunks_.empty());

		auto frame = video_frames_.front();
		video_frames_.pop_front();

		auto frame1 = make_safe<core::write_frame>(*frame); // make a copy
		frame1->audio_data() = audio_chunks_.front();
		audio_chunks_.pop_front();

		auto frame2 = frame;
		frame2->audio_data() = audio_chunks_.front();
		audio_chunks_.pop_front();

		frame_buffer_.push(frame1);
		frame_buffer_.push(frame2);
	}

	void half()
	{	
		CASPAR_ASSERT(!video_frames_.empty());
		CASPAR_ASSERT(!audio_chunks_.empty());

		if(video_frames_.size() < 2 && !input_.eof())
			return;
		
		if(video_frames_.size() < 2)
			video_frames_.push_back(create_color_frame(this, frame_factory_, L"#00000000"));

		CASPAR_ASSERT(video_frames_.size() == 2);
				
		auto frame1 =video_frames_.front();
		video_frames_.pop_front();
		frame1->audio_data() = audio_chunks_.front();
		audio_chunks_.pop_front();
				
		video_frames_.pop_front(); // Throw away

		frame_buffer_.push(frame1);
	}
	
	void interlace()
	{		
		CASPAR_ASSERT(!video_frames_.empty());
		CASPAR_ASSERT(!audio_chunks_.empty());

		if(video_frames_.size() < 2 && !input_.eof())
			return;

		if(video_frames_.size() < 2)
			video_frames_.push_back(create_color_frame(this, frame_factory_, L"#00000000"));
		
		CASPAR_ASSERT(video_frames_.size() == 2);

		auto frame1 = video_frames_.front();
		video_frames_.pop_front();

		frame1->audio_data() = audio_chunks_.front();
		audio_chunks_.pop_front();
				
		auto frame2 = video_frames_.front();
		video_frames_.pop_front();

		frame_buffer_.push(core::basic_frame::interlace(frame1, frame2, format_desc_.mode));		
	}
	
	void deinterlace()
	{
		BOOST_THROW_EXCEPTION(not_implemented() << msg_info("deinterlace"));
	}

	void deinterlace_half()
	{
		BOOST_THROW_EXCEPTION(not_implemented() << msg_info("deinterlace_half"));
	}
				
	virtual std::wstring print() const
	{
		return L"ffmpeg[" + boost::filesystem::wpath(filename_).filename() + L"]";
	}
};

safe_ptr<core::frame_producer> create_ffmpeg_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::vector<std::wstring>& params)
{		
	static const std::vector<std::wstring> extensions = boost::assign::list_of
		(L"mpg")(L"mpeg")(L"avi")(L"mov")(L"qt")(L"webm")(L"dv")(L"mp4")(L"f4v")(L"flv")(L"mkv")(L"mka")(L"wmv")(L"wma")(L"ogg")(L"divx")(L"xvid")(L"wav")(L"mp3")(L"m2v");
	std::wstring filename = env::media_folder() + L"\\" + params[0];
	
	auto ext = boost::find_if(extensions, [&](const std::wstring& ex)
	{					
		return boost::filesystem::is_regular_file(boost::filesystem::wpath(filename + L"." + ex));
	});

	if(ext == extensions.end())
		return core::frame_producer::empty();

	std::wstring path = filename + L"." + *ext;
	bool loop = boost::find(params, L"LOOP") != params.end();

	int start = -1;
	int length = -1;
	
	auto seek_it = std::find(params.begin(), params.end(), L"SEEK");
	if(seek_it != params.end())
	{
		if(++seek_it != params.end())
			start = boost::lexical_cast<int>(*seek_it);
	}

	return make_safe<ffmpeg_producer>(frame_factory, path, L"", loop, start, length);
}

}