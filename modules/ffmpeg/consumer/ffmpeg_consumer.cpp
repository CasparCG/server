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

#include "../ffmpeg_error.h"
#include "../producer/tbb_avcodec.h"

#include "ffmpeg_consumer.h"

#include <core/mixer/read_frame.h>
#include <core/mixer/audio/audio_util.h>
#include <core/consumer/frame_consumer.h>
#include <core/video_format.h>

#include <common/concurrency/executor.h>
#include <common/utility/string.h>
#include <common/env.h>

#include <boost/thread/once.hpp>
#include <boost/algorithm/string.hpp>

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

namespace caspar { namespace ffmpeg {
	
struct ffmpeg_consumer : boost::noncopyable
{		
	const std::string						filename_;
		
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
	ffmpeg_consumer(const std::string& filename, const core::video_format_desc& format_desc, const std::string& codec, int bitrate)
		: filename_(filename + ".mov")
		, video_outbuf_(1920*1080*8)
		, audio_outbuf_(48000)
		, oc_(avformat_alloc_context(), av_free)
		, format_desc_(format_desc)
		, executor_(print())
	{
		executor_.set_capacity(25);
		
		oc_->oformat = av_guess_format(nullptr, filename_.c_str(), nullptr);
		if (!oc_->oformat)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Could not find suitable output format."));
		
		THROW_ON_ERROR2(av_set_parameters(oc_.get(), nullptr), "[ffmpeg_consumer]");

		strcpy_s(oc_->filename, filename_.c_str());
		
		auto video_codec = avcodec_find_encoder_by_name(codec.c_str());
		if(video_codec == nullptr)
			BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info(codec));

		//  Add the audio and video streams using the default format codecs	and initialize the codecs .
		video_st_ = add_video_stream(video_codec->id, bitrate);
		audio_st_ = add_audio_stream();
				
		dump_format(oc_.get(), 0, filename_.c_str(), 1);
		 
		// Open the output ffmpeg, if needed.
		if (!(oc_->oformat->flags & AVFMT_NOFILE)) 
			THROW_ON_ERROR2(avio_open(&oc_->pb, filename_.c_str(), URL_WRONLY), "[ffmpeg_consumer]");
				
		THROW_ON_ERROR2(av_write_header(oc_.get()), "[ffmpeg_consumer]");

		CASPAR_LOG(info) << print() << L" Successfully Initialized.";	
	}

	~ffmpeg_consumer()
	{    
		executor_.stop();
		executor_.join();
		
		try
		{
			THROW_ON_ERROR2(av_write_trailer(oc_.get()), "[ffmpeg_consumer]");
		
			audio_st_.reset();
			video_st_.reset();
			  
			for(size_t i = 0; i < oc_->nb_streams; i++) 
			{
				av_freep(&oc_->streams[i]->codec);
				av_freep(&oc_->streams[i]);
			}

			if (!(oc_->oformat->flags & AVFMT_NOFILE)) 
				THROW_ON_ERROR2(avio_close(oc_->pb), "[ffmpeg_consumer]"); // Close the output ffmpeg.

			CASPAR_LOG(info) << print() << L" Successfully Uninitialized.";	
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}

	}
			
	std::wstring print() const
	{
		return L"ffmpeg[" + widen(filename_) + L"]";
	}

	std::shared_ptr<AVStream> add_video_stream(enum CodecID codec_id, int bitrate)
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
		st->codec->width			= format_desc_.width;
		st->codec->height			= format_desc_.height;
		st->codec->time_base.den	= format_desc_.time_scale;
		st->codec->time_base.num	= format_desc_.duration;

		if(st->codec->codec_id == CODEC_ID_PRORES)
		{			
			st->codec->bit_rate	= bitrate > 0 ? bitrate : format_desc_.width < 1280 ? 42*1000000 : 147*1000000;
			st->codec->pix_fmt	= PIX_FMT_YUV422P10;
		}
		else if(st->codec->codec_id == CODEC_ID_DNXHD)
		{
			st->codec->bit_rate	= bitrate > 0 ? bitrate : 145*1000000;
			st->codec->width	= std::min<size_t>(1280, format_desc_.width);
			st->codec->height	= std::min<size_t>(720, format_desc_.height);
			st->codec->pix_fmt	= PIX_FMT_YUV422P;
		}
		else
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("unsupported codec"));
		
