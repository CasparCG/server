/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/
 
#include "../StdAfx.h"

#include "../ffmpeg_error.h"

#include "ffmpeg_consumer.h"

#include <core/frame/data_frame.h>
#include <core/mixer/audio/audio_util.h>
#include <core/consumer/frame_consumer.h>
#include <core/video_format.h>

#include <common/env.h>
#include <common/concurrency/executor.h>
#include <common/diagnostics/graph.h>
#include <common/memory/safe_ptr.h>
#include <common/utf.h>
#include <common/param.h>

#include <boost/algorithm/string.hpp>
#include <boost/timer.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/filesystem.hpp>

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
	#include <libavutil/opt.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar { namespace ffmpeg {
	
struct ffmpeg_consumer : boost::noncopyable
{		
	const std::wstring						filename_;
		
	const std::shared_ptr<AVFormatContext>	oc_;
	const core::video_format_desc			format_desc_;
	
	const safe_ptr<diagnostics::graph>		graph_;
	boost::timer							frame_timer_;
	boost::timer							write_timer_;

	executor								executor_;
	executor								file_write_executor_;

	// Audio
	std::shared_ptr<AVStream>				audio_st_;
	
	// Video
	std::shared_ptr<AVStream>				video_st_;

	std::vector<uint8_t>					video_outbuf_;
	std::vector<uint8_t>					picture_buf_;
	std::shared_ptr<SwsContext>				sws_;

	int64_t									frame_number_;
	
public:
	ffmpeg_consumer(const std::wstring& filename, const core::video_format_desc& format_desc, const std::wstring& codec, const std::vector<std::wstring>& options)
		: filename_(filename)
		, video_outbuf_(1920*1080*8)
		, oc_(avformat_alloc_context(), av_free)
		, format_desc_(format_desc)
		, executor_(print())
		, file_write_executor_(print() + L"/output")
		, frame_number_(0)
	{
		// TODO: Ask stakeholders about case where file already exists.
		boost::filesystem::remove(boost::filesystem::path(env::media_folder() + filename)); // Delete the file if it exists

		graph_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
		graph_->set_color("write-time", diagnostics::color(0.5f, 0.5f, 0.1f));
		graph_->set_text(print());
		diagnostics::register_graph(graph_);

		executor_.set_capacity(8);
		file_write_executor_.set_capacity(8);

		oc_->oformat = av_guess_format(nullptr, u8(filename_).c_str(), nullptr);
		if (!oc_->oformat)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Could not find suitable output format."));
		
		THROW_ON_ERROR2(av_set_parameters(oc_.get(), nullptr), "[ffmpeg_consumer]");

		strcpy_s(oc_->filename, u8(filename_).c_str());
		
		auto video_codec = avcodec_find_encoder_by_name(u8(codec).c_str());
		if(video_codec == nullptr)
			BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info(codec));

		//  Add the audio and video streams using the default format codecs	and initialize the codecs .
		video_st_ = add_video_stream(video_codec->id, options);
		audio_st_ = add_audio_stream();
				
		dump_format(oc_.get(), 0, u8(filename_).c_str(), 1);
		 
		// Open the output ffmpeg, if needed.
		if (!(oc_->oformat->flags & AVFMT_NOFILE)) 
			THROW_ON_ERROR2(avio_open(&oc_->pb, u8(filename_).c_str(), URL_WRONLY), "[ffmpeg_consumer]");
				
		THROW_ON_ERROR2(av_write_header(oc_.get()), "[ffmpeg_consumer]");

