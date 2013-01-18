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

#include "../producer/audio/audio_resampler.h"

#include <core/mixer/read_frame.h>
#include <core/mixer/audio/audio_util.h>
#include <core/consumer/frame_consumer.h>
#include <core/video_format.h>

#include <common/concurrency/executor.h>
#include <common/concurrency/future_util.h>
#include <common/diagnostics/graph.h>
#include <common/env.h>
#include <common/utility/string.h>
#include <common/utility/param.h>
#include <common/memory/memshfl.h>

#include <boost/algorithm/string.hpp>
#include <boost/timer.hpp>
#include <boost/property_tree/ptree.hpp>

#include <tbb/cache_aligned_allocator.h>
#include <tbb/parallel_invoke.h>

#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/lexical_cast.hpp>

#include <string>

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
	#include <libavutil/pixdesc.h>
	#include <libavutil/parseutils.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar { namespace ffmpeg {
	
int av_opt_set(void *obj, const char *name, const char *val, int search_flags)
{
	AVClass* av_class = *(AVClass**)obj;

	if((strcmp(name, "pix_fmt") == 0 || strcmp(name, "pixel_format") == 0) && strcmp(av_class->class_name, "AVCodecContext") == 0)
	{
		AVCodecContext* c = (AVCodecContext*)obj;		
		auto pix_fmt = av_get_pix_fmt(val);
		if(pix_fmt == PIX_FMT_NONE)
			return -1;		
		c->pix_fmt = pix_fmt;
		return 0;
	}
	if((strcmp(name, "r") == 0 || strcmp(name, "frame_rate") == 0) && strcmp(av_class->class_name, "AVCodecContext") == 0)
	{
		AVCodecContext* c = (AVCodecContext*)obj;	

		if(c->codec_type != AVMEDIA_TYPE_VIDEO)
			return -1;

		AVRational rate;
		int ret = av_parse_video_rate(&rate, val);
		if(ret < 0)
			return ret;

		c->time_base.num = rate.den;
		c->time_base.den = rate.num;
		return 0;
	}

	return ::av_opt_set(obj, name, val, search_flags);
}

struct option
{
	std::string name;
	std::string value;

	option(std::string name, std::string value)
		: name(std::move(name))
		, value(std::move(value))
	{
	}
};
	
struct output_format
{
	AVOutputFormat* format;
	int				width;
	int				height;
	CodecID			vcodec;
	CodecID			acodec;

	output_format(const core::video_format_desc& format_desc, const std::string& filename, std::vector<option>& options)
		: format(av_guess_format(nullptr, filename.c_str(), nullptr))
		, width(format_desc.width)
		, height(format_desc.height)
		, vcodec(CODEC_ID_NONE)
		, acodec(CODEC_ID_NONE)
	{
		boost::range::remove_erase_if(options, [&](const option& o)
		{
			return set_opt(o.name, o.value);
		});
		
		if(vcodec == CODEC_ID_NONE)
			vcodec = format->video_codec;

		if(acodec == CODEC_ID_NONE)
			acodec = format->audio_codec;
		
		if(vcodec == CODEC_ID_NONE)
			vcodec = CODEC_ID_H264;
		
		if(acodec == CODEC_ID_NONE)
			acodec = CODEC_ID_PCM_S16LE;
	}
	
	bool set_opt(const std::string& name, const std::string& value)
	{
		//if(name == "target")
		//{ 
		//	enum { PAL, NTSC, FILM, UNKNOWN } norm = UNKNOWN;
		//	
		//	if(name.find("pal-") != std::string::npos)
		//		norm = PAL;
		//	else if(name.find("ntsc-") != std::string::npos)
		//		norm = NTSC;

		//	if(norm == UNKNOWN)
		//		BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("target"));
		//	
		//	if (name.find("-dv") != std::string::npos) 
		//	{
		//		set_opt("f", "dv");
		//		set_opt("s", norm == PAL ? "720x576" : "720x480");
		//		//set_opt("pix_fmt", name.find("-dv50") != std::string::npos ? "yuv422p" : norm == PAL ? "yuv420p" : "yuv411p");
		//		//set_opt("ar", "48000");
		//		//set_opt("ac", "2");
		//	} 
		//}
		if(name == "f")
		{
			format = av_guess_format(value.c_str(), nullptr, nullptr);

			if(format == nullptr)
				BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("f"));

			return true;
		}
		else if(name == "vcodec")
		{
			auto c = avcodec_find_encoder_by_name(value.c_str());
			if(c == nullptr)
				BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("vcodec"));

