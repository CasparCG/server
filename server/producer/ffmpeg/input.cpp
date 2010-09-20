#include "..\..\stdafx.h"

#include "input.h"

#include "../../frame/system_frame.h"
#include "../../frame/format.h"
#include "../../../common/image/image.h"
#include "../../../common/utility/scope_exit.h"

#include <tbb/concurrent_queue.h>

#include <boost/thread.hpp>

#include <errno.h>
#include <system_error>

#pragma warning(disable : 4482)
		
namespace caspar{ namespace ffmpeg{
		
struct input::implementation : boost::noncopyable
{
	implementation(const frame_format_desc& format_desc) 
		: video_frame_rate_(25.0), video_s_index_(-1), audio_s_index_(-1), video_codec_(nullptr), audio_codec_a(nullptr), format_desc_(format_desc)
	{
		loop_ = false;
		video_packet_buffer_.set_capacity(28);
		audio_packet_buffer_.set_capacity(28);
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
			AVFormatContext* weak_format_context;
			if((errn = -av_open_input_file(&weak_format_context, filename.c_str(), nullptr, 0, nullptr)) > 0)
				BOOST_THROW_EXCEPTION(file_read_error() << boost::errinfo_errno(errn));
			format_context.reset(weak_format_context, av_close_input_file);
			
			if((errn = -av_find_stream_info(format_context.get())) > 0)
				BOOST_THROW_EXCEPTION(file_read_error() << boost::errinfo_errno(errn));

			video_codec_context_ = open_video_stream();
			if(!video_codec_context_)
				CASPAR_LOG(info) << "No video stream found.";
		
			audio_codex_context = open_audio_stream();
			if(!audio_codex_context)
				CASPAR_LOG(info) << "No audio stream found.";

			if(!video_codec_context_ && !audio_codex_context)
				BOOST_THROW_EXCEPTION(file_read_error() << msg_info("No video or audio codec found."));
			
			video_frame_rate_ = static_cast<double>(video_codec_context_->time_base.den) / static_cast<double>(video_codec_context_->time_base.num);

			is_running_ = true;
			io_thread_ = boost::thread([=]{this->read_file();});
		}
		catch(...)
		{
			video_codec_context_.reset();
			audio_codex_context.reset();
			format_context.reset();
			video_frame_rate_ = 25.0;
			video_s_index_ = -1;
			audio_s_index_ = -1;	
			throw;
		}
		filename_ = filename;
	}
			
	std::shared_ptr<AVCodecContext> open_video_stream()
	{
		bool succeeded = false;

		CASPAR_SCOPE_EXIT([&]
		{
			if(!succeeded)
			{
				video_s_index_ = -1;
				video_codec_ = nullptr;
			}
		});

		AVStream** streams_end = format_context->streams+format_context->nb_streams;
		AVStream** video_stream = std::find_if(format_context->streams, streams_end, 
			[](AVStream* stream) { return stream != nullptr && stream->codec->codec_type == CODEC_TYPE_VIDEO ;});

		video_s_index_ = video_stream != streams_end ? (*video_stream)->index : -1;
		if(video_s_index_ == -1) 
			return nullptr;
		
		video_codec_ = avcodec_find_decoder((*video_stream)->codec->codec_id);			
		if(video_codec_ == nullptr)
			return nullptr;
			
		if((-avcodec_open((*video_stream)->codec, video_codec_)) > 0)		
			return nullptr;

		succeeded = true;

		return std::shared_ptr<AVCodecContext>((*video_stream)->codec, avcodec_close);
	}

