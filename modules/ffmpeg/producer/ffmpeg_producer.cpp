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

#include <common/utility/timer.h>
#include <common/diagnostics/graph.h>

#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/write_frame.h>
#include <core/producer/frame/audio_transform.h>
#include <core/video_format.h>

#include <common/env.h>
#include <common/utility/timer.h>
#include <common/utility/assert.h>

#include <tbb/parallel_invoke.h>

#include <boost/timer.hpp>

#include <deque>
#include <functional>

namespace caspar {
	
struct ffmpeg_producer : public core::frame_producer
{
	const std::wstring					filename_;
	const bool							loop_;
	
	std::shared_ptr<diagnostics::graph>	graph_;
	boost::timer						perf_timer_;
		
	std::deque<safe_ptr<core::write_frame>>	video_frame_buffer_;	
	std::deque<std::vector<short>>			audio_chunk_buffer_;

	std::queue<safe_ptr<core::basic_frame>>	ouput_channel_;
		
	std::shared_ptr<core::frame_factory>	frame_factory_;

	input								input_;	
	std::unique_ptr<video_decoder>		video_decoder_;
	std::unique_ptr<audio_decoder>		audio_decoder_;
public:
	explicit ffmpeg_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filename, bool loop, int start_frame, int end_frame) 
		: filename_(filename)
		, loop_(loop) 
		, graph_(diagnostics::create_graph(narrow(print())))
		, frame_factory_(frame_factory)		
		, input_(safe_ptr<diagnostics::graph>(graph_), filename_, loop_, start_frame, end_frame)
		, video_decoder_(input_.get_video_codec_context().get() ? new video_decoder(input_.get_video_codec_context().get(), frame_factory) : nullptr)
		, audio_decoder_(input_.get_audio_codec_context().get() ? new audio_decoder(input_.get_audio_codec_context().get(), frame_factory->get_video_format_desc().fps) : nullptr)
	{
		graph_->add_guide("frame-time", 0.5);
		graph_->set_color("frame-time",  diagnostics::color(1.0f, 0.0f, 0.0f));
		graph_->set_color("underflow", diagnostics::color(0.6f, 0.3f, 0.9f));		
		
		double frame_time = 1.0f/input_.fps();
		double format_frame_time = 1.0/frame_factory->get_video_format_desc().fps;
		if(abs(frame_time - format_frame_time) > 0.0001)
			CASPAR_LOG(warning) << print() << L" Invalid framerate detected. This may cause distorted audio during playback. frame-time: " << frame_time;
	}

	virtual safe_ptr<core::basic_frame> receive()
	{
		perf_timer_.restart();

		for(size_t n = 0; ouput_channel_.size() < 2 && input_.has_packet() && n < 32; ++n) // 32 packets should be enough. Otherwise there probably was an error and we want to avoid infinite recursion.
		{	
			tbb::parallel_invoke
			(
				[&]
				{
					if(video_frame_buffer_.size() < 3)
						try_decode_video_packet(input_.get_video_packet());
				}, 
				[&]
				{
					if(audio_chunk_buffer_.size() < 3)
						try_decode_audio_packet(input_.get_audio_packet());
				}
			);			

			merge_audio_and_video();	
		}
		
		graph_->update_value("frame-time", static_cast<float>(perf_timer_.elapsed()*frame_factory_->get_video_format_desc().fps*0.5));
					
		return get_next_frame();
	}

	virtual std::wstring print() const
	{
		return L"ffmpeg[" + boost::filesystem::wpath(filename_).filename() + L"]";
	}

	void try_decode_video_packet(const std::shared_ptr<aligned_buffer>& video_packet)
	{
		if(video_decoder_) // Video Decoding.
		{
			try
			{
				auto frame = video_decoder_->execute(this, video_packet);
				if(frame)
					video_frame_buffer_.push_back(make_safe(std::move(frame)));
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				video_decoder_.reset();
				CASPAR_LOG(warning) << print() << " removed video-stream.";
			}
		}
	}

