#include <core/frame/pixel_format.h>

enum AVPixelFormat;

namespace caspar { namespace ffmpeg {

core::pixel_format get_pixel_format(AVPixelFormat pix_fmt);
core::pixel_format_desc pixel_format_desc(AVPixelFormat pix_fmt, int width, int height);

}}