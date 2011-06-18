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

#include <common/env.h>

#include <tbb/parallel_invoke.h>

#include <boost/timer.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>

#include <deque>

namespace caspar {
	
struct ffmpeg_producer : public core::frame_producer
{
	const std::wstring						filename_;
	
	const safe_ptr<diagnostics::graph>		graph_;
	boost::timer							frame_timer_;
					
	const safe_ptr<core::frame_factory>		frame_factory_;

	input									input_;	
	std::unique_ptr<video_decoder>			video_decoder_;
	std::unique_ptr<audio_decoder>			audio_decoder_;

	std::deque<std::pair<int, std::vector<short>>> audio_chunks_;
	std::deque<std::pair<int, safe_ptr<core::write_frame>>> video_frames_;
public:
	explicit ffmpeg_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filename, const std::string& filter, bool loop, int start, int length) 
		: filename_(filename)
		, graph_(diagnostics::create_graph(narrow(print())))
		, frame_factory_(frame_factory)		
		, input_(safe_ptr<diagnostics::graph>(graph_), filename_, loop, start, length)
	{
		graph_->add_guide("frame-time", 0.5);
		graph_->set_color("frame-time",  diagnostics::color(1.0f, 0.0f, 0.0f));
		graph_->set_color("underflow", diagnostics::color(0.6f, 0.3f, 0.9f));		
		
		double frame_time = 1.0f/input_.fps();
		double format_frame_time = 1.0/frame_factory->get_video_format_desc().fps;
		if(abs(frame_time - format_frame_time) > 0.0001 && abs(frame_time - format_frame_time/2) > 0.0001)
			CASPAR_LOG(warning) << print() << L" Invalid framerate detected. This may cause distorted audio during playback. frame-time: " << frame_time;
		
		video_decoder_.reset(input_.get_video_codec_context() ? 
			new video_decoder(input_, frame_factory, filter) : nullptr);
			
		audio_decoder_.reset(input_.get_audio_codec_context() ? 
			new audio_decoder(input_, frame_factory->get_video_format_desc()) : nullptr);		
					
		// Fill buffers.
		for(size_t n = 0; n < 2; ++n)
			decode_packets();
	}

	virtual safe_ptr<core::basic_frame> receive()
	{
		frame_timer_.restart();

		auto result = decode_frame();
				
		graph_->update_value("frame-time", static_cast<float>(frame_timer_.elapsed()*frame_factory_->get_video_format_desc().fps*0.5));
					
		return result;
	}
	
	virtual std::wstring print() const
	{
		return L"ffmpeg[" + boost::filesystem::wpath(filename_).filename() + L"]";
	}

	void decode_packets()
	{
		tbb::parallel_invoke
		(
			[&]
			{
				if(video_decoder_ && video_frames_.size() < 3)
					boost::range::push_back(video_frames_, video_decoder_->receive());		
			}, 
			[&]
			{
				if(audio_decoder_ && audio_chunks_.size() < 3)
					boost::range::push_back(audio_chunks_, audio_decoder_->receive());				
			}
		);
		
		// If video is on first frame, sync with audio
		if(audio_decoder_ && video_decoder_ && !video_frames_.empty() && !audio_chunks_.empty() &&
		   video_frames_.front().first == 0 && audio_chunks_.front().first != 0)
		{
			audio_decoder_->restart(); // Notify decoder to wait for eof which was sent with video eof.
			audio_chunks_ = audio_decoder_->receive();		
		}
		
		CASPAR_ASSERT(!(video_decoder_ && audio_decoder_ && !video_frames_.empty() && !audio_chunks_.empty()) ||
				      video_frames_.front().first == audio_chunks_.front().first);
	}
	
	safe_ptr<core::basic_frame> decode_frame()
	{
		decode_packets();

		if(video_decoder_ && audio_decoder_ && !video_frames_.empty() && !audio_chunks_.empty())
		{
			auto frame		  = std::move(video_frames_.front().second);		
			auto frame_number = video_frames_.front().first;
			video_frames_.pop_front();
				
			frame->audio_data() = std::move(audio_chunks_.front().second);
			audio_chunks_.pop_front();
			
			if(!video_frames_.empty())
			{
				if(video_frames_.front().first == frame_number)
				{
					auto frame2 = video_frames_.front().second;
					video_frames_.pop_front();

					return core::basic_frame::interlace(frame, frame2, frame_factory_->get_video_format_desc().mode);
				}
			}

			return frame;
		}
		else if(video_decoder_ && !audio_decoder_ && !video_frames_.empty())
		{
			auto frame		  = std::move(video_frames_.front().second);		
			auto frame_number = video_frames_.front().first;
			video_frames_.pop_front();

			frame->get_audio_transform().set_has_audio(false);	
			
			if(!video_frames_.empty())
			{
				if(video_frames_.front().first == frame_number)
				{
					auto frame2 = video_frames_.front().second;
					video_frames_.pop_front();

					return core::basic_frame::interlace(frame, frame2, frame_factory_->get_video_format_desc().mode);
				}
			}

			return frame;
		}
		else if(audio_decoder_ && !video_decoder_ && !audio_chunks_.empty())
		{
			auto frame = frame_factory_->create_frame(this, 1, 1);
			std::fill(frame->image_data().begin(), frame->image_data().end(), 0);
				
			frame->audio_data() = std::move(audio_chunks_.front().second);
			audio_chunks_.pop_front();

			return frame;
		}
		else if(!input_.is_running() || (!video_decoder_ && !audio_decoder_))
		{
			return core::basic_frame::eof();
		}
		else
		{
			graph_->add_tag("underflow");
			return core::basic_frame::late();
		}
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
	
	std::string filter;

	auto filter_it = std::find(params.begin(), params.end(), L"FILTER");
	if(filter_it != params.end())
	{
		if(++filter_it != params.end())
			filter = narrow(*filter_it);
		std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
	}

	return make_safe<ffmpeg_producer>(frame_factory, path, filter, loop, start, length);
}

}