		CASPAR_LOG(info) << print() << L" Successfully Initialized.";	
	}

	~ffmpeg_consumer()
	{    
		executor_.wait();
		executor_.stop();
		executor_.join();

		file_write_executor_.wait();
		file_write_executor_.stop();
		file_write_executor_.join();
		
		LOG_ON_ERROR2(av_write_trailer(oc_.get()), "[ffmpeg_consumer]");
		
		audio_st_.reset();
		video_st_.reset();
			  
		if (!(oc_->oformat->flags & AVFMT_NOFILE)) 
			LOG_ON_ERROR2(avio_close(oc_->pb), "[ffmpeg_consumer]"); // Close the output ffmpeg.

		CASPAR_LOG(info) << print() << L" Successfully Uninitialized.";	
	}
			
	std::wstring print() const
	{
		return L"ffmpeg[" + filename_ + L"]";
	}

	std::shared_ptr<AVStream> add_video_stream(enum CodecID codec_id, const std::vector<std::wstring>& options)
	{ 
		auto st = av_new_stream(oc_.get(), 0);
		if (!st) 		
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Could not allocate video-stream") << boost::errinfo_api_function("av_new_stream"));		

		auto encoder = avcodec_find_encoder(codec_id);
		if (!encoder)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("codec not found"));

		auto c = st->codec;

		avcodec_get_context_defaults3(c, encoder);
				
		c->codec_id			= codec_id;
		c->codec_type		= AVMEDIA_TYPE_VIDEO;
		c->width			= format_desc_.width;
		c->height			= format_desc_.height;
		c->time_base.den	= format_desc_.time_scale;
		c->time_base.num	= format_desc_.duration;
		c->gop_size			= 25;
		c->flags		   |= format_desc_.field_mode == core::field_mode::progressive ? 0 : (CODEC_FLAG_INTERLACED_ME | CODEC_FLAG_INTERLACED_DCT);

		if(c->codec_id == CODEC_ID_PRORES)
		{			
			c->bit_rate	= format_desc_.width < 1280 ? 63*1000000 : 220*1000000;
			c->pix_fmt	= PIX_FMT_YUV422P10;
		}
		else if(c->codec_id == CODEC_ID_DNXHD)
		{
			if(format_desc_.width < 1280 || format_desc_.height < 720)
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("unsupported dimension"));

			c->bit_rate	= 220*1000000;
			c->pix_fmt	= PIX_FMT_YUV422P;
		}
		else if(c->codec_id == CODEC_ID_DVVIDEO)
		{
			c->bit_rate	= format_desc_.width < 1280 ? 50*1000000 : 100*1000000;
			c->pix_fmt	= PIX_FMT_YUV422P;
			
			c->width = format_desc_.height == 1280 ? 960  : c->width;

			if(format_desc_.duration == 1001)			
				c->width = format_desc_.height == 1080 ? 1280 : c->width;			
			else
				c->width = format_desc_.height == 1080 ? 1440 : c->width;			
		}
		else if(c->codec_id == CODEC_ID_H264)
		{			   
			c->pix_fmt = PIX_FMT_YUV420P;    
			if(options.empty())
			{
				av_opt_set(c->priv_data, "preset", "ultrafast", 0);
				av_opt_set(c->priv_data, "tune",   "fastdecode",   0);
				av_opt_set(c->priv_data, "crf",    "5",     0);
			}
		}
		else if(c->codec_id == CODEC_ID_QTRLE)
		{
			c->pix_fmt = PIX_FMT_ARGB;
		}
		else
		{
			BOOST_THROW_EXCEPTION(invalid_argument() << msg_info("Unsupported output parameters."));
		}
		
		c->max_b_frames = 0; // b-frames not supported.

		for(size_t n = 0; n < options.size()/2; ++n)
			THROW_ON_ERROR2(av_opt_set(c, u8(options[n*2+0]).c_str(), u8(options[n*2+1]).c_str(), AV_OPT_SEARCH_CHILDREN), "[ffmpeg_consumer]");
		
		if(oc_->oformat->flags & AVFMT_GLOBALHEADER)
			c->flags |= CODEC_FLAG_GLOBAL_HEADER;
		
		c->thread_count = boost::thread::hardware_concurrency();
		THROW_ON_ERROR2(avcodec_open(c, encoder), "[ffmpeg_consumer]");

		return std::shared_ptr<AVStream>(st, [](AVStream* st)
		{
			LOG_ON_ERROR2(avcodec_close(st->codec), "[ffmpeg_consumer]");
			av_freep(&st->codec);
			av_freep(&st);
		});
	}
	
	std::shared_ptr<AVStream> add_audio_stream()
	{
		auto st = av_new_stream(oc_.get(), 1);
		if(!st)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Could not allocate audio-stream") << boost::errinfo_api_function("av_new_stream"));		

		st->codec->codec_id			= CODEC_ID_PCM_S16LE;
		st->codec->codec_type		= AVMEDIA_TYPE_AUDIO;
		st->codec->sample_rate		= 48000;
		st->codec->channels			= 2;
		st->codec->sample_fmt		= SAMPLE_FMT_S16;
		
		if(oc_->oformat->flags & AVFMT_GLOBALHEADER)
			st->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
		
		auto codec = avcodec_find_encoder(st->codec->codec_id);
		if (!codec)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("codec not found"));

		THROW_ON_ERROR2(avcodec_open(st->codec, codec), "[ffmpeg_consumer]");

		return std::shared_ptr<AVStream>(st, [](AVStream* st)
		{
			LOG_ON_ERROR2(avcodec_close(st->codec), "[ffmpeg_consumer]");;
			av_freep(&st->codec);
			av_freep(&st);
		});
	}

	std::shared_ptr<AVFrame> convert_video_frame(const safe_ptr<const core::data_frame>& frame, AVCodecContext* c)
	{
		if(!sws_) 
		{
			sws_.reset(sws_getContext(format_desc_.width, format_desc_.height, PIX_FMT_BGRA, c->width, c->height, c->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr), sws_freeContext);
			if (sws_ == nullptr) 
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Cannot initialize the conversion context"));
		}

		std::shared_ptr<AVFrame> av_frame(avcodec_alloc_frame(), av_free);
		avpicture_fill(reinterpret_cast<AVPicture*>(av_frame.get()), const_cast<uint8_t*>(frame->image_data().begin()), PIX_FMT_BGRA, format_desc_.width, format_desc_.height);
				
		std::shared_ptr<AVFrame> local_av_frame(avcodec_alloc_frame(), av_free);
		picture_buf_.resize(avpicture_get_size(c->pix_fmt, format_desc_.width, format_desc_.height));
		avpicture_fill(reinterpret_cast<AVPicture*>(local_av_frame.get()), picture_buf_.data(), c->pix_fmt, format_desc_.width, format_desc_.height);

		sws_scale(sws_.get(), av_frame->data, av_frame->linesize, 0, c->height, local_av_frame->data, local_av_frame->linesize);

		return local_av_frame;
	}
  
	std::shared_ptr<AVPacket> encode_video_frame(const safe_ptr<const core::data_frame>& frame)
	{ 
		auto c = video_st_->codec;
 
		auto av_frame = convert_video_frame(frame, c);
		av_frame->interlaced_frame	= format_desc_.field_mode != core::field_mode::progressive;
		av_frame->top_field_first	= format_desc_.field_mode == core::field_mode::upper;
		av_frame->pts				= frame_number_++;
		
		int out_size = THROW_ON_ERROR2(avcodec_encode_video(c, video_outbuf_.data(), static_cast<int>(video_outbuf_.size()), av_frame.get()), "[ffmpeg_consumer]");
		if(out_size > 0)
		{
			safe_ptr<AVPacket> pkt(new AVPacket, [](AVPacket* p)
			{
				av_free_packet(p);
				delete p;
			});
			av_init_packet(pkt.get());
 
			if (c->coded_frame->pts != AV_NOPTS_VALUE)
				pkt->pts = av_rescale_q(c->coded_frame->pts, c->time_base, video_st_->time_base);

			if(c->coded_frame->key_frame)
				pkt->flags |= AV_PKT_FLAG_KEY;

			pkt->stream_index	= video_st_->index;
			pkt->data			= video_outbuf_.data();
			pkt->size			= out_size;
 
			av_dup_packet(pkt.get());
			return pkt;
		}	
		return nullptr;
	}
		
	std::shared_ptr<AVPacket> encode_audio_frame(const safe_ptr<const core::data_frame>& frame)
	{			
		auto c = audio_st_->codec;

		auto audio_data = core::audio_32_to_16(frame->audio_data());
		
		safe_ptr<AVPacket> pkt(new AVPacket, [](AVPacket* p)
		{
			av_free_packet(p);
			delete p;
		});
		av_init_packet(pkt.get());
		
		if (c->coded_frame && c->coded_frame->pts != AV_NOPTS_VALUE)
			pkt->pts = av_rescale_q(c->coded_frame->pts, c->time_base, audio_st_->time_base);

		pkt->flags		 |= AV_PKT_FLAG_KEY;
		pkt->stream_index = audio_st_->index;
		pkt->size		 = static_cast<int>(audio_data.size()*2);
		pkt->data		 = reinterpret_cast<uint8_t*>(audio_data.data());
		
		av_dup_packet(pkt.get());
		return pkt;
	}
		 
	void send(const safe_ptr<const core::data_frame>& frame)
	{
		executor_.begin_invoke([=]
		{		
			frame_timer_.restart();

			auto video = encode_video_frame(frame);
			auto audio = encode_audio_frame(frame);

			graph_->set_value("frame-time", frame_timer_.elapsed()*format_desc_.fps*0.5);
			
			file_write_executor_.begin_invoke([=]
			{
				write_timer_.restart();

				if(video)
					av_write_frame(oc_.get(), video.get());
				if(audio)
					av_write_frame(oc_.get(), audio.get());

				graph_->set_value("write-time", write_timer_.elapsed()*format_desc_.fps*0.5);
			});
		});
	}
};