		if(oc_->oformat->flags & AVFMT_GLOBALHEADER)
			st->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

		auto codec = avcodec_find_encoder(st->codec->codec_id);
		if (!codec)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("codec not found"));

		THROW_ON_ERROR2(tbb_avcodec_open(st->codec, codec), "[ffmpeg_consumer]");

		return std::shared_ptr<AVStream>(st, [](AVStream* st)
		{
			tbb_avcodec_close(st->codec);
		});
	}
	
	std::shared_ptr<AVStream> add_audio_stream()
	{
		auto st = av_new_stream(oc_.get(), 1);
		if (!st) 
		{
			BOOST_THROW_EXCEPTION(caspar_exception() 
				<< msg_info("Could not alloc audio-stream")				
				<< boost::errinfo_api_function("av_new_stream"));
		}

		st->codec->codec_id		= CODEC_ID_PCM_S16LE;
		st->codec->codec_type	= AVMEDIA_TYPE_AUDIO;
		st->codec->sample_rate	= 48000;
		st->codec->channels		= 2;
		st->codec->sample_fmt	= SAMPLE_FMT_S16;
		
		if(oc_->oformat->flags & AVFMT_GLOBALHEADER)
			st->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
		
		auto codec = avcodec_find_encoder(st->codec->codec_id);
		if (!codec)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("codec not found"));

		THROW_ON_ERROR2(avcodec_open(st->codec, codec), "[ffmpeg_consumer]");

		return std::shared_ptr<AVStream>(st, [](AVStream* st)
		{
			avcodec_close(st->codec);
		});
	}
  
	void encode_video_frame(const safe_ptr<core::read_frame>& frame)
	{ 
		auto c = video_st_->codec;
 
		if(!img_convert_ctx_) 
		{
			img_convert_ctx_.reset(sws_getContext(format_desc_.width, format_desc_.height, PIX_FMT_BGRA, c->width, c->height, c->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr), sws_freeContext);
			if (img_convert_ctx_ == nullptr) 
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Cannot initialize the conversion context"));
		}

		std::shared_ptr<AVFrame> av_frame(avcodec_alloc_frame(), av_free);
		avpicture_fill(reinterpret_cast<AVPicture*>(av_frame.get()), const_cast<uint8_t*>(frame->image_data().begin()), PIX_FMT_BGRA, format_desc_.width, format_desc_.height);
				
		std::shared_ptr<AVFrame> local_av_frame(avcodec_alloc_frame(), av_free);
		local_av_frame->interlaced_frame = format_desc_.field_mode != core::field_mode::progressive;
		local_av_frame->top_field_first  = format_desc_.field_mode == core::field_mode::upper;

		picture_buf_.resize(avpicture_get_size(c->pix_fmt, format_desc_.width, format_desc_.height));
		avpicture_fill(reinterpret_cast<AVPicture*>(local_av_frame.get()), picture_buf_.data(), c->pix_fmt, format_desc_.width, format_desc_.height);

		sws_scale(img_convert_ctx_.get(), av_frame->data, av_frame->linesize, 0, c->height, local_av_frame->data, local_av_frame->linesize);
				
		int out_size = THROW_ON_ERROR2(avcodec_encode_video(c, video_outbuf_.data(), video_outbuf_.size(), local_av_frame.get()), "[ffmpeg_consumer]");
		if(out_size > 0)
		{
			AVPacket pkt;
			av_init_packet(&pkt);
 
			if (c->coded_frame->pts != AV_NOPTS_VALUE)
				pkt.pts= av_rescale_q(c->coded_frame->pts, c->time_base, video_st_->time_base);

			if(c->coded_frame->key_frame)
				pkt.flags |= AV_PKT_FLAG_KEY;

			pkt.stream_index	= video_st_->index;
			pkt.data			= video_outbuf_.data();
			pkt.size			= out_size;
 
			THROW_ON_ERROR2(av_interleaved_write_frame(oc_.get(), &pkt), L"[ffmpeg_consumer]");
		}	
	}
		
	void encode_audio_frame(const safe_ptr<core::read_frame>& frame)
	{			
		auto c = audio_st_->codec;

		auto audio_data = core::audio_32_to_16(frame->audio_data());

		AVPacket pkt;
		av_init_packet(&pkt);
		
		if (c->coded_frame && c->coded_frame->pts != AV_NOPTS_VALUE)
			pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, audio_st_->time_base);

		pkt.flags		 |= AV_PKT_FLAG_KEY;
		pkt.stream_index = audio_st_->index;
		pkt.size		 = audio_data.size()*2;
		pkt.data		 = reinterpret_cast<uint8_t*>(audio_data.data());
		
		THROW_ON_ERROR2(av_interleaved_write_frame(oc_.get(), &pkt), L"[ffmpeg_consumer]");
	}
		 
	void send(const safe_ptr<core::read_frame>& frame)
	{
		executor_.begin_invoke([=]
		{				
			encode_video_frame(frame);
			encode_audio_frame(frame);
		});
	}
};

