#include "av_util.h"

#include "av_assert.h"

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

std::shared_ptr<AVFrame> alloc_frame()
{
    const auto frame = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *ptr) { av_frame_free(&ptr); });
    if (!frame)
        FF_RET(AVERROR(ENOMEM), "av_frame_alloc");
    return frame;
}

std::shared_ptr<AVPacket> alloc_packet()
{
    const auto packet = std::shared_ptr<AVPacket>(av_packet_alloc(), [](AVPacket *ptr) { av_packet_free(&ptr); });
    if (!packet)
        FF_RET(AVERROR(ENOMEM), "av_packet_alloc");
    return packet;
}

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

std::shared_ptr<AVFrame> make_av_video_frame(const core::const_frame& frame)
{
    auto av_frame = alloc_frame();

    auto pix_desc = frame.pixel_format_desc();

    auto planes = pix_desc.planes;
    auto format = pix_desc.format;

    av_frame->width = planes[0].width;
    av_frame->height = planes[0].height;

    switch (format) {
    case core::pixel_format::rgb:
        av_frame->format = AVPixelFormat::AV_PIX_FMT_RGB24;
        break;
    case core::pixel_format::bgr:
        av_frame->format = AVPixelFormat::AV_PIX_FMT_BGR24;
        break;
    case core::pixel_format::rgba:
        av_frame->format = AVPixelFormat::AV_PIX_FMT_RGBA;
        break;
    case core::pixel_format::argb:
        av_frame->format = AVPixelFormat::AV_PIX_FMT_ARGB;
        break;
    case core::pixel_format::bgra:
        av_frame->format = AVPixelFormat::AV_PIX_FMT_BGRA;
        break;
    case core::pixel_format::abgr:
        av_frame->format = AVPixelFormat::AV_PIX_FMT_ABGR;
        break;
    case core::pixel_format::gray:
        av_frame->format = AVPixelFormat::AV_PIX_FMT_GRAY8;
        break;
    case core::pixel_format::ycbcr:
    {
        int y_w = planes[0].width;
        int y_h = planes[0].height;
        int c_w = planes[1].width;
        int c_h = planes[1].height;

        if (c_h == y_h && c_w == y_w)
            av_frame->format = AVPixelFormat::AV_PIX_FMT_YUV444P;
        else if (c_h == y_h && c_w * 2 == y_w)
            av_frame->format = AVPixelFormat::AV_PIX_FMT_YUV422P;
        else if (c_h == y_h && c_w * 4 == y_w)
            av_frame->format = AVPixelFormat::AV_PIX_FMT_YUV411P;
        else if (c_h * 2 == y_h && c_w * 2 == y_w)
            av_frame->format = AVPixelFormat::AV_PIX_FMT_YUV420P;
        else if (c_h * 2 == y_h && c_w * 4 == y_w)
            av_frame->format = AVPixelFormat::AV_PIX_FMT_YUV410P;

        break;
    }
    case core::pixel_format::ycbcra:
        av_frame->format = AVPixelFormat::AV_PIX_FMT_YUVA420P;
        break;
    }

    FF(av_frame_get_buffer(av_frame.get(), 32));

    // TODO (perf) Avoid extra memcpy.
    for (int n = 0; n < planes.size(); ++n) {
        for (int y = 0; y < av_frame->height; ++y) {
            std::memcpy(av_frame->data[n] + y * av_frame->linesize[n], frame.image_data(n).data() + y * planes[n].linesize, planes[n].linesize);
        }
    }

    return av_frame;
}

std::shared_ptr<AVFrame> make_av_audio_frame(const core::const_frame& frame)
{
    auto av_frame = alloc_frame();

    const auto& buffer = frame.audio_data();

    // TODO (fix) Use sample_format_desc.
    av_frame->channels = 8;
    av_frame->channel_layout = av_get_default_channel_layout(av_frame->channels);
    av_frame->sample_rate = 48000;
    av_frame->format = AV_SAMPLE_FMT_S32;
    av_frame->nb_samples = static_cast<int>(buffer.size() / av_frame->channels);
    FF(av_frame_get_buffer(av_frame.get(), 32));
    std::memcpy(av_frame->data[0], buffer.data(), buffer.size() * sizeof(buffer.data()[0]));

    return av_frame;
}

}
}