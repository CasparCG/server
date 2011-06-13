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

	stream() : index_(-1)
	{
		buffer_.set_capacity(PACKET_BUFFER_COUNT);
	}
	
	int open(std::shared_ptr<AVFormatContext>& fctx, AVMediaType media_type)
	{		
		const auto streams = boost::iterator_range<AVStream**>(fctx->streams, fctx->streams+fctx->nb_streams);
		const auto it = boost::find_if(streams, [&](AVStream* stream) 
		{
			return stream && stream->codec->codec_type == media_type;
		});
		
		if(it == streams.end()) 
			return AVERROR_STREAM_NOT_FOUND;
		
		auto codec = avcodec_find_decoder((*it)->codec->codec_id);			
		if(!codec)
			return AVERROR_DECODER_NOT_FOUND;
			
		index_ = (*it)->index;

		int errn = tbb_avcodec_open((*it)->codec, codec);
		if(errn < 0)
			return errn;
				
		ctx_.reset((*it)->codec, tbb_avcodec_close);

		// Some files give an invalid time_base numerator, try to fix it.
		if(ctx_ && ctx_->time_base.num == 1)
			ctx_->time_base.num = static_cast<int>(std::pow(10.0, static_cast<int>(std::log10(static_cast<float>(ctx_->time_base.den)))-1));
		
		return errn;	
	}

	bool try_pop(std::shared_ptr<AVPacket>& pkt)
	{
		return buffer_.try_pop(pkt);
	}

	void push(const std::shared_ptr<AVPacket>& pkt)
	{
		if(pkt && pkt->stream_index != index_)
			return;

		if(!ctx_)
			return;

		if(pkt)
			av_dup_packet(pkt.get());

		buffer_.push(pkt);	
	}

	int index() const {return index_;}
	
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

		for(size_t n = 0; n < 32; ++n) // Read some packets for pre-rolling.
			read_next_packet();
						
		if(audio_stream_)
			graph_->set_color("audio-input-buffer", diagnostics::color(0.5f, 1.0f, 0.2f));
		
		if(video_stream_)
			graph_->set_color("video-input-buffer", diagnostics::color(0.2f, 0.5f, 1.0f));
		
		graph_->set_color("seek", diagnostics::color(0.5f, 1.0f, 0.5f));	

		executor_.begin_invoke([this]{read_file();});
		CASPAR_LOG(info) << print() << " Started.";
	}

	~implementation()
	{
		stop();
	}
		
	bool try_pop_video_packet(std::shared_ptr<AVPacket>& packet)
	{
		bool result = video_stream_.try_pop(packet);
		if(result && !packet)
			graph_->add_tag("video-input-buffer");
		return result;
	}

	bool try_pop_audio_packet(std::shared_ptr<AVPacket>& packet)
	{	
		bool result = audio_stream_.try_pop(packet);
		if(result && !packet)
			graph_->add_tag("audio-input-buffer");
		return result;
	}

	double fps()
	{
		return fps_;
	}

private:

	void stop()
	{
		executor_.stop();

		// Unblock thread.
		std::shared_ptr<AVPacket> packet;
		try_pop_video_packet(packet);
		try_pop_audio_packet(packet);

		CASPAR_LOG(info) << print() << " Stopping.";
	}

	void read_file()
	{		
		if(video_stream_.size() > 4 || audio_stream_.size() > 4) // audio is always before video.
			Sleep(5); // There are enough packets, no hurry.

		read_next_packet();

		executor_.begin_invoke([this]{read_file();});
	}
			
	void read_next_packet()
	{		
		try
		{
			std::shared_ptr<AVPacket> read_packet(new AVPacket, [](AVPacket* p)
			{
				av_free_packet(p);
				delete p;
			});
			av_init_packet(read_packet.get());

			const int errn = av_read_frame(format_context_.get(), read_packet.get()); // read_packet is only valid until next call of av_read_frame.
			if(is_eof(errn))														  // Use av_dup_packet to extend its life.
			{
				if(loop_)
				{
					seek_frame(start_, AVSEEK_FLAG_BACKWARD);
					graph_->add_tag("seek");		
					//CASPAR_LOG(info) << print() << " Received EOF. Looping.";			
				}	
				else
				{
					stop();
					//CASPAR_LOG(info) << print() << " Received EOF. Stopping.";
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
				if(video_stream_)
				{	
					video_stream_.push(read_packet);
					graph_->update_value("video-input-buffer", static_cast<float>(video_stream_.size())/static_cast<float>(PACKET_BUFFER_COUNT));		
				}
				if(audio_stream_)
				{	
					audio_stream_.push(read_packet);
					graph_->update_value("audio-input-buffer", static_cast<float>(audio_stream_.size())/static_cast<float>(PACKET_BUFFER_COUNT));	
				}
			}							
		}
		catch(...)
		{
			stop();
			CASPAR_LOG_CURRENT_EXCEPTION();
			return;
		}	
	}

	void seek_frame(int64_t frame, int flags = 0)
	{  	
		static const AVRational base_q = {1, AV_TIME_BASE};

		// Convert from frames into seconds.
		auto seek_target = frame*static_cast<int64_t>(AV_TIME_BASE/fps_);

		int stream_index = video_stream_.index() >= 0 ? video_stream_.index() : audio_stream_.index();

		if(stream_index >= 0)		
			seek_target = av_rescale_q(seek_target, base_q, format_context_->streams[stream_index]->time_base);

		const int errn = av_seek_frame(format_context_.get(), stream_index, seek_target, flags);
		if(errn < 0)
		{	
			BOOST_THROW_EXCEPTION(
				invalid_operation() << 
				source_info(narrow(print())) << 
				msg_info(av_error_str(errn)) <<
				boost::errinfo_api_function("av_seek_frame") <<
				boost::errinfo_errno(AVUNERROR(errn)));
		}

		video_stream_.push(nullptr);
		audio_stream_.push(nullptr);
	}		

	bool is_eof(int errn)
	{
		//if(errn == AVERROR(EIO))
		//	CASPAR_LOG(warning) << print() << " Received EIO, assuming EOF";

		return errn == AVERROR_EOF || errn == AVERROR(EIO); // av_read_frame doesn't always correctly return AVERROR_EOF;
	}
	
	std::wstring print() const
	{
		return L"ffmpeg_input[" + filename_ + L"]";
	}
};

input::input(const safe_ptr<diagnostics::graph>& graph, const std::wstring& filename, bool loop, int start, int length) 
	: impl_(new implementation(graph, filename, loop, start)){}
const std::shared_ptr<AVCodecContext>& input::get_video_codec_context() const{return impl_->video_stream_.ctx();}
const std::shared_ptr<AVCodecContext>& input::get_audio_codec_context() const{return impl_->audio_stream_.ctx();}
bool input::is_running() const {return impl_->executor_.is_running();}
bool input::try_pop_video_packet(std::shared_ptr<AVPacket>& packet){return impl_->try_pop_video_packet(packet);}
bool input::try_pop_audio_packet(std::shared_ptr<AVPacket>& packet){return impl_->try_pop_audio_packet(packet);}
double input::fps() const { return impl_->fps(); }
}