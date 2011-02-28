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
 
#include "../../StdAfx.h"

#include "ffmpeg_consumer.h"

#include <mixer/frame/read_frame.h>

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

namespace caspar { namespace core {
	
struct ffmpeg_consumer::implementation : boost::noncopyable
{		
	printer parent_printer_;
	const std::string filename_;

	// Audio
	AVStream* audio_st_;
	std::vector<unsigned char, tbb::cache_aligned_allocator<unsigned char>> audio_outbuf_;

	// Video
	AVStream* video_st_;
	std::vector<uint8_t, tbb::cache_aligned_allocator<unsigned char>> picture_buf_;

	std::vector<unsigned char, tbb::cache_aligned_allocator<unsigned char>> video_outbuf_;
	SwsContext* img_convert_ctx_;
	
	AVOutputFormat* fmt_;
	std::shared_ptr<AVFormatContext> oc_;
	video_format_desc format_desc_;

	std::vector<short, tbb::cache_aligned_allocator<short>> audio_input_buffer_;

	boost::unique_future<void> active_;

	executor executor_;
public:
	implementation(const std::string& filename)
		: filename_(filename)
		, audio_st_(nullptr)
		, video_st_(nullptr)
		, fmt_(nullptr)
		, img_convert_ctx_(nullptr)
		, video_outbuf_(1920*1080*4)
		, audio_outbuf_(48000)
	{}

	~implementation()
	{    
		executor_.invoke([]{});
		executor_.stop();

		av_write_trailer(oc_.get());

		// Close each codec.
		if (video_st_)		
			avcodec_close(video_st_->codec);
		
		if (audio_st_)
			avcodec_close(audio_st_->codec);
				
		// Free the streams.
		for(size_t i = 0; i < oc_->nb_streams; ++i) 
		{
			av_freep(&oc_->streams[i]->codec);
			av_freep(&oc_->streams[i]);
		}

		if (!(fmt_->flags & AVFMT_NOFILE)) 
			url_fclose(oc_->pb); // Close the output ffmpeg.
	}

	void initialize(const video_format_desc& format_desc)
	{
		format_desc_ = format_desc;
		executor_.start();
		active_ = executor_.begin_invoke([]{});

		fmt_ = av_guess_format(nullptr, filename_.c_str(), nullptr);
		if (!fmt_) 
		{
			CASPAR_LOG(warning) << "Could not deduce output format from ffmpeg extension: using MPEG.";
			fmt_ = av_guess_format("mpeg", nullptr, nullptr);
		}
		if (!fmt_)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Could not find suitable output format"));
		
		oc_.reset(avformat_alloc_context(), av_free);
		if (!oc_)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Memory error"));
		std::copy_n(filename_.c_str(), filename_.size(), oc_->filename);

		oc_->oformat = fmt_;
		// To avoid mpeg buffer underflow (http://www.mail-archive.com/libav-user@mplayerhq.hu/msg00194.html)
		oc_->preload = static_cast<int>(0.5*AV_TIME_BASE);
		oc_->max_delay = static_cast<int>(0.7*AV_TIME_BASE);
			
		//  Add the audio and video streams using the default format codecs	and initialize the codecs .
		if (fmt_->video_codec != CODEC_ID_NONE)		
			video_st_ = add_video_stream(fmt_->video_codec);
		
		if (fmt_->audio_codec != CODEC_ID_NONE) 
			audio_st_ = add_audio_stream(fmt_->audio_codec);	

		// Set the output parameters (must be done even if no parameters).		
		int errn = 0;
		if ((errn = -av_set_parameters(oc_.get(), nullptr)) > 0)
			BOOST_THROW_EXCEPTION(
				file_read_error() << 
				msg_info("Invalid output format parameters") <<
				boost::errinfo_api_function("avcodec_open") <<
				boost::errinfo_errno(errn) <<
				boost::errinfo_file_name(filename_));
		
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
		if (!(fmt_->flags & AVFMT_NOFILE)) 
		{
			int errn = 0;
			if ((errn = -url_fopen(&oc_->pb, filename_.c_str(), URL_WRONLY)) > 0) 
				BOOST_THROW_EXCEPTION(
					file_not_found() << 
					msg_info("Could not open file") <<
					boost::errinfo_api_function("url_fopen") <<
					boost::errinfo_errno(errn) <<
					boost::errinfo_file_name(filename_));
		}
		
		av_write_header(oc_.get()); // write the stream header, if any 
	}
	
