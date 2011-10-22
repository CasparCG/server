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
#if defined(_MSC_VER)
#pragma warning (disable : 4244)
#endif

#include "../stdafx.h"

#include "input.h"
#include "util.h"
#include "../ffmpeg_error.h"

#include <core/video_format.h>

#include <common/diagnostics/graph.h>
#include <common/exception/exceptions.h>
#include <common/exception/win32_exception.h>

#include <tbb/atomic.h>

#include <agents.h>
#include <concrt_extras.h>
#include <semaphore.h>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavformat/avformat.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

using namespace Concurrency;

namespace caspar { namespace ffmpeg {

static const size_t MAX_TOKENS = 32;
	
struct input::implementation : public Concurrency::agent, boost::noncopyable
{
	input::target_t&						target_;

	const std::wstring						filename_;
	const safe_ptr<AVFormatContext>			format_context_; // Destroy this last
	int										default_stream_index_;
	const boost::iterator_range<AVStream**>	streams_;

	safe_ptr<diagnostics::graph>			graph_;
		
	const bool								loop_;
	const size_t							start_;		
	const size_t							length_;
	size_t									frame_number_;
			
	tbb::atomic<size_t>						nb_frames_;
	tbb::atomic<size_t>						nb_loops_;	
	tbb::atomic<size_t>						packets_count_;
	tbb::atomic<size_t>						packets_size_;

	bool									stop_;

	safe_ptr<semaphore>						semaphore_;
		
public:
	explicit implementation(input::target_t& target,
							const safe_ptr<diagnostics::graph>& graph, 
							const std::wstring& filename, 
							bool loop, 
							size_t start,
							size_t length)
		: target_(target)
		, filename_(filename)
		, format_context_(open_input(filename))		
		, default_stream_index_(av_find_default_stream_index(format_context_.get()))
		, streams_(format_context_->streams, format_context_->streams + format_context_->nb_streams)
		, graph_(graph)
		, loop_(loop)
		, start_(start)
		, length_(length)
		, frame_number_(0)
		, stop_(false)
		, semaphore_(make_safe<semaphore>(8))
	{		
		packets_count_	= 0;
		packets_size_	= 0;
		nb_frames_		= 0;
		nb_loops_		= 0;
				
		av_dump_format(format_context_.get(), 0, narrow(filename).c_str(), 0);
				
		if(start_ > 0)			
			seek_frame(start_);
								
		graph_->set_color("seek", diagnostics::color(1.0f, 0.5f, 0.0f));
	}
	
	void stop()
	{
		stop_ = true;
		agent::wait(this);
	}
	
	virtual void run()
	{
		try
		{
			while(!stop_)
			{
				auto packet = read_next_packet();
				if(!packet)
					break;

				Concurrency::asend(target_, make_safe_ptr(packet));
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}	
	
		BOOST_FOREACH(auto stream, streams_)
			Concurrency::send(target_, eof_packet(stream->index));	

		done();
	}

	std::shared_ptr<AVPacket> read_next_packet()
	{		
		auto packet = create_packet();
		
		int ret = [&]() -> int
		{
			Concurrency::scoped_oversubcription_token oversubscribe;
			return av_read_frame(format_context_.get(), packet.get()); // packet is only valid until next call of av_read_frame. Use av_dup_packet to extend its life.	
		}();

		if(is_eof(ret))														     
		{
			++nb_loops_;
			frame_number_ = 0;

			if(loop_)
			{
				CASPAR_LOG(trace) << print() << " Looping.";
				seek_frame(start_, AVSEEK_FLAG_BACKWARD);		
				return read_next_packet();
			}	
			else
			{
				CASPAR_LOG(trace) << print() << " Stopping.";
				return nullptr;
			}
		}

		THROW_ON_ERROR(ret, print(), "av_read_frame");

		if(packet->stream_index == default_stream_index_)
		{
			if(nb_loops_ == 0)
				++nb_frames_;
			++frame_number_;
		}

		THROW_ON_ERROR2(av_dup_packet(packet.get()), print());
				
		// Make sure that the packet is correctly deallocated even if size and data is modified during decoding.
		auto size = packet->size;
		auto data = packet->data;			

		packet = safe_ptr<AVPacket>(packet.get(), [=](AVPacket*)
		{
			packet->size = size;
			packet->data = data;
			--packets_count_;
			semaphore_->release();
		});
		semaphore_->acquire();

		++packets_count_;
							
		return packet;
	}

	void seek_frame(int64_t frame, int flags = 0)
	{  			
		if(flags == AVSEEK_FLAG_BACKWARD)
		{
			// Fix VP6 seeking
			int vid_stream_index = av_find_best_stream(format_context_.get(), AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0);
			if(vid_stream_index >= 0)
			{
				auto codec_id = format_context_->streams[vid_stream_index]->codec->codec_id;
				if(codec_id == CODEC_ID_VP6A || codec_id == CODEC_ID_VP6F || codec_id == CODEC_ID_VP6)
					flags |= AVSEEK_FLAG_BYTE;
			}
		}

		THROW_ON_ERROR2(av_seek_frame(format_context_.get(), default_stream_index_, frame, flags), print());	
		auto packet = create_packet();
		packet->size = 0;

		BOOST_FOREACH(auto stream, streams_)
			Concurrency::asend(target_, loop_packet(stream->index));	

		graph_->add_tag("seek");		
	}		

	bool is_eof(int ret)
	{
		if(ret == AVERROR(EIO))
			CASPAR_LOG(trace) << print() << " Received EIO, assuming EOF. " << nb_frames_;
		if(ret == AVERROR_EOF)
			CASPAR_LOG(trace) << print() << " Received EOF. " << nb_frames_;

		CASPAR_VERIFY(ret >= 0 || ret == AVERROR_EOF || ret == AVERROR(EIO), ffmpeg_error() << source_info(narrow(print())));

		return ret == AVERROR_EOF || ret == AVERROR(EIO) || frame_number_ >= length_; // av_read_frame doesn't always correctly return AVERROR_EOF;
	}
	
	std::wstring print() const
	{
		return L"ffmpeg_input[" + filename_ + L")]";
	}
};

input::input(input::target_t& target,
			 const safe_ptr<diagnostics::graph>& graph, 
			 const std::wstring& filename, 
			 bool loop, 
			 size_t start, 
			 size_t length)
	: impl_(new implementation(target, graph, filename, loop, start, length))
{
}

safe_ptr<AVFormatContext> input::context()
{
	return safe_ptr<AVFormatContext>(impl_->format_context_);
}

size_t input::nb_frames() const
{
	return impl_->nb_frames_;
}

size_t input::nb_loops() const 
{
	return impl_->nb_loops_;
}

void input::start()
{
	impl_->start();
}

void input::stop()
{
	impl_->stop();
}

}}