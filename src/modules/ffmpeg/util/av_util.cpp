#include "av_util.h"
#include "av_assert.h"
#include <core/frame/frame_side_data.h>

#include <common/bit_depth.h>
#include <cstdint>
#include <cstring>
#include <libavutil/opt.h>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixfmt.h>
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <tbb/parallel_for.h>
#include <tbb/parallel_invoke.h>

#include <boost/log/utility/manipulators/dump.hpp>

#include <tuple>

namespace caspar { namespace ffmpeg {

std::shared_ptr<AVFrame> alloc_frame()
{
    const auto frame = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* ptr) { av_frame_free(&ptr); });
    if (!frame)
        FF_RET(AVERROR(ENOMEM), "av_frame_alloc");
    return frame;
}

std::shared_ptr<AVPacket> alloc_packet()
{
    const auto packet = std::shared_ptr<AVPacket>(av_packet_alloc(), [](AVPacket* ptr) { av_packet_free(&ptr); });
    if (!packet)
        FF_RET(AVERROR(ENOMEM), "av_packet_alloc");
    return packet;
}

static void add_side_data(std::vector<core::mutable_frame_side_data>& out, const AVFrameSideData& in)
{
    auto add_no_metadata = [&](core::frame_side_data_type type) {
        out.push_back(core::mutable_frame_side_data(type, std::vector<uint8_t>(in.data, in.data + in.size)));
    };
    switch (in.type) {
        case AV_FRAME_DATA_A53_CC:
            CASPAR_LOG(trace) << L"got A53_CC side data: " << boost::log::dump(in.data, in.size, 16);
            add_no_metadata(core::frame_side_data_type::a53_cc);
            break;
        case AV_FRAME_DATA_PANSCAN:
        case AV_FRAME_DATA_STEREO3D:
        case AV_FRAME_DATA_MATRIXENCODING:
        case AV_FRAME_DATA_DOWNMIX_INFO:
        case AV_FRAME_DATA_REPLAYGAIN:
        case AV_FRAME_DATA_DISPLAYMATRIX:
        case AV_FRAME_DATA_AFD:
        case AV_FRAME_DATA_MOTION_VECTORS:
        case AV_FRAME_DATA_SKIP_SAMPLES:
        case AV_FRAME_DATA_AUDIO_SERVICE_TYPE:
        case AV_FRAME_DATA_MASTERING_DISPLAY_METADATA:
        case AV_FRAME_DATA_GOP_TIMECODE:
        case AV_FRAME_DATA_SPHERICAL:
        case AV_FRAME_DATA_CONTENT_LIGHT_LEVEL:
        case AV_FRAME_DATA_ICC_PROFILE:
        case AV_FRAME_DATA_S12M_TIMECODE:
        case AV_FRAME_DATA_DYNAMIC_HDR_PLUS:
        case AV_FRAME_DATA_REGIONS_OF_INTEREST:
        case AV_FRAME_DATA_VIDEO_ENC_PARAMS:
        case AV_FRAME_DATA_SEI_UNREGISTERED:
        case AV_FRAME_DATA_FILM_GRAIN_PARAMS:
        case AV_FRAME_DATA_DETECTION_BBOXES:
        case AV_FRAME_DATA_DOVI_RPU_BUFFER:
        case AV_FRAME_DATA_DOVI_METADATA:
        case AV_FRAME_DATA_DYNAMIC_HDR_VIVID:
        case AV_FRAME_DATA_AMBIENT_VIEWING_ENVIRONMENT:
        case AV_FRAME_DATA_VIDEO_HINT:
        case AV_FRAME_DATA_LCEVC:
        case AV_FRAME_DATA_VIEW_ID:
            // TODO: add to core::frame_side_data_type and add metadata to core::*_frame_side_data if necessary
            break;
    }
}