	void set_parent_printer(const printer& parent_printer) 
	{
		parent_printer_ = parent_printer;
	}

	AVStream* add_video_stream(enum CodecID codec_id)
	{ 
		auto st = av_new_stream(oc_.get(), 0);
		if (!st) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Could not alloc stream"));
 
		auto c = st->codec;
		c->codec_id = codec_id;
		c->codec_type = AVMEDIA_TYPE_VIDEO;
 
		// Put sample parameters.
		c->bit_rate = static_cast<int>(static_cast<double>(format_desc_.size)*format_desc_.fps*0.1326);
		c->width = format_desc_.width;
		c->height = format_desc_.height;
		c->time_base.den = static_cast<int>(format_desc_.fps);
		c->time_base.num = 1;
		c->pix_fmt = c->pix_fmt == -1 ? PIX_FMT_YUV420P : c->pix_fmt;

		// Some formats want stream headers to be separate.
		if(oc_->oformat->flags & AVFMT_GLOBALHEADER)
			c->flags |= CODEC_FLAG_GLOBAL_HEADER;
 
		return st;
	}
	 
	void open_video(AVStream* st)
	{ 
		auto c = st->codec;
 
		auto codec = avcodec_find_encoder(c->codec_id);
		if (!codec)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("codec not found"));
		
		int errn = 0;
		if ((errn = -avcodec_open(c, codec)) > 0)
			BOOST_THROW_EXCEPTION(
				file_read_error() << 
				msg_info("Could not open video codec.") <<
				boost::errinfo_api_function("avcodec_open") <<
				boost::errinfo_errno(errn) <<
				boost::errinfo_file_name(filename_));		 
	}
  
	void encode_video_frame(const safe_ptr<const read_frame>& frame)
	{ 
		AVCodecContext* c = video_st_->codec;
 
		if (img_convert_ctx_ == nullptr) 
		{
			img_convert_ctx_ = sws_getContext(format_desc_.width, format_desc_.height, PIX_FMT_BGRA, c->width, c->height, c->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr);
			if (img_convert_ctx_ == nullptr) 
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Cannot initialize the conversion context"));
		}

		std::shared_ptr<AVFrame> av_frame(avcodec_alloc_frame(), av_free);
		avpicture_fill(reinterpret_cast<AVPicture*>(av_frame.get()), const_cast<uint8_t*>(frame->image_data().begin()), PIX_FMT_BGRA, format_desc_.width, format_desc_.height);
				
		AVFrame local_av_frame;
		avcodec_get_frame_defaults(&local_av_frame);
		picture_buf_.resize(avpicture_get_size(c->pix_fmt, format_desc_.width, format_desc_.height));
		avpicture_fill(reinterpret_cast<AVPicture*>(&local_av_frame), picture_buf_.data(), c->pix_fmt, format_desc_.width, format_desc_.height);

		sws_scale(img_convert_ctx_, av_frame->data, av_frame->linesize, 0, c->height, local_av_frame.data, local_av_frame.linesize);
				
		int ret = avcodec_encode_video(c, video_outbuf_.data(), video_outbuf_.size(), &local_av_frame);
				
		int errn = -ret;
		if (errn > 0) 
			BOOST_THROW_EXCEPTION(
				invalid_operation() << 
				msg_info("Could not encode video frame.") <<
				boost::errinfo_api_function("avcodec_encode_video") <<
				boost::errinfo_errno(errn) <<
				boost::errinfo_file_name(filename_));

		auto out_size = ret;
		AVPacket pkt;
		av_init_packet(&pkt);
		pkt.size = out_size;

		// If zero size, it means the image was buffered.
		if (out_size > 0) 
		{ 
			if (c->coded_frame->pts != AV_NOPTS_VALUE)
				pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, video_st_->time_base);
			if(c->coded_frame->key_frame)
				pkt.flags |= AV_PKT_FLAG_KEY;

			pkt.stream_index = video_st_->index;
			pkt.data = video_outbuf_.data();

			if (av_interleaved_write_frame(oc_.get(), &pkt) != 0)
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Error while writing video frame"));
		} 		
	}

