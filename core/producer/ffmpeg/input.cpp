#include "..\..\stdafx.h"

#include "input.h"

#include "../../format/video_format.h"
#include "../../../common/utility/scope_exit.h"

#include <tbb/concurrent_queue.h>
#include <tbb/queuing_mutex.h>

#include <boost/thread.hpp>
#include <boost/exception/error_info.hpp>

#include <errno.h>
#include <system_error>
		
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

namespace caspar { namespace core { namespace ffmpeg{
		
struct input::implementation : boost::noncopyable
{
	implementation() : video_s_index_(-1), audio_s_index_(-1), video_codec_(nullptr), audio_codec_(nullptr)
	{
		loop_ = false;	
		video_packet_buffer_.set_capacity(50);
		audio_packet_buffer_.set_capacity(50);
	}

	~implementation()
	{		
		is_running_ = false;
		audio_packet_buffer_.clear();
		video_packet_buffer_.clear();
		io_thread_.join();
	}
	
	void load(const std::string& filename)
	{	
		try
		{
			int errn;
			AVFormatContext* weak_format_context_;
			if((errn = -av_open_input_file(&weak_format_context_, filename.c_str(), nullptr, 0, nullptr)) > 0)
				BOOST_THROW_EXCEPTION(
					file_read_error() << 
					msg_info("No format context found.") << 
					boost::errinfo_api_function("av_open_input_file") <<
					boost::errinfo_errno(errn) <<
					boost::errinfo_file_name(filename));

			format_context_.reset(weak_format_context_, av_close_input_file);
			
			if((errn = -av_find_stream_info(format_context_.get())) > 0)
				BOOST_THROW_EXCEPTION(
					file_read_error() << 
					boost::errinfo_api_function("av_find_stream_info") <<
					msg_info("No stream found.") << 
					boost::errinfo_errno(errn));

			video_codec_context_ = open_video_stream();
			if(!video_codec_context_)
				CASPAR_LOG(warning) << "No video stream found.";
		
			audio_codex_context_ = open_audio_stream();
			if(!audio_codex_context_)
				CASPAR_LOG(warning) << "No audio stream found.";

			if(!video_codec_context_ && !audio_codex_context_)
				BOOST_THROW_EXCEPTION(file_read_error() << msg_info("No video or audio codec context found."));		
		}
		catch(...)
		{
			video_codec_context_.reset();
			audio_codex_context_.reset();
			format_context_.reset();
			video_s_index_ = -1;
			audio_s_index_ = -1;	
			throw;
		}
		filename_ = filename;
		io_thread_ = boost::thread([=]{read_file();});
	}
				
	std::shared_ptr<AVCodecContext> open_video_stream()
	{		
		AVStream** streams_end = format_context_->streams+format_context_->nb_streams;
		AVStream** video_stream = std::find_if(format_context_->streams, streams_end, 
			[](AVStream* stream) { return stream != nullptr && stream->codec->codec_type == CODEC_TYPE_VIDEO ;});

		video_s_index_ = video_stream != streams_end ? (*video_stream)->index : -1;
		if(video_s_index_ == -1) 
			return nullptr;
		
		video_codec_ = avcodec_find_decoder((*video_stream)->codec->codec_id);			
		if(video_codec_ == nullptr)
			return nullptr;
			
		if((-avcodec_open((*video_stream)->codec, video_codec_)) > 0)		
			return nullptr;

		return std::shared_ptr<AVCodecContext>((*video_stream)->codec, avcodec_close);
	}

	std::shared_ptr<AVCodecContext> open_audio_stream()
	{	
		AVStream** streams_end = format_context_->streams+format_context_->nb_streams;
		AVStream** audio_stream = std::find_if(format_context_->streams, streams_end, 
			[](AVStream* stream) { return stream != nullptr && stream->codec->codec_type == CODEC_TYPE_AUDIO;});

		audio_s_index_ = audio_stream != streams_end ? (*audio_stream)->index : -1;
		if(audio_s_index_ == -1)
			return nullptr;
		
		audio_codec_ = avcodec_find_decoder((*audio_stream)->codec->codec_id);
		if(audio_codec_ == nullptr)
			return nullptr;

		if((-avcodec_open((*audio_stream)->codec, audio_codec_)) > 0)		
			return nullptr;

		return std::shared_ptr<AVCodecContext>((*audio_stream)->codec, avcodec_close);
	}	