core::mutable_frame make_frame(void*                            tag,
                               core::frame_factory&             frame_factory,
                               std::shared_ptr<AVFrame>         video,
                               std::shared_ptr<AVFrame>         audio,
                               core::color_space                color_space,
                               core::frame_geometry::scale_mode scale_mode,
                               bool                             is_straight_alpha)
{
    std::vector<int> data_map; // TODO(perf) when using data_map, avoid uploading duplicate planes

    auto pix_desc =
        video ? pixel_format_desc(
                    static_cast<AVPixelFormat>(video->format), video->width, video->height, data_map, color_space)
              : core::pixel_format_desc(core::pixel_format::invalid);
    pix_desc.is_straight_alpha = is_straight_alpha;

    auto frame = frame_factory.create_frame(tag, pix_desc);
    if (scale_mode != core::frame_geometry::scale_mode::stretch) {
        frame.geometry() = core::frame_geometry::get_default(scale_mode);
    }

    if (video) {
        for (int i = 0; i < video->nb_side_data; i++) {
            add_side_data(frame.side_data(), *video->side_data[i]);
        }
    }

    if (audio) {
        for (int i = 0; i < audio->nb_side_data; i++) {
            add_side_data(frame.side_data(), *audio->side_data[i]);
        }
    }

    tbb::parallel_invoke(
        [&]() {
            if (video) {
                for (int n = 0; n < static_cast<int>(pix_desc.planes.size()); ++n) {
                    auto frame_plan_index = data_map.empty() ? n : data_map.at(n);

                    tbb::parallel_for(0, pix_desc.planes[n].height, [&](int y) {
                        std::memcpy(frame.image_data(n).begin() + y * pix_desc.planes[n].linesize,
                                    video->data[frame_plan_index] + y * video->linesize[frame_plan_index],
                                    pix_desc.planes[n].linesize);
                    });
                }
            }
        },
        [&]() {
            if (audio) {
                const int channel_count = 16;
                frame.audio_data()      = std::vector<int32_t>(audio->nb_samples * channel_count, 0);

#if FFMPEG_NEW_CHANNEL_LAYOUT
                auto source_channel_count = audio->ch_layout.nb_channels;
#else
                auto source_channel_count = audio->channels;
#endif

                if (source_channel_count == channel_count) {
                    std::memcpy(frame.audio_data().data(),
                                reinterpret_cast<int32_t*>(audio->data[0]),
                                sizeof(int32_t) * channel_count * audio->nb_samples);
                } else {
                    // This isn't pretty, but some callers may not provide 16 channels

                    auto dst = frame.audio_data().data();
                    auto src = reinterpret_cast<int32_t*>(audio->data[0]);
                    for (auto i = 0; i < audio->nb_samples; i++) {
                        for (auto j = 0; j < std::min(channel_count, source_channel_count); ++j) {
                            dst[i * channel_count + j] = src[i * source_channel_count + j];
                        }
                    }
                }
            }
        });

    return frame;
}

