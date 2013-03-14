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

#include <core/producer/frame/frame_transform.h>
#include <core/producer/frame/frame_factory.h>
#include <core/producer/frame_producer.h>
#include <core/mixer/write_frame.h>

#include <common/exception/exceptions.h>
#include <common/utility/assert.h>
#include <common/memory/memcpy.h>

#include <tbb/parallel_for.h>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

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
		
std::shared_ptr<core::audio_buffer> flush_audio()
{
	static std::shared_ptr<core::audio_buffer> audio(new core::audio_buffer());
	return audio;
}

std::shared_ptr<core::audio_buffer> empty_audio()
{
	static std::shared_ptr<core::audio_buffer> audio(new core::audio_buffer());
	return audio;
}

std::shared_ptr<AVFrame>			flush_video()
{
	static std::shared_ptr<AVFrame> video(avcodec_alloc_frame(), av_free);
	return video;
}

std::shared_ptr<AVFrame>			empty_video()
{
	static std::shared_ptr<AVFrame> video(avcodec_alloc_frame(), av_free);
	return video;
}

core::field_mode::type get_mode(const AVFrame& frame)
{
	if(!frame.interlaced_frame)
		return core::field_mode::progressive;

	return frame.top_field_first ? core::field_mode::upper : core::field_mode::lower;
}

core::pixel_format::type get_pixel_format(PixelFormat pix_fmt)
{
	switch(pix_fmt)
	{
	case CASPAR_PIX_FMT_LUMA:	return core::pixel_format::luma;
	case PIX_FMT_GRAY8:			return core::pixel_format::gray;
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

core::pixel_format_desc get_pixel_format_desc(PixelFormat pix_fmt, size_t width, size_t height)
{
	// Get linesizes
	AVPicture dummy_pict;	
	avpicture_fill(&dummy_pict, nullptr, pix_fmt == CASPAR_PIX_FMT_LUMA ? PIX_FMT_GRAY8 : pix_fmt, width, height);

	core::pixel_format_desc desc;
	desc.pix_fmt = get_pixel_format(pix_fmt);
		
	switch(desc.pix_fmt)
	{
	case core::pixel_format::gray:
	case core::pixel_format::luma:
		{
			desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0], height, 1));						
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
			size_t size2 = dummy_pict.data[2] - dummy_pict.data[1];
			size_t h2 = size2/dummy_pict.linesize[1];			

			desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0], height, 1));
			desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[1], h2, 1));
			desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[2], h2, 1));

			if(desc.pix_fmt == core::pixel_format::ycbcra)						
				desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[3], height, 1));	
			return desc;
		}		
	default:		
		desc.pix_fmt = core::pixel_format::invalid;
		return desc;
	}
}

int make_alpha_format(int format)
{
	switch(get_pixel_format(static_cast<PixelFormat>(format)))
	{
	case core::pixel_format::ycbcr:
	case core::pixel_format::ycbcra:
		return CASPAR_PIX_FMT_LUMA;
	default:
		return format;
	}
}