			vcodec = avcodec_find_encoder_by_name(value.c_str())->id;
			return true;

		}
		else if(name == "acodec")
		{
			auto c = avcodec_find_encoder_by_name(value.c_str());
			if(c == nullptr)
				BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("acodec"));

			acodec = avcodec_find_encoder_by_name(value.c_str())->id;

			return true;
		}
		else if(name == "s")
		{
			if(av_parse_video_size(&width, &height, value.c_str()) < 0)
				BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("s"));
			
			return true;
		}

		return false;
	}
};

typedef std::vector<uint8_t, tbb::cache_aligned_allocator<uint8_t>>	byte_vector;

struct ffmpeg_consumer : boost::noncopyable
{		
	const std::string						filename_;
		
	const std::shared_ptr<AVFormatContext>	oc_;
	const core::video_format_desc			format_desc_;
	
	const safe_ptr<diagnostics::graph>		graph_;

	executor								encode_executor_;
	
	std::shared_ptr<AVStream>				audio_st_;
	std::shared_ptr<AVStream>				video_st_;
	
	byte_vector								audio_outbuf_;
	byte_vector								audio_buf_;
	byte_vector								video_outbuf_;
	byte_vector								key_picture_buf_;
	byte_vector								picture_buf_;
	std::shared_ptr<audio_resampler>		swr_;
	std::shared_ptr<SwsContext>				sws_;

	int64_t									in_frame_number_;
	int64_t									out_frame_number_;

	output_format							output_format_;
	bool									key_only_;
	
public:
	ffmpeg_consumer(const std::string& filename, const core::video_format_desc& format_desc, std::vector<option> options, bool key_only)
		: filename_(filename)
		, video_outbuf_(1920*1080*8)
		, audio_outbuf_(10000)
		, oc_(avformat_alloc_context(), av_free)
		, format_desc_(format_desc)
		, encode_executor_(print())
		, in_frame_number_(0)
		, out_frame_number_(0)
		, output_format_(format_desc, filename, options)
		, key_only_(key_only)
	{
		// TODO: Ask stakeholders about case where file already exists.
		boost::filesystem2::remove(boost::filesystem2::wpath(env::media_folder() + widen(filename))); // Delete the file if it exists

		graph_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
		graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
		graph_->set_text(print());
		diagnostics::register_graph(graph_);

		encode_executor_.set_capacity(8);

		oc_->oformat = output_format_.format;
				
		THROW_ON_ERROR2(av_set_parameters(oc_.get(), nullptr), "[ffmpeg_consumer]");

		strcpy_s(oc_->filename, filename_.c_str());
		
		//  Add the audio and video streams using the default format codecs	and initialize the codecs.
		auto options2 = options;
		video_st_ = add_video_stream(options2);

		if (!key_only)
			audio_st_ = add_audio_stream(options);
				
		dump_format(oc_.get(), 0, filename_.c_str(), 1);
		 
		// Open the output ffmpeg, if needed.
		if (!(oc_->oformat->flags & AVFMT_NOFILE)) 
			THROW_ON_ERROR2(avio_open(&oc_->pb, filename_.c_str(), URL_WRONLY), "[ffmpeg_consumer]");
				
		THROW_ON_ERROR2(av_write_header(oc_.get()), "[ffmpeg_consumer]");

		if(options.size() > 0)
		{
			BOOST_FOREACH(auto& option, options)
				CASPAR_LOG(warning) << L"Invalid option: -" << widen(option.name) << L" " << widen(option.value);
		}

		CASPAR_LOG(info) << print() << L" Successfully Initialized.";	
	}