	void read_file()
	{	
		CASPAR_LOG(info) << "Started ffmpeg_producer::read_file Thread for " << filename_.c_str();
		win32_exception::install_handler();
		
		is_running_ = true;
		AVPacket tmp_packet;
		while(is_running_)
		{
			std::shared_ptr<AVPacket> packet(&tmp_packet, av_free_packet);	
			tbb::queuing_mutex::scoped_lock lock(seek_mutex_);	

			if (av_read_frame(format_context_.get(), packet.get()) >= 0) // NOTE: Packet is only valid until next call of av_safe_ptr<read_frame> or av_close_input_file
			{
				auto buffer = std::make_shared<aligned_buffer>(packet->data, packet->data + packet->size);
				if(packet->stream_index == video_s_index_) 						
					video_packet_buffer_.push(buffer);						
				else if(packet->stream_index == audio_s_index_) 	
					audio_packet_buffer_.push(buffer);		
			}
			else if(!loop_ || av_seek_frame(format_context_.get(), -1, 0, AVSEEK_FLAG_BACKWARD) < 0) // TODO: av_seek_frame does not work for all formats
				is_running_ = false;
		}
		
		is_running_ = false;
		
		CASPAR_LOG(info) << " Ended ffmpeg_producer::read_file Thread for " << filename_.c_str();
	}
	
	aligned_buffer get_video_packet()
	{
		std::shared_ptr<aligned_buffer> video_packet;
		if(video_packet_buffer_.try_pop(video_packet))		
			return std::move(*video_packet);		
		return aligned_buffer();
	}

	aligned_buffer get_audio_packet()
	{
		std::shared_ptr<aligned_buffer> audio_packet;
		if(audio_packet_buffer_.try_pop(audio_packet))
			return std::move(*audio_packet);
		return aligned_buffer();
	}

	bool is_eof() const
	{
		return !is_running_ && video_packet_buffer_.empty() && audio_packet_buffer_.empty();
	}
		
	bool seek(unsigned long long seek_target)
	{
		tbb::queuing_mutex::scoped_lock lock(seek_mutex_);
		if(av_seek_frame(format_context_.get(), -1, seek_target*AV_TIME_BASE, 0) < 0)
			return false;
		
		video_packet_buffer_.clear();
		audio_packet_buffer_.clear();
		// TODO: Not sure its enough to jsut flush in input class
		if(video_codec_context_)
			avcodec_flush_buffers(video_codec_context_.get());
		if(audio_codex_context_)
			avcodec_flush_buffers(audio_codex_context_.get());
		return true;
	}
				
	std::shared_ptr<AVFormatContext>	format_context_;	// Destroy this last

	tbb::queuing_mutex					seek_mutex_;

	std::string							filename_;

	std::shared_ptr<AVCodecContext>		video_codec_context_;
	AVCodec*							video_codec_;

	std::shared_ptr<AVCodecContext>		audio_codex_context_;
	AVCodec*							audio_codec_;

	tbb::atomic<bool>					loop_;
	int									video_s_index_;
	int									audio_s_index_;
	
	tbb::concurrent_bounded_queue<std::shared_ptr<aligned_buffer>> video_packet_buffer_;
	tbb::concurrent_bounded_queue<std::shared_ptr<aligned_buffer>> audio_packet_buffer_;
	boost::thread	io_thread_;
	tbb::atomic<bool> is_running_;
};

input::input() : impl_(new implementation()){}
void input::load(const std::string& filename){impl_->load(filename);}
void input::set_loop(bool value){impl_->loop_ = value;}
const std::shared_ptr<AVCodecContext>& input::get_video_codec_context() const{return impl_->video_codec_context_;}
const std::shared_ptr<AVCodecContext>& input::get_audio_codec_context() const{return impl_->audio_codex_context_;}
bool input::is_eof() const{return impl_->is_eof();}
aligned_buffer input::get_video_packet(){return impl_->get_video_packet();}
aligned_buffer input::get_audio_packet(){return impl_->get_audio_packet();}
bool input::seek(unsigned long long frame){return impl_->seek(frame);}
}}}