	void try_decode_audio_packet(const std::shared_ptr<aligned_buffer>& audio_packet)
	{
		if(audio_decoder_) // Audio Decoding.
		{
			try
			{
				auto chunks = audio_decoder_->execute(audio_packet);
				audio_chunk_buffer_.insert(audio_chunk_buffer_.end(), chunks.begin(), chunks.end());
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				audio_decoder_.reset();
				CASPAR_LOG(warning) << print() << " removed audio-stream.";
			}
		}
	}

	void merge_audio_and_video()
	{		
		std::shared_ptr<core::write_frame> frame;	

		if(!video_frame_buffer_.empty() && !audio_chunk_buffer_.empty())
		{
			frame = video_frame_buffer_.front();				
			video_frame_buffer_.pop_front();
				
			frame->audio_data() = std::move(audio_chunk_buffer_.front());
			audio_chunk_buffer_.pop_front();	
		}
		else if(!video_frame_buffer_.empty() && !audio_decoder_)
		{
			frame = video_frame_buffer_.front();				
			video_frame_buffer_.pop_front();
			frame->get_audio_transform().set_has_audio(false);	
		}
		else if(!audio_chunk_buffer_.empty() && !video_decoder_)
		{
			frame = frame_factory_->create_frame(this, 1, 1);
			std::fill(frame->image_data().begin(), frame->image_data().end(), 0);
				
			frame->audio_data() = std::move(audio_chunk_buffer_.front());
			audio_chunk_buffer_.pop_front();
		}
		
		if(frame)
			ouput_channel_.push(make_safe(frame));			
	}
		
	safe_ptr<core::basic_frame> get_next_frame()
	{
		if(is_eof())
			return core::basic_frame::eof();

		if(ouput_channel_.empty())
		{
			graph_->add_tag("underflow");
			return core::basic_frame::late();			
		}

		auto frame = std::move(ouput_channel_.front());
		ouput_channel_.pop();		
		return frame;
	}

	bool is_eof() const
	{
		return ouput_channel_.empty() && (!video_decoder_ && !audio_decoder_) || !input_.is_running();
	}
};

safe_ptr<core::frame_producer> create_ffmpeg_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::vector<std::wstring>& params)
{		
	static const std::vector<std::wstring> extensions = boost::assign::list_of
		(L"mpg")(L"mpeg")(L"avi")(L"mov")(L"qt")(L"webm")(L"dv")(L"mp4")(L"f4v")(L"flv")(L"mkv")(L"mka")(L"wmv")(L"wma")(L"ogg")(L"divx")(L"xvid")(L"wav")(L"mp3")(L"m2v");
	std::wstring filename = env::media_folder() + L"\\" + params[0];
	
	auto ext = std::find_if(extensions.begin(), extensions.end(), [&](const std::wstring& ex) -> bool
		{					
			return boost::filesystem::is_regular_file(boost::filesystem::wpath(filename).replace_extension(ex));
		});

	if(ext == extensions.end())
		return core::frame_producer::empty();

	std::wstring path = filename + L"." + *ext;
	bool loop = std::find(params.begin(), params.end(), L"LOOP") != params.end();

	static const boost::wregex expr(L"\\((?<START>\\d+)(,(?<END>\\d+)?)?\\)");//(,(?<END>\\d+))?\\]"); // boost::regex has no repeated captures?
	boost::wsmatch what;
	auto it = std::find_if(params.begin(), params.end(), [&](const std::wstring& str)
	{
		return boost::regex_match(str, what, expr);
	});

	int start = -1;
	int end = -1;

	if(it != params.end())
	{
		start = lexical_cast_or_default(what["START"].str(), -1);
		if(what["END"].matched)
			end = lexical_cast_or_default(what["END"].str(), -1);
	}
	
	return make_safe<ffmpeg_producer>(frame_factory, path, loop, start, end);
}

}