safe_ptr<core::write_frame> make_write_frame(const void* tag, const safe_ptr<AVFrame>& decoded_frame, const safe_ptr<core::frame_factory>& frame_factory, int hints)
{			
	static tbb::concurrent_unordered_map<int64_t, tbb::concurrent_queue<std::shared_ptr<SwsContext>>> sws_contexts_;
	
	if(decoded_frame->width < 1 || decoded_frame->height < 1)
		return make_safe<core::write_frame>(tag);

	const auto width  = decoded_frame->width;
	const auto height = decoded_frame->height;
	auto desc		  = get_pixel_format_desc(static_cast<PixelFormat>(decoded_frame->format), width, height);
	
	if(hints & core::frame_producer::ALPHA_HINT)
		desc = get_pixel_format_desc(static_cast<PixelFormat>(make_alpha_format(decoded_frame->format)), width, height);

	std::shared_ptr<core::write_frame> write;

	if(desc.pix_fmt == core::pixel_format::invalid)
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
		
		auto target_desc = get_pixel_format_desc(target_pix_fmt, width, height);

		write = frame_factory->create_frame(tag, target_desc);
		write->set_type(get_mode(*decoded_frame));

		std::shared_ptr<SwsContext> sws_context;

		//CASPAR_LOG(warning) << "Hardware accelerated color transform not supported.";
		
		int64_t key = ((static_cast<int64_t>(width)			 << 32) & 0xFFFF00000000) | 
					  ((static_cast<int64_t>(height)		 << 16) & 0xFFFF0000) | 
					  ((static_cast<int64_t>(pix_fmt)		 <<  8) & 0xFF00) | 
					  ((static_cast<int64_t>(target_pix_fmt) <<  0) & 0xFF);
			
		auto& pool = sws_contexts_[key];
						
		if(!pool.try_pop(sws_context))
		{
			double param;
			sws_context.reset(sws_getContext(width, height, pix_fmt, width, height, target_pix_fmt, SWS_BILINEAR, nullptr, nullptr, &param), sws_freeContext);
		}
			
		if(!sws_context)
		{
			BOOST_THROW_EXCEPTION(operation_failed() << msg_info("Could not create software scaling context.") << 
									boost::errinfo_api_function("sws_getContext"));
		}	
		
		safe_ptr<AVFrame> av_frame(avcodec_alloc_frame(), av_free);	
		avcodec_get_frame_defaults(av_frame.get());			
		if(target_pix_fmt == PIX_FMT_BGRA)
		{
			auto size = avpicture_fill(reinterpret_cast<AVPicture*>(av_frame.get()), write->image_data().begin(), PIX_FMT_BGRA, width, height);
			CASPAR_VERIFY(size == write->image_data().size()); 
		}
		else
		{
			av_frame->width	 = width;
			av_frame->height = height;
			for(size_t n = 0; n < target_desc.planes.size(); ++n)
			{
				av_frame->data[n]		= write->image_data(n).begin();
				av_frame->linesize[n]	= target_desc.planes[n].linesize;
			}
		}

		sws_scale(sws_context.get(), decoded_frame->data, decoded_frame->linesize, 0, height, av_frame->data, av_frame->linesize);	
		pool.push(sws_context);

		write->commit();		
	}
	else
	{
		write = frame_factory->create_frame(tag, desc);
		write->set_type(get_mode(*decoded_frame));

		for(int n = 0; n < static_cast<int>(desc.planes.size()); ++n)
		{
			auto plane            = desc.planes[n];
			auto result           = write->image_data(n).begin();
			auto decoded          = decoded_frame->data[n];
			auto decoded_linesize = decoded_frame->linesize[n];
			
			CASPAR_ASSERT(decoded);
			CASPAR_ASSERT(write->image_data(n).begin());

			if(decoded_linesize != static_cast<int>(plane.width))
			{
				// Copy line by line since ffmpeg sometimes pads each line.
				tbb::parallel_for<size_t>(0, desc.planes[n].height, [&](size_t y)
				{
					fast_memcpy(result + y*plane.linesize, decoded + y*decoded_linesize, plane.linesize);
				});
			}
			else
			{
				fast_memcpy(result, decoded, plane.size);
			}

			write->commit(n);
		}
	}

	if(decoded_frame->height == 480) // NTSC DV
	{
		write->get_frame_transform().fill_translation[1] += 2.0/static_cast<double>(frame_factory->get_video_format_desc().height);
		write->get_frame_transform().fill_scale[1] = 1.0 - 6.0*1.0/static_cast<double>(frame_factory->get_video_format_desc().height);
	}
	
	// Fix field-order if needed
	if(write->get_type() == core::field_mode::lower && frame_factory->get_video_format_desc().field_mode == core::field_mode::upper)
		write->get_frame_transform().fill_translation[1] += 1.0/static_cast<double>(frame_factory->get_video_format_desc().height);
	else if(write->get_type() == core::field_mode::upper && frame_factory->get_video_format_desc().field_mode == core::field_mode::lower)
		write->get_frame_transform().fill_translation[1] -= 1.0/static_cast<double>(frame_factory->get_video_format_desc().height);

	return make_safe_ptr(write);
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

		if(boost::filesystem2::path(context.filename).extension() == ".flv")
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
			auto format = core::video_format_desc::get(static_cast<core::video_format::type>(n));

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
						
		if(boost::filesystem2::path(context.filename).extension() == ".flv")
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