std::tuple<core::pixel_format, common::bit_depth> get_pixel_format(AVPixelFormat pix_fmt)
{
    switch (pix_fmt) {
        case AV_PIX_FMT_GRAY8:
            return {core::pixel_format::gray, common::bit_depth::bit8};
        case AV_PIX_FMT_RGB24:
            return {core::pixel_format::rgb, common::bit_depth::bit8};
        case AV_PIX_FMT_BGR24:
            return {core::pixel_format::bgr, common::bit_depth::bit8};
        case AV_PIX_FMT_BGRA:
            return {core::pixel_format::bgra, common::bit_depth::bit8};
        case AV_PIX_FMT_ARGB:
            return {core::pixel_format::argb, common::bit_depth::bit8};
        case AV_PIX_FMT_RGBA:
            return {core::pixel_format::rgba, common::bit_depth::bit8};
        case AV_PIX_FMT_ABGR:
            return {core::pixel_format::abgr, common::bit_depth::bit8};
        case AV_PIX_FMT_YUV444P:
            return {core::pixel_format::ycbcr, common::bit_depth::bit8};
        case AV_PIX_FMT_YUV422P:
            return {core::pixel_format::ycbcr, common::bit_depth::bit8};
        case AV_PIX_FMT_YUV422P10:
            return {core::pixel_format::ycbcr, common::bit_depth::bit10};
        case AV_PIX_FMT_YUV422P12:
            return {core::pixel_format::ycbcr, common::bit_depth::bit12};
        case AV_PIX_FMT_YUV420P:
            return {core::pixel_format::ycbcr, common::bit_depth::bit8};
        case AV_PIX_FMT_YUV420P10:
            return {core::pixel_format::ycbcr, common::bit_depth::bit10};
        case AV_PIX_FMT_YUV420P12:
            return {core::pixel_format::ycbcr, common::bit_depth::bit12};
        case AV_PIX_FMT_YUV411P:
            return {core::pixel_format::ycbcr, common::bit_depth::bit8};
        case AV_PIX_FMT_YUV410P:
            return {core::pixel_format::ycbcr, common::bit_depth::bit8};
        case AV_PIX_FMT_YUVA420P:
            return {core::pixel_format::ycbcra, common::bit_depth::bit8};
        case AV_PIX_FMT_YUVA422P:
            return {core::pixel_format::ycbcra, common::bit_depth::bit8};
        case AV_PIX_FMT_YUVA444P:
            return {core::pixel_format::ycbcra, common::bit_depth::bit8};
        case AV_PIX_FMT_UYVY422:
            return {core::pixel_format::uyvy, common::bit_depth::bit8};
        case AV_PIX_FMT_GBRP:
            return {core::pixel_format::gbrp, common::bit_depth::bit8};
        case AV_PIX_FMT_GBRP10:
            return {core::pixel_format::gbrp, common::bit_depth::bit10};
        case AV_PIX_FMT_GBRP12:
            return {core::pixel_format::gbrp, common::bit_depth::bit12};
        case AV_PIX_FMT_GBRP16:
            return {core::pixel_format::gbrp, common::bit_depth::bit16};
        case AV_PIX_FMT_GBRAP:
            return {core::pixel_format::gbrap, common::bit_depth::bit8};
        case AV_PIX_FMT_GBRAP16:
            return {core::pixel_format::gbrap, common::bit_depth::bit16};
        default:
            return {core::pixel_format::invalid, common::bit_depth::bit8};
    }
}

core::pixel_format_desc pixel_format_desc(AVPixelFormat     pix_fmt,
                                          int               width,
                                          int               height,
                                          std::vector<int>& data_map,
                                          core::color_space color_space)
{
    // Get linesizes
    int linesizes[4];
    av_image_fill_linesizes(linesizes, pix_fmt, width);

    const auto fmt   = get_pixel_format(pix_fmt);
    auto       desc  = core::pixel_format_desc(std::get<0>(fmt), color_space);
    auto       depth = std::get<1>(fmt);
    auto       bpc   = depth == common::bit_depth::bit8 ? 1 : 2;

    for (int i = 0; i < 4; i++)
        linesizes[i] /= bpc;

    switch (desc.format) {
        case core::pixel_format::gray:
        case core::pixel_format::luma: {
            desc.planes.push_back(core::pixel_format_desc::plane(linesizes[0], height, 1, depth));
            return desc;
        }
        case core::pixel_format::bgr:
        case core::pixel_format::rgb: {
            desc.planes.push_back(core::pixel_format_desc::plane(linesizes[0] / 3, height, 3, depth));
            return desc;
        }
        case core::pixel_format::gbrp: {
            desc.planes.push_back(core::pixel_format_desc::plane(linesizes[0], height, 1, depth));
            desc.planes.push_back(core::pixel_format_desc::plane(linesizes[1], height, 1, depth));
            desc.planes.push_back(core::pixel_format_desc::plane(linesizes[2], height, 1, depth));
            return desc;
        }
        case core::pixel_format::gbrap: {
            desc.planes.push_back(core::pixel_format_desc::plane(linesizes[0], height, 1, depth));
            desc.planes.push_back(core::pixel_format_desc::plane(linesizes[1], height, 1, depth));
            desc.planes.push_back(core::pixel_format_desc::plane(linesizes[2], height, 1, depth));
            desc.planes.push_back(core::pixel_format_desc::plane(linesizes[3], height, 1, depth));
            return desc;
        }
        case core::pixel_format::bgra:
        case core::pixel_format::argb:
        case core::pixel_format::rgba:
        case core::pixel_format::abgr: {
            desc.planes.push_back(core::pixel_format_desc::plane(linesizes[0] / 4, height, 4, depth));
            return desc;
        }
        case core::pixel_format::ycbcr:
        case core::pixel_format::ycbcra: {
            // Find chroma height
            // av_image_fill_plane_sizes is not available until ffmpeg 4.4, but we still need to support ffmpeg 4.2, so
            // we fall back to calling av_image_fill_pointers with a NULL image buffer. We can't unconditionally use
            // av_image_fill_pointers because it will not accept a NULL buffer on ffmpeg >= 5.0.
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(56, 56, 100)
            size_t    sizes[4];
            ptrdiff_t linesizes1[4];
            for (int i = 0; i < 4; i++)
                linesizes1[i] = linesizes[i];
            av_image_fill_plane_sizes(sizes, pix_fmt, height, linesizes1);
            auto size2 = static_cast<int>(sizes[1]);
#else
            uint8_t* dummy_pict_data[4];
            av_image_fill_pointers(dummy_pict_data, pix_fmt, height, NULL, linesizes);
            auto size2 = static_cast<int>(dummy_pict_data[2] - dummy_pict_data[1]);
#endif
            auto h2 = size2 / linesizes[1];

            desc.planes.push_back(core::pixel_format_desc::plane(linesizes[0], height, 1, depth));
            desc.planes.push_back(core::pixel_format_desc::plane(linesizes[1], h2, 1, depth));
            desc.planes.push_back(core::pixel_format_desc::plane(linesizes[2], h2, 1, depth));

            if (desc.format == core::pixel_format::ycbcra)
                desc.planes.push_back(core::pixel_format_desc::plane(linesizes[3], height, 1, depth));

            return desc;
        }
        case core::pixel_format::uyvy: {
            desc.planes.push_back(core::pixel_format_desc::plane(linesizes[0] / 2, height, 2, depth));
            desc.planes.push_back(core::pixel_format_desc::plane(linesizes[0] / 4, height, 4, depth));

            data_map.clear();
            data_map.push_back(0);
            data_map.push_back(0);

            return desc;
        }
        default:
            desc.format = core::pixel_format::invalid;
            return desc;
    }
}

