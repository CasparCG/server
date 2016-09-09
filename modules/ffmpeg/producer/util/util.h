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

#pragma once

#include <common/forward.h>
#include <common/memory.h>

#include <core/video_format.h>
#include <core/frame/pixel_format.h>
#include <core/mixer/audio/audio_mixer.h>
#include <core/fwd.h>

#include <boost/rational.hpp>

#include <array>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C"
{
#include <libavutil/avutil.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

struct AVFrame;
struct AVFormatContext;
struct AVPacket;
struct AVRational;
struct AVCodecContext;

namespace caspar { namespace ffmpeg {
		
// Utils

core::field_mode					get_mode(const AVFrame& frame);
core::mutable_frame					make_frame(const void* tag, const spl::shared_ptr<AVFrame>& decoded_frame, core::frame_factory& frame_factory, const core::audio_channel_layout& channel_layout);
spl::shared_ptr<AVFrame>			make_av_frame(core::mutable_frame& frame);
spl::shared_ptr<AVFrame>			make_av_frame(std::array<uint8_t*, 4> data, const core::pixel_format_desc& pix_desc);

core::pixel_format_desc				pixel_format_desc(PixelFormat pix_fmt, int width, int height);

spl::shared_ptr<AVPacket> create_packet();
spl::shared_ptr<AVFrame>  create_frame();

spl::shared_ptr<AVCodecContext> open_codec(AVFormatContext& context, AVMediaType type, int& index, bool single_threaded);
spl::shared_ptr<AVFormatContext> open_input(const std::wstring& filename);

bool is_sane_fps(AVRational time_base);
AVRational fix_time_base(AVRational time_base);

std::shared_ptr<AVFrame> flush();

double read_fps(AVFormatContext& context, double fail_value);
boost::rational<int> read_framerate(AVFormatContext& context, const boost::rational<int>& fail_value);

std::wstring print_mode(int width, int height, double fps, bool interlaced);

std::wstring probe_stem(const std::wstring& stem, bool only_video);
bool is_valid_file(const std::wstring& filename, bool only_video);
bool try_get_duration(const std::wstring filename, std::int64_t& duration, boost::rational<std::int64_t>& time_base);

core::audio_channel_layout get_audio_channel_layout(int num_channels, std::uint64_t layout, const std::wstring& channel_layout_spec);

// av_get_default_channel_layout does not work for layouts not predefined in ffmpeg. This is needed to support > 8 channels.
std::int64_t create_channel_layout_bitmask(int num_channels);

}}
