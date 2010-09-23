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
 
#include "../../StdAfx.h"

#include "file_frame_consumer.h"

#include "../../frame/audio_chunk.h"
#include "../../frame/frame.h"
#include "../../frame/format.h"

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

namespace caspar { namespace file {	
	
struct file_frame_consumer::implementation : boost::noncopyable
{		
	implementation(const std::string& filename, const frame_format_desc& format_desc)
		: format_desc_(format_desc), audio_st_(0), t_(0.0f), tincr_(0.0f), tincr2_(0.0f), empty_audio_(0),
			audio_outbuf_(0), audio_outbuf_size_(0), audio_input_frame_size_(0), video_st_(0), picture_(0),
				video_outbuf_(0), video_outbuf_size_(0), fmt_(0), oc_(0), img_convert_ctx_(0)
	{
		fmt_ = av_guess_format(nullptr, filename.c_str(), nullptr);
		if (!fmt_) 
		{
			CASPAR_LOG(warning) << "Could not deduce output format from file extension: using MPEG.";
			fmt_ = av_guess_format("mpeg", nullptr, nullptr);
		}
		if (!fmt_)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Could not find suitable output format"));
		
		// allocate the output media context
		oc_ = avformat_alloc_context();
		if (!oc_)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Memory error"));

		oc_->oformat = fmt_;
		// To avoid mpeg buffer underflow (http://www.mail-archive.com/libav-user@mplayerhq.hu/msg00194.html)
		oc_->preload = static_cast<int>(0.5*AV_TIME_BASE);
		oc_->max_delay = static_cast<int>(0.7*AV_TIME_BASE);
		_snprintf(oc_->filename, sizeof(oc_->filename), "%s", filename.c_str());
		    
		//  add the audio and video streams using the default format codecs	and initialize the codecs 
		if (fmt_->video_codec != CODEC_ID_NONE)		
			video_st_ = add_video_stream(fmt_->video_codec);
		
		if (fmt_->audio_codec != CODEC_ID_NONE) 
			audio_st_ = add_audio_stream(fmt_->audio_codec);	

		// set the output parameters (must be done even if no parameters).
		if (av_set_parameters(oc_, nullptr) < 0) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Invalid output format parameters"));

		dump_format(oc_, 0, filename.c_str(), 1);

		// now that all the parameters are set, we can open the audio and
		// video codecs and allocate the necessary encode buffers
		if (video_st_)
			open_video(video_st_);
		if (audio_st_)
			open_audio(audio_st_);
 
		/* open the output file, if needed */
		if (!(fmt_->flags & AVFMT_NOFILE)) 
		{
			if (url_fopen(&oc_->pb, filename.c_str(), URL_WRONLY) < 0) 
				BOOST_THROW_EXCEPTION(file_not_found() << msg_info("Could not open file"));
		}
 		
		av_write_header(oc_); // write the stream header, if any 
	}

	~implementation()
	{    
		// write the trailer, if any.  the trailer must be written
		// before you close the CodecContexts open when you wrote the
		// header; otherwise write_trailer may try to use memory that
		// was freed on av_codec_close()
		av_write_trailer(oc_);

		/* close each codec */
		if (video_st_)		
			avcodec_close(video_st_->codec);
		
		if (audio_st_)
			avcodec_close(audio_st_->codec);

		if(empty_audio_)
			av_free(empty_audio_);
		if(audio_outbuf_)
			av_free(audio_outbuf_);
		
		if(picture_)
		{
			av_free(picture_->data[0]);
			av_free(picture_);
		}
		if(video_outbuf_)
			av_free(video_outbuf_);

		// free the streams
		for(size_t i = 0; i < oc_->nb_streams; ++i) 
		{
			av_freep(&oc_->streams[i]->codec);
			av_freep(&oc_->streams[i]);
		}

		if (!(fmt_->flags & AVFMT_NOFILE)) 
			url_fclose(oc_->pb); //  close the output file 		
				
		av_free(oc_); // free the stream 
	}

