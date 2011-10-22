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

#include <tbb/parallel_invoke.h>

namespace caspar { namespace ffmpeg {
				
struct ffmpeg_producer : public core::frame_producer
{
	const std::wstring								filename_;
	
	const safe_ptr<diagnostics::graph>				graph_;
	boost::timer									frame_timer_;
	boost::timer									video_timer_;
	boost::timer									audio_timer_;
					
	const safe_ptr<core::frame_factory>				frame_factory_;
	const core::video_format_desc					format_desc_;

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
		, graph_(diagnostics::create_graph(""))
		, frame_factory_(frame_factory)		
		, format_desc_(frame_factory->get_video_format_desc())
		, input_(graph_, filename_, loop, start, length)
		, video_decoder_(input_.context(), frame_factory, filter)
		, audio_decoder_(input_.context(), frame_factory->get_video_format_desc())
		, fps_(video_decoder_.fps())
		, muxer_(fps_, frame_factory)
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
		
		// Do some pre-work in order to not block rendering thread for initialization and allocations.

		push_packets();
		auto video_frames = video_decoder_.poll();
		if(!video_frames.empty())
		{
			auto& video_frame = video_frames.front();
			auto  desc		  = get_pixel_format_desc(static_cast<PixelFormat>(video_frame->format), video_frame->width, video_frame->height);
			if(desc.pix_fmt == core::pixel_format::invalid)
				get_pixel_format_desc(PIX_FMT_BGRA, video_frame->width, video_frame->height);
			
			for(int n = 0; n < 3; ++n)
				frame_factory->create_frame(this, desc);
		}
		BOOST_FOREACH(auto& video, video_frames)			
			muxer_.push(video, 0);	
	}
			
	virtual safe_ptr<core::basic_frame> receive(int hints)
	{
		auto frame = core::basic_frame::late();
		
		frame_timer_.restart();
		
		for(int n = 0; n < 64 && muxer_.empty(); ++n)
			decode_frame(hints);
		
		graph_->update_value("frame-time", static_cast<float>(frame_timer_.elapsed()*format_desc_.fps*0.5));

		if(!muxer_.empty())
			frame = last_frame_ = muxer_.pop();	
		else
		{
			if(input_.eof())
				return core::basic_frame::eof();
			else			
				graph_->add_tag("underflow");	
		}

		graph_->update_text(narrow(print()));
		
		return frame;
	}

	virtual safe_ptr<core::basic_frame> last_frame() const
	{
		return disable_audio(last_frame_);
	}

	void push_packets()
	{
		for(int n = 0; n < 16 && ((!muxer_.video_ready() && !video_decoder_.ready()) ||	(!muxer_.audio_ready() && !audio_decoder_.ready())); ++n) 
		{
			std::shared_ptr<AVPacket> pkt;
			if(input_.try_pop(pkt))
			{
				video_decoder_.push(pkt);
				audio_decoder_.push(pkt);
			}
		}
	}

	void decode_frame(int hints)
	{
		push_packets();
		
		tbb::parallel_invoke(
		[&]
		{
			if(muxer_.video_ready())
				return;

			auto video_frames = video_decoder_.poll();
			BOOST_FOREACH(auto& video, video_frames)	
			{
				is_progressive_ = video ? video->interlaced_frame == 0 : is_progressive_;
				muxer_.push(video, hints);	
			}
		},
		[&]
		{
			if(muxer_.audio_ready())
				return;
					
			auto audio_samples = audio_decoder_.poll();
			BOOST_FOREACH(auto& audio, audio_samples)
				muxer_.push(audio);				
		});

		muxer_.commit();
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
			int64_t audio_nb_frames = audio_decoder_.nb_frames();

			nb_frames = std::min(static_cast<int64_t>(length_), std::max(nb_frames, std::max(video_nb_frames, audio_nb_frames)));
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