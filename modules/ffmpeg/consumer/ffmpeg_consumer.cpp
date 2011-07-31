/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This ffmpeg is part of CasparCG.
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
 
#include "../StdAfx.h"

#include "ffmpeg_consumer.h"

#include <core/mixer/read_frame.h>

#include <common/concurrency/executor.h>
#include <common/utility/string.h>
#include <common/env.h>

#include <boost/thread/once.hpp>

#include <tbb/cache_aligned_allocator.h>
#include <tbb/parallel_invoke.h>

#include <cstdio>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar { 
	
struct ffmpeg_consumer : boost::noncopyable
{		
	const std::string						filename_;
	const size_t							bitrate_;
		
	const std::shared_ptr<AVFormatContext>	oc_;
	const core::video_format_desc			format_desc_;
	
	executor								executor_;

	// Audio
	std::shared_ptr<AVStream>				audio_st_;
	std::vector<uint8_t>					audio_outbuf_;

	std::vector<int16_t>					audio_input_buffer_;

	// Video
	std::shared_ptr<AVStream>				video_st_;
	std::vector<uint8_t>					video_outbuf_;

	std::vector<uint8_t>					picture_buf_;
	std::shared_ptr<SwsContext>				img_convert_ctx_;
	
public:
	ffmpeg_consumer(const std::string& filename, const core::video_format_desc& format_desc, size_t bitrate)
		: filename_(filename)
		, bitrate_(bitrate)
		, video_outbuf_(1920*1080*4)
		, audio_outbuf_(48000)
		, oc_(avformat_alloc_context(), av_free)
		, format_desc_(format_desc)
		, executor_(print())
	{
		if (!oc_)
		{
			BOOST_THROW_EXCEPTION(caspar_exception()
				<< msg_info("Could not alloc format-context")				
				<< boost::errinfo_api_function("avformat_alloc_context"));
		}

		executor_.set_capacity(core::consumer_buffer_depth());

		oc_->oformat = av_guess_format(nullptr, filename_.c_str(), nullptr);
		if (!oc_->oformat)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Could not find suitable output format."));
		
		std::copy_n(filename_.c_str(), filename_.size(), oc_->filename);
					
		//  Add the audio and video streams using the default format codecs	and initialize the codecs .
		if (oc_->oformat->video_codec != CODEC_ID_NONE)		
			video_st_ = add_video_stream(oc_->oformat->video_codec);
		
		//if (oc_->oformat->audio_codec != CODEC_ID_NONE) 
		//	audio_st_ = add_audio_stream(oc_->oformat->audio_codec);	

		// Set the output parameters (must be done even if no parameters).		
		int errn = av_set_parameters(oc_.get(), nullptr);
		if (errn < 0)
		{
			BOOST_THROW_EXCEPTION(
				file_read_error() << 
				msg_info("Invalid output format parameters") <<
				boost::errinfo_api_function("avcodec_open") <<
				boost::errinfo_errno(AVUNERROR(errn)) <<
				boost::errinfo_file_name(filename_));
		}
		
		dump_format(oc_.get(), 0, filename_.c_str(), 1);

		// Now that all the parameters are set, we can open the audio and
		// video codecs and allocate the necessary encode buffers.
		if (video_st_)
			open_video(video_st_);
		
		try
		{
			if (audio_st_)
				open_audio(audio_st_);
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			audio_st_ = nullptr;
		}
 
		// Open the output ffmpeg, if needed.
		if (!(oc_->oformat->flags & AVFMT_NOFILE)) 
		{
			int errn = url_fopen(&oc_->pb, filename_.c_str(), URL_WRONLY);
			if (errn < 0) 
			{
				BOOST_THROW_EXCEPTION(
					file_not_found() << 
					msg_info("Could not open file") <<
					boost::errinfo_api_function("url_fopen") <<
					boost::errinfo_errno(AVUNERROR(errn)) <<
					boost::errinfo_file_name(filename_));
			}
		}
		
		av_write_header(oc_.get()); // write the stream header, if any 

		CASPAR_LOG(info) << print() << L" Successfully initialized.";	
	}

	~ffmpeg_consumer()
	{    
		executor_.stop();
		executor_.join();

		audio_st_.reset();
		video_st_.reset();

		av_write_trailer(oc_.get());
		
		if (!(oc_->oformat->flags & AVFMT_NOFILE)) 
			url_fclose(oc_->pb); // Close the output ffmpeg.
	}

	const core::video_format_desc& get_video_format_desc() const
	{
		return format_desc_;
	}
		
	std::wstring print() const
	{
		return L"ffmpeg[" + widen(filename_) + L"]";
	}