	~ffmpeg_consumer()
	{    
		encode_executor_.stop_execute_rest();
		encode_executor_.join();

		// Flush
		LOG_ON_ERROR2(av_interleaved_write_frame(oc_.get(), nullptr), "[ffmpeg_consumer]");
		
		LOG_ON_ERROR2(av_write_trailer(oc_.get()), "[ffmpeg_consumer]");
		
		if (!key_only_)
			audio_st_.reset();
		video_st_.reset();
			  
		if (!(oc_->oformat->flags & AVFMT_NOFILE)) 
			LOG_ON_ERROR2(avio_close(oc_->pb), "[ffmpeg_consumer]"); // Close the output ffmpeg.

		CASPAR_LOG(info) << print() << L" Successfully Uninitialized.";	
	}
			
	std::wstring print() const
	{
		return L"ffmpeg[" + widen(filename_) + L"]";
	}

	std::shared_ptr<AVStream> add_video_stream(std::vector<option>& options)
	{ 
		if(output_format_.vcodec == CODEC_ID_NONE)
			return nullptr;

		auto st = av_new_stream(oc_.get(), 0);
		if (!st) 		
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Could not allocate video-stream.") << boost::errinfo_api_function("av_new_stream"));		

		auto encoder = avcodec_find_encoder(output_format_.vcodec);
		if (!encoder)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Codec not found."));

		auto c = st->codec;

		avcodec_get_context_defaults3(c, encoder);
				
		c->codec_id			= output_format_.vcodec;
		c->codec_type		= AVMEDIA_TYPE_VIDEO;
		c->width			= output_format_.width;
		c->height			= output_format_.height;
		c->time_base.den	= format_desc_.time_scale;
		c->time_base.num	= format_desc_.duration;
		c->gop_size			= 25;
		c->flags		   |= format_desc_.field_mode == core::field_mode::progressive ? 0 : (CODEC_FLAG_INTERLACED_ME | CODEC_FLAG_INTERLACED_DCT);
		if(c->pix_fmt == PIX_FMT_NONE)
			c->pix_fmt = PIX_FMT_YUV420P;

		if(c->codec_id == CODEC_ID_PRORES)
		{			
			c->bit_rate	= c->width < 1280 ? 63*1000000 : 220*1000000;
			c->pix_fmt	= PIX_FMT_YUV422P10;
		}
		else if(c->codec_id == CODEC_ID_DNXHD)
		{
			if(c->width < 1280 || c->height < 720)
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Unsupported video dimensions."));

			c->bit_rate	= 220*1000000;
			c->pix_fmt	= PIX_FMT_YUV422P;
		}
		else if(c->codec_id == CODEC_ID_DVVIDEO)
		{
			c->width = c->height == 1280 ? 960  : c->width;
			
			if(format_desc_.format == core::video_format::ntsc)
				c->pix_fmt = PIX_FMT_YUV411P;
			else if(format_desc_.format == core::video_format::pal)
				c->pix_fmt = PIX_FMT_YUV420P;
			else // dv50
				c->pix_fmt = PIX_FMT_YUV422P;
			
			if(format_desc_.duration == 1001)			
				c->width = c->height == 1080 ? 1280 : c->width;			
			else
				c->width = c->height == 1080 ? 1440 : c->width;			
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
				
		c->max_b_frames = 0; // b-frames not supported.
				
		boost::range::remove_erase_if(options, [&](const option& o)
		{
			return ffmpeg::av_opt_set(c, o.name.c_str(), o.value.c_str(), AV_OPT_SEARCH_CHILDREN) > -1 ||
				   ffmpeg::av_opt_set(c->priv_data, o.name.c_str(), o.value.c_str(), AV_OPT_SEARCH_CHILDREN) > -1;
		});
				
		if(output_format_.format->flags & AVFMT_GLOBALHEADER)
			c->flags |= CODEC_FLAG_GLOBAL_HEADER;
		
		c->thread_count = boost::thread::hardware_concurrency();
		if(avcodec_open(c, encoder) < 0)
		{
			c->thread_count = 1;
			THROW_ON_ERROR2(avcodec_open(c, encoder), "[ffmpeg_consumer]");
		}

		return std::shared_ptr<AVStream>(st, [](AVStream* st)
		{
			LOG_ON_ERROR2(avcodec_close(st->codec), "[ffmpeg_consumer]");
			av_freep(&st->codec);
			av_freep(&st);
		});
	}
		
	std::shared_ptr<AVStream> add_audio_stream(std::vector<option>& options)
	{
		if(output_format_.acodec == CODEC_ID_NONE)
			return nullptr;

		auto st = av_new_stream(oc_.get(), 1);
		if(!st)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Could not allocate audio-stream") << boost::errinfo_api_function("av_new_stream"));		
		
		auto encoder = avcodec_find_encoder(output_format_.acodec);
		if (!encoder)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("codec not found"));
		
		auto c = st->codec;

		avcodec_get_context_defaults3(c, encoder);

		c->codec_id			= output_format_.acodec;
		c->codec_type		= AVMEDIA_TYPE_AUDIO;
		c->sample_rate		= 48000;
		c->channels			= 2;
		c->sample_fmt		= SAMPLE_FMT_S16;

		if(output_format_.vcodec == CODEC_ID_FLV1)		
			c->sample_rate	= 44100;		

		if(output_format_.format->flags & AVFMT_GLOBALHEADER)
			c->flags |= CODEC_FLAG_GLOBAL_HEADER;
				
		boost::range::remove_erase_if(options, [&](const option& o)
		{
			return ffmpeg::av_opt_set(c, o.name.c_str(), o.value.c_str(), AV_OPT_SEARCH_CHILDREN) > -1;
		});

		THROW_ON_ERROR2(avcodec_open(c, encoder), "[ffmpeg_consumer]");

		return std::shared_ptr<AVStream>(st, [](AVStream* st)
		{
			LOG_ON_ERROR2(avcodec_close(st->codec), "[ffmpeg_consumer]");;
			av_freep(&st->codec);
			av_freep(&st);
		});
	}

