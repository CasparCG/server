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
#include "audio/audio_decoder.h"
#include "video/video_decoder.h"

#include <common/utility/assert.h>
#include <common/utility/timer.h>
#include <common/diagnostics/graph.h>

#include <core/mixer/write_frame.h>
#include <core/video_format.h>
#include <core/producer/frame/audio_transform.h>
#include <core/producer/frame/basic_frame.h>
#include <core/producer/color/color_producer.h>

#include <common/env.h>

#include <boost/timer.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>

#include <tbb/task_group.h>

#include <deque>
#include <vector>

namespace caspar {
			
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
	
	frame_muxer										muxer_;

	const int										start_;
	int64_t											nb_frames_;
	const bool										loop_;

	safe_ptr<core::basic_frame>						last_frame_;
	
	tbb::task_group									tasks_;

public:
	explicit ffmpeg_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filename, const std::wstring& filter, bool loop, int start, int length) 
		: filename_(filename)
		, graph_(diagnostics::create_graph(narrow(print())))
		, frame_factory_(frame_factory)		
		, format_desc_(frame_factory->get_video_format_desc())
		, input_(graph_, filename_, loop, start, length)
		, video_decoder_(input_.context(), frame_factory, filter)
		, audio_decoder_(input_.context(), frame_factory->get_video_format_desc())
		, muxer_(video_decoder_.fps(), format_desc_, frame_factory)
		, start_(start)
		, nb_frames_(video_decoder_.nb_frames() - start)
		, loop_(loop)
		, last_frame_(core::basic_frame::empty())
	{
		graph_->add_guide("frame-time", 0.5);
		graph_->set_color("frame-time", diagnostics::color(1.0f, 0.0f, 0.0f));
		graph_->set_color("underflow", diagnostics::color(0.6f, 0.3f, 0.9f));		
		
		for(int n = 0; n < 128 && muxer_.size() < 2; ++n)
			decode_frame();
	}

	~ffmpeg_producer()
	{
		tasks_.cancel();
		tasks_.wait();
	}
	
	virtual safe_ptr<core::basic_frame> receive()
	{
		tasks_.wait();

		auto frame = core::basic_frame::late();
		
		if(!muxer_.empty())
			frame = last_frame_ = muxer_.pop();	
		else
		{
			if(input_.eof())
				frame =  core::basic_frame::eof();
			else
			{
				graph_->add_tag("underflow");	
				++nb_frames_;		
			}
		}

		tasks_.run([this]
		{
			frame_timer_.restart();

			for(int n = 0; n < 64 && muxer_.size() < 2; ++n)
				decode_frame();

			graph_->update_value("frame-time", static_cast<float>(frame_timer_.elapsed()*format_desc_.fps*0.5));
		});
		
		return frame;
	}

	virtual safe_ptr<core::basic_frame> last_frame() const
	{
		return disable_audio(last_frame_);
	}

	void decode_frame()
	{
		for(int n = 0; n < 32 && ((!muxer_.video_ready() && !video_decoder_.ready()) ||	(!muxer_.audio_ready() && !audio_decoder_.ready())); ++n) 
		{
			std::shared_ptr<AVPacket> pkt;
			if(input_.try_pop(pkt))
			{
				video_decoder_.push(pkt);
				audio_decoder_.push(pkt);
			}
		}

		decltype(video_decoder_.poll()) video_frames;
		decltype(audio_decoder_.poll()) audio_samples;
		
		tbb::parallel_invoke(
		[&]
		{
			if(!muxer_.video_ready())
				video_frames = video_decoder_.poll();
		},
		[&]
		{
			if(!muxer_.audio_ready())
				audio_samples = audio_decoder_.poll();
		});
		
		BOOST_FOREACH(auto& audio, audio_samples)
			muxer_.push(audio);

		BOOST_FOREACH(auto& video, video_frames)
			muxer_.push(video);		
	}

	virtual int64_t nb_frames() const
	{
		return loop_ ? 0 : nb_frames_;
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

	std::wstring filter = L"";
	auto filter_it = std::find(params.begin(), params.end(), L"FILTER");
	if(filter_it != params.end())
	{
		if(++filter_it != params.end())
			filter = *filter_it;
	}

	return make_safe<ffmpeg_producer>(frame_factory, path, filter, loop, start, length);
}

}