	AVStream* add_video_stream(enum CodecID codec_id)
	{ 
		auto st = av_new_stream(oc_, 0);
		if (!st) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Could not alloc stream"));
 
		auto c = st->codec;
		c->codec_id = codec_id;
		c->codec_type = AVMEDIA_TYPE_VIDEO;
 
		// put sample parameters
		c->bit_rate = format_desc_.size*static_cast<int>(format_desc_.output_fps);
		// resolution must be a multiple of two */
		c->width = format_desc_.width;
		c->height = format_desc_.height;
		// time base: this is the fundamental unit of time (in seconds) in terms
		// of which frame timestamps are represented. for fixed-fps content,
		// timebase should be 1/framerate and timestamp increments should be
		// identically 1.
		c->time_base.den = static_cast<int>(format_desc_.output_fps);
		c->time_base.num = 1;
		c->gop_size = 12; // emit one intra frame every twelve frames at most
		c->pix_fmt = PIX_FMT_YUV420P;
		if (c->codec_id == CODEC_ID_MPEG2VIDEO) 
		{
			// just for testing, we also add B frames
			c->max_b_frames = 2;
		}
		if (c->codec_id == CODEC_ID_MPEG1VIDEO)
		{
			// Needed to avoid using macroblocks in which some coeffs overflow.
			// This does not happen with normal video, it just happens here as
			// the motion of the chroma plane does not match the luma plane.
			c->mb_decision = 2;
		}
		// some formats want stream headers to be separate
		if(oc_->oformat->flags & AVFMT_GLOBALHEADER)
			c->flags |= CODEC_FLAG_GLOBAL_HEADER;
 
		return st;
	}

	AVFrame* alloc_picture(enum PixelFormat pix_fmt, int width, int height)
	{ 
		AVFrame* picture = avcodec_alloc_frame();
		if (!picture)
			return nullptr;

		size_t size = avpicture_get_size(pix_fmt, width, height);

		uint8_t* picture_buf = static_cast<uint8_t*>(av_malloc(size));
		if (!picture_buf) 
		{
			av_free(picture);
			return nullptr;
		}

		avpicture_fill(reinterpret_cast<AVPicture*>(picture), picture_buf, pix_fmt, width, height);

		return picture;
	}
 
	void open_video(AVStream *st)
	{ 
		 AVCodecContext* c = st->codec;
 
		 AVCodec* codec = avcodec_find_encoder(c->codec_id);
		 if (!codec)
			 BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("codec not found"));
 
		 if (avcodec_open(c, codec) < 0)
			 BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("could not open codec"));		 
 
		 video_outbuf_ = nullptr;
		 if (!(oc_->oformat->flags & AVFMT_RAWPICTURE)) 
		 {
			 // allocate output buffer 
			 // XXX: API change will be done 
			 // buffers passed into lav* can be allocated any way you prefer,
			 //	as long as they're aligned enough for the architecture, and
			 //	they're freed appropriately (such as using av_free for buffers
			 //	allocated with av_malloc) 
			 video_outbuf_size_ = 200000;
			 video_outbuf_ = static_cast<uint8_t*>(av_malloc(video_outbuf_size_));
		 }
 
		 // allocate the encoded raw picture
		 picture_ = alloc_picture(c->pix_fmt, c->width, c->height);
		 if (!picture_)
			 BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Could not allocate picture")); 
	}
  
	void write_video_frame(const frame_ptr& frame)
	{
		int out_size, ret;
 
		AVCodecContext* c = video_st_->codec;
 
		// as we only generate a YUV420P picture, we must convert it to the codec pixel format if needed 
		if (img_convert_ctx_ == nullptr) 
		{
			img_convert_ctx_ = sws_getContext(frame->width(), frame->height(), PIX_FMT_BGRA, c->width, c->height,
											c->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr);
			if (img_convert_ctx_ == nullptr) 
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Cannot initialize the conversion context"));
		}
		std::shared_ptr<AVFrame> av_frame(avcodec_alloc_frame(), av_free);
		avpicture_fill(reinterpret_cast<AVPicture*>(av_frame.get()), frame->data(), PIX_FMT_BGRA, frame->width(), frame->height());

		sws_scale(img_convert_ctx_, av_frame->data, av_frame->linesize, 0, c->height, picture_->data, picture_->linesize);
				 		  
		if (oc_->oformat->flags & AVFMT_RAWPICTURE) 
		{
			// raw video case. The API will change slightly in the near future for that
			AVPacket pkt;
			av_init_packet(&pkt);
 
			pkt.flags |= AV_PKT_FLAG_KEY;
			pkt.stream_index = video_st_->index;
			pkt.data = reinterpret_cast<uint8_t*>(picture_);
			pkt.size = sizeof(AVPicture);
 
			ret = av_interleaved_write_frame(oc_, &pkt);
		} 
		else 
		{
			// encode the image
			out_size = avcodec_encode_video(c, video_outbuf_, video_outbuf_size_, picture_);
			// if zero size, it means the image was buffered
			if (out_size > 0) 
			{
				AVPacket pkt;
				av_init_packet(&pkt);
 
				if (c->coded_frame->pts != AV_NOPTS_VALUE)
					pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, video_st_->time_base);
				if(c->coded_frame->key_frame)
					pkt.flags |= AV_PKT_FLAG_KEY;

				pkt.stream_index = video_st_->index;
				pkt.data = video_outbuf_;
				pkt.size = out_size;
 
				/* write the compressed frame in the media file */
				ret = av_interleaved_write_frame(oc_, &pkt);
			} 
			else 
				ret = 0;		 		
		}

