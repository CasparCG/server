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

#include "../../stdafx.h"

#include "util.h"

#include "flv.h"

#include "../tbb_avcodec.h"
#include "../../ffmpeg_error.h"

#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_queue.h>

#include <core/frame/frame_transform.h>
#include <core/frame/frame_factory.h>
#include <core/frame/frame.h>
#include <core/producer/frame_producer.h>

#include <common/except.h>
#include <common/array.h>

#include <tbb/parallel_for.h>

#include <common/assert.h>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include <asmlib.h>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#include <libswscale/swscale.h>
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar { namespace ffmpeg {
		
core::field_mode get_mode(const AVFrame& frame)
{
	if(!frame.interlaced_frame)
		return core::field_mode::progressive;

	return frame.top_field_first ? core::field_mode::upper : core::field_mode::lower;
}

core::pixel_format get_pixel_format(PixelFormat pix_fmt)
{
	switch(pix_fmt)
	{
	case PIX_FMT_GRAY8:			return core::pixel_format::gray;
	case PIX_FMT_RGB24:			return core::pixel_format::rgb;
	case PIX_FMT_BGR24:			return core::pixel_format::bgr;
	case PIX_FMT_BGRA:			return core::pixel_format::bgra;
	case PIX_FMT_ARGB:			return core::pixel_format::argb;
	case PIX_FMT_RGBA:			return core::pixel_format::rgba;
	case PIX_FMT_ABGR:			return core::pixel_format::abgr;
	case PIX_FMT_YUV444P:		return core::pixel_format::ycbcr;
	case PIX_FMT_YUV422P:		return core::pixel_format::ycbcr;
	case PIX_FMT_YUV420P:		return core::pixel_format::ycbcr;
	case PIX_FMT_YUV411P:		return core::pixel_format::ycbcr;
	case PIX_FMT_YUV410P:		return core::pixel_format::ycbcr;
	case PIX_FMT_YUVA420P:		return core::pixel_format::ycbcra;
	default:					return core::pixel_format::invalid;
	}
}

core::pixel_format_desc pixel_format_desc(PixelFormat pix_fmt, int width, int height)
{
	// Get linesizes
	AVPicture dummy_pict;	
	avpicture_fill(&dummy_pict, nullptr, pix_fmt, width, height);

	core::pixel_format_desc desc = get_pixel_format(pix_fmt);
		
	switch(desc.format.value())
	{
	case core::pixel_format::gray:
	case core::pixel_format::luma:
		{
			desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0], height, 1));						
			return desc;
		}
	case core::pixel_format::bgr:
	case core::pixel_format::rgb:
		{
			desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0]/3, height, 3));						
			return desc;
		}
	case core::pixel_format::bgra:
	case core::pixel_format::argb:
	case core::pixel_format::rgba:
	case core::pixel_format::abgr:
		{
			desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0]/4, height, 4));						
			return desc;
		}
	case core::pixel_format::ycbcr:
	case core::pixel_format::ycbcra:
		{		
			// Find chroma height
			int size2 = static_cast<int>(dummy_pict.data[2] - dummy_pict.data[1]);
			int h2 = size2/dummy_pict.linesize[1];			

			desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0], height, 1));
			desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[1], h2, 1));
			desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[2], h2, 1));

			if(desc.format == core::pixel_format::ycbcra)						
				desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[3], height, 1));	
			return desc;
		}		
	default:		
		desc.format = core::pixel_format::invalid;
		return desc;
	}
}

