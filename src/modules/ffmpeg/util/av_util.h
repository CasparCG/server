#pragma once

extern "C" {
#include <libavutil/pixfmt.h>
}

#include <core/frame/frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/geometry.h>
#include <core/frame/pixel_format.h>
#include <core/video_format.h>

#include <map>
#include <memory>
#include <vector>

#include "../defines.h"

struct AVFrame;
struct AVPacket;
struct AVFilterContext;
struct AVCodecContext;
struct AVDictionary;

namespace caspar { namespace ffmpeg {

std::shared_ptr<AVFrame>  alloc_frame();
std::shared_ptr<AVPacket> alloc_packet();

core::pixel_format_desc pixel_format_desc(AVPixelFormat     pix_fmt,
                                          int               width,
                                          int               height,
                                          std::vector<int>& data_map,
                                          core::color_space color_space = core::color_space::bt709);
core::mutable_frame     make_frame(void*                    tag,
                                   core::frame_factory&     frame_factory,
                                   std::shared_ptr<AVFrame> video,
                                   std::shared_ptr<AVFrame> audio,
                                   core::color_space        color_space = core::color_space::bt709,
                                   core::frame_geometry::scale_mode     = core::frame_geometry::scale_mode::stretch,
                                   bool is_straight_alpha               = false);

std::shared_ptr<AVFrame> make_av_video_frame(const core::const_frame& frame, const core::video_format_desc& format_des);
std::shared_ptr<AVFrame> make_av_audio_frame(const core::const_frame& frame, const core::video_format_desc& format_des);

AVDictionary*                      to_dict(std::map<std::string, std::string>&& map);
std::map<std::string, std::string> to_map(AVDictionary** dict);

uint64_t get_channel_layout_mask_for_channels(int channel_count);

}} // namespace caspar::ffmpeg
