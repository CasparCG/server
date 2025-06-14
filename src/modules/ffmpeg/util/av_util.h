#pragma once

#if defined(_MSC_VER)
#pragma warning(push, 1)
#endif

extern "C" {
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS
#include <libavfilter/avfilter.h>
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <core/frame/frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/frame_side_data.h>
#include <core/frame/geometry.h>
#include <core/frame/pixel_format.h>
#include <core/video_format.h>

#include <cstdint>
#include <initializer_list>
#include <map>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

#include "../defines.h"

struct AVFrame;
struct AVPacket;
struct AVFilterContext;
struct AVFilterGraph;
struct AVCodec;
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
core::mutable_frame     make_frame(void*                                        tag,
                                   core::frame_factory&                         frame_factory,
                                   std::shared_ptr<AVFrame>                     video,
                                   std::shared_ptr<AVFrame>                     audio,
                                   std::shared_ptr<core::frame_side_data_queue> side_data_queue,
                                   core::color_space                            color_space = core::color_space::bt709,
                                   core::frame_geometry::scale_mode = core::frame_geometry::scale_mode::stretch,
                                   bool is_straight_alpha           = false);

std::shared_ptr<AVFrame> make_av_video_frame(const core::const_frame& frame, const core::video_format_desc& format_des);
std::shared_ptr<AVFrame> make_av_audio_frame(const core::const_frame& frame, const core::video_format_desc& format_des);

AVDictionary*                      to_dict(std::map<std::string, std::string>&& map);
std::map<std::string, std::string> to_map(AVDictionary** dict);

uint64_t get_channel_layout_mask_for_channels(int channel_count);

template <typename T>
class av_opt_array_ref final
{
  private:
    const T*    data_;
    std::size_t size_;

  public:
    constexpr av_opt_array_ref() noexcept
        : av_opt_array_ref(nullptr, 0)
    {
    }
    constexpr av_opt_array_ref(const T* data, std::size_t size) noexcept
        : data_(size == 0 ? nullptr : data)
        , size_(size)
    {
    }
    constexpr av_opt_array_ref(std::initializer_list<T> data) noexcept
        : av_opt_array_ref(data.begin(), data.size())
    {
    }
    template <typename = std::enable_if<std::is_integral_v<T> || std::is_enum_v<T>>>
    static constexpr av_opt_array_ref terminated(const T* data, T terminator)
    {
        if (!data) {
            return av_opt_array_ref();
        }
        std::size_t size = 0;
        while (data[size] != terminator)
            size++;
        return av_opt_array_ref(data, size);
    }
    template <typename F,
              typename = std::enable_if<std::is_same_v<bool, decltype(std::declval<F&>()(std::declval<const T&>()))>>>
    static constexpr av_opt_array_ref terminated(const T* data, F&& is_terminator)
    {
        if (!data) {
            return av_opt_array_ref();
        }
        std::size_t size = 0;
        while (!is_terminator(data[size]))
            size++;
        return av_opt_array_ref(data, size);
    }
    constexpr const T*    begin() const noexcept { return data_; }
    constexpr const T*    end() const noexcept { return data_ + size_; }
    constexpr const T*    data() const noexcept { return data_; }
    constexpr std::size_t size() const noexcept { return size_; }
    constexpr bool        empty() const noexcept { return size_ == 0; }
};

av_opt_array_ref<AVPixelFormat> get_supported_pixel_formats(const AVCodecContext* avctx, const AVCodec* codec);
void set_pixel_formats(AVFilterContext* target, av_opt_array_ref<AVPixelFormat> pixel_formats);

av_opt_array_ref<AVSampleFormat> get_supported_sample_formats(const AVCodecContext* avctx, const AVCodec* codec);
void set_sample_formats(AVFilterContext* target, av_opt_array_ref<AVSampleFormat> sample_formats);

av_opt_array_ref<int> get_supported_sample_rates(const AVCodecContext* avctx, const AVCodec* codec);
void                  set_sample_rates(AVFilterContext* target, av_opt_array_ref<int> sample_rates);

#if FFMPEG_NEW_CHANNEL_LAYOUT
using channel_layouts_array_element = AVChannelLayout;
#else
using channel_layouts_array_element = std::uint64_t;
#endif

av_opt_array_ref<channel_layouts_array_element> get_supported_channel_layouts(const AVCodecContext* avctx,
                                                                              const AVCodec*        codec);
void set_channel_layouts(AVFilterContext* target, av_opt_array_ref<channel_layouts_array_element> channel_layouts);
channel_layouts_array_element get_channel_layout_default(int nb_channels);

AVFilterContext*
create_buffersink(AVFilterGraph* graph, const char* name, std::optional<av_opt_array_ref<AVPixelFormat>> pixel_formats);

AVFilterContext* create_abuffersink(AVFilterGraph*                                                 graph,
                                    const char*                                                    name,
                                    std::optional<av_opt_array_ref<AVSampleFormat>>                sample_formats,
                                    std::optional<av_opt_array_ref<int>>                           sample_rates,
                                    std::optional<av_opt_array_ref<channel_layouts_array_element>> channel_layouts);

}} // namespace caspar::ffmpeg
