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

#include "muxer/frame_muxer.h"
#include "input/input.h"
#include "util/util.h"
#include "audio/audio_decoder.h"
#include "video/video_decoder.h"
#include "../ffmpeg_error.h"

#include <common/concurrency/throw_away.h>
#include <common/env.h>
#include <common/diagnostics/graph.h>
#include <common/utility/assert.h>

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
#include <boost/algorithm/string/split.hpp>

#include <agents.h>
#include <concrt.h>

#include <vector>
#include <string>

using namespace Concurrency;

namespace caspar { namespace ffmpeg {
		
struct ffmpeg_producer : public core::frame_producer
{	
	const std::wstring														filename_;
	const int																start_;
	const bool																loop_;
	const size_t															length_;
	
	std::unique_ptr<input>													input_;	

	unbounded_buffer<input::target_element_t>								packets_;
	std::shared_ptr<unbounded_buffer<frame_muxer2::video_source_element_t>>	video_;
	std::shared_ptr<unbounded_buffer<frame_muxer2::audio_source_element_t>>	audio_;
	unbounded_buffer<frame_muxer2::target_element_t>						frames_;
		
	const safe_ptr<diagnostics::graph>										graph_;
					
	std::unique_ptr<video_decoder>											video_decoder_;
	std::unique_ptr<audio_decoder>											audio_decoder_;	
	std::unique_ptr<frame_muxer2>											muxer_;

	safe_ptr<core::basic_frame>												last_frame_;
	
public:
	explicit ffmpeg_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filename, const std::wstring& filter, bool loop, int start, size_t length) 
		: filename_(filename)
		, start_(start)
		, loop_(loop)
		, length_(length)
		, last_frame_(core::basic_frame::empty())
	{		
		input_.reset(new input(packets_, graph_, filename_, loop, start, length));

		try
		{
			auto video = std::make_shared<unbounded_buffer<frame_muxer2::video_source_element_t>>();
			video_decoder_.reset(new video_decoder(packets_, *video, *input_->context()));
			video_ = video;
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
			auto audio = std::make_shared<unbounded_buffer<frame_muxer2::audio_source_element_t>>();
			audio_decoder_.reset(new audio_decoder(packets_, *audio, *input_->context(), frame_factory->get_video_format_desc()));
			audio_ = audio;
		}
		catch(averror_stream_not_found&)
		{
			CASPAR_LOG(warning) << "No audio-stream found. Running without video.";	
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			CASPAR_LOG(warning) << "Failed to open audio-stream. Running without audio.";		
		}		

		CASPAR_VERIFY(video_decoder_ || audio_decoder_, ffmpeg_error());
		
		link_throw_away(packets_);
		muxer_.reset(new frame_muxer2(video_.get(), audio_.get(), frames_, video_decoder_ ? video_decoder_->fps() : frame_factory->get_video_format_desc().fps, frame_factory));
				
		graph_->set_color("underflow", diagnostics::color(0.6f, 0.3f, 0.9f));	
		graph_->set_text(print());
		diagnostics::register_graph(graph_);
	}

	~ffmpeg_producer()
	{
		input_->stop();	
	}
						
	virtual safe_ptr<core::basic_frame> receive(int hints)
	{
		win32_exception::install_handler();

		auto frame = core::basic_frame::late();
		
		try
		{		
			frame = last_frame_ = Concurrency::receive(frames_, 10).first;
			graph_->set_text(narrow(print()));
		}
		catch(operation_timed_out&)
		{		
			graph_->add_tag("underflow");	
		}

		return frame;
	}

	virtual safe_ptr<core::basic_frame> last_frame() const
	{
		return disable_audio(last_frame_);
	}
	
	virtual int64_t nb_frames() const
	{
		if(loop_)
			return std::numeric_limits<int64_t>::max();

		// This function estimates nb_frames until input has read all packets for one loop, at which point the count should be accurate.

		int64_t nb_frames = input_->nb_frames();
		if(input_->nb_loops() < 1) // input still hasn't counted all frames
		{
			int64_t video_nb_frames = video_decoder_->nb_frames();
			int64_t audio_nb_frames = audio_decoder_->nb_frames();

			nb_frames = std::min(static_cast<int64_t>(length_), std::max(nb_frames, std::max(video_nb_frames, audio_nb_frames)));
		}

		nb_frames = muxer_->calc_nb_frames(nb_frames);
		
		return nb_frames - start_;
	}

	virtual bool param(const std::wstring& param)
	{
		std::vector<std::wstring> params;
		boost::split(params, param, boost::is_any_of(" "), boost::token_compress_on);

		if(boost::iequals(params.at(0), L"LOOP"))
		{
			input_->loop(boost::lexical_cast<bool>(params.at(1)));
			return true;
		}
		return false;
	}
				
	virtual std::wstring print() const
	{
		if(video_decoder_)
		{
			return L"ffmpeg[" + boost::filesystem::wpath(filename_).filename() + L"|" 
							  + boost::lexical_cast<std::wstring>(video_decoder_->width()) + L"x" + boost::lexical_cast<std::wstring>(video_decoder_->height())
							  + (video_decoder_->is_progressive() ? L"p" : L"i")  + boost::lexical_cast<std::wstring>(video_decoder_->is_progressive() ? video_decoder_->fps() : 2.0 * video_decoder_->fps())
							  + L"]";
		}
		
		return L"ffmpeg[" + boost::filesystem::wpath(filename_).filename() + L"]";
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

	std::vector<std::wstring> params2 = params;
	std::for_each(std::begin(params2), std::end(params2), std::bind(&boost::to_upper<std::wstring>, std::placeholders::_1, std::locale()));

	auto path		= filename + L"." + *ext;
	auto loop		= boost::range::find(params2, L"LOOP") != params2.end();
	auto start		= core::get_param(L"SEEK", params2, 0);
	auto length		= core::get_param(L"LENGTH", params2, std::numeric_limits<size_t>::max());
	auto filter_str = core::get_param<std::wstring>(L"FILTER", params2, L""); 	
		
	boost::replace_all(filter_str, L"DEINTERLACE", L"YADIF=0:-1");
	boost::replace_all(filter_str, L"DEINTERLACE_BOB", L"YADIF=1:-1");
	
	return make_safe<ffmpeg_producer>(frame_factory, path, filter_str, loop, start, length);
}

}}