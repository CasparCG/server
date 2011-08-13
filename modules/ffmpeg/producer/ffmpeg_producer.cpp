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

#include <boost/assign.hpp>
#include <boost/timer.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/find.hpp>

#include <tbb/parallel_invoke.h>

namespace caspar {
			
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
	frame_muxer										muxer_;

	int												late_frames_;
	const int										start_;
	const bool										loop_;

	safe_ptr<core::basic_frame>						last_frame_;
	bool											eof_;
	
public:
	explicit ffmpeg_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filename, const std::wstring& filter, bool loop, int start, int length) 
		: filename_(filename)
		, graph_(diagnostics::create_graph(narrow(print())))
		, frame_factory_(frame_factory)		
		, format_desc_(frame_factory->get_video_format_desc())
		, input_(graph_, filename_, loop, start, length)
		, video_decoder_(input_.context(), frame_factory, filter)
		, audio_decoder_(input_.context(), frame_factory->get_video_format_desc())
		, muxer_(video_decoder_.fps(), frame_factory)
		, late_frames_(0)
		, start_(start)
		, loop_(loop)
		, last_frame_(core::basic_frame::empty())
		, eof_(false)
	{
		graph_->add_guide("frame-time", 0.5);
		graph_->set_color("frame-time", diagnostics::color(1.0f, 0.0f, 0.0f));
		graph_->set_color("underflow", diagnostics::color(0.6f, 0.3f, 0.9f));		
		
		for(int n = 0; n < 32 && muxer_.empty(); ++n)
			decode_frame(0);
	}
			
	virtual safe_ptr<core::basic_frame> receive(int hints)
	{
		if(eof_)
			return last_frame();

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
			{
				eof_ = true;
				return last_frame();
			}
			else
			{
				graph_->add_tag("underflow");	
				++late_frames_;		
			}
		}
		
		return frame;
	}

	virtual safe_ptr<core::basic_frame> last_frame() const
	{
		return disable_audio(last_frame_);
	}

	void decode_frame(int hints)
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
		
		tbb::parallel_invoke(
		[&]
		{
			if(muxer_.video_ready())
				return;

			auto video_frames = video_decoder_.poll();
			BOOST_FOREACH(auto& video, video_frames)		
				muxer_.push(video, hints);	
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

			nb_frames = std::max(nb_frames, std::max(video_nb_frames, audio_nb_frames));
		}

		nb_frames = muxer_.calc_nb_frames(nb_frames);

		// TODO: Might need to scale nb_frames av frame_muxer transformations.

		return nb_frames + late_frames_ - start_;
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

	size_t start = 0;
	size_t length = std::numeric_limits<size_t>::max();
	
	auto seek_it = boost::find(params, L"SEEK");
	if(seek_it != params.end())
	{
		if(++seek_it != params.end())
			start = boost::lexical_cast<size_t>(*seek_it);
	}
	
	auto length_it = boost::find(params, L"LENGTH");
	if(length_it != params.end())
	{
		if(++length_it != params.end())
			length = boost::lexical_cast<size_t>(*length_it);
	}

	std::wstring filter = L"";
	auto filter_it = boost::find(params, L"FILTER");
	if(filter_it != params.end())
	{
		if(++filter_it != params.end())
			filter = *filter_it;
	}

	return make_safe<ffmpeg_producer>(frame_factory, path, filter, loop, start, length);
}

}