safe_ptr<AVPacket> create_packet()
{
	safe_ptr<AVPacket> packet(new AVPacket, [](AVPacket* p)
	{
		av_free_packet(p);
		delete p;
	});
	
	av_init_packet(packet.get());
	return packet;
}

safe_ptr<AVCodecContext> open_codec(AVFormatContext& context, enum AVMediaType type, int& index)
{	
	AVCodec* decoder;
	index = THROW_ON_ERROR2(av_find_best_stream(&context, type, -1, -1, &decoder, 0), "");
	//if(strcmp(decoder->name, "prores") == 0 && decoder->next && strcmp(decoder->next->name, "prores_lgpl") == 0)
	//	decoder = decoder->next;

	THROW_ON_ERROR2(tbb_avcodec_open(context.streams[index]->codec, decoder), "");
	return safe_ptr<AVCodecContext>(context.streams[index]->codec, tbb_avcodec_close);
}

safe_ptr<AVFormatContext> open_input(const std::wstring& filename)
{
	AVFormatContext* weak_context = nullptr;
	THROW_ON_ERROR2(avformat_open_input(&weak_context, narrow(filename).c_str(), nullptr, nullptr), filename);
	safe_ptr<AVFormatContext> context(weak_context, av_close_input_file);			
	THROW_ON_ERROR2(avformat_find_stream_info(weak_context, nullptr), filename);
	fix_meta_data(*context);
	return context;
}

std::wstring print_mode(size_t width, size_t height, double fps, bool interlaced)
{
	std::wostringstream fps_ss;
	fps_ss << std::fixed << std::setprecision(2) << (!interlaced ? fps : 2.0 * fps);

	return boost::lexical_cast<std::wstring>(width) + L"x" + boost::lexical_cast<std::wstring>(height) + (!interlaced ? L"p" : L"i") + fps_ss.str();
}

bool is_valid_file(const std::wstring filename, const std::vector<std::wstring>& invalid_exts)
{
	static std::vector<std::wstring> valid_exts = boost::assign::list_of(L".m2t")(L".mov")(L".mp4")(L".dv")(L".flv")(L".mpg")(L".wav")(L".mp3")(L".dnxhd")(L".h264")(L".prores");

	auto ext = boost::to_lower_copy(boost::filesystem::wpath(filename).extension());
		
	if(std::find(invalid_exts.begin(), invalid_exts.end(), ext) != invalid_exts.end())
		return false;	

	if(std::find(valid_exts.begin(), valid_exts.end(), ext) != valid_exts.end())
		return true;	

	auto filename2 = narrow(filename);

	if(boost::filesystem::path(filename2).extension() == ".m2t")
		return true;

	std::ifstream file(filename);

	std::vector<unsigned char> buf;
	for(auto file_it = std::istreambuf_iterator<char>(file); file_it != std::istreambuf_iterator<char>() && buf.size() < 2048; ++file_it)
		buf.push_back(*file_it);

	if(buf.empty())
		return nullptr;

	AVProbeData pb;
	pb.filename = filename2.c_str();
	pb.buf		= buf.data();
	pb.buf_size = buf.size();

	int score = 0;
	return av_probe_input_format2(&pb, true, &score) != nullptr;
}

bool is_valid_file(const std::wstring filename)
{
	static const std::vector<std::wstring> invalid_exts = boost::assign::list_of(L".png")(L".tga")(L".bmp")(L".jpg")(L".jpeg")(L".gif")(L".tiff")(L".tif")(L".jp2")(L".jpx")(L".j2k")(L".j2c")(L".swf")(L".ct");
	
	return is_valid_file(filename, invalid_exts);
}

std::wstring probe_stem(const std::wstring stem, const std::vector<std::wstring>& invalid_exts)
{
	auto stem2 = boost::filesystem2::wpath(stem);
	auto dir = stem2.parent_path();
	for(auto it = boost::filesystem2::wdirectory_iterator(dir); it != boost::filesystem2::wdirectory_iterator(); ++it)
	{
		if(boost::iequals(it->path().stem(), stem2.filename()) && is_valid_file(it->path().file_string(), invalid_exts))
			return it->path().file_string();
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