#pragma once

extern "C" {
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
}

#include <core/frame/frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/geometry.h>
#include <core/frame/pixel_format.h>
#include <core/video_format.h>

#include <map>
#include <memory>
#include <span>
#include <vector>

struct AVChannelLayout;
struct AVCodec;
struct AVCodecContext;
struct AVDictionary;
struct AVFilterContext;
struct AVFilterGraph;
struct AVFrame;
struct AVPacket;

namespace caspar { namespace ffmpeg {

std::shared_ptr<AVFrame>  alloc_frame();
std::shared_ptr<AVPacket> alloc_packet();

core::color_space get_color_space(const std::shared_ptr<AVFrame>& video);

core::pixel_format_desc pixel_format_desc(AVPixelFormat     pix_fmt,
                                          int               width,
                                          int               height,
                                          std::vector<int>& data_map,
                                          core::color_space color_space = core::color_space::unknown);
core::mutable_frame     make_frame(void*                    tag,
                                   core::frame_factory&     frame_factory,
                                   std::shared_ptr<AVFrame> video,
                                   std::shared_ptr<AVFrame> audio,
                                   core::color_space        color_space = core::color_space::unknown,
                                   core::frame_geometry::scale_mode     = core::frame_geometry::scale_mode::stretch,
                                   bool is_straight_alpha               = false);

std::shared_ptr<AVFrame> make_av_video_frame(const core::const_frame& frame, const core::video_format_desc& format_des);
std::shared_ptr<AVFrame> make_av_audio_frame(const core::const_frame& frame, const core::video_format_desc& format_des);

AVDictionary*                      to_dict(std::map<std::string, std::string>&& map);
std::map<std::string, std::string> to_map(AVDictionary** dict);

uint64_t get_channel_layout_mask_for_channels(int channel_count);

std::span<const AVPixelFormat> get_supported_pixel_formats(const AVCodecContext*, const AVCodec*);
void set_pixel_formats(AVFilterContext*, std::span<const AVPixelFormat>);

std::span<const AVSampleFormat> get_supported_sample_formats(const AVCodecContext*, const AVCodec*);
void set_sample_formats(AVFilterContext*, std::span<const AVSampleFormat>);

std::span<const int> get_supported_sample_rates(const AVCodecContext*, const AVCodec*);
void set_sample_rates(AVFilterContext*, std::span<const int>);

std::span<const AVChannelLayout> get_supported_channel_layouts(const AVCodecContext*, const AVCodec*);
void set_channel_layouts(AVFilterContext*, std::span<const AVChannelLayout>);
AVChannelLayout get_channel_layout_default(int nb_channels);

AVFilterContext* create_buffersink(AVFilterGraph*, const char* name, std::span<const AVPixelFormat> = {});

AVFilterContext* create_abuffersink(AVFilterGraph*, const char* name,
    std::span<const AVSampleFormat> = {}, std::span<const int> = {}, std::span<const AVChannelLayout> = {}
);

}} // namespace caspar::ffmpeg