std::shared_ptr<AVFrame> make_av_video_frame(const core::const_frame& frame, const core::video_format_desc& format_desc)
{
    auto av_frame = alloc_frame();

    auto pix_desc = frame.pixel_format_desc();

    auto planes = pix_desc.planes;
    auto format = pix_desc.format;

    const auto sar = boost::rational<int>(format_desc.square_width, format_desc.square_height) /
                     boost::rational<int>(format_desc.width, format_desc.height);

    av_frame->sample_aspect_ratio = {sar.numerator(), sar.denominator()};
    av_frame->width               = format_desc.width;
    av_frame->height              = format_desc.height;

    const auto is_16bit = planes[0].depth != common::bit_depth::bit8;
    switch (format) {
        case core::pixel_format::rgb:
            av_frame->format = is_16bit ? AVPixelFormat::AV_PIX_FMT_RGB48 : AVPixelFormat::AV_PIX_FMT_RGB24;
            break;
        case core::pixel_format::bgr:
            av_frame->format = is_16bit ? AVPixelFormat::AV_PIX_FMT_BGR48 : AVPixelFormat::AV_PIX_FMT_BGR24;
            break;
        case core::pixel_format::rgba:
            av_frame->format = is_16bit ? AVPixelFormat::AV_PIX_FMT_RGBA64 : AVPixelFormat::AV_PIX_FMT_RGBA;
            break;
        case core::pixel_format::argb:
            av_frame->format = is_16bit ? AVPixelFormat::AV_PIX_FMT_BGRA64 : AVPixelFormat::AV_PIX_FMT_ARGB;
            break;
        case core::pixel_format::bgra:
            av_frame->format = is_16bit ? AVPixelFormat::AV_PIX_FMT_BGRA64 : AVPixelFormat::AV_PIX_FMT_BGRA;
            break;
        case core::pixel_format::abgr:
            av_frame->format = is_16bit ? AVPixelFormat::AV_PIX_FMT_BGRA64 : AVPixelFormat::AV_PIX_FMT_ABGR;
            break;
        case core::pixel_format::gray:
        case core::pixel_format::luma:
            av_frame->format = is_16bit ? AVPixelFormat::AV_PIX_FMT_GRAY16 : AVPixelFormat::AV_PIX_FMT_GRAY8;
            break;
        case core::pixel_format::ycbcr: {
            int y_w = planes[0].width;
            int y_h = planes[0].height;
            int c_w = planes[1].width;
            int c_h = planes[1].height;

            if (c_h == y_h && c_w == y_w)
                av_frame->format = is_16bit ? AVPixelFormat::AV_PIX_FMT_YUV444P10 : AVPixelFormat::AV_PIX_FMT_YUV444P;
            else if (c_h == y_h && c_w * 2 == y_w)
                av_frame->format = is_16bit ? AVPixelFormat::AV_PIX_FMT_YUV422P10 : AVPixelFormat::AV_PIX_FMT_YUV422P;
            else if (c_h == y_h && c_w * 4 == y_w)
                av_frame->format = is_16bit ? AVPixelFormat::AV_PIX_FMT_YUV422P10 : AVPixelFormat::AV_PIX_FMT_YUV411P;
            else if (c_h * 2 == y_h && c_w * 2 == y_w)
                av_frame->format = is_16bit ? AVPixelFormat::AV_PIX_FMT_YUV420P10 : AVPixelFormat::AV_PIX_FMT_YUV420P;
            else if (c_h * 2 == y_h && c_w * 4 == y_w)
                av_frame->format = is_16bit ? AVPixelFormat::AV_PIX_FMT_YUV420P10 : AVPixelFormat::AV_PIX_FMT_YUV410P;

            break;
        }
        case core::pixel_format::ycbcra:
            av_frame->format = is_16bit ? AVPixelFormat::AV_PIX_FMT_YUVA420P10 : AVPixelFormat::AV_PIX_FMT_YUVA420P;
            break;
        case core::pixel_format::uyvy:
            // TODO
            break;
        case core::pixel_format::gbrp:
        case core::pixel_format::gbrap:
            // TODO
            break;
        case core::pixel_format::count:
        case core::pixel_format::invalid:
            break;
    }

    FF(av_frame_get_buffer(av_frame.get(), is_16bit ? 64 : 32));

    // TODO (perf) Avoid extra memcpy.
    for (int n = 0; n < planes.size(); ++n) {
        for (int y = 0; y < av_frame->height; ++y) {
            std::memcpy(av_frame->data[n] + y * av_frame->linesize[n],
                        frame.image_data(n).data() + y * planes[n].linesize,
                        planes[n].linesize);
        }
    }

    return av_frame;
}

