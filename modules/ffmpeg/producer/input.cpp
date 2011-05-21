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

#include "..\stdafx.h"

#include "input.h"
#include "../ffmpeg_error.h"
#include "../tbb_avcodec.h"

#include <core/video_format.h>

#include <common/concurrency/executor.h>
#include <common/diagnostics/graph.h>

#include <tbb/concurrent_queue.h>
#include <tbb/mutex.h>

#include <boost/range/iterator_range.hpp>
#include <boost/range/algorithm.hpp>

extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavformat/avformat.h>
}

namespace caspar {
	
static const size_t PACKET_BUFFER_COUNT = 100; // Assume that av_read_frame distance between audio and video packets is less than PACKET_BUFFER_COUNT.

class stream
{
	std::shared_ptr<AVCodecContext>	ctx_;
	int index_;
	tbb::concurrent_bounded_queue<std::shared_ptr<AVPacket>> buffer_;

public:

	stream()
		: index_(-1)
	{
		buffer_.set_capacity(PACKET_BUFFER_COUNT);
	}

	int open(std::shared_ptr<AVFormatContext>& fctx, AVMediaType media_type)
	{		
		const auto streams = boost::iterator_range<AVStream**>(fctx->streams, fctx->streams+fctx->nb_streams);
		const auto stream = boost::find_if(streams, [&](AVStream* stream) 
		{
			return stream && stream->codec->codec_type == media_type;
		});
		
		if(stream == streams.end()) 
			return AVERROR_STREAM_NOT_FOUND;
		
		auto codec = avcodec_find_decoder((*stream)->codec->codec_id);			
		if(!codec)
			return AVERROR_DECODER_NOT_FOUND;
			
		index_ = (*stream)->index;

		int errn = tbb_avcodec_open((*stream)->codec, codec);
		if(errn >= 0)
		{
			ctx_.reset((*stream)->codec, tbb_avcodec_close);

			// Some files give an invalid time_base numerator, try to fix it.
			if(ctx_ && ctx_->time_base.num == 1)
				ctx_->time_base.num = static_cast<int>(std::pow(10.0, static_cast<int>(std::log10(static_cast<float>(ctx_->time_base.den)))-1));
		}
		return errn;	
	}

	std::shared_ptr<AVPacket> pop()
	{
		std::shared_ptr<AVPacket> pkt;
		buffer_.try_pop(pkt);
		return pkt;
	}

	void push(const std::shared_ptr<AVPacket>& pkt)
	{
		if(pkt->stream_index != index_)
			return;

		av_dup_packet(pkt.get());
		buffer_.push(pkt);	
	}

	const std::shared_ptr<AVCodecContext>& ctx() { return ctx_; }

	operator bool(){return ctx_ != nullptr;}

	double fps() const { return !ctx_ ? -1.0 : static_cast<double>(ctx_->time_base.den) / static_cast<double>(ctx_->time_base.num); }

	bool empty() const { return buffer_.empty();}
	int size() const { return buffer_.size();}
};
		
struct input::implementation : boost::noncopyable
{		
	safe_ptr<diagnostics::graph> graph_;

	std::shared_ptr<AVFormatContext> format_context_;	// Destroy this last
		
	const std::wstring	filename_;
	const bool			loop_;
	const int			start_;		
	double				fps_;

	stream video_stream_;
	stream audio_stream_;
		
	std::exception_ptr exception_;
	executor executor_;
public:
	explicit implementation(const safe_ptr<diagnostics::graph>& graph, const std::wstring& filename, bool loop, int start) 
		: graph_(graph)
		, loop_(loop)
		, filename_(filename)
		, executor_(print())
		, start_(std::max(start, 0))
	{		
		graph_->set_color("input-buffer", diagnostics::color(1.0f, 1.0f, 0.0f));
		graph_->set_color("seek", diagnostics::color(0.5f, 1.0f, 0.5f));	
		
		int errn;

		AVFormatContext* weak_format_context_ = nullptr;
		errn = av_open_input_file(&weak_format_context_, narrow(filename).c_str(), nullptr, 0, nullptr);
		if(errn < 0 || weak_format_context_ == nullptr)
		{	
			BOOST_THROW_EXCEPTION(
				file_read_error() << 
				source_info(narrow(print())) << 
				msg_info(av_error_str(errn)) <<
				boost::errinfo_api_function("av_open_input_file") <<
				boost::errinfo_errno(AVUNERROR(errn)) <<
				boost::errinfo_file_name(narrow(filename)));
		}

		format_context_.reset(weak_format_context_, av_close_input_file);
			
		errn = av_find_stream_info(format_context_.get());
		if(errn < 0)
		{	
			BOOST_THROW_EXCEPTION(
				file_read_error() << 
				source_info(narrow(print())) << 
				msg_info(av_error_str(errn)) <<
				boost::errinfo_api_function("av_find_stream_info") <<
				boost::errinfo_errno(AVUNERROR(errn)));
		}
		
		errn = video_stream_.open(format_context_, AVMEDIA_TYPE_VIDEO);
		if(errn < 0)
			CASPAR_LOG(warning) << print() << L" Could not open video stream: " << widen(av_error_str(errn));
		
		errn = audio_stream_.open(format_context_, AVMEDIA_TYPE_AUDIO);
		if(errn < 0)
			CASPAR_LOG(warning) << print() << L" Could not open audio stream: " << widen(av_error_str(errn));
		
		if(!video_stream_ && !audio_stream_)
		{	
			BOOST_THROW_EXCEPTION(
				file_read_error() << 
				source_info(narrow(print())) << 
				msg_info("No video or audio codec context found."));	
		}

		fps_ = video_stream_ ? video_stream_.fps() : audio_stream_.fps();

		if(start_ != 0)			
			seek_frame(start_);
					
		executor_.start();
		executor_.begin_invoke([this]{read_file();});
		CASPAR_LOG(info) << print() << " Started.";
	}