core::mutable_frame make_frame(const void* tag, const spl::shared_ptr<AVFrame>& decoded_frame, double fps, core::frame_factory& frame_factory)
{			
	static tbb::concurrent_unordered_map<int64_t, tbb::concurrent_queue<std::shared_ptr<SwsContext>>> sws_contvalid_exts_;
	
	if(decoded_frame->width < 1 || decoded_frame->height < 1)
		return frame_factory.create_frame(tag, core::pixel_format_desc(core::pixel_format::invalid));

	const auto width  = decoded_frame->width;
	const auto height = decoded_frame->height;
	auto desc		  = pixel_format_desc(static_cast<PixelFormat>(decoded_frame->format), width, height);
		
	if(desc.format == core::pixel_format::invalid)
	{
		auto pix_fmt = static_cast<PixelFormat>(decoded_frame->format);
		auto target_pix_fmt = PIX_FMT_BGRA;

		if(pix_fmt == PIX_FMT_UYVY422)
			target_pix_fmt = PIX_FMT_YUV422P;
		else if(pix_fmt == PIX_FMT_YUYV422)
			target_pix_fmt = PIX_FMT_YUV422P;
		else if(pix_fmt == PIX_FMT_UYYVYY411)
			target_pix_fmt = PIX_FMT_YUV411P;
		else if(pix_fmt == PIX_FMT_YUV420P10)
			target_pix_fmt = PIX_FMT_YUV420P;
		else if(pix_fmt == PIX_FMT_YUV422P10)
			target_pix_fmt = PIX_FMT_YUV422P;
		else if(pix_fmt == PIX_FMT_YUV444P10)
			target_pix_fmt = PIX_FMT_YUV444P;
		
		auto target_desc = pixel_format_desc(target_pix_fmt, width, height);

		auto write = frame_factory.create_frame(tag, target_desc);

		std::shared_ptr<SwsContext> sws_context;

		//CASPAR_LOG(warning) << "Hardware accelerated color transform not supported.";
		
		int64_t key = ((static_cast<int64_t>(width)			 << 32) & 0xFFFF00000000) | 
					  ((static_cast<int64_t>(height)		 << 16) & 0xFFFF0000) | 
					  ((static_cast<int64_t>(pix_fmt)		 <<  8) & 0xFF00) | 
					  ((static_cast<int64_t>(target_pix_fmt) <<  0) & 0xFF);
			
		auto& pool = sws_contvalid_exts_[key];
						
		if(!pool.try_pop(sws_context))
		{
			double param;
			sws_context.reset(sws_getContext(width, height, pix_fmt, width, height, target_pix_fmt, SWS_BILINEAR, nullptr, nullptr, &param), sws_freeContext);
		}
			
		if(!sws_context)
		{
			CASPAR_THROW_EXCEPTION(operation_failed() << msg_info("Could not create software scaling context.") << 
									boost::errinfo_api_function("sws_getContext"));
		}	
		
		spl::shared_ptr<AVFrame> av_frame(avcodec_alloc_frame(), av_free);	
		avcodec_get_frame_defaults(av_frame.get());			
		if(target_pix_fmt == PIX_FMT_BGRA)
		{
			auto size = avpicture_fill(reinterpret_cast<AVPicture*>(av_frame.get()), write.image_data(0).begin(), PIX_FMT_BGRA, width, height);
			CASPAR_VERIFY(size == write.image_data(0).size()); 
		}
		else
		{
			av_frame->width	 = width;
			av_frame->height = height;
			for(int n = 0; n < target_desc.planes.size(); ++n)
			{
				av_frame->data[n]		= write.image_data(n).begin();
				av_frame->linesize[n]	= target_desc.planes[n].linesize;
			}
		}

		sws_scale(sws_context.get(), decoded_frame->data, decoded_frame->linesize, 0, height, av_frame->data, av_frame->linesize);	
		pool.push(sws_context);	

		return std::move(write);
	}
	else
	{
		auto write = frame_factory.create_frame(tag, desc);
		
		for(int n = 0; n < static_cast<int>(desc.planes.size()); ++n)
		{
			auto plane            = desc.planes[n];
			auto result           = write.image_data(n).begin();
			auto decoded          = decoded_frame->data[n];
			auto decoded_linesize = decoded_frame->linesize[n];
			
			CASPAR_ASSERT(decoded);
			CASPAR_ASSERT(write.image_data(n).begin());

			// Copy line by line since ffmpeg sometimes pads each line.
			tbb::affinity_partitioner ap;
			tbb::parallel_for(tbb::blocked_range<int>(0, desc.planes[n].height), [&](const tbb::blocked_range<int>& r)
			{
				for(int y = r.begin(); y != r.end(); ++y)
					A_memcpy(result + y*plane.linesize, decoded + y*decoded_linesize, plane.linesize);
			}, ap);
		}
	
		return std::move(write);
	}
}

