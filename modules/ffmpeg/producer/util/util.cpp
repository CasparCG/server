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

#include "../../StdAfx.h"

#include "util.h"

#include "flv.h"

#include "../tbb_avcodec.h"
#include "../../ffmpeg_error.h"
#include "../../ffmpeg.h"

#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_queue.h>

#include <core/frame/frame_transform.h>
#include <core/frame/frame_factory.h>
#include <core/frame/frame.h>
#include <core/frame/audio_channel_layout.h>
#include <core/producer/frame_producer.h>

#include <common/except.h>
#include <common/array.h>
#include <common/os/filesystem.h>
#include <common/memcpy.h>

#include <tbb/parallel_for.h>

#include <common/assert.h>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/rational.hpp>

#include <fstream>

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

	switch(desc.format)
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

core::mutable_frame make_frame(const void* tag, const spl::shared_ptr<AVFrame>& decoded_frame, core::frame_factory& frame_factory, const core::audio_channel_layout& channel_layout)
{
	static tbb::concurrent_unordered_map<int64_t, tbb::concurrent_queue<std::shared_ptr<SwsContext>>> sws_contvalid_exts_;

	if(decoded_frame->width < 1 || decoded_frame->height < 1)
		return frame_factory.create_frame(tag, core::pixel_format_desc(core::pixel_format::invalid), core::audio_channel_layout::invalid());

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

		auto write = frame_factory.create_frame(tag, target_desc, channel_layout);

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

		auto av_frame = create_frame();
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
		auto write = frame_factory.create_frame(tag, desc, channel_layout);

		for(int n = 0; n < static_cast<int>(desc.planes.size()); ++n)
		{
			auto plane            = desc.planes[n];
			auto result           = write.image_data(n).begin();
			auto decoded          = decoded_frame->data[n];
			auto decoded_linesize = decoded_frame->linesize[n];

			CASPAR_ASSERT(decoded);
			CASPAR_ASSERT(write.image_data(n).begin());

			if (decoded_linesize != plane.linesize)
			{
				// Copy line by line since ffmpeg sometimes pads each line.
				tbb::affinity_partitioner ap;
				tbb::parallel_for(tbb::blocked_range<int>(0, desc.planes[n].height), [&](const tbb::blocked_range<int>& r)
				{
					for (int y = r.begin(); y != r.end(); ++y)
						A_memcpy(result + y*plane.linesize, decoded + y*decoded_linesize, plane.linesize);
				}, ap);
			}
			else
			{
				fast_memcpy(result, decoded, plane.size);
			}
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
	auto av_frame = create_frame();

	auto planes		 = pix_desc.planes;
	auto format		 = pix_desc.format;

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
	auto framerate = read_framerate(context, boost::rational<int>(static_cast<int>(fail_value * 1000000.0), 1000000));

	return static_cast<double>(framerate.numerator()) / static_cast<double>(framerate.denominator());
}

boost::rational<int> read_framerate(AVFormatContext& context, const boost::rational<int>& fail_value)
{
	auto video_index = av_find_best_stream(&context, AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0);
	auto audio_index = av_find_best_stream(&context, AVMEDIA_TYPE_AUDIO, -1, -1, 0, 0);

	if (video_index > -1)
	{
		const auto video_context = context.streams[video_index]->codec;
		const auto video_stream = context.streams[video_index];

		auto frame_rate_time_base = video_stream->avg_frame_rate;
		std::swap(frame_rate_time_base.num, frame_rate_time_base.den);

		if (is_sane_fps(frame_rate_time_base))
		{
			return boost::rational<int>(frame_rate_time_base.den, frame_rate_time_base.num);
		}

		AVRational time_base = video_context->time_base;

		if (boost::filesystem::path(context.filename).extension().string() == ".flv")
		{
			try
			{
				auto meta = read_flv_meta_info(context.filename);
				return boost::rational<int>(static_cast<int>(boost::lexical_cast<double>(meta["framerate"]) * 1000000.0), 1000000);
			}
			catch (...)
			{
				return fail_value;
			}
		}
		else
		{
			time_base.num *= video_context->ticks_per_frame;

			if (!is_sane_fps(time_base))
			{
				time_base = fix_time_base(time_base);

				if (!is_sane_fps(time_base) && audio_index > -1)
				{
					auto& audio_context = *context.streams[audio_index]->codec;
					auto& audio_stream = *context.streams[audio_index];

					double duration_sec = audio_stream.duration / static_cast<double>(audio_context.sample_rate);

					time_base.num = static_cast<int>(duration_sec*100000.0);
					time_base.den = static_cast<int>(video_stream->nb_frames * 100000);
				}
			}
		}

		boost::rational<int> fps(time_base.den, time_base.num);
		boost::rational<int> closest_fps(0);

		for (auto video_mode : enum_constants<core::video_format>())
		{
			auto format = core::video_format_desc(core::video_format(video_mode));

			auto diff1 = boost::abs(boost::rational<int>(format.time_scale, format.duration) - fps);
			auto diff2 = boost::abs(closest_fps - fps);

			if (diff1 < diff2)
				closest_fps = boost::rational<int>(format.time_scale, format.duration);
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
	spl::shared_ptr<AVFrame> frame(av_frame_alloc(), [](AVFrame* p)
	{
		av_frame_free(&p);
	});
	return frame;
}

std::shared_ptr<core::mutable_audio_buffer> flush_audio()
{
	static std::shared_ptr<core::mutable_audio_buffer> audio(new core::mutable_audio_buffer());
	return audio;
}

std::shared_ptr<core::mutable_audio_buffer> empty_audio()
{
	static std::shared_ptr<core::mutable_audio_buffer> audio(new core::mutable_audio_buffer());
	return audio;
}

std::shared_ptr<AVFrame> flush_video()
{
	static auto video = create_frame();
	return video;
}

std::shared_ptr<AVFrame> empty_video()
{
	static auto video = create_frame();
	return video;
}

spl::shared_ptr<AVCodecContext> open_codec(AVFormatContext& context, enum AVMediaType type, int& index, bool single_threaded)
{
	AVCodec* decoder;
	index = THROW_ON_ERROR2(av_find_best_stream(&context, type, index, -1, &decoder, 0), "");
	//if(strcmp(decoder->name, "prores") == 0 && decoder->next && strcmp(decoder->next->name, "prores_lgpl") == 0)
	//	decoder = decoder->next;

	THROW_ON_ERROR2(tbb_avcodec_open(context.streams[index]->codec, decoder, single_threaded), "");
	return spl::shared_ptr<AVCodecContext>(context.streams[index]->codec, tbb_avcodec_close);
}

spl::shared_ptr<AVFormatContext> open_input(const std::wstring& filename)
{
	AVFormatContext* weak_context = nullptr;
	THROW_ON_ERROR2(avformat_open_input(&weak_context, u8(filename).c_str(), nullptr, nullptr), filename);
	spl::shared_ptr<AVFormatContext> context(weak_context, [](AVFormatContext* p)
	{
		avformat_close_input(&p);
	});
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

bool is_valid_file(const std::wstring& filename, bool only_video)
{
	static const auto invalid_exts = {
		L".png",
		L".tga",
		L".bmp",
		L".jpg",
		L".jpeg",
		L".gif",
		L".tiff",
		L".tif",
		L".jp2",
		L".jpx",
		L".j2k",
		L".j2c",
		L".swf",
		L".ct"
	};
	static const auto only_audio = {
		L".mp3",
		L".wav",
		L".wma"
	};
	static const auto valid_exts = {
		L".m2t",
		L".mov",
		L".mp4",
		L".dv",
		L".flv",
		L".mpg",
		L".dnxhd",
		L".h264",
		L".prores"
	};

	auto ext = boost::to_lower_copy(boost::filesystem::path(filename).extension().wstring());

	if(std::find(valid_exts.begin(), valid_exts.end(), ext) != valid_exts.end())
		return true;

	if (!only_video && std::find(only_audio.begin(), only_audio.end(), ext) != only_audio.end())
		return true;

	if(std::find(invalid_exts.begin(), invalid_exts.end(), ext) != invalid_exts.end())
		return false;

	if (only_video && std::find(only_audio.begin(), only_audio.end(), ext) != only_audio.end())
		return false;

	auto u8filename = u8(filename);

	int score = 0;
	AVProbeData pb = {};
	pb.filename = u8filename.c_str();

	if(av_probe_input_format2(&pb, false, &score) != nullptr)
		return true;

	std::ifstream file(u8filename);

	std::vector<unsigned char> buf;
	for(auto file_it = std::istreambuf_iterator<char>(file); file_it != std::istreambuf_iterator<char>() && buf.size() < 1024; ++file_it)
		buf.push_back(*file_it);

	if(buf.empty())
		return false;

	pb.buf		= buf.data();
	pb.buf_size = static_cast<int>(buf.size());

	return av_probe_input_format2(&pb, true, &score) != nullptr;
}

bool try_get_duration(const std::wstring filename, std::int64_t& duration, boost::rational<std::int64_t>& time_base)
{
	AVFormatContext* weak_context = nullptr;
	if (avformat_open_input(&weak_context, u8(filename).c_str(), nullptr, nullptr) < 0)
		return false;

	std::shared_ptr<AVFormatContext> context(weak_context, [](AVFormatContext* p)
	{
		avformat_close_input(&p);
	});

	context->probesize = context->probesize / 10;
	context->max_analyze_duration = context->probesize / 10;

	if (avformat_find_stream_info(context.get(), nullptr) < 0)
		return false;

	const auto fps = read_fps(*context, 1.0);

	const auto rational_fps = boost::rational<std::int64_t>(static_cast<int>(fps * AV_TIME_BASE), AV_TIME_BASE);

	duration = boost::rational_cast<std::int64_t>(context->duration * rational_fps / AV_TIME_BASE);

	if (rational_fps == 0)
		return false;

	time_base = 1 / rational_fps;

	return true;
}

std::wstring probe_stem(const std::wstring& stem, bool only_video)
{
	auto stem2 = boost::filesystem::path(stem);
	auto parent = find_case_insensitive(stem2.parent_path().wstring());

	if (!parent)
		return L"";

	auto dir = boost::filesystem::path(*parent);

	for(auto it = boost::filesystem::directory_iterator(dir); it != boost::filesystem::directory_iterator(); ++it)
	{
		if(boost::iequals(it->path().stem().wstring(), stem2.filename().wstring()) && is_valid_file(it->path().wstring(), only_video))
			return it->path().wstring();
	}
	return L"";
}

core::audio_channel_layout get_audio_channel_layout(int num_channels, std::uint64_t layout, const std::wstring& channel_layout_spec)
{
	if (!channel_layout_spec.empty())
	{
		if (boost::contains(channel_layout_spec, L":")) // Custom on the fly layout specified.
		{
			std::vector<std::wstring> type_and_channel_order;
			boost::split(type_and_channel_order, channel_layout_spec, boost::is_any_of(L":"), boost::algorithm::token_compress_off);
			auto& type			= type_and_channel_order.at(0);
			auto& order			= type_and_channel_order.at(1);

			return core::audio_channel_layout(num_channels, std::move(type), order);
		}
		else // Preconfigured named channel layout selected.
		{
			auto channel_layout = core::audio_channel_layout_repository::get_default()->get_layout(channel_layout_spec);

			if (!channel_layout)
				CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"No channel layout with name " + channel_layout_spec + L" registered"));

			channel_layout->num_channels = num_channels;

			return *channel_layout;
		}
	}

	if (!layout)
	{
		if (num_channels == 1)
			return core::audio_channel_layout(num_channels, L"mono", L"FC");
		else if (num_channels == 2)
			return core::audio_channel_layout(num_channels, L"stereo", L"FL FR");
		else
			return core::audio_channel_layout(num_channels, L"", L""); // Passthru without named channels as is.
	}

	// What FFmpeg calls "channel layout" is only the "layout type" of a channel layout in
	// CasparCG where the channel layout supports different orders as well.
	// The user needs to provide additional mix-configs in casparcg.config to support more
	// than the most common (5.1, mono and stereo) types.

	// Based on information in https://ffmpeg.org/ffmpeg-utils.html#Channel-Layout
	switch (layout)
	{
	case AV_CH_LAYOUT_MONO:
		return core::audio_channel_layout(num_channels, L"mono",			L"FC");
	case AV_CH_LAYOUT_STEREO:
		return core::audio_channel_layout(num_channels, L"stereo",			L"FL FR");
	case AV_CH_LAYOUT_2POINT1:
		return core::audio_channel_layout(num_channels, L"2.1",				L"FL FR LFE");
	case AV_CH_LAYOUT_SURROUND:
		return core::audio_channel_layout(num_channels, L"3.0",				L"FL FR FC");
	case AV_CH_LAYOUT_2_1:
		return core::audio_channel_layout(num_channels, L"3.0(back)",		L"FL FR BC");
	case AV_CH_LAYOUT_4POINT0:
		return core::audio_channel_layout(num_channels, L"4.0",				L"FL FR FC BC");
	case AV_CH_LAYOUT_QUAD:
		return core::audio_channel_layout(num_channels, L"quad",			L"FL FR BL BR");
	case AV_CH_LAYOUT_2_2:
		return core::audio_channel_layout(num_channels, L"quad(side)",		L"FL FR SL SR");
	case AV_CH_LAYOUT_3POINT1:
		return core::audio_channel_layout(num_channels, L"3.1",				L"FL FR FC LFE");
	case AV_CH_LAYOUT_5POINT0_BACK:
		return core::audio_channel_layout(num_channels, L"5.0",				L"FL FR FC BL BR");
	case AV_CH_LAYOUT_5POINT0:
		return core::audio_channel_layout(num_channels, L"5.0(side)",		L"FL FR FC SL SR");
	case AV_CH_LAYOUT_4POINT1:
		return core::audio_channel_layout(num_channels, L"4.1",				L"FL FR FC LFE BC");
	case AV_CH_LAYOUT_5POINT1_BACK:
		return core::audio_channel_layout(num_channels, L"5.1",				L"FL FR FC LFE BL BR");
	case AV_CH_LAYOUT_5POINT1:
		return core::audio_channel_layout(num_channels, L"5.1(side)",		L"FL FR FC LFE SL SR");
	case AV_CH_LAYOUT_6POINT0:
		return core::audio_channel_layout(num_channels, L"6.0",				L"FL FR FC BC SL SR");
	case AV_CH_LAYOUT_6POINT0_FRONT:
		return core::audio_channel_layout(num_channels, L"6.0(front)",		L"FL FR FLC FRC SL SR");
	case AV_CH_LAYOUT_HEXAGONAL:
		return core::audio_channel_layout(num_channels, L"hexagonal",		L"FL FR FC BL BR BC");
	case AV_CH_LAYOUT_6POINT1:
		return core::audio_channel_layout(num_channels, L"6.1",				L"FL FR FC LFE BC SL SR");
	case AV_CH_LAYOUT_6POINT1_BACK:
		return core::audio_channel_layout(num_channels, L"6.1(back)",		L"FL FR FC LFE BL BR BC");
	case AV_CH_LAYOUT_6POINT1_FRONT:
		return core::audio_channel_layout(num_channels, L"6.1(front)",		L"FL FR LFE FLC FRC SL SR");
	case AV_CH_LAYOUT_7POINT0:
		return core::audio_channel_layout(num_channels, L"7.0",				L"FL FR FC BL BR SL SR");
	case AV_CH_LAYOUT_7POINT0_FRONT:
		return core::audio_channel_layout(num_channels, L"7.0(front)",		L"FL FR FC FLC FRC SL SR");
	case AV_CH_LAYOUT_7POINT1:
		return core::audio_channel_layout(num_channels, L"7.1",				L"FL FR FC LFE BL BR SL SR");
	case AV_CH_LAYOUT_7POINT1_WIDE_BACK:
		return core::audio_channel_layout(num_channels, L"7.1(wide)",		L"FL FR FC LFE BL BR FLC FRC");
	case AV_CH_LAYOUT_7POINT1_WIDE:
		return core::audio_channel_layout(num_channels, L"7.1(wide-side)",	L"FL FR FC LFE FLC FRC SL SR");
	case AV_CH_LAYOUT_STEREO_DOWNMIX:
		return core::audio_channel_layout(num_channels, L"downmix",			L"DL DR");
	default:
		// Passthru
		return core::audio_channel_layout(num_channels, L"", L"");
	}
}

// av_get_default_channel_layout does not work for layouts not predefined in ffmpeg. This is needed to support > 8 channels.
std::uint64_t create_channel_layout_bitmask(int num_channels)
{
	if (num_channels > 63)
		CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info(L"FFmpeg cannot handle more than 63 audio channels"));

	const auto ALL_63_CHANNELS = 0x7FFFFFFFFFFFFFFFULL;

	auto to_shift = 63 - num_channels;
	auto result = ALL_63_CHANNELS >> to_shift;

	return static_cast<std::uint64_t>(result);
}

std::string to_string(const boost::rational<int>& framerate)
{
	return boost::lexical_cast<std::string>(framerate.numerator())
		+ "/" + boost::lexical_cast<std::string>(framerate.denominator()) + " (" + boost::lexical_cast<std::string>(static_cast<double>(framerate.numerator()) / static_cast<double>(framerate.denominator())) + ") fps";
}

std::vector<int> find_audio_cadence(const boost::rational<int>& framerate)
{
	static std::map<boost::rational<int>, std::vector<int>> CADENCES_BY_FRAMERATE = []
	{
		std::map<boost::rational<int>, std::vector<int>> result;

		for (core::video_format format : enum_constants<core::video_format>())
		{
			core::video_format_desc desc(format);
			boost::rational<int> format_rate(desc.time_scale, desc.duration);

			result.insert(std::make_pair(format_rate, desc.audio_cadence));
		}

		return result;
	}();

	auto exact_match = CADENCES_BY_FRAMERATE.find(framerate);

	if (exact_match != CADENCES_BY_FRAMERATE.end())
		return exact_match->second;

	boost::rational<int> closest_framerate_diff = std::numeric_limits<int>::max();
	boost::rational<int> closest_framerate = 0;

	for (auto format_framerate : CADENCES_BY_FRAMERATE | boost::adaptors::map_keys)
	{
		auto diff = boost::abs(framerate - format_framerate);

		if (diff < closest_framerate_diff)
		{
			closest_framerate_diff = diff;
			closest_framerate = format_framerate;
		}
	}

	if (is_logging_quiet_for_thread())
		CASPAR_LOG(debug) << "No exact audio cadence match found for framerate " << to_string(framerate)
		<< "\nClosest match is " << to_string(closest_framerate)
		<< "\nwhich is a " << to_string(closest_framerate_diff) << " difference.";
	else
		CASPAR_LOG(warning) << "No exact audio cadence match found for framerate " << to_string(framerate)
		<< "\nClosest match is " << to_string(closest_framerate)
		<< "\nwhich is a " << to_string(closest_framerate_diff) << " difference.";

	return CADENCES_BY_FRAMERATE[closest_framerate];
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