	std::shared_ptr<AVCodecContext> open_audio_stream()
	{	
		bool succeeded = false;

		CASPAR_SCOPE_EXIT([&]
		{
			if(!succeeded)
			{
				audio_s_index_ = -1;
				audio_codec_a = nullptr;
			}
		});

		AVStream** streams_end = format_context->streams+format_context->nb_streams;
		AVStream** audio_stream = std::find_if(format_context->streams, streams_end, 
			[](AVStream* stream) { return stream != nullptr && stream->codec->codec_type == CODEC_TYPE_AUDIO;});

		audio_s_index_ = audio_stream != streams_end ? (*audio_stream)->index : -1;
		if(audio_s_index_ == -1)
			return nullptr;
		
		audio_codec_a = avcodec_find_decoder((*audio_stream)->codec->codec_id);
		if(audio_codec_a == nullptr)
			return nullptr;

		if((-avcodec_open((*audio_stream)->codec, audio_codec_a)) > 0)		
			return nullptr;

		succeeded = true;

		return std::shared_ptr<AVCodecContext>((*audio_stream)->codec, avcodec_close);
	}	

	void read_file()
	{	
		CASPAR_LOG(info) << "Started ffmpeg_producer::read_file Thread for " << filename_.c_str();
		win32_exception::install_handler();

		try
		{
			AVPacket tmp_packet;
			while(is_running_)
			{
				std::shared_ptr<AVPacket> packet(&tmp_packet, av_free_packet);

				if (av_read_frame(format_context.get(), packet.get()) >= 0) // NOTE: Packet is only valid until next call of av_read_frame or av_close_input_file
				{
					if(packet->stream_index == video_s_index_) 				
						video_packet_buffer_.push(std::make_shared<video_packet>(packet, std::make_shared<system_frame>(format_desc_.size), format_desc_, video_codec_context_.get(), video_codec_));		 // NOTE: video_packet makes a copy of AVPacket
					else if(packet->stream_index == audio_s_index_) 	
						audio_packet_buffer_.push(std::make_shared<audio_packet>(packet, audio_codex_context.get(), audio_codec_a, video_frame_rate_));			
				}
				else if(!loop_ || av_seek_frame(format_context.get(), -1, 0, AVSEEK_FLAG_BACKWARD) < 0) // TODO: av_seek_frame does not work for all formats
					is_running_ = false;
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}

		is_running_ = false;
		
		CASPAR_LOG(info) << " Ended ffmpeg_producer::read_file Thread for " << filename_.c_str();
	}
	
	video_packet_ptr get_video_packet()
	{
		video_packet_ptr video_packet;
		video_packet_buffer_.try_pop(video_packet);
		return video_packet;
	}

	audio_packet_ptr get_audio_packet()
	{
		audio_packet_ptr audio_packet;
		audio_packet_buffer_.try_pop(audio_packet);
		return audio_packet;
	}

	bool is_eof() const
	{
		return !is_running_ && video_packet_buffer_.empty() && audio_packet_buffer_.empty();
	}
				
	std::string							filename_;
	std::shared_ptr<AVFormatContext>	format_context;	// Destroy this last

	std::shared_ptr<AVCodecContext>		video_codec_context_;
	AVCodec*							video_codec_;

	std::shared_ptr<AVCodecContext>		audio_codex_context;
	AVCodec*							audio_codec_a;

	tbb::atomic<bool>					loop_;
	int									video_s_index_;
	int									audio_s_index_;

	frame_format_desc format_desc_;

	tbb::concurrent_bounded_queue<video_packet_ptr> video_packet_buffer_;
	tbb::concurrent_bounded_queue<audio_packet_ptr> audio_packet_buffer_;
	boost::thread	io_thread_;
	tbb::atomic<bool> is_running_;

	double video_frame_rate_;
};

input::input(const frame_format_desc& format_desc) : impl_(new implementation(format_desc)){}
void input::load(const std::string& filename){impl_->load(filename);}
void input::set_loop(bool value){impl_->loop_ = value;}
const std::shared_ptr<AVCodecContext>& input::get_video_codec_context() const{return impl_->video_codec_context_;}
const std::shared_ptr<AVCodecContext>& input::get_audio_codec_context() const{return impl_->audio_codex_context;}
bool input::is_eof() const{return impl_->is_eof();}
video_packet_ptr input::get_video_packet(){return impl_->get_video_packet();}
audio_packet_ptr input::get_audio_packet(){return impl_->get_audio_packet();}
}}