spl::shared_ptr<AVFrame> make_av_frame(core::mutable_frame& frame)
{
	std::array<uint8_t*, 4> data = {};
	for(int n = 0; n < frame.pixel_format_desc().planes.size(); ++n)
		data[n] = frame.image_data(n).begin();

	return make_av_frame(data, frame.pixel_format_desc());
}

spl::shared_ptr<AVFrame> make_av_frame(std::array<uint8_t*, 4> data, const core::pixel_format_desc& pix_desc)
{
	spl::shared_ptr<AVFrame> av_frame(avcodec_alloc_frame(), av_free);	
	avcodec_get_frame_defaults(av_frame.get());
	
	auto planes		 = pix_desc.planes;
	auto format		 = pix_desc.format.value();

	av_frame->width  = planes[0].width;
	av_frame->height = planes[0].height;
	for(int n = 0; n < planes.size(); ++n)	
	{
		av_frame->data[n]	  = data[n];
		av_frame->linesize[n] = planes[n].linesize;	
	}

	switch(format)
	{
	case core::pixel_format::rgb:
		av_frame->format = PIX_FMT_RGB24;
		break;
	case core::pixel_format::bgr:
		av_frame->format = PIX_FMT_BGR24;
		break;
	case core::pixel_format::rgba:
		av_frame->format = PIX_FMT_RGBA; 
		break;
	case core::pixel_format::argb:
		av_frame->format = PIX_FMT_ARGB; 
		break;
	case core::pixel_format::bgra:
		av_frame->format = PIX_FMT_BGRA; 
		break;
	case core::pixel_format::abgr:
		av_frame->format = PIX_FMT_ABGR; 
		break;
	case core::pixel_format::gray:
		av_frame->format = PIX_FMT_GRAY8; 
		break;
	case core::pixel_format::ycbcr:
	{
		int y_w = planes[0].width;
		int y_h = planes[0].height;
		int c_w = planes[1].width;
		int c_h = planes[1].height;

		if(c_h == y_h && c_w == y_w)
			av_frame->format = PIX_FMT_YUV444P;
		else if(c_h == y_h && c_w*2 == y_w)
			av_frame->format = PIX_FMT_YUV422P;
		else if(c_h == y_h && c_w*4 == y_w)
			av_frame->format = PIX_FMT_YUV411P;
		else if(c_h*2 == y_h && c_w*2 == y_w)
			av_frame->format = PIX_FMT_YUV420P;
		else if(c_h*2 == y_h && c_w*4 == y_w)
			av_frame->format = PIX_FMT_YUV410P;

		break;
	}
	case core::pixel_format::ycbcra:
		av_frame->format = PIX_FMT_YUVA420P;
		break;
	}
	return av_frame;
}

bool is_sane_fps(AVRational time_base)
{
	double fps = static_cast<double>(time_base.den) / static_cast<double>(time_base.num);
	return fps > 20.0 && fps < 65.0;
}

AVRational fix_time_base(AVRational time_base)
{
	if(time_base.num == 1)
		time_base.num = static_cast<int>(std::pow(10.0, static_cast<int>(std::log10(static_cast<float>(time_base.den)))-1));	
			
	if(!is_sane_fps(time_base))
	{
		auto tmp = time_base;
		tmp.den /= 2;
		if(is_sane_fps(tmp))
			time_base = tmp;
	}

	return time_base;
}

