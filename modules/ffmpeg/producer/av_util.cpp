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

core::pixel_format get_pixel_format(AVPixelFormat pix_fmt)
{
    switch (pix_fmt)
    {
    case AVPixelFormat::AV_PIX_FMT_GRAY8:       return core::pixel_format::gray;
    case AVPixelFormat::AV_PIX_FMT_RGB24:       return core::pixel_format::rgb;
    case AVPixelFormat::AV_PIX_FMT_BGR24:       return core::pixel_format::bgr;
    case AVPixelFormat::AV_PIX_FMT_BGRA:        return core::pixel_format::bgra;
    case AVPixelFormat::AV_PIX_FMT_ARGB:        return core::pixel_format::argb;
    case AVPixelFormat::AV_PIX_FMT_RGBA:        return core::pixel_format::rgba;
    case AVPixelFormat::AV_PIX_FMT_ABGR:        return core::pixel_format::abgr;
    case AVPixelFormat::AV_PIX_FMT_YUV444P:     return core::pixel_format::ycbcr;
    case AVPixelFormat::AV_PIX_FMT_YUV422P:     return core::pixel_format::ycbcr;
    case AVPixelFormat::AV_PIX_FMT_YUV420P:     return core::pixel_format::ycbcr;
    case AVPixelFormat::AV_PIX_FMT_YUV411P:     return core::pixel_format::ycbcr;
    case AVPixelFormat::AV_PIX_FMT_YUV410P:     return core::pixel_format::ycbcr;
    case AVPixelFormat::AV_PIX_FMT_YUVA420P:    return core::pixel_format::ycbcra;
    case AVPixelFormat::AV_PIX_FMT_YUVA422P:    return core::pixel_format::ycbcra;
    case AVPixelFormat::AV_PIX_FMT_YUVA444P:    return core::pixel_format::ycbcra;
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