	std::shared_ptr<AVFrame> convert_video(core::read_frame& frame, AVCodecContext* c)
	{
		if(!sws_) 
		{
			sws_.reset(sws_getContext(format_desc_.width, format_desc_.height, PIX_FMT_BGRA, c->width, c->height, c->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr), sws_freeContext);
			if (sws_ == nullptr) 
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Cannot initialize the conversion context"));
		}

		std::shared_ptr<AVFrame> in_frame(avcodec_alloc_frame(), av_free);
		auto in_picture = reinterpret_cast<AVPicture*>(in_frame.get());

		if (key_only_)
		{
			key_picture_buf_.resize(frame.image_data().size());
			in_picture->linesize[0] = format_desc_.width * 4;
			in_picture->data[0] = key_picture_buf_.data();

			fast_memshfl(in_picture->data[0], frame.image_data().begin(), frame.image_data().size(), 0x0F0F0F0F, 0x0B0B0B0B, 0x07070707, 0x03030303);
		}
		else
		{
			avpicture_fill(in_picture, const_cast<uint8_t*>(frame.image_data().begin()), PIX_FMT_BGRA, format_desc_.width, format_desc_.height);
		}

		std::shared_ptr<AVFrame> out_frame(avcodec_alloc_frame(), av_free);
		picture_buf_.resize(avpicture_get_size(c->pix_fmt, c->width, c->height));
		avpicture_fill(reinterpret_cast<AVPicture*>(out_frame.get()), picture_buf_.data(), c->pix_fmt, c->width, c->height);

		sws_scale(sws_.get(), in_frame->data, in_frame->linesize, 0, format_desc_.height, out_frame->data, out_frame->linesize);

		return out_frame;
	}
  