double read_fps(AVFormatContext& context, double fail_value)
{						
	auto video_index = av_find_best_stream(&context, AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0);
	auto audio_index = av_find_best_stream(&context, AVMEDIA_TYPE_AUDIO, -1, -1, 0, 0);
	
	if(video_index > -1)
	{
		const auto video_context = context.streams[video_index]->codec;
		const auto video_stream  = context.streams[video_index];
						
		AVRational time_base = video_context->time_base;

		if(boost::filesystem::path(context.filename).extension().string() == ".flv")
		{
			try
			{
				auto meta = read_flv_meta_info(context.filename);
				return boost::lexical_cast<double>(meta["framerate"]);
			}
			catch(...)
			{
				return 0.0;
			}
		}
		else
		{
			time_base.num *= video_context->ticks_per_frame;

			if(!is_sane_fps(time_base))
			{			
				time_base = fix_time_base(time_base);

				if(!is_sane_fps(time_base) && audio_index > -1)
				{
					auto& audio_context = *context.streams[audio_index]->codec;
					auto& audio_stream  = *context.streams[audio_index];

					double duration_sec = audio_stream.duration / static_cast<double>(audio_context.sample_rate);
								
					time_base.num = static_cast<int>(duration_sec*100000.0);
					time_base.den = static_cast<int>(video_stream->nb_frames*100000);
				}
			}
		}
		
		double fps = static_cast<double>(time_base.den) / static_cast<double>(time_base.num);

		double closest_fps = 0.0;
		for(int n = 0; n < core::video_format::count; ++n)
		{
			auto format = core::video_format_desc(core::video_format(n));

			double diff1 = std::abs(format.fps - fps);
			double diff2 = std::abs(closest_fps - fps);

			if(diff1 < diff2)
				closest_fps = format.fps;
		}
	
		return closest_fps;
	}

	return fail_value;	
}

void fix_meta_data(AVFormatContext& context)
{
	auto video_index = av_find_best_stream(&context, AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0);

	if(video_index > -1)
	{
		auto video_stream   = context.streams[video_index];
		auto video_context  = context.streams[video_index]->codec;
						
		if(boost::filesystem::path(context.filename).extension().string() == ".flv")
		{
			try
			{
				auto meta = read_flv_meta_info(context.filename);
				double fps = boost::lexical_cast<double>(meta["framerate"]);
				video_stream->nb_frames = static_cast<int64_t>(boost::lexical_cast<double>(meta["duration"])*fps);
			}
			catch(...){}
		}
		else
		{
			auto stream_time = video_stream->time_base;
			auto duration	 = video_stream->duration;
			auto codec_time  = video_context->time_base;
			auto ticks		 = video_context->ticks_per_frame;

			if(video_stream->nb_frames == 0)
				video_stream->nb_frames = (duration*stream_time.num*codec_time.den)/(stream_time.den*codec_time.num*ticks);	
		}
	}
}

spl::shared_ptr<AVPacket> create_packet()
{
	spl::shared_ptr<AVPacket> packet(new AVPacket(), [](AVPacket* p)
	{
		av_free_packet(p);
		delete p;
	});
	
	av_init_packet(packet.get());
	return packet;
}

spl::shared_ptr<AVFrame> create_frame()
{	
	spl::shared_ptr<AVFrame> frame(avcodec_alloc_frame(), av_free);
	avcodec_get_frame_defaults(frame.get());
	return frame;
}

spl::shared_ptr<AVCodecContext> open_codec(AVFormatContext& context, enum AVMediaType type, int& index)
{	
	AVCodec* decoder;
	index = THROW_ON_ERROR2(av_find_best_stream(&context, type, -1, -1, &decoder, 0), "");
	//if(strcmp(decoder->name, "prores") == 0 && decoder->next && strcmp(decoder->next->name, "prores_lgpl") == 0)
	//	decoder = decoder->next;

	THROW_ON_ERROR2(tbb_avcodec_open(context.streams[index]->codec, decoder), "");
	return spl::shared_ptr<AVCodecContext>(context.streams[index]->codec, tbb_avcodec_close);
}