	~implementation()
	{
		stop();
	}
		
	std::shared_ptr<AVPacket> get_video_packet()
	{
		return video_stream_.pop();
	}

	std::shared_ptr<AVPacket> get_audio_packet()
	{
		return audio_stream_.pop();
	}

	bool has_packet() const
	{
		return !video_stream_.empty() || !audio_stream_.empty();
	}
				
	double fps()
	{
		return fps_;
	}

private:

	void stop()
	{
		executor_.stop();
		get_video_packet();
		get_audio_packet();
		CASPAR_LOG(info) << print() << " Stopping.";
	}
			
	void read_file()
	{		
		if(audio_stream_.size() > 4 && video_stream_.size() > 4)
			boost::this_thread::yield(); // There are enough packets, no hurry.

		try
		{
			std::shared_ptr<AVPacket> read_packet(new AVPacket(), [](AVPacket* p)
			{
				av_free_packet(p);
				delete p;
			});

			const int errn = av_read_frame(format_context_.get(), read_packet.get()); // read_packet is only valid until next call of av_read_frame.
			if(is_eof(errn))														  // Use av_dup_packet to extend its life.
			{
				if(loop_)
				{
					seek_frame(start_, AVSEEK_FLAG_BACKWARD);
					graph_->add_tag("seek");		
					CASPAR_LOG(info) << print() << " Received EOF. Looping.";			
				}	
				else
				{
					stop();
					CASPAR_LOG(info) << print() << " Received EOF. Stopping.";
				}
			}
			else if(errn < 0)
			{
				BOOST_THROW_EXCEPTION(
					file_read_error() <<
					msg_info(av_error_str(errn)) <<
					source_info(narrow(print())) << 
					boost::errinfo_api_function("av_read_frame") <<
					boost::errinfo_errno(AVUNERROR(errn)));
			}
			else
			{
				video_stream_.push(read_packet);
				audio_stream_.push(read_packet);
			}
						
			graph_->update_value("input-buffer", static_cast<float>(std::max(video_stream_.size(), audio_stream_.size()))/static_cast<float>(PACKET_BUFFER_COUNT));		
		}
		catch(...)
		{
			stop();
			CASPAR_LOG_CURRENT_EXCEPTION();
			return;
		}
						
		executor_.begin_invoke([this]{read_file();});		
	}

	void seek_frame(int64_t frame, int flags = 0)
	{  	
		// Convert from frames into seconds.
		const auto ts = frame*static_cast<int64_t>(AV_TIME_BASE/fps_);

		const int errn = av_seek_frame(format_context_.get(), -1, ts, flags | AVSEEK_FLAG_FRAME);
		if(errn < 0)
		{	
			BOOST_THROW_EXCEPTION(
				invalid_operation() << 
				source_info(narrow(print())) << 
				msg_info(av_error_str(errn)) <<
				boost::errinfo_api_function("av_seek_frame") <<
				boost::errinfo_errno(AVUNERROR(errn)));
		}
	}		

	bool is_eof(int errn)
	{
		if(errn == AVERROR(EIO))
			CASPAR_LOG(warning) << print() << " Received EIO, assuming EOF";

		return errn == AVERROR_EOF || errn == AVERROR(EIO); // av_read_frame doesn't always correctly return AVERROR_EOF;
	}
	
	std::wstring print() const
	{
		return L"ffmpeg_input[" + filename_ + L"]";
	}
};

input::input(const safe_ptr<diagnostics::graph>& graph, const std::wstring& filename, bool loop, int start) 
	: impl_(new implementation(graph, filename, loop, start)){}
const std::shared_ptr<AVCodecContext>& input::get_video_codec_context() const{return impl_->video_stream_.ctx();}
const std::shared_ptr<AVCodecContext>& input::get_audio_codec_context() const{return impl_->audio_stream_.ctx();}
bool input::has_packet() const{return impl_->has_packet();}
bool input::is_running() const {return impl_->executor_.is_running();}
std::shared_ptr<AVPacket> input::get_video_packet(){return impl_->get_video_packet();}
std::shared_ptr<AVPacket> input::get_audio_packet(){return impl_->get_audio_packet();}
double input::fps() const { return impl_->fps(); }
}