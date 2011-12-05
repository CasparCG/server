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

#include <common/memory/safe_ptr.h>

#include <core/video_format.h>
#include <core/producer/frame/pixel_format.h>
#include <core/mixer/audio/audio_mixer.h>

enum PixelFormat;
struct AVFrame;
struct AVFormatContext;
struct AVPacket;
struct AVRational;
struct AVCodecContext;

namespace caspar {

namespace core {

struct pixel_format_desc;
class write_frame;
struct frame_factory;

}

namespace ffmpeg {
		
std::shared_ptr<core::audio_buffer> flush_audio();
std::shared_ptr<core::audio_buffer> empty_audio();
std::shared_ptr<AVFrame>			flush_video();
std::shared_ptr<AVFrame>			empty_video();

// Utils

static const int CASPAR_PIX_FMT_LUMA = 10; // Just hijack some unual pixel format.

core::field_mode::type		get_mode(const AVFrame& frame);
int							make_alpha_format(int format); // NOTE: Be careful about CASPAR_PIX_FMT_LUMA, change it to PIX_FMT_GRAY8 if you want to use the frame inside some ffmpeg function.
safe_ptr<core::write_frame> make_write_frame(const void* tag, const safe_ptr<AVFrame>& decoded_frame, const safe_ptr<core::frame_factory>& frame_factory, int hints);

safe_ptr<AVPacket> create_packet();

safe_ptr<AVCodecContext> open_codec(AVFormatContext& context,  enum AVMediaType type, int& index);
safe_ptr<AVFormatContext> open_input(const std::wstring& filename);

bool is_sane_fps(AVRational time_base);
AVRational fix_time_base(AVRational time_base);

double read_fps(AVFormatContext& context, double fail_value);

std::wstring print_mode(size_t width, size_t height, double fps, bool interlaced);

std::wstring probe_stem(const std::wstring stem);
bool is_valid_file(const std::wstring filename);

}}