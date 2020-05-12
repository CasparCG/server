#include "av_util.h"

#include "av_assert.h"

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <tbb/parallel_for.h>

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

core::mutable_frame make_frame(void*                    tag,
                               core::frame_factory&     frame_factory,
                               std::shared_ptr<AVFrame> video,
                               std::shared_ptr<AVFrame> audio)
{
    std::vector<int> data_map; // TODO(perf) when using data_map, avoid uploading duplicate planes

    const auto pix_desc =
        video ? pixel_format_desc(static_cast<AVPixelFormat>(video->format), video->width, video->height, data_map)
              : core::pixel_format_desc(core::pixel_format::invalid);

    auto frame = frame_factory.create_frame(tag, pix_desc);

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

    if (audio) {
        // TODO This is a bit of a hack
        frame.audio_data() = std::vector<int32_t>(audio->nb_samples * 8, 0);
        auto dst           = frame.audio_data().data();
        auto src           = reinterpret_cast<int32_t*>(audio->data[0]);
        tbb::parallel_for(0, audio->nb_samples, [&](int i) {
            for (auto j = 0; j < std::min(8, audio->channels); ++j) {
                dst[i * 8 + j] = src[i * audio->channels + j];
            }
        });
    }

    return frame;
}

core::pixel_format get_pixel_format(AVPixelFormat pix_fmt)
{
    switch (pix_fmt) {
        case AV_PIX_FMT_GRAY8:
            return core::pixel_format::gray;
        case AV_PIX_FMT_RGB24:
            return core::pixel_format::rgb;
        case AV_PIX_FMT_BGR24:
            return core::pixel_format::bgr;
        case AV_PIX_FMT_BGRA:
            return core::pixel_format::bgra;
        case AV_PIX_FMT_ARGB:
            return core::pixel_format::argb;
        case AV_PIX_FMT_RGBA:
            return core::pixel_format::rgba;
        case AV_PIX_FMT_ABGR:
            return core::pixel_format::abgr;
        case AV_PIX_FMT_YUV444P:
            return core::pixel_format::ycbcr;
        case AV_PIX_FMT_YUV422P:
            return core::pixel_format::ycbcr;
        case AV_PIX_FMT_YUV420P:
            return core::pixel_format::ycbcr;
        case AV_PIX_FMT_YUV411P:
            return core::pixel_format::ycbcr;
        case AV_PIX_FMT_YUV410P:
            return core::pixel_format::ycbcr;
        case AV_PIX_FMT_YUVA420P:
            return core::pixel_format::ycbcra;
        case AV_PIX_FMT_YUVA422P:
            return core::pixel_format::ycbcra;
        case AV_PIX_FMT_YUVA444P:
            return core::pixel_format::ycbcra;
        case AV_PIX_FMT_UYVY422:
            return core::pixel_format::uyvy;
        default:
            return core::pixel_format::invalid;
    }
}

core::pixel_format_desc pixel_format_desc(AVPixelFormat pix_fmt, int width, int height, std::vector<int>& data_map)
{
    // Get linesizes
    AVPicture dummy_pict;
    avpicture_fill(&dummy_pict, nullptr, pix_fmt, width, height);

    core::pixel_format_desc desc = get_pixel_format(pix_fmt);

    switch (desc.format) {
        case core::pixel_format::gray:
        case core::pixel_format::luma: {
            desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0], height, 1));
            return desc;
        }
        case core::pixel_format::bgr:
        case core::pixel_format::rgb: {
            desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0] / 3, height, 3));
            return desc;
        }
        case core::pixel_format::bgra:
        case core::pixel_format::argb:
        case core::pixel_format::rgba:
        case core::pixel_format::abgr: {
            desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0] / 4, height, 4));
            return desc;
        }
        case core::pixel_format::ycbcr:
        case core::pixel_format::ycbcra: {
            // Find chroma height
            auto size2 = static_cast<int>(dummy_pict.data[2] - dummy_pict.data[1]);
            auto h2    = size2 / dummy_pict.linesize[1];

            desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0], height, 1));
            desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[1], h2, 1));
            desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[2], h2, 1));

            if (desc.format == core::pixel_format::ycbcra)
                desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[3], height, 1));

            return desc;
        }
        case core::pixel_format::uyvy: {
            desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0] / 2, height, 2));
            desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0] / 4, height, 4));

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
        case core::pixel_format::luma:
            av_frame->format = AVPixelFormat::AV_PIX_FMT_GRAY8;
            break;
        case core::pixel_format::ycbcr: {
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
        case core::pixel_format::count:
        case core::pixel_format::invalid:
            break;
    }

    FF(av_frame_get_buffer(av_frame.get(), 32));

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
    av_frame->channels       = format_desc.audio_channels;
    av_frame->channel_layout = av_get_default_channel_layout(av_frame->channels);
    av_frame->sample_rate    = format_desc.audio_sample_rate;
    av_frame->format         = AV_SAMPLE_FMT_S32;
    av_frame->nb_samples     = static_cast<int>(buffer.size() / av_frame->channels);
    FF(av_frame_get_buffer(av_frame.get(), 32));
    std::memcpy(av_frame->data[0], buffer.data(), buffer.size() * sizeof(buffer.data()[0]));

    return av_frame;
}

int graph_execute(struct AVFilterContext* ctx, avfilter_action_func* func, void* arg, int* ret, int count)
{
    if (count == 0) {
        return 0;
    }

    tbb::parallel_for(0, count, [&](int n) {
        int r = func(ctx, arg, n, count);
        if (ret) {
            ret[n] = r;
        }
    });

    return 0;
}

int codec_execute(AVCodecContext* c,
                  int (*func)(AVCodecContext* c2, void* arg),
                  void* arg2,
                  int*  ret,
                  int   count,
                  int   size)
{
    tbb::parallel_for(0, count, 1, [&](int i) {
        int r = func(c, (char*)arg2 + i * size);
        if (ret) {
            ret[i] = r;
        }
    });

    return 0;
}

int codec_execute2(AVCodecContext* c,
                   int (*func)(AVCodecContext* c2, void* arg, int jobnr, int threadnr),
                   void* arg2,
                   int*  ret,
                   int   count)
{
    std::array<std::vector<int>, 128> jobs;

    for (int jobnr = 0; jobnr < count; ++jobnr) {
        jobs[jobnr * c->thread_count / count].push_back(jobnr);
    }

    tbb::parallel_for<int>(0, c->thread_count, [&](int threadnr) {
        for (auto jobnr : jobs[threadnr]) {
            int r = func(c, arg2, jobnr, threadnr);
            if (ret) {
                ret[jobnr] = r;
            }
        }
    });

    return 0;
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

}} // namespace caspar::ffmpeg