	void encode_video_frame(core::read_frame& frame)
	{ 
		auto c = video_st_->codec;
		
		auto in_time  = static_cast<double>(in_frame_number_) / format_desc_.fps;
		auto out_time = static_cast<double>(out_frame_number_) / (static_cast<double>(c->time_base.den) / static_cast<double>(c->time_base.num));
		
		in_frame_number_++;

		if(out_time - in_time > 0.01)
			return;
 
		auto av_frame = convert_video(frame, c);
		av_frame->interlaced_frame	= format_desc_.field_mode != core::field_mode::progressive;
		av_frame->top_field_first	= format_desc_.field_mode == core::field_mode::upper;
		av_frame->pts				= out_frame_number_++;

		int out_size = THROW_ON_ERROR2(avcodec_encode_video(c, video_outbuf_.data(), video_outbuf_.size(), av_frame.get()), "[ffmpeg_consumer]");
		if(out_size == 0)
			return;
				
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
 			
		av_interleaved_write_frame(oc_.get(), pkt.get());		
	}
		
	byte_vector convert_audio(core::read_frame& frame, AVCodecContext* c)
	{
		if(!swr_) 		
			swr_.reset(new audio_resampler(c->channels, format_desc_.audio_channels, 
										   c->sample_rate, format_desc_.audio_sample_rate,
										   c->sample_fmt, AV_SAMPLE_FMT_S32));
		

		auto audio_data = frame.audio_data();

		std::vector<int8_t,  tbb::cache_aligned_allocator<int8_t>> audio_resample_buffer;
		std::copy(reinterpret_cast<const uint8_t*>(audio_data.begin()), 
				  reinterpret_cast<const uint8_t*>(audio_data.begin()) + audio_data.size()*4, 
				  std::back_inserter(audio_resample_buffer));
		
		audio_resample_buffer = swr_->resample(std::move(audio_resample_buffer));
		
		return byte_vector(audio_resample_buffer.begin(), audio_resample_buffer.end());
	}

	void encode_audio_frame(core::read_frame& frame)
	{			
		auto c = audio_st_->codec;

		boost::range::push_back(audio_buf_, convert_audio(frame, c));
		
		std::size_t frame_size = c->frame_size;
		auto input_audio_size = frame_size * av_get_bytes_per_sample(c->sample_fmt) * c->channels;
		
		while(audio_buf_.size() >= input_audio_size)
		{
			safe_ptr<AVPacket> pkt(new AVPacket, [](AVPacket* p)
			{
				av_free_packet(p);
				delete p;
			});
			av_init_packet(pkt.get());

			if(frame_size > 1)
			{								
				pkt->size = avcodec_encode_audio(c, audio_outbuf_.data(), audio_outbuf_.size(), reinterpret_cast<short*>(audio_buf_.data()));
				audio_buf_.erase(audio_buf_.begin(), audio_buf_.begin() + input_audio_size);
			}
			else
			{
				audio_outbuf_ = std::move(audio_buf_);		
				audio_buf_.clear();
				pkt->size = audio_outbuf_.size();
				pkt->data = audio_outbuf_.data();
			}
		
			if(pkt->size == 0)
				return;

			if (c->coded_frame && c->coded_frame->pts != AV_NOPTS_VALUE)
				pkt->pts = av_rescale_q(c->coded_frame->pts, c->time_base, audio_st_->time_base);

			pkt->flags		 |= AV_PKT_FLAG_KEY;
			pkt->stream_index = audio_st_->index;
			pkt->data		  = reinterpret_cast<uint8_t*>(audio_outbuf_.data());
		
			av_interleaved_write_frame(oc_.get(), pkt.get());
		}
	}
		 
	void send(const safe_ptr<core::read_frame>& frame)
	{
		encode_executor_.begin_invoke([=]
		{		
			boost::timer frame_timer;

			encode_video_frame(*frame);

			if (!key_only_)
				encode_audio_frame(*frame);

			graph_->set_value("frame-time", frame_timer.elapsed()*format_desc_.fps*0.5);			
		});
	}