std::shared_ptr<AVFrame> make_av_audio_frame(const core::const_frame& frame, const core::video_format_desc& format_desc)
{
    auto av_frame = alloc_frame();

    const auto& buffer = frame.audio_data();

    // TODO (fix) Use sample_format_desc.
#if FFMPEG_NEW_CHANNEL_LAYOUT
    av_channel_layout_default(&av_frame->ch_layout, format_desc.audio_channels);
#else
    av_frame->channels       = format_desc.audio_channels;
    av_frame->channel_layout = av_get_default_channel_layout(av_frame->channels);
#endif
    av_frame->sample_rate = format_desc.audio_sample_rate;
    av_frame->format      = AV_SAMPLE_FMT_S32;
    av_frame->nb_samples  = static_cast<int>(buffer.size() / format_desc.audio_channels);
    FF(av_frame_get_buffer(av_frame.get(), 32));
    std::memcpy(av_frame->data[0], buffer.data(), buffer.size() * sizeof(buffer.data()[0]));

    return av_frame;
}

AVDictionary* to_dict(std::map<std::string, std::string>&& map)
{
    AVDictionary* dict = nullptr;
    for (auto& p : map) {
        if (!p.second.empty()) {
            av_dict_set(&dict, p.first.c_str(), p.second.c_str(), 0);
        }
    }
    return dict;
}

std::map<std::string, std::string> to_map(AVDictionary** dict)
{
    std::map<std::string, std::string> map;
    AVDictionaryEntry*                 t = nullptr;
    while (*dict) {
        t = av_dict_get(*dict, "", t, AV_DICT_IGNORE_SUFFIX);
        if (!t) {
            break;
        }
        if (t->value) {
            map[t->key] = t->value;
        }
    }
    av_dict_free(dict);
    return map;
}