struct ffmpeg_consumer_proxy : public core::frame_consumer
{
	const std::wstring				filename_;
	const bool						key_only_;
	const std::wstring				codec_;
	const std::vector<std::wstring>	options_;

	std::unique_ptr<ffmpeg_consumer> consumer_;

public:

	ffmpeg_consumer_proxy(const std::wstring& filename, bool key_only, const std::wstring& codec, const std::vector<std::wstring>& options)
		: filename_(filename)
		, key_only_(key_only)
		, codec_(std::move(codec))
		, options_(options)
	{
	}
	
	virtual void initialize(const core::video_format_desc& format_desc, int)
	{
		consumer_.reset();
		consumer_.reset(new ffmpeg_consumer(filename_, format_desc, codec_, options_));
	}
	
	virtual bool send(const safe_ptr<const core::data_frame>& frame) override
	{
		consumer_->send(frame);
		return true;
	}
	
	virtual std::wstring print() const override
	{
		return consumer_ ? consumer_->print() : L"[ffmpeg_consumer]";
	}

	virtual boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"ffmpeg-consumer");
		info.add(L"key-only", key_only_);
		info.add(L"filename", filename_);
		info.add(L"vcodec", codec_);
		return info;
	}
		
	virtual bool has_synchronization_clock() const override
	{
		return false;
	}

	virtual int buffer_depth() const override
	{
		return 1;
	}

	virtual int index() const override
	{
		return 200;
	}
};	