struct ffmpeg_consumer_proxy : public core::frame_consumer
{
	const std::wstring	filename_;
	const bool			key_only_;
	const std::string	codec_;
	const int			bitrate_;

	std::unique_ptr<ffmpeg_consumer> consumer_;

public:

	ffmpeg_consumer_proxy(const std::wstring& filename, bool key_only, const std::string codec, int bitrate)
		: filename_(filename)
		, key_only_(key_only)
		, codec_(boost::to_lower_copy(codec))
		, bitrate_(bitrate)
	{
	}
	
	virtual void initialize(const core::video_format_desc& format_desc, int, int)
	{
		consumer_.reset();
		consumer_.reset(new ffmpeg_consumer(narrow(filename_), format_desc, codec_, bitrate_));
	}
	
	virtual bool send(const safe_ptr<core::read_frame>& frame) override
	{
		consumer_->send(frame);
		return true;
	}
	
	virtual std::wstring print() const override
	{
		return consumer_ ? consumer_->print() : L"[ffmpeg_consumer]";
	}
		
	virtual bool has_synchronization_clock() const override
	{
		return false;
	}

	virtual size_t buffer_depth() const override
	{
		return 1;
	}
};	

safe_ptr<core::frame_consumer> create_ffmpeg_consumer(const std::vector<std::wstring>& params)
{
	if(params.size() < 2 || params[0] != L"FILE")
		return core::frame_consumer::empty();
	
	// TODO: Ask stakeholders about case where file already exists.
	boost::filesystem::remove(boost::filesystem::wpath(env::media_folder() + params[1])); // Delete the file if it exists
	bool key_only = std::find(params.begin(), params.end(), L"KEY_ONLY") != params.end();

	std::string codec = "dnxhd";
	auto codec_it = std::find(params.begin(), params.end(), L"CODEC");
	if(codec_it++ != params.end())
		codec = narrow(*codec_it);

	int bitrate = 0;	
	auto bitrate_it = std::find(params.begin(), params.end(), L"BITRATE");
	if(bitrate_it++ != params.end())
		bitrate = boost::lexical_cast<int>(*codec_it);

	return make_safe<ffmpeg_consumer_proxy>(env::media_folder() + params[1], key_only, codec, bitrate);
}

safe_ptr<core::frame_consumer> create_ffmpeg_consumer(const boost::property_tree::ptree& ptree)
{
	std::string filename = ptree.get<std::string>("path");
	auto key_only		 = ptree.get("key-only", false);
	auto codec			 = ptree.get("codec", "dnxhd");
	auto bitrate		 = ptree.get("bitrate", 0);
	
	return make_safe<ffmpeg_consumer_proxy>(env::media_folder() + widen(filename), key_only, codec, bitrate);
}

}}