		if (ret != 0) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Error while writing video frame"));
	}

	AVStream* add_audio_stream(enum CodecID codec_id)
	{
		AVCodecContext *c;

		audio_st_ = av_new_stream(oc_, 1);
		if (!audio_st_)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Could not alloc stream"));

		c = audio_st_->codec;
		c->codec_id = codec_id;
		c->codec_type = AVMEDIA_TYPE_AUDIO;

		/* put sample parameters */
		c->bit_rate = 64000;
		c->sample_rate = 48000;
		c->channels = 2;

		// some formats want stream headers to be separate
		if(oc_->oformat->flags & AVFMT_GLOBALHEADER)
			c->flags |= CODEC_FLAG_GLOBAL_HEADER;

		return audio_st_;
	}

	void open_audio(AVStream *st)
	{
		AVCodecContext *c;
		AVCodec *codec;

		c = st->codec;

		// find the audio encoder
		codec = avcodec_find_encoder(c->codec_id);
		if (!codec) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("codec not found"));
		
		// open it
		if (avcodec_open(c, codec) < 0)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("could not open codec"));
		
		audio_outbuf_size_ = 10000;
		audio_outbuf_ = static_cast<uint8_t*>(av_malloc(audio_outbuf_size_));

		// ugly hack for PCM codecs (will be removed ASAP with new PCM
		// support to compute the input frame size in samples
		if (c->frame_size <= 1) 
		{
			audio_input_frame_size_ = audio_outbuf_size_ / c->channels;
			switch(st->codec->codec_id) 
			{
			case CODEC_ID_PCM_S16LE:
			case CODEC_ID_PCM_S16BE:
			case CODEC_ID_PCM_U16LE:
			case CODEC_ID_PCM_U16BE:
				audio_input_frame_size_ >>= 1;
				break;
			default:
				break;
			}
		} 
		else 
			audio_input_frame_size_ = c->frame_size;
		
		empty_audio_ = static_cast<int16_t*>(av_malloc(audio_input_frame_size_ * 2 * c->channels));
		memset(empty_audio_, 0, audio_input_frame_size_ * 2 * c->channels);
	}
	
	void write_audio_frame(const frame_ptr& frame)
	{
		if(!audio_st_)
			return;

		auto audio_data = empty_audio_;

		AVCodecContext *c;
		AVPacket pkt;
		av_init_packet(&pkt);

		c = audio_st_->codec;
		
		audio_data = empty_audio_;
		if(frame->audio_data().size() > 0)
		{	
			assert(frame->audio_data()[0]->size() == audio_input_frame_size_);
			audio_data = reinterpret_cast<int16_t*>(frame->audio_data()[0]->data());
		}

		pkt.size = avcodec_encode_audio(c, audio_outbuf_, audio_outbuf_size_, audio_data);

		if (c->coded_frame && c->coded_frame->pts != AV_NOPTS_VALUE)
			pkt.pts= av_rescale_q(c->coded_frame->pts, c->time_base, audio_st_->time_base);
		pkt.flags |= AV_PKT_FLAG_KEY;
		pkt.stream_index = audio_st_->index;
		pkt.data = audio_outbuf_;

		// write the compressed frame in the media file
		if (av_interleaved_write_frame(oc_, &pkt) != 0)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Error while writing audio frame"));
	}
	 
	void display(const frame_ptr& frame)
	{
		write_video_frame(frame);
		write_audio_frame(frame);
	}
	
	// Audio
	AVStream* audio_st_;
	float t_;
	float tincr_;
	float tincr2_;
	int16_t* empty_audio_;
	uint8_t* audio_outbuf_;
	int audio_outbuf_size_;
	int audio_input_frame_size_;

	// Video
	AVStream* video_st_;
	AVFrame* picture_;
	uint8_t* video_outbuf_;
	int video_outbuf_size_;
	SwsContext* img_convert_ctx_;
	
	AVOutputFormat* fmt_;
	AVFormatContext* oc_;
	caspar::frame_format_desc format_desc_;
};

file_frame_consumer::file_frame_consumer(const std::string& filename, const caspar::frame_format_desc& format_desc) : impl_(new implementation(filename, format_desc)){}
const caspar::frame_format_desc& file_frame_consumer::get_frame_format_desc() const{return impl_->format_desc_;}
void file_frame_consumer::display(const frame_ptr& frame){impl_->display(frame);}
}}