uint64_t get_channel_layout_mask_for_channels(int channel_count)
{
#if FFMPEG_NEW_CHANNEL_LAYOUT
    AVChannelLayout layout = AV_CHANNEL_LAYOUT_STEREO;
    av_channel_layout_default(&layout, channel_count);
    uint64_t channel_layout = layout.u.mask;
    av_channel_layout_uninit(&layout);

    return channel_layout;
#else
    return av_get_default_channel_layout(channel_count);
#endif
}

#if LIBAVFILTER_VERSION_INT < AV_VERSION_INT(10, 6, 0) || 1
#define GET_SET_ATTRIBUTE(                                                                                             \
    ty, fn_suffix, config_type, opt_type, old_config_name, new_config_name, codec_type, field, terminator)             \
    av_opt_array_ref<ty> get_supported_##fn_suffix(const AVCodecContext* avctx, const AVCodec* codec)                  \
    {                                                                                                                  \
        if (!codec)                                                                                                    \
            codec = avctx->codec;                                                                                      \
        if (codec->type != codec_type)                                                                                 \
            FF_RET(AVERROR(EINVAL), THROW_ON_ERROR_STR(get_supported_##fn_suffix));                                    \
        return av_opt_array_ref<ty>::terminated(codec->field, terminator);                                             \
    }                                                                                                                  \
                                                                                                                       \
    void set_##fn_suffix(AVFilterContext* target, av_opt_array_ref<ty> array_ref)                                      \
    {                                                                                                                  \
        FF(av_opt_set_bin(target,                                                                                      \
                          old_config_name,                                                                             \
                          reinterpret_cast<const uint8_t*>(array_ref.data()),                                          \
                          sizeof(ty) * array_ref.size(),                                                               \
                          AV_OPT_SEARCH_CHILDREN));                                                                    \
    }

#else
#define GET_SET_ATTRIBUTE(                                                                                             \
    ty, fn_suffix, config_type, opt_type, old_config_name, new_config_name, codec_type, field, terminator)             \
    av_opt_array_ref<ty> get_supported_##fn_suffix(const AVCodecContext* avctx, const AVCodec* codec)                  \
    {                                                                                                                  \
        const ty* data = nullptr;                                                                                      \
        int       size = 0;                                                                                            \
        FF(avcodec_get_supported_config(avctx, codec, config_type, 0, reinterpret_cast<const void**>(&data), &size));  \
        return av_opt_array_ref<ty>(data, size);                                                                       \
    }                                                                                                                  \
                                                                                                                       \
    void set_##fn_suffix(AVFilterContext* target, av_opt_array_ref<ty> array_ref)                                      \
    {                                                                                                                  \
        FF(av_opt_set_array(target,                                                                                    \
                            new_config_name,                                                                           \
                            AV_OPT_SEARCH_CHILDREN | AV_OPT_ARRAY_REPLACE,                                             \
                            0,                                                                                         \
                            array_ref.size(),                                                                          \
                            opt_type,                                                                                  \
                            array_ref.data()));                                                                        \
    }
#endif

GET_SET_ATTRIBUTE(AVPixelFormat,
                  pixel_formats,
                  AV_CODEC_CONFIG_PIX_FORMAT,
                  AV_OPT_TYPE_PIXEL_FMT,
                  "pix_fmts",
                  "pixel_formats",
                  AVMEDIA_TYPE_VIDEO,
                  pix_fmts,
                  AV_PIX_FMT_NONE)

GET_SET_ATTRIBUTE(AVSampleFormat,
                  sample_formats,
                  AV_CODEC_CONFIG_SAMPLE_FORMAT,
                  AV_OPT_TYPE_SAMPLE_FMT,
                  "sample_fmts",
                  "sample_formats",
                  AVMEDIA_TYPE_AUDIO,
                  sample_fmts,
                  AV_SAMPLE_FMT_NONE)

GET_SET_ATTRIBUTE(int,
                  sample_rates,
                  AV_CODEC_CONFIG_SAMPLE_RATE,
                  AV_OPT_TYPE_INT,
                  "sample_rates",
                  "samplerates",
                  AVMEDIA_TYPE_AUDIO,
                  supported_samplerates,
                  0)

av_opt_array_ref<channel_layouts_array_element> get_supported_channel_layouts(const AVCodecContext* avctx,
                                                                              const AVCodec*        codec)
{
#if LIBAVFILTER_VERSION_INT < AV_VERSION_INT(10, 6, 0)
    if (!codec)
        codec = avctx->codec;
    if (codec->type != AVMEDIA_TYPE_AUDIO)
        FF_RET(AVERROR(EINVAL), THROW_ON_ERROR_STR(get_supported_channel_layouts));
#if FFMPEG_NEW_CHANNEL_LAYOUT
    static constexpr AVChannelLayout terminator = {};
    return av_opt_array_ref<channel_layouts_array_element>::terminated(codec->ch_layouts, [](const AVChannelLayout& v) {
        return 0 == std::memcmp(&v, &terminator, sizeof(terminator));
    });
#else
    return av_opt_array_ref<channel_layouts_array_element>::terminated(codec->channel_layouts, 0);
#endif
#else
    const channel_layouts_array_element* data = nullptr;
    int                                  size = 0;
    FF(avcodec_get_supported_config(
        avctx, codec, AV_CODEC_CONFIG_CHANNEL_LAYOUT, 0, reinterpret_cast<const void**>(&data), &size));
    return av_opt_array_ref<channel_layouts_array_element>(data, size);
#endif
}

void set_channel_layouts(AVFilterContext* target, av_opt_array_ref<channel_layouts_array_element> array_ref)
{
#if LIBAVFILTER_VERSION_INT < AV_VERSION_INT(10, 6, 0)
#if FFMPEG_NEW_CHANNEL_LAYOUT
    // TODO: need to translate into something that can be passed via av_opt_set_*
    // FF(av_opt_set_chlayout(sink, "ch_layouts", ch_layouts, AV_OPT_SEARCH_CHILDREN));
#else
    FF(av_opt_set_bin(target,
                      "channel_layouts",
                      reinterpret_cast<const uint8_t*>(array_ref.data()),
                      sizeof(channel_layouts_array_element) * array_ref.size(),
                      AV_OPT_SEARCH_CHILDREN));
#endif
#else
    FF(av_opt_set_array(target,
                        "channel_layouts",
                        AV_OPT_SEARCH_CHILDREN | AV_OPT_ARRAY_REPLACE,
                        0,
                        array_ref.size(),
                        AV_OPT_TYPE_CHLAYOUT,
                        array_ref.data()));
#endif
}

channel_layouts_array_element get_channel_layout_default(int nb_channels)
{
#if FFMPEG_NEW_CHANNEL_LAYOUT
    AVChannelLayout retval{};
    av_channel_layout_default(&retval, nb_channels);
    return retval;
#else
    return av_get_default_channel_layout(nb_channels);
#endif
}

AVFilterContext*
create_buffersink(AVFilterGraph* graph, const char* name, std::optional<av_opt_array_ref<AVPixelFormat>> pixel_formats)
{
    AVFilterContext* retval = FFMEM(avfilter_graph_alloc_filter(graph, avfilter_get_by_name("buffersink"), name));
    if (pixel_formats) {
        set_pixel_formats(retval, *pixel_formats);
    }
    FF(avfilter_init_str(retval, nullptr));
    return retval;
}

AVFilterContext* create_abuffersink(AVFilterGraph*                                   graph,
                                    const char*                                      name,
                                    std::optional<av_opt_array_ref<AVSampleFormat>>  sample_formats,
                                    std::optional<av_opt_array_ref<int>>             sample_rates,
                                    std::optional<av_opt_array_ref<AVChannelLayout>> channel_layouts)
{
    AVFilterContext* retval = FFMEM(avfilter_graph_alloc_filter(graph, avfilter_get_by_name("abuffersink"), name));
    if (sample_formats) {
        set_sample_formats(retval, *sample_formats);
    }
    if (sample_rates) {
        set_sample_rates(retval, *sample_rates);
    }
    if (channel_layouts) {
        set_channel_layouts(retval, *channel_layouts);
    } else {
        FF(av_opt_set_int(retval, "all_channel_counts", 1, AV_OPT_SEARCH_CHILDREN));
    }
    FF(avfilter_init_str(retval, nullptr));
    return retval;
}
}} // namespace caspar::ffmpeg