	std::shared_ptr<AVStream> add_video_stream(enum CodecID codec_id)
	{ 
		auto st = av_new_stream(oc_.get(), 0);
		if (!st) 
		{
			BOOST_THROW_EXCEPTION(caspar_exception() 
				<< msg_info("Could not alloc video-stream")				
				<< boost::errinfo_api_function("av_new_stream"));
		}

		st->codec->codec_id			= codec_id;
		st->codec->codec_type		= AVMEDIA_TYPE_VIDEO;
		st->codec->bit_rate			= bitrate_;
		st->codec->width			= format_desc_.width;
		st->codec->height			= format_desc_.height;
		st->codec->time_base.den	= format_desc_.time_scale;
		st->codec->time_base.num	= format_desc_.duration;
		st->codec->pix_fmt			= st->codec->pix_fmt == -1 ? PIX_FMT_YUV420P : st->codec->pix_fmt;
 
		return std::shared_ptr<AVStream>(st, [](AVStream* st)
		{
			avcodec_close(st->codec);
			//av_freep(st);
		});
	}
	
	std::shared_ptr<AVStream> add_audio_stream(enum CodecID codec_id)
	{
		auto st = av_new_stream(oc_.get(), 1);
		if (!st) 
		{
			BOOST_THROW_EXCEPTION(caspar_exception() 
				<< msg_info("Could not alloc audio-stream")				
				<< boost::errinfo_api_function("av_new_stream"));
		}

		st->codec->codec_id		= codec_id;
		st->codec->codec_type	= AVMEDIA_TYPE_AUDIO;
		st->codec->sample_rate	= 48000;
		st->codec->channels		= 2;
		st->codec->sample_fmt	= SAMPLE_FMT_S16;
		
		return std::shared_ptr<AVStream>(st, [](AVStream* st)
		{
			avcodec_close(st->codec);
			//av_freep(st);
		});
	}
	 
	void open_video(std::shared_ptr<AVStream>& st)
	{  
		auto codec = avcodec_find_encoder(st->codec->codec_id);
		if (!codec)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("codec not found"));
		
		int errn = avcodec_open(st->codec, codec);
		if (errn < 0)
		{
			BOOST_THROW_EXCEPTION(
				file_read_error() << 
				msg_info("Could not open video codec.") <<
				boost::errinfo_api_function("avcodec_open") <<
				boost::errinfo_errno(AVUNERROR(errn)) <<
				boost::errinfo_file_name(filename_));		
		}
	}

	void open_audio(std::shared_ptr<AVStream>& st)
	{
		auto codec = avcodec_find_encoder(st->codec->codec_id);
		if (!codec) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("codec not found"));
		
		int errn = avcodec_open(st->codec, codec);
		if (errn < 0)
		{
			BOOST_THROW_EXCEPTION(
				file_read_error() << 
				msg_info("Could not open audio codec") <<
				boost::errinfo_api_function("avcodec_open") <<
				boost::errinfo_errno(AVUNERROR(errn)) <<
				boost::errinfo_file_name(filename_));
		}
	}
  
	void encode_video_frame(const safe_ptr<core::read_frame>& frame)
	{ 
		if(!video_st_)
			return;

		AVCodecContext* c = video_st_->codec;
 
		if(!img_convert_ctx_) 
		{
			img_convert_ctx_.reset(sws_getContext(format_desc_.width, format_desc_.height, PIX_FMT_BGRA, c->width, c->height, c->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr), sws_freeContext);
			if (img_convert_ctx_ == nullptr) 
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Cannot initialize the conversion context"));
		}

		std::shared_ptr<AVFrame> av_frame(avcodec_alloc_frame(), av_free);
		avpicture_fill(reinterpret_cast<AVPicture*>(av_frame.get()), const_cast<uint8_t*>(frame->image_data().begin()), PIX_FMT_BGRA, format_desc_.width, format_desc_.height);
				
		std::shared_ptr<AVFrame> local_av_frame(avcodec_alloc_frame(), av_free);
		picture_buf_.resize(avpicture_get_size(c->pix_fmt, format_desc_.width, format_desc_.height));
		avpicture_fill(reinterpret_cast<AVPicture*>(local_av_frame.get()), picture_buf_.data(), c->pix_fmt, format_desc_.width, format_desc_.height);

		sws_scale(img_convert_ctx_.get(), av_frame->data, av_frame->linesize, 0, c->height, local_av_frame->data, local_av_frame->linesize);
				
		int errn = avcodec_encode_video(c, video_outbuf_.data(), video_outbuf_.size(), local_av_frame.get());
		if (errn < 0) 
		{
			BOOST_THROW_EXCEPTION(
				invalid_operation() << 
				msg_info("Could not encode video frame.") <<
				boost::errinfo_api_function("avcodec_encode_video") <<
				boost::errinfo_errno(AVUNERROR(errn)) <<
				boost::errinfo_file_name(filename_));
		}

		AVPacket pkt;
		av_init_packet(&pkt);
		pkt.size = errn;

		// If zero size, it means the image was buffered.
		if (errn > 0) 
		{ 
			if (c->coded_frame->pts != AV_NOPTS_VALUE)
				pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, video_st_->time_base);
			
			if(c->coded_frame->key_frame)
				pkt.flags |= AV_PKT_FLAG_KEY;

			pkt.stream_index = video_st_->index;
			pkt.data	     = video_outbuf_.data();

			if (av_interleaved_write_frame(oc_.get(), &pkt) != 0)
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Error while writing video frame"));
		} 		
	}
		
	void encode_audio_frame(const safe_ptr<core::read_frame>& frame)
	{	
		if(!audio_st_)
			return;

		if(!frame->audio_data().empty())
			audio_input_buffer_.insert(audio_input_buffer_.end(), frame->audio_data().begin(), frame->audio_data().end());
		else
			audio_input_buffer_.insert(audio_input_buffer_.end(), 3840, 0);

		while(encode_audio_packet()){}
	}

	bool encode_audio_packet()
	{		
		auto c = audio_st_->codec;

		auto frame_bytes = c->frame_size * 2 * 2; // samples per frame * 2 channels * 2 bytes per sample
		if(static_cast<int>(audio_input_buffer_.size()) < frame_bytes/2)
			return false;

		AVPacket pkt;
		av_init_packet(&pkt);
		
		int errn = avcodec_encode_audio(c, audio_outbuf_.data(), audio_outbuf_.size(), audio_input_buffer_.data());
		if (errn < 0) 
		{
			BOOST_THROW_EXCEPTION(
				invalid_operation() << 
				msg_info("Could not encode audio samples.") <<
				boost::errinfo_api_function("avcodec_encode_audio") <<
				boost::errinfo_errno(AVUNERROR(errn)) <<
				boost::errinfo_file_name(filename_));
		}

		pkt.size = errn;
		audio_input_buffer_ = std::vector<int16_t>(audio_input_buffer_.begin() + frame_bytes/2, audio_input_buffer_.end());

		if (c->coded_frame && c->coded_frame->pts != AV_NOPTS_VALUE)
			pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, audio_st_->time_base);

		pkt.flags		 |= AV_PKT_FLAG_KEY;
		pkt.stream_index = audio_st_->index;
		pkt.data		 = audio_outbuf_.data();
		
		if (av_interleaved_write_frame(oc_.get(), &pkt) != 0)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Error while writing audio frame"));

		return true;
	}
	 
	void send(const safe_ptr<core::read_frame>& frame)
	{
		executor_.begin_invoke([=]
		{				
			encode_video_frame(frame);
			encode_audio_frame(frame);
		});
	}

	size_t buffer_depth() const { return 1; }
};