	bool ready_for_frame()
	{
		return encode_executor_.size() < encode_executor_.capacity();
	}

	void mark_dropped()
	{
		graph_->set_tag("dropped-frame");

		// TODO: adjust PTS accordingly to make dropped frames contribute
		//       to the total playing time
	}
};

struct ffmpeg_consumer_proxy : public core::frame_consumer
{
	const std::wstring				filename_;
	const std::vector<option>		options_;
	const bool						separate_key_;

	std::unique_ptr<ffmpeg_consumer> consumer_;
	std::unique_ptr<ffmpeg_consumer> key_only_consumer_;

public:

	ffmpeg_consumer_proxy(const std::wstring& filename, const std::vector<option>& options, bool separate_key_)
		: filename_(filename)
		, options_(options)
		, separate_key_(separate_key_)
	{
	}
	
	virtual void initialize(const core::video_format_desc& format_desc, int)
	{
		consumer_.reset();
		key_only_consumer_.reset();
		consumer_.reset(new ffmpeg_consumer(narrow(filename_), format_desc, options_, false));

		if (separate_key_)
		{
			boost::filesystem::wpath fill_file(filename_);
			auto without_extension = fill_file.stem();
			auto key_file = env::media_folder() + without_extension + L"_A" + fill_file.extension();
			
			key_only_consumer_.reset(new ffmpeg_consumer(narrow(key_file), format_desc, options_, true));
		}
	}
	
	virtual boost::unique_future<bool> send(const safe_ptr<core::read_frame>& frame) override
	{
		bool ready_for_frame = consumer_->ready_for_frame();

		if (ready_for_frame && separate_key_)
			ready_for_frame = ready_for_frame && key_only_consumer_->ready_for_frame();

		if (ready_for_frame)
		{
			consumer_->send(frame);

			if (separate_key_)
				key_only_consumer_->send(frame);
		}
		else
		{
			consumer_->mark_dropped();

			if (separate_key_)
				key_only_consumer_->mark_dropped();
		}

		return caspar::wrap_as_future(true);
	}
	
	virtual std::wstring print() const override
	{
		return consumer_ ? consumer_->print() : L"[ffmpeg_consumer]";
	}

	virtual boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"ffmpeg-consumer");
		info.add(L"filename", filename_);
		info.add(L"separate_key", separate_key_);
		return info;
	}
		
	virtual bool has_synchronization_clock() const override
	{
		return false;
	}

	virtual size_t buffer_depth() const override
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

	std::vector<std::wstring> params2 = params;
	
	auto filename	= (params2.size() > 1 ? params2[1] : L"");
	auto separate_key_it = std::find(params2.begin(), params2.end(), L"SEPARATE_KEY");
	bool separate_key = false;

	if (separate_key_it != params2.end())
	{
		separate_key = true;
		params2.erase(separate_key_it);
	}
			
	std::vector<option> options;
	
	if(params2.size() >= 3)
	{
		for(auto opt_it = params2.begin()+2; opt_it != params2.end();)
		{
			auto name  = narrow(boost::trim_copy(boost::to_lower_copy(*opt_it++))).substr(1);
			auto value = narrow(boost::trim_copy(boost::to_lower_copy(*opt_it++)));
				
			if(value == "h264")
				value = "libx264";
			else if(value == "dvcpro")
				value = "dvvideo";

			options.push_back(option(name, value));
		}
	}
		
	return make_safe<ffmpeg_consumer_proxy>(env::media_folder() + filename, options, separate_key);
}

safe_ptr<core::frame_consumer> create_consumer(const boost::property_tree::wptree& ptree)
{
	auto filename		= ptree.get<std::wstring>(L"path");
	auto codec			= ptree.get(L"vcodec", L"libx264");
	auto separate_key	= ptree.get(L"separate-key", false);

	std::vector<option> options;
	options.push_back(option("vcodec", narrow(codec)));
	
	return make_safe<ffmpeg_consumer_proxy>(env::media_folder() + filename, options, separate_key);
}

}}
