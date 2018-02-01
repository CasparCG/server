#include "av_util.h"

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" {
#include <libavutil/pixfmt.h>
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar {
namespace ffmpeg {

core::mutable_frame make_frame(void* tag, core::frame_factory& frame_factory, std::shared_ptr<AVFrame> video, std::shared_ptr<AVFrame> audio)
{
    const auto pix_desc = video
        ? pixel_format_desc(static_cast<AVPixelFormat>(video->format), video->width, video->height)
        : core::pixel_format_desc(core::pixel_format::invalid);

    auto frame = frame_factory.create_frame(tag, pix_desc);

    if (video) {
        for (int n = 0; n < static_cast<int>(pix_desc.planes.size()); ++n) {
            for (int y = 0; y < pix_desc.planes[n].height; ++y) {
                std::memcpy(
                    frame.image_data(n).begin() + y * pix_desc.planes[n].linesize,
                    video->data[n] + y * video->linesize[n],
                    pix_desc.planes[n].linesize
                );
            }
        }
    }

    if (audio) {
        // TODO This is a bit of a hack
        frame.audio_data() = core::mutable_audio_buffer(audio->nb_samples * 8, 0);
        auto dst = frame.audio_data().data();
        auto src = reinterpret_cast<int32_t*>(audio->data[0]);
        for (auto i = 0; i < audio->nb_samples; ++i) {
            for (auto j = 0; j < audio->channels; ++j) {
                dst[i * 8 + j] = src[i * audio->channels + j];
            }
        }
    }

    return frame;
}

core::pixel_format get_pixel_format(AVPixelFormat pix_fmt)
{
    switch (pix_fmt)
    {
    case AV_PIX_FMT_GRAY8:       return core::pixel_format::gray;
    case AV_PIX_FMT_RGB24:       return core::pixel_format::rgb;
    case AV_PIX_FMT_BGR24:       return core::pixel_format::bgr;
    case AV_PIX_FMT_BGRA:        return core::pixel_format::bgra;
    case AV_PIX_FMT_ARGB:        return core::pixel_format::argb;
    case AV_PIX_FMT_RGBA:        return core::pixel_format::rgba;
    case AV_PIX_FMT_ABGR:        return core::pixel_format::abgr;
    case AV_PIX_FMT_YUV444P:     return core::pixel_format::ycbcr;
    case AV_PIX_FMT_YUV422P:     return core::pixel_format::ycbcr;
    case AV_PIX_FMT_YUV420P:     return core::pixel_format::ycbcr;
    case AV_PIX_FMT_YUV411P:     return core::pixel_format::ycbcr;
    case AV_PIX_FMT_YUV410P:     return core::pixel_format::ycbcr;
    case AV_PIX_FMT_YUVA420P:    return core::pixel_format::ycbcra;
    case AV_PIX_FMT_YUVA422P:    return core::pixel_format::ycbcra;
    case AV_PIX_FMT_YUVA444P:    return core::pixel_format::ycbcra;
    default:                                    return core::pixel_format::invalid;
    }
}

core::pixel_format_desc pixel_format_desc(AVPixelFormat pix_fmt, int width, int height)
{
    // Get linesizes
    AVPicture dummy_pict;
    avpicture_fill(&dummy_pict, nullptr, pix_fmt, width, height);

    core::pixel_format_desc desc = get_pixel_format(pix_fmt);

    switch (desc.format)
    {
    case core::pixel_format::gray:
    case core::pixel_format::luma:
    {
        desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0], height, 1));
        return desc;
    }
    case core::pixel_format::bgr:
    case core::pixel_format::rgb:
    {
    desc.planes.push_back(core::pixel_format_desc::plane(
        dummy_pict.linesize[0] / 3, height, 3));
        return desc;
    }
    case core::pixel_format::bgra:
    case core::pixel_format::argb:
    case core::pixel_format::rgba:
    case core::pixel_format::abgr:
    {
        desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0] / 4, height, 4));
        return desc;
    }
    case core::pixel_format::ycbcr:
    case core::pixel_format::ycbcra:
    {
        // Find chroma height
        auto size2 = static_cast<int>(dummy_pict.data[2] - dummy_pict.data[1]);
        auto h2 = size2 / dummy_pict.linesize[1];

        desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0], height, 1));
        desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[1], h2, 1));
        desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[2], h2, 1));

        if (desc.format == core::pixel_format::ycbcra)
        desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[3], height, 1));

        return desc;
    }
    default:
        desc.format = core::pixel_format::invalid;
        return desc;
    }
}

}
}