spl::shared_ptr<AVFormatContext> open_input(const std::wstring& filename)
{
	AVFormatContext* weak_context = nullptr;
	THROW_ON_ERROR2(avformat_open_input(&weak_context, u8(filename).c_str(), nullptr, nullptr), filename);
	spl::shared_ptr<AVFormatContext> context(weak_context, av_close_input_file);			
	THROW_ON_ERROR2(avformat_find_stream_info(weak_context, nullptr), filename);
	fix_meta_data(*context);
	return context;
}

std::wstring print_mode(int width, int height, double fps, bool interlaced)
{
	std::wostringstream fps_ss;
	fps_ss << std::fixed << std::setprecision(2) << (!interlaced ? fps : 2.0 * fps);

	return boost::lexical_cast<std::wstring>(width) + L"x" + boost::lexical_cast<std::wstring>(height) + (!interlaced ? L"p" : L"i") + fps_ss.str();
}

bool is_valid_file(const std::wstring filename)
{				
	static const std::vector<std::wstring> invalid_exts = boost::assign::list_of(L".png")(L".tga")(L".bmp")(L".jpg")(L".jpeg")(L".gif")(L".tiff")(L".tif")(L".jp2")(L".jpx")(L".j2k")(L".j2c")(L".swf")(L".ct");
	static std::vector<std::wstring>	   valid_exts   = boost::assign::list_of(L".m2t")(L".mov")(L".mp4")(L".dv")(L".flv")(L".mpg")(L".wav")(L".mp3")(L".dnxhd")(L".h264")(L".prores");

	auto ext = boost::to_lower_copy(boost::filesystem::path(filename).extension().wstring());
		
	if(std::find(valid_exts.begin(), valid_exts.end(), ext) != valid_exts.end())
		return true;	
	
	if(std::find(invalid_exts.begin(), invalid_exts.end(), ext) != invalid_exts.end())
		return false;	

	auto u8filename = u8(filename);
	
	int score = 0;
	AVProbeData pb = {};
	pb.filename = u8filename.c_str();

	if(av_probe_input_format2(&pb, false, &score) != nullptr)
		return true;

	std::ifstream file(filename);

	std::vector<unsigned char> buf;
	for(auto file_it = std::istreambuf_iterator<char>(file); file_it != std::istreambuf_iterator<char>() && buf.size() < 1024; ++file_it)
		buf.push_back(*file_it);

	if(buf.empty())
		return nullptr;

	pb.buf		= buf.data();
	pb.buf_size = static_cast<int>(buf.size());

	return av_probe_input_format2(&pb, true, &score) != nullptr;
}

std::wstring probe_stem(const std::wstring stem)
{
	auto stem2 = boost::filesystem::path(stem);
	auto dir = stem2.parent_path();
	for(auto it = boost::filesystem::directory_iterator(dir); it != boost::filesystem::directory_iterator(); ++it)
	{
		if(boost::iequals(it->path().stem().wstring(), stem2.filename().wstring()) && is_valid_file(it->path().wstring()))
			return it->path().wstring();
	}
	return L"";
}
//
//void av_dup_frame(AVFrame* frame)
//{
//	AVFrame* new_frame = avcodec_alloc_frame();
//
//
//	const uint8_t *src_data[4] = {0};
//	memcpy(const_cast<uint8_t**>(&src_data[0]), frame->data, 4);
//	const int src_linesizes[4] = {0};
//	memcpy(const_cast<int*>(&src_linesizes[0]), frame->linesize, 4);
//
//	av_image_alloc(new_frame->data, new_frame->linesize, new_frame->width, new_frame->height, frame->format, 16);
//
//	av_image_copy(new_frame->data, new_frame->linesize, src_data, src_linesizes, frame->format, new_frame->width, new_frame->height);
//
//	frame =
//}

}}