	AVStream* add_audio_stream(enum CodecID codec_id)
	{
		audio_st_ = av_new_stream(oc_.get(), 1);
		if (!audio_st_)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Could not alloc stream"));

		auto c = audio_st_->codec;
		c->codec_id = codec_id;
		c->codec_type = AVMEDIA_TYPE_AUDIO;

		// Put sample parameters.
		c->bit_rate = 192000;
		c->sample_rate = 48000;
		c->channels = 2;

		// Some formats want stream headers to be separate.
		if(oc_->oformat->flags & AVFMT_GLOBALHEADER)
			c->flags |= CODEC_FLAG_GLOBAL_HEADER;

		return audio_st_;
	}

	void open_audio(AVStream* st)
	{
		auto c = st->codec;

		// Find the audio encoder.
		auto codec = avcodec_find_encoder(c->codec_id);
		if (!codec) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("codec not found"));
		
		// Open it.
		int errn = 0;
		if ((errn = -avcodec_open(c, codec)) > 0)
			BOOST_THROW_EXCEPTION(
				file_read_error() << 
				msg_info("Could not open audio codec") <<
				boost::errinfo_api_function("avcodec_open") <<
				boost::errinfo_errno(errn) <<
				boost::errinfo_file_name(filename_));
	}
	
	void encode_audio_frame(const safe_ptr<const read_frame>& frame)
	{	
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
		
		int ret = avcodec_encode_audio(c, audio_outbuf_.data(), audio_outbuf_.size(), audio_input_buffer_.data());
		
		int errn = -ret;
		if (errn > 0) 
			BOOST_THROW_EXCEPTION(
				invalid_operation() << 
				msg_info("Could not encode audio samples.") <<
				boost::errinfo_api_function("avcodec_encode_audio") <<
				boost::errinfo_errno(errn) <<
				boost::errinfo_file_name(filename_));

		pkt.size = ret;
		audio_input_buffer_ = std::vector<short, tbb::cache_aligned_allocator<short>>(audio_input_buffer_.begin() + frame_bytes/2, audio_input_buffer_.end());

		if (c->coded_frame && c->coded_frame->pts != AV_NOPTS_VALUE)
			pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, audio_st_->time_base);
		pkt.flags |= AV_PKT_FLAG_KEY;
		pkt.stream_index = audio_st_->index;
		pkt.data = audio_outbuf_.data();
		
		if (av_interleaved_write_frame(oc_.get(), &pkt) != 0)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Error while writing audio frame"));

		return true;
	}
	 
	void send(const safe_ptr<const read_frame>& frame)
	{
		active_.get();
		active_ = executor_.begin_invoke([=]
		{				
			auto my_frame = frame;
			encode_video_frame(my_frame);
			encode_audio_frame(my_frame);
		});
	}

	size_t buffer_depth() const { return 1; }
};

ffmpeg_consumer::ffmpeg_consumer(const std::wstring& filename) : impl_(new implementation(narrow(filename))){}
ffmpeg_consumer::ffmpeg_consumer(ffmpeg_consumer&& other) : impl_(std::move(other.impl_)){}
void ffmpeg_consumer::send(const safe_ptr<const read_frame>& frame){impl_->send(frame);}
size_t ffmpeg_consumer::buffer_depth() const{return impl_->buffer_depth();}
void ffmpeg_consumer::initialize(const video_format_desc& format_desc) {impl_->initialize(format_desc);}
void ffmpeg_consumer::set_parent_printer(const printer& parent_printer){impl_->set_parent_printer(parent_printer);}

safe_ptr<frame_consumer> create_ffmpeg_consumer(const std::vector<std::wstring>& params)
{
	if(params.size() < 2 || params[0] != L"FILE")
		return frame_consumer::empty();
	
	// TODO: Ask stakeholders about case where file already exists.
	boost::filesystem::remove(boost::filesystem::wpath(env::media_folder() + params[2])); // Delete the file if it exists
	return make_safe<ffmpeg_consumer>(env::media_folder() + params[2]);
}

}}
