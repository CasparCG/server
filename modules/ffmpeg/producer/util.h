#pragma once

#include <common/memory/safe_ptr.h>

#include <core/video_format.h>
#include <core/producer/frame/pixel_format.h>

struct AVFrame;
enum PixelFormat;

namespace caspar {

namespace core {

struct pixel_format_desc;
class write_frame;
struct frame_factory;

}

core::video_mode::type		get_mode(AVFrame& frame);
core::pixel_format::type	get_pixel_format(PixelFormat pix_fmt);
core::pixel_format_desc		get_pixel_format_desc(PixelFormat pix_fmt, size_t width, size_t height);
PixelFormat					get_ffmpeg_pixel_format(const core::pixel_format_desc& format_desc);
safe_ptr<AVFrame>			as_av_frame(const safe_ptr<core::write_frame>& frame);
bool						try_make_gray(const safe_ptr<AVFrame>& frame);
safe_ptr<core::write_frame> make_write_frame(const void* tag, const safe_ptr<AVFrame>& decoded_frame, const safe_ptr<core::frame_factory>& frame_factory, int hints);

}