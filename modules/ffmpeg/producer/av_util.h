#include <core/frame/pixel_format.h>
#include <core/frame/frame.h>
#include <core/frame/frame_factory.h>

#include <memory>
enum AVPixelFormat;

namespace caspar { namespace ffmpeg {

core::pixel_format get_pixel_format(AVPixelFormat pix_fmt);
core::pixel_format_desc pixel_format_desc(AVPixelFormat pix_fmt, int width, int height);
core::mutable_frame make_frame(void* tag, core::frame_factory& frame_factory, std::shared_ptr<AVFrame> video, std::shared_ptr<AVFrame> audio);

}}