safe_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>& params)
{
	if(params.size() < 1 || params[0] != L"FILE")
		return core::frame_consumer::empty();
	
	auto filename	= (params.size() > 1 ? params[1] : L"");
	bool key_only	= get_param(L"KEY_ONLY", params, false);
	auto codec		= get_param(L"-VCODEC", params,  get_param(L"CODEC", params, L"libx264"));

	std::vector<std::wstring> options;
	auto opt_it = std::find(params.begin(), params.end(), L"-VCODEC");

	if(std::distance(opt_it, params.end()) > 2)
	{
		std::advance(opt_it, 2);
		while(opt_it != params.end())
		{
			auto str = *opt_it++;
			if(str.size() > 0 && str.at(0) == L'-')
				str = str.substr(1);
			options.push_back(boost::to_lower_copy(str));		
		}
	}
		
	if(codec == L"H264")
		codec = L"libx264";

	if(codec == L"DVCPRO")
		codec = L"dvvideo";

	codec = boost::to_lower_copy(codec);
	
	return make_safe<ffmpeg_consumer_proxy>(env::media_folder() + filename, key_only, codec, options);
}

safe_ptr<core::frame_consumer> create_consumer(const boost::property_tree::wptree& ptree)
{
	auto filename	= ptree.get<std::wstring>(L"path");
	auto key_only	= ptree.get(L"key-only", false);
	auto codec		= ptree.get(L"vcodec", L"libx264");
	
	return make_safe<ffmpeg_consumer_proxy>(env::media_folder() + filename, key_only, codec, std::vector<std::wstring>());
}

}}
