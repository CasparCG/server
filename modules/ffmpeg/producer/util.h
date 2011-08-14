#pragma once

#include <common/memory/safe_ptr.h>

#include <core/video_format.h>
#include <core/producer/frame/pixel_format.h>

extern "C"
{
	#include <libavutil/pixfmt.h>
}

struct AVFrame;

namespace caspar {

namespace core {

struct pixel_format_desc;
class write_frame;
struct frame_factory;

}

static const PixelFormat	CASPAR_PIX_FMT_LUMA = PIX_FMT_MONOBLACK; // Just hijack some unual pixel format.

core::video_mode::type		get_mode(AVFrame& frame);
core::pixel_format::type	get_pixel_format(PixelFormat pix_fmt);
core::pixel_format_desc		get_pixel_format_desc(PixelFormat pix_fmt, size_t width, size_t height);
int							make_alpha_format(int format); // NOTE: Be careful about CASPAR_PIX_FMT_LUMA, change it to PIX_FMT_GRAY8 if you want to use the frame inside some ffmpeg function.
safe_ptr<core::write_frame> make_write_frame(const void* tag, const safe_ptr<AVFrame>& decoded_frame, const safe_ptr<core::frame_factory>& frame_factory, int hints);

}