struct ffmpeg_consumer_proxy : public core::frame_consumer
{
	const std::wstring filename_;
	const bool key_only_;
	const size_t bitrate_;

	std::unique_ptr<ffmpeg_consumer> consumer_;

public:

	ffmpeg_consumer_proxy(const std::wstring& filename, bool key_only, size_t bitrate)
		: filename_(filename)
		, key_only_(key_only)
		, bitrate_(bitrate){}
	
	virtual void initialize(const core::video_format_desc& format_desc)
	{
		consumer_.reset(new ffmpeg_consumer(narrow(filename_), format_desc, bitrate_));
	}
	
	virtual void send(const safe_ptr<core::read_frame>& frame)
	{
		consumer_->send(frame);
	}
	
	virtual std::wstring print() const
	{
		return consumer_->print();
	}

	virtual bool key_only() const
	{
		return key_only_;
	}
	
	virtual bool has_synchronization_clock() const 
	{
		return false;
	}

	virtual const core::video_format_desc& get_video_format_desc() const
	{
		return consumer_->get_video_format_desc();
	}
};	

safe_ptr<core::frame_consumer> create_ffmpeg_consumer(const std::vector<std::wstring>& params)
{
	if(params.size() < 2 || params[0] != L"FILE")
		return core::frame_consumer::empty();
	
	// TODO: Ask stakeholders about case where file already exists.
	boost::filesystem::remove(boost::filesystem::wpath(env::media_folder() + params[1])); // Delete the file if it exists
	bool key_only = std::find(params.begin(), params.end(), L"KEY_ONLY") != params.end();

	return make_safe<ffmpeg_consumer_proxy>(env::media_folder() + params[1], key_only, 100000000);
}

safe_ptr<core::frame_consumer> create_ffmpeg_consumer(const boost::property_tree::ptree& ptree)
{
	std::string filename = ptree.get<std::string>("path");
	bool key_only		 = ptree.get("key-only", false);
	size_t bitrate		 = ptree.get("bitrate", 100000000);
	
	return make_safe<ffmpeg_consumer_proxy>(env::media_folder() + widen(filename), key_only, bitrate);
}

}
