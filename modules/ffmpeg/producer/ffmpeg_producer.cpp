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

#include "frame_muxer.h"
#include "input.h"
#include "util.h"
#include "audio/audio_decoder.h"
#include "video/video_decoder.h"

#include <common/env.h>
#include <common/utility/assert.h>
#include <common/diagnostics/graph.h>

#include <core/video_format.h>
#include <core/producer/frame_producer.h>
#include <core/producer/frame/frame_factory.h>
#include <core/producer/frame/basic_frame.h>

#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>
#include <boost/timer.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/find.hpp>

#include <ppl.h>

#include <agents.h>

using namespace Concurrency;

namespace caspar { namespace ffmpeg {
				
struct ffmpeg_producer : public core::frame_producer
{
	const safe_ptr<diagnostics::graph>				graph_;

	const std::wstring								filename_;
	
	boost::timer									frame_timer_;
	boost::timer									video_timer_;
	boost::timer									audio_timer_;
					
	const safe_ptr<core::frame_factory>				frame_factory_;
	const core::video_format_desc					format_desc_;
	
	unbounded_buffer<safe_ptr<AVPacket>>			packets_;
	unbounded_buffer<safe_ptr<AVPacket>>			video_packets_;
	unbounded_buffer<safe_ptr<AVPacket>>			audio_packets_;
	unbounded_buffer<safe_ptr<core::basic_frame>>	frames_;

	input											input_;	
	video_decoder									video_decoder_;
	audio_decoder									audio_decoder_;	
	double											fps_;
	frame_muxer										muxer_;

	const int										start_;
	const bool										loop_;
	const size_t									length_;

	safe_ptr<core::basic_frame>						last_frame_;

	const size_t									width_;
	const size_t									height_;
	bool											is_progressive_;

	
public:
	explicit ffmpeg_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filename, const std::wstring& filter, bool loop, int start, size_t length) 
		: filename_(filename)
		, frame_factory_(frame_factory)		
		, format_desc_(frame_factory->get_video_format_desc())
		, input_(packets_, graph_, filename_, loop, start, length)
		, video_decoder_(video_packets_, input_.context(), frame_factory)
		, audio_decoder_(audio_packets_, input_.context(), frame_factory->get_video_format_desc())
		, fps_(video_decoder_.fps())
		, muxer_(frames_, fps_, frame_factory, filter)
		, start_(start)
		, loop_(loop)
		, length_(length)
		, last_frame_(core::basic_frame::empty())
		, width_(video_decoder_.width())
		, height_(video_decoder_.height())
		, is_progressive_(true)
	{
		graph_->add_guide("frame-time", 0.5);
		graph_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
		graph_->set_color("underflow", diagnostics::color(0.6f, 0.3f, 0.9f));	
		diagnostics::register_graph(graph_);
	}

	~ffmpeg_producer()
	{
		input_.stop();
	}
			
	virtual safe_ptr<core::basic_frame> receive(int hints)
	{
		auto frame = core::basic_frame::late();
		
		frame_timer_.restart();
		
		for(int n = 0; n < 64 && !try_receive(frames_, frame); ++n)
			decode_frame(hints);
		
		graph_->update_value("frame-time", static_cast<float>(frame_timer_.elapsed()*format_desc_.fps*0.5));

		if(frame != core::basic_frame::late())
			last_frame_ = frame;	
		else
		{
			if(input_.eof())
				return core::basic_frame::eof();
			else			
				graph_->add_tag("underflow");	
		}

		graph_->set_text(narrow(print()));
		
		return frame;
	}

	virtual safe_ptr<core::basic_frame> last_frame() const
	{
		return disable_audio(last_frame_);
	}
	
	void decode_frame(int hints)
	{
		if(!muxer_.need_video())
		{
			std::shared_ptr<AVFrame> video;
			while(!video)
			{
				auto pkt = create_packet();
				if(try_receive(packets_, pkt))
				{
					send(video_packets_, pkt);
					send(audio_packets_, pkt);
				}
				video = video_decoder_.poll();
				Context::Yield();
			}
			is_progressive_ = video ? video->interlaced_frame == 0 : is_progressive_;
			muxer_.push(make_safe_ptr(video), hints);	
		}

		Context::Yield();
		
		if(!muxer_.need_audio())
		{
			std::shared_ptr<core::audio_buffer> audio;
			while(!audio)
			{
				auto pkt = create_packet();
				if(try_receive(packets_, pkt))
				{
					send(video_packets_, pkt);
					send(audio_packets_, pkt);
				}
				audio = audio_decoder_.poll();
				Context::Yield();
			}
			muxer_.push(make_safe_ptr(audio));	
		}
	}

	virtual int64_t nb_frames() const 
	{
		if(loop_)
			return std::numeric_limits<int64_t>::max();

		// This function estimates nb_frames until input has read all packets for one loop, at which point the count should be accurate.

		int64_t nb_frames = input_.nb_frames();
		if(input_.nb_loops() < 1) // input still hasn't counted all frames
		{
			int64_t video_nb_frames = video_decoder_.nb_frames();

			nb_frames = std::min(static_cast<int64_t>(length_), std::max(nb_frames, video_nb_frames));
		}

		nb_frames = muxer_.calc_nb_frames(nb_frames);

		// TODO: Might need to scale nb_frames av frame_muxer transformations.

		return nb_frames - start_;
	}
				
	virtual std::wstring print() const
	{
		return L"ffmpeg[" + boost::filesystem::wpath(filename_).filename() + L"|" 
						  + boost::lexical_cast<std::wstring>(width_) + L"x" + boost::lexical_cast<std::wstring>(height_)
						  + (is_progressive_ ? L"p" : L"i")  + boost::lexical_cast<std::wstring>(is_progressive_ ? fps_ : 2.0 * fps_)
						  + L"]";
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
	
	return make_safe<ffmpeg_producer>(frame_factory, path, filter_str, loop, start, length);
}

}}