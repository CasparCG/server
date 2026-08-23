#include "av_producer.h"

#include "av_input.h"
#include "video_strategy.h"

#include "../util/av_assert.h"
#include "../util/av_util.h"

#include <boost/exception/exception.hpp>
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/range/algorithm/rotate.hpp>
#include <boost/rational.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

#include <common/assert.h>
#include <common/diagnostics/graph.h>
#include <common/env.h>
#include <common/except.h>
#include <common/executor.h>
#include <common/os/thread.h>
#include <common/scope_exit.h>
#include <common/timer.h>

#include <core/frame/draw_frame.h>
#include <core/frame/frame_factory.h>
#include <core/monitor/monitor.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
}

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <deque>
#include <iomanip>
#include <memory>
#include <optional>
#include <queue>
#include <sstream>
#include <string>
#include <thread>

namespace caspar { namespace ffmpeg {

const AVRational TIME_BASE_Q = {1, AV_TIME_BASE};

struct Frame
{
    std::shared_ptr<AVFrame> video;
    std::shared_ptr<AVFrame> audio;
    core::draw_frame         frame;
    int64_t                  start_time  = AV_NOPTS_VALUE;
    int64_t                  pts         = AV_NOPTS_VALUE;
    int64_t                  duration    = 0;
    int64_t                  frame_count = 0;
};

AVPixelFormat get_pix_fmt_with_alpha(AVPixelFormat fmt)
{
    switch (fmt) {
        case AV_PIX_FMT_YUV420P:
            return AV_PIX_FMT_YUVA420P;
        case AV_PIX_FMT_YUV422P:
            return AV_PIX_FMT_YUVA422P;
        case AV_PIX_FMT_YUV444P:
            return AV_PIX_FMT_YUVA444P;
        default:
            break;
    }
    return fmt;
}

const AVCodec* get_decoder(AVCodecID codec_id)
{
    // enforce use of libvpx for vp8 and vp9 codecs to be able
    // to decode webm files with alpha channel
    const AVCodec* result = nullptr;
    if (codec_id == AV_CODEC_ID_VP9)
        result = avcodec_find_decoder_by_name("libvpx-vp9");
    else if (codec_id == AV_CODEC_ID_VP8)
        result = avcodec_find_decoder_by_name("libvpx");
    return result != nullptr ? result : avcodec_find_decoder(codec_id);
}

// TODO (fix) Handle ts discontinuities.
// TODO (feat) Forward options.

/// Whether a frame the decoder handed over carries anything, as opposed to the empty frame pop()
/// returns at end of file.
///
/// Not `data[0]`: a hardware frame keeps its payload wherever its API does, and VideoToolbox
/// leaves data[0] null and puts the CVPixelBuffer in data[3] — reading data[0] there reports end
/// of file for every picture in the file. Every real frame, hardware or software, arrives with a
/// buffer reference; only the end-of-file sentinel, a bare alloc_frame(), has none.
bool has_content(const AVFrame& frame) { return frame.buf[0] != nullptr; }

class Decoder
{
    Decoder(const Decoder&)            = delete;
    Decoder& operator=(const Decoder&) = delete;

    AVStream*         st       = nullptr;
    int64_t           next_pts = AV_NOPTS_VALUE;
    std::atomic<bool> eof      = {false};

    std::queue<std::shared_ptr<AVPacket>> input;
    mutable boost::mutex                  input_mutex;
    boost::condition_variable             input_cond;
    int                                   input_capacity = 2;

    std::queue<std::shared_ptr<AVFrame>> output;
    mutable boost::mutex                 output_mutex;
    boost::condition_variable            output_cond;
    int                                  output_capacity = 8;

    boost::thread thread;

    bool claimed_ = false;

  public:
    std::shared_ptr<AVCodecContext> ctx;

    Decoder() = default;

    /// `strategy`, when given, gets first refusal on the stream before the codec is opened —
    /// that is where a hardware strategy attaches its device context. Audio streams and
    /// strategies that decline are decoded the ordinary way.
    explicit Decoder(AVStream* stream, video_strategy* strategy = nullptr)
        : st(stream)
    {
        const auto codec = get_decoder(stream->codecpar->codec_id);

        if (!codec) {
            FF_RET(AVERROR_DECODER_NOT_FOUND, "avcodec_find_decoder");
        }

        ctx = std::shared_ptr<AVCodecContext>(avcodec_alloc_context3(codec),
                                              [](AVCodecContext* ptr) { avcodec_free_context(&ptr); });

        if (!ctx) {
            FF_RET(AVERROR(ENOMEM), "avcodec_alloc_context3");
        }

        FF(avcodec_parameters_to_context(ctx.get(), stream->codecpar));

        if (stream->metadata != NULL) {
            auto entry = av_dict_get(stream->metadata, "alpha_mode", NULL, AV_DICT_MATCH_CASE);
            if (entry != NULL && entry->value != NULL && *entry->value == '1')
                ctx->pix_fmt = get_pix_fmt_with_alpha(ctx->pix_fmt);
        }

        int thread_count = env::properties().get(L"configuration.ffmpeg.producer.threads", 0);
        FF(av_opt_set_int(ctx.get(), "threads", thread_count, 0));

        ctx->pkt_timebase = stream->time_base;

        if (ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
            ctx->framerate           = av_guess_frame_rate(nullptr, stream, nullptr);
            ctx->sample_aspect_ratio = av_guess_sample_aspect_ratio(nullptr, stream, nullptr);

            if (strategy) {
                claimed_ = strategy->open_decoder(*ctx, *codec);
                if (!claimed_) {
                    CASPAR_LOG(debug) << L"[ffmpeg] " << strategy->name() << L" video strategy declined stream "
                                      << stream->index << L"; decoding it in software.";
                }
            }
        } else if (ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
        }

        FF(avcodec_open2(ctx.get(), codec, nullptr));

        thread = boost::thread([this]() {
            try {
                while (!thread.interruption_requested()) {
                    auto av_frame = alloc_frame();
                    auto ret      = avcodec_receive_frame(ctx.get(), av_frame.get());

                    if (ret == AVERROR(EAGAIN)) {
                        std::shared_ptr<AVPacket> packet;
                        {
                            boost::unique_lock<boost::mutex> lock(input_mutex);
                            input_cond.wait(lock, [&]() { return !input.empty(); });
                            packet = std::move(input.front());
                            input.pop();
                        }
                        FF(avcodec_send_packet(ctx.get(), packet.get()));
                    } else if (ret == AVERROR_EOF) {
                        avcodec_flush_buffers(ctx.get());
                        av_frame->pts = next_pts;
                        next_pts      = AV_NOPTS_VALUE;
                        eof           = true;

                        {
                            boost::unique_lock<boost::mutex> lock(output_mutex);
                            output_cond.wait(lock, [&]() { return output.size() < output_capacity; });
                            output.push(std::move(av_frame));
                        }
                    } else {
                        FF_RET(ret, "avcodec_receive_frame");

                        // TODO: Maybe Fixed in:
                        // https://github.com/FFmpeg/FFmpeg/commit/33203a08e0a26598cb103508327a1dc184b27bc6
                        // NOTE This is a workaround for DVCPRO HD.
#if LIBAVCODEC_VERSION_MAJOR < 61
                        if (av_frame->width > 1024 && av_frame->interlaced_frame) {
                            av_frame->top_field_first = 1;
                        }
#else
                        if (av_frame->width > 1024 && (av_frame->flags & AV_FRAME_FLAG_INTERLACED)) {
                            av_frame->flags |= AV_FRAME_FLAG_TOP_FIELD_FIRST;
                        }
#endif

                        // TODO (fix) is this always best?
                        av_frame->pts = av_frame->best_effort_timestamp;

                        auto duration_pts = av_frame->duration;
                        if (duration_pts <= 0) {
                            if (ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
#if LIBAVCODEC_VERSION_MAJOR < 62
                                const int ticks_per_frame = ctx->ticks_per_frame;
#else
                                // https://github.com/FFmpeg/FFmpeg/commit/e930b834a928546f9cbc937f6633709053448232#diff-115616f8a2b59cab3aac4e7f4c8c31e69e94e7fcfa339b9f65b0bf34308aa80fR682
                                const int ticks_per_frame =
                                    (ctx->codec_descriptor && (ctx->codec_descriptor->props & AV_CODEC_PROP_FIELDS))
                                        ? 2
                                        : 1;
#endif
                                const auto ticks = av_stream_get_parser(st) ? av_stream_get_parser(st)->repeat_pict + 1
                                                                            : ticks_per_frame;
                                duration_pts     = static_cast<int64_t>(AV_TIME_BASE) * ctx->framerate.den * ticks /
                                               ctx->framerate.num / ticks_per_frame;
                                duration_pts = av_rescale_q(duration_pts, {1, AV_TIME_BASE}, st->time_base);
                            } else if (ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
                                duration_pts = av_rescale_q(av_frame->nb_samples, {1, ctx->sample_rate}, st->time_base);
                            }
                        }

                        if (duration_pts > 0) {
                            next_pts = av_frame->pts + duration_pts;
                        } else {
                            next_pts = AV_NOPTS_VALUE;
                        }

                        {
                            boost::unique_lock<boost::mutex> lock(output_mutex);
                            output_cond.wait(lock, [&]() { return output.size() < output_capacity; });
                            output.push(std::move(av_frame));
                        }
                    }
                }
            } catch (boost::thread_interrupted&) {
                // Do nothing...
            } catch (...) {
                eof = true;
                CASPAR_LOG_CURRENT_EXCEPTION();
            }
        });
    }

    ~Decoder()
    {
        try {
            if (thread.joinable()) {
                thread.interrupt();
                thread.join();
            }
        } catch (boost::thread_interrupted&) {
            // Do nothing...
        }
    }

    /// True when the video strategy took the stream before the codec was opened. It says
    /// nothing about whether the decoder actually ended up producing hardware frames — that is
    /// only known once it has chosen a pixel format (see ctx->hw_frames_ctx).
    bool claimed_by_strategy() const { return claimed_; }

    bool want_packet() const
    {
        if (eof) {
            return false;
        }

        {
            boost::lock_guard<boost::mutex> lock(input_mutex);
            return input.size() < input_capacity;
        }
    }

    void push(std::shared_ptr<AVPacket> packet)
    {
        if (eof) {
            return;
        }

        {
            boost::lock_guard<boost::mutex> lock(input_mutex);
            input.push(std::move(packet));
        }

        input_cond.notify_all();
    }

    std::shared_ptr<AVFrame> pop()
    {
        std::shared_ptr<AVFrame> frame;

        {
            boost::lock_guard<boost::mutex> lock(output_mutex);

            if (!output.empty()) {
                frame = std::move(output.front());
                output.pop();
            }
        }

        if (frame) {
            output_cond.notify_all();
        } else if (eof) {
            frame = alloc_frame();
        }

        return frame;
    }
};

struct Filter
{
    std::shared_ptr<AVFilterGraph>  graph;
    AVFilterContext*                sink = nullptr;
    std::map<int, AVFilterContext*> sources;
    std::shared_ptr<AVFrame>        frame;
    bool                            eof = false;

    /// Set on a video filter whose graph has not been built yet, because the frame that
    /// describes its source has not been decoded yet (see Impl::configure_video_filter). Such a
    /// filter has nothing to pull and is *not* at end of file; the producer's ordinary "waiting
    /// for a frame" path covers the gap, which is why there is no separate wait anywhere.
    bool pending = false;

    Filter() = default;

    /// `strategy` is the video strategy for AVMEDIA_TYPE_VIDEO and null for audio. It decides
    /// the deinterlacer, the formats the sink may end in, and — through the decoder it opened —
    /// whether the source carries hardware frames.
    /// `source_frame` is the first frame the video decoder produced, and is what the graph's
    /// video source is described from. It is null for audio, and for the rare video graph that
    /// reads more than one stream — see the comment where it is used.
    Filter(std::string                    filter_spec,
           const Input&                   input,
           std::map<int, Decoder>&        streams,
           int64_t                        start_time,
           AVMediaType                    media_type,
           const core::video_format_desc& format_desc,
           video_strategy*                strategy,
           const AVFrame*                 source_frame = nullptr)
    {
        if (media_type == AVMEDIA_TYPE_VIDEO) {
            CASPAR_VERIFY(strategy);

            if (filter_spec.empty()) {
                filter_spec = "null";
            }

            auto deint = u8(
                env::properties().get<std::wstring>(L"configuration.ffmpeg.producer.auto-deinterlace", L"interlaced"));

            if (deint != "none") {
                auto deinterlacer = strategy->deinterlace_filter(deint);
                if (!deinterlacer.empty()) {
                    filter_spec += "," + deinterlacer;
                }
            }

            filter_spec += (boost::format(",fps=fps=%d/%d:start_time=%f") %
                            (format_desc.framerate.numerator() * format_desc.field_count) %
                            format_desc.framerate.denominator() % (static_cast<double>(start_time) / AV_TIME_BASE))
                               .str();
        } else if (media_type == AVMEDIA_TYPE_AUDIO) {
            if (filter_spec.empty()) {
                filter_spec = "anull";
            }

            // Find first audio stream to get a time_base for the first_pts calculation
            AVRational tb = {1, format_desc.audio_sample_rate};
            for (auto n = 0U; n < input->nb_streams; ++n) {
                const auto st             = input->streams[n];
                const auto codec_channels = st->codecpar->ch_layout.nb_channels;
                if (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && codec_channels > 0) {
                    tb = {1, st->codecpar->sample_rate};
                    break;
                }
            }
            filter_spec += (boost::format(",aresample=async=1000:first_pts=%d:min_comp=0.01:osr=%d,"
                                          "asetnsamples=n=1024:p=0") %
                            av_rescale_q(start_time, TIME_BASE_Q, tb) % format_desc.audio_sample_rate)
                               .str();
        }

        AVFilterInOut* outputs = nullptr;
        AVFilterInOut* inputs  = nullptr;

        CASPAR_SCOPE_EXIT
        {
            avfilter_inout_free(&inputs);
            avfilter_inout_free(&outputs);
        };

        int video_input_count = 0;
        int audio_input_count = 0;
        {
            auto graph2 = avfilter_graph_alloc();
            if (!graph2) {
                FF_RET(AVERROR(ENOMEM), "avfilter_graph_alloc");
            }

            CASPAR_SCOPE_EXIT
            {
                avfilter_graph_free(&graph2);
                avfilter_inout_free(&inputs);
                avfilter_inout_free(&outputs);
            };

            FF(avfilter_graph_parse2(graph2, filter_spec.c_str(), &inputs, &outputs));

            for (auto cur = inputs; cur; cur = cur->next) {
                const auto type = avfilter_pad_get_type(cur->filter_ctx->input_pads, cur->pad_idx);
                if (type == AVMEDIA_TYPE_VIDEO) {
                    video_input_count += 1;
                } else if (type == AVMEDIA_TYPE_AUDIO) {
                    audio_input_count += 1;
                }
            }
        }

        std::deque<AVStream*> video_streams, audio_streams;
        for (auto n = 0U; n < input->nb_streams; ++n) {
            const auto st = input->streams[n];

            const auto codec_channels = st->codecpar->ch_layout.nb_channels;

            auto disposition = st->disposition;
            if (!disposition || disposition == AV_DISPOSITION_DEFAULT) {
                switch (st->codecpar->codec_type) {
                    case AVMEDIA_TYPE_VIDEO:
                        video_streams.push_back(st);
                        break;
                    case AVMEDIA_TYPE_AUDIO:
                        if (codec_channels > 0) {
                            audio_streams.push_back(st);
                        }
                        break;
                    default:
                        // ignore stream
                        break;
                }
            }
        }

        if (audio_input_count == 1) {
            // TODO (fix) Use some form of stream meta data to do this.
            // https://github.com/CasparCG/server/issues/833
            if (audio_streams.size() > 1) {
                filter_spec = (boost::format("amerge=inputs=%d,") % audio_streams.size()).str() + filter_spec;
            }
        }

        if (video_input_count == 1) {
            std::stable_sort(video_streams.begin(), video_streams.end(), [&](AVStream* lhs, AVStream* rhs) {
                return lhs->codecpar->height > rhs->codecpar->height;
            });

            auto same_properties = [](AVStream* lhs, AVStream* rhs) {
                auto lcp = lhs->codecpar;
                auto rcp = rhs->codecpar;
                return lcp->width == rcp->width && lcp->height == rcp->height &&
                       lcp->sample_aspect_ratio.num == rcp->sample_aspect_ratio.num &&
                       lcp->sample_aspect_ratio.den == rcp->sample_aspect_ratio.den &&
                       lcp->field_order == rcp->field_order;
            };

            // TODO (fix) Use some form of stream meta data to do this.
            // https://github.com/CasparCG/server/issues/832
            if (video_streams.size() > 1 && same_properties(video_streams[0], video_streams[1])) {
                if (video_streams.size() == 2 || !same_properties(video_streams[0], video_streams[2])) {
                    filter_spec = "alphamerge," + filter_spec;
                }
            }
        }

        graph = std::shared_ptr<AVFilterGraph>(avfilter_graph_alloc(),
                                               [](AVFilterGraph* ptr) { avfilter_graph_free(&ptr); });

        if (!graph) {
            FF_RET(AVERROR(ENOMEM), "avfilter_graph_alloc");
        }

        FF(avfilter_graph_parse2(graph.get(), filter_spec.c_str(), &inputs, &outputs));

        // inputs
        {
            for (auto cur = inputs; cur; cur = cur->next) {
                const auto type = avfilter_pad_get_type(cur->filter_ctx->input_pads, cur->pad_idx);

                AVStream* stream;

                switch (type) {
                    case AVMEDIA_TYPE_VIDEO:
                        if (video_streams.empty()) {
                            graph = nullptr;
                            return;
                        }
                        // TODO find stream based on link name
                        stream = video_streams.front();
                        video_streams.pop_front();
                        break;
                    case AVMEDIA_TYPE_AUDIO:
                        if (audio_streams.empty()) {
                            graph = nullptr;
                            return;
                        }
                        // TODO find stream based on link name
                        stream = audio_streams.front();
                        audio_streams.pop_front();
                        break;
                    default:
                        CASPAR_THROW_EXCEPTION(ffmpeg_error_t()
                                               << boost::errinfo_errno(EINVAL)
                                               << msg_info_t("only video and audio filters supported"));
                }

                auto it = streams.find(stream->index);
                if (it == streams.end()) {
                    it = streams
                             .emplace(std::piecewise_construct,
                                      std::forward_as_tuple(stream->index),
                                      std::forward_as_tuple(stream, type == AVMEDIA_TYPE_VIDEO ? strategy : nullptr))
                             .first;
                }

                auto st = it->second.ctx;

                if (st->codec_type == AVMEDIA_TYPE_VIDEO) {
                    auto name = (boost::format("in_%d") % stream->index).str();

                    // Described through AVBufferSrcParameters rather than an option string,
                    // because hw_frames_ctx has no textual form: a hardware decoder's frames
                    // belong to a pool the source has to be handed before the graph is
                    // configured. It is null for software decoding, which leaves this exactly
                    // the plain buffer source it has always been.
                    auto* source =
                        FFMEM(avfilter_graph_alloc_filter(graph.get(), avfilter_get_by_name("buffer"), name.c_str()));

                    auto* params = av_buffersrc_parameters_alloc();
                    if (!params) {
                        FF_RET(AVERROR(ENOMEM), "av_buffersrc_parameters_alloc");
                    }
                    CASPAR_SCOPE_EXIT { av_free(params); };

                    // Describe the source from the frame the decoder actually produced, never
                    // from the codec context. The context cannot be trusted for any of this: a
                    // hardware decoder settles its pixel format and frames context inside
                    // get_format(), which FFmpeg runs on the Decoder's own thread when the first
                    // packet arrives, so reading the context is both premature and a data race.
                    // The frame carries the finished answer.
                    //
                    // source_frame is null only when this graph reads a video stream the producer
                    // could not wait on — several video streams feeding one graph, which a user
                    // FILTER can arrange and which is software-only. There the container's
                    // description is exact, because nothing negotiates a format.
                    const auto sar = source_frame ? source_frame->sample_aspect_ratio : st->sample_aspect_ratio;

                    params->format        = source_frame ? source_frame->format : st->pix_fmt;
                    params->width         = source_frame ? source_frame->width : st->width;
                    params->height        = source_frame ? source_frame->height : st->height;
                    params->hw_frames_ctx = source_frame ? source_frame->hw_frames_ctx : st->hw_frames_ctx;

                    // Timing is ours: we set both before the codec was opened, and the decoder
                    // never touches them, so there is nothing to learn from the frame.
                    params->time_base = st->pkt_timebase;

#if LIBAVFILTER_VERSION_INT >= AV_VERSION_INT(9, 16, 100)
                    // Declare the colour properties up front. Left unset, the source configures its
                    // link as "unspecified" and then complains that every incoming frame changes
                    // them, which also denies downstream filters the range/matrix they need.
                    params->color_space = source_frame ? source_frame->colorspace : st->colorspace;
                    params->color_range = source_frame ? source_frame->color_range : st->color_range;
#endif

                    if (sar.num > 0 && sar.den > 0) {
                        params->sample_aspect_ratio = sar;
                    }

                    if (st->framerate.num > 0 && st->framerate.den > 0) {
                        params->frame_rate = st->framerate;
                    }

                    FF(av_buffersrc_parameters_set(source, params));
                    FF(avfilter_init_str(source, nullptr));

                    FF(avfilter_link(source, 0, cur->filter_ctx, cur->pad_idx));
                    sources.emplace(stream->index, source);
                } else if (st->codec_type == AVMEDIA_TYPE_AUDIO) {
                    char channel_layout[128];
                    FF(av_channel_layout_describe(&st->ch_layout, channel_layout, sizeof(channel_layout)));

                    auto args = (boost::format("time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%#x") %
                                 st->pkt_timebase.num % st->pkt_timebase.den % st->sample_rate %
                                 av_get_sample_fmt_name(st->sample_fmt) % channel_layout)
                                    .str();
                    auto name = (boost::format("in_%d") % stream->index).str();

                    AVFilterContext* source = nullptr;
                    FF(avfilter_graph_create_filter(
                        &source, avfilter_get_by_name("abuffer"), name.c_str(), args.c_str(), nullptr, graph.get()));
                    FF(avfilter_link(source, 0, cur->filter_ctx, cur->pad_idx));
                    sources.emplace(stream->index, source);
                } else {
                    CASPAR_THROW_EXCEPTION(ffmpeg_error_t() << boost::errinfo_errno(EINVAL)
                                                            << msg_info_t("invalid filter input media type"));
                }
            }
        }

        if (media_type == AVMEDIA_TYPE_VIDEO) {
            sink = create_buffersink(graph.get(), "out", strategy->sink_formats());
        } else if (media_type == AVMEDIA_TYPE_AUDIO) {
            sink = create_abuffersink(
                graph.get(), "out", std::array{AV_SAMPLE_FMT_S32}, std::array{format_desc.audio_sample_rate});
        } else {
            CASPAR_THROW_EXCEPTION(ffmpeg_error_t()
                                   << boost::errinfo_errno(EINVAL) << msg_info_t("invalid output media type"));
        }

        // output
        {
            const auto cur = outputs;

            if (!cur || cur->next) {
                CASPAR_THROW_EXCEPTION(ffmpeg_error_t() << boost::errinfo_errno(EINVAL)
                                                        << msg_info_t("invalid filter graph output count"));
            }

            if (avfilter_pad_get_type(cur->filter_ctx->output_pads, cur->pad_idx) != media_type) {
                CASPAR_THROW_EXCEPTION(ffmpeg_error_t() << boost::errinfo_errno(EINVAL)
                                                        << msg_info_t("invalid filter output media type"));
            }

            FF(avfilter_link(cur->filter_ctx, cur->pad_idx, sink, 0));
        }

        FF(avfilter_graph_config(graph.get(), nullptr));

        CASPAR_LOG(debug) << avfilter_graph_dump(graph.get(), nullptr);
    }

    bool operator()(int nb_samples = -1)
    {
        if (frame || eof || pending) {
            return false;
        }

        if (!sink || sources.empty()) {
            eof   = true;
            frame = nullptr;
            return true;
        }

        auto av_frame = alloc_frame();
        auto ret      = nb_samples >= 0 ? av_buffersink_get_samples(sink, av_frame.get(), nb_samples)
                                        : av_buffersink_get_frame(sink, av_frame.get());

        if (ret == AVERROR(EAGAIN)) {
            return false;
        }
        if (ret == AVERROR_EOF) {
            eof   = true;
            frame = nullptr;
            return true;
        }
        FF_RET(ret, "av_buffersink_get_frame");
        frame = std::move(av_frame);
        return true;
    }
};

struct AVProducer::Impl
{
    caspar::core::monitor::state state_;
    mutable boost::mutex         state_mutex_;

    spl::shared_ptr<diagnostics::graph> graph_;

    const std::shared_ptr<core::frame_factory> frame_factory_;
    const core::video_format_desc              format_desc_;
    const AVRational                           format_tb_;
    const std::string                          name_;
    const std::string                          path_;

    Input                  input_;
    std::map<int, Decoder> decoders_;
    Filter                 video_filter_;
    Filter                 audio_filter_;

    /// How video is decoded, filtered and handed to the mixer: the candidates in preference
    /// order, hardware first and the CPU one always last, and which of them is in use. A
    /// candidate that turns out not to fit the file is abandoned for the rest of this producer's
    /// life and the next one tried (see reset()), so this only ever moves forwards.
    std::vector<std::shared_ptr<video_strategy>> video_candidates_;
    size_t                                       video_candidate_ = 0;

    /// Where the current reset() started. Kept because the video graph is built later than
    /// reset() returns, and a strategy that falls through has to rewind the input to here.
    int64_t reset_time_ = 0;

    const std::shared_ptr<video_strategy>& video() const { return video_candidates_[video_candidate_]; }

    std::map<int, std::vector<AVFilterContext*>> sources_;

    std::atomic<int64_t> start_{AV_NOPTS_VALUE};
    std::atomic<int64_t> duration_{AV_NOPTS_VALUE};
    std::atomic<int64_t> input_duration_{AV_NOPTS_VALUE};
    std::atomic<int64_t> seek_{AV_NOPTS_VALUE};
    std::atomic<bool>    loop_{false};

    std::string afilter_;
    std::string vfilter_;

    int                              seekable_ = 2;
    core::frame_geometry::scale_mode scale_mode_;
    int64_t                          frame_count_    = 0;
    bool                             frame_flush_    = true;
    int64_t                          frame_time_     = AV_NOPTS_VALUE;
    int64_t                          frame_duration_ = AV_NOPTS_VALUE;
    core::draw_frame                 frame_;

    std::deque<Frame>         buffer_;
    mutable boost::mutex      buffer_mutex_;
    boost::condition_variable buffer_cond_;
    std::atomic<bool>         buffer_eof_{false};
    int                       buffer_capacity_ = static_cast<int>(format_desc_.fps) / 4;

    std::optional<caspar::executor> video_executor_;
    std::optional<caspar::executor> audio_executor_;

    int latency_ = 0;

    boost::thread thread_;

    Impl(std::shared_ptr<core::frame_factory> frame_factory,
         core::video_format_desc              format_desc,
         std::string                          name,
         std::string                          path,
         std::string                          vfilter,
         std::string                          afilter,
         std::optional<int64_t>               start,
         std::optional<int64_t>               seek,
         std::optional<int64_t>               duration,
         bool                                 loop,
         int                                  seekable,
         core::frame_geometry::scale_mode     scale_mode)
        : frame_factory_(frame_factory)
        , format_desc_(format_desc)
        , format_tb_({format_desc.duration, format_desc.time_scale * format_desc.field_count})
        , name_(name)
        , path_(path)
        , input_(path, graph_, seekable >= 0 && seekable < 2 ? std::optional<bool>(false) : std::optional<bool>())
        , start_(start ? av_rescale_q(*start, format_tb_, TIME_BASE_Q) : AV_NOPTS_VALUE)
        , duration_(duration ? av_rescale_q(*duration, format_tb_, TIME_BASE_Q) : AV_NOPTS_VALUE)
        , loop_(loop)
        , afilter_(afilter)
        , vfilter_(vfilter)
        , seekable_(seekable)
        , scale_mode_(scale_mode)
        , video_executor_(L"video-executor")
        , audio_executor_(L"audio-executor")
    {
        video_candidates_ = build_video_strategies();

        diagnostics::register_graph(graph_);
        graph_->set_color("underflow", diagnostics::color(0.6f, 0.3f, 0.9f));
        graph_->set_color("frame-time", diagnostics::color(0.0f, 1.0f, 0.0f));
        graph_->set_color("decode-time", diagnostics::color(0.0f, 1.0f, 1.0f));
        graph_->set_color("buffer", diagnostics::color(1.0f, 1.0f, 0.0f));

        state_["file/name"] = u8(name_);
        state_["file/path"] = u8(path_);
        state_["loop"]      = loop;
        update_state();

        CASPAR_LOG(debug) << print() << " seekable: " << seekable_;

        thread_ = boost::thread([=, this] {
            try {
                run(seek);
            } catch (boost::thread_interrupted&) {
                // Do nothing...
            } catch (ffmpeg::ffmpeg_error_t& ex) {
                if (auto errn = boost::get_error_info<ffmpeg_errn_info>(ex)) {
                    if (*errn == AVERROR_EXIT) {
                        return;
                    }
                }
                CASPAR_LOG_CURRENT_EXCEPTION();
            } catch (...) {
                CASPAR_LOG_CURRENT_EXCEPTION();
            }
        });
    }

    ~Impl()
    {
        input_.abort();

        try {
            if (thread_.joinable()) {
                thread_.interrupt();
                thread_.join();
            }
        } catch (boost::thread_interrupted&) {
            // Do nothing...
        }

        video_executor_.reset();
        audio_executor_.reset();

        CASPAR_LOG(debug) << print() << " Joined";
    }

    void run(std::optional<int64_t> firstSeek)
    {
        std::vector<int> audio_cadence = format_desc_.audio_cadence;

        input_.reset();
        {
            core::monitor::state streams;
            for (auto n = 0UL; n < input_->nb_streams; ++n) {
                auto st                             = input_->streams[n];
                auto framerate                      = av_guess_frame_rate(nullptr, st, nullptr);
                streams[std::to_string(n) + "/fps"] = {framerate.num, framerate.den};
            }

            boost::lock_guard<boost::mutex> lock(state_mutex_);
            state_["file/streams"] = streams;
        }

        if (input_duration_ == AV_NOPTS_VALUE) {
            input_duration_ = input_->duration;
        }

        {
            const auto start = start_.load();
            if (duration_ == AV_NOPTS_VALUE && input_->duration > 0) {
                if (start != AV_NOPTS_VALUE) {
                    duration_ = input_->duration - start;
                } else {
                    duration_ = input_->duration;
                }
            }

            const auto firstStart = firstSeek ? av_rescale_q(*firstSeek, format_tb_, TIME_BASE_Q) : start;
            if (firstStart != AV_NOPTS_VALUE) {
                seek_internal(firstStart);
            } else {
                reset(input_->start_time != AV_NOPTS_VALUE ? input_->start_time : 0);
            }
        }

        set_thread_name(L"[ffmpeg::av_producer]");

        boost::range::rotate(audio_cadence, std::end(audio_cadence) - 1);

        Frame frame;
        timer frame_timer;
        timer decode_timer;

        int warning_debounce = 0;

        while (!thread_.interruption_requested()) {
            {
                const auto seek = seek_.exchange(AV_NOPTS_VALUE);

                if (seek != AV_NOPTS_VALUE) {
                    seek_internal(seek);
                    frame = Frame{};
                    continue;
                }
            }

            {
                // TODO (perf) seek as soon as input is past duration or eof.

                auto start    = start_.load();
                auto duration = duration_.load();

                start       = start != AV_NOPTS_VALUE ? start : 0;
                auto end    = duration != AV_NOPTS_VALUE ? start + duration : INT64_MAX;
                auto time   = frame.pts != AV_NOPTS_VALUE ? frame.pts + frame.duration : 0;
                buffer_eof_ = (video_filter_.eof && audio_filter_.eof) ||
                              av_rescale_q(time, TIME_BASE_Q, format_tb_) >= av_rescale_q(end, TIME_BASE_Q, format_tb_);

                if (buffer_eof_) {
                    if (loop_ && frame_count_ > 2) {
                        frame = Frame{};
                        seek_internal(start);
                    } else {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                    // TODO (fix) Limit live polling due to bugs.
                    continue;
                }
            }

            bool progress = false;
            {
                progress |= schedule();

                // The video graph is described by the first decoded frame, so it is built here
                // rather than in reset() — by which point the decoder has had packets. Until it
                // exists video_filter_ yields nothing and reports no end of file, so the loop
                // below simply waits, exactly as it waits for any frame that is not ready yet.
                if (video_filter_.pending) {
                    progress |= configure_video_filter();
                }

                std::vector<std::future<bool>> futures;

                if (!video_filter_.frame) {
                    futures.push_back(video_executor_->begin_invoke([&]() { return video_filter_(); }));
                }

                if (!audio_filter_.frame) {
                    futures.push_back(audio_executor_->begin_invoke([&]() { return audio_filter_(audio_cadence[0]); }));
                }

                for (auto& future : futures) {
                    progress |= future.get();
                }
            }

            if ((!video_filter_.frame && !video_filter_.eof) || (!audio_filter_.frame && !audio_filter_.eof)) {
                if (!progress) {
                    if (warning_debounce++ % 500 == 100) {
                        if (!video_filter_.frame && !video_filter_.eof) {
                            CASPAR_LOG(warning) << print() << " Waiting for video frame...";
                        } else if (!audio_filter_.frame && !audio_filter_.eof) {
                            CASPAR_LOG(warning) << print() << " Waiting for audio frame...";
                        } else {
                            CASPAR_LOG(warning) << print() << " Waiting for frame...";
                        }
                    }

                    // TODO (perf): Avoid live loop.
                    std::this_thread::sleep_for(std::chrono::milliseconds(warning_debounce > 25 ? 20 : 5));
                }
                continue;
            }

            warning_debounce = 0;

            // TODO (fix)
            // if (start_ != AV_NOPTS_VALUE && frame.pts < start_) {
            //    seek_internal(start_);
            //    continue;
            //}

            const auto start_time = input_->start_time != AV_NOPTS_VALUE ? input_->start_time : 0;

            if (video_filter_.frame) {
                frame.video      = std::move(video_filter_.frame);
                const auto tb    = av_buffersink_get_time_base(video_filter_.sink);
                const auto fr    = av_buffersink_get_frame_rate(video_filter_.sink);
                frame.start_time = start_time;
                frame.pts        = av_rescale_q(frame.video->pts, tb, TIME_BASE_Q) - start_time;
                frame.duration   = av_rescale_q(1, av_inv_q(fr), TIME_BASE_Q);
            }

            if (audio_filter_.frame) {
                frame.audio      = std::move(audio_filter_.frame);
                const auto tb    = av_buffersink_get_time_base(audio_filter_.sink);
                const auto sr    = av_buffersink_get_sample_rate(audio_filter_.sink);
                frame.start_time = start_time;
                frame.pts        = av_rescale_q(frame.audio->pts, tb, TIME_BASE_Q) - start_time;
                frame.duration   = av_rescale_q(frame.audio->nb_samples, {1, sr}, TIME_BASE_Q);
            }

            frame.frame =
                video()->make_frame(this, frame.video, frame.audio, get_color_space(frame.video), scale_mode_);
            frame.frame_count = frame_count_++;

            graph_->set_value("decode-time", decode_timer.elapsed() * format_desc_.fps * 0.5);

            {
                boost::unique_lock<boost::mutex> buffer_lock(buffer_mutex_);
                buffer_cond_.wait(buffer_lock, [&] { return buffer_.size() < buffer_capacity_; });
                if (seek_ == AV_NOPTS_VALUE) {
                    buffer_.push_back(frame);
                }
            }

            if (format_desc_.field_count != 2 || frame_count_ % 2 == 1) {
                // Update the frame-time every other frame when interlaced
                graph_->set_value("frame-time", frame_timer.elapsed() * format_desc_.hz * 0.5);
                frame_timer.restart();
            }

            decode_timer.restart();

            graph_->set_value("buffer", static_cast<double>(buffer_.size()) / static_cast<double>(buffer_capacity_));

            boost::range::rotate(audio_cadence, std::end(audio_cadence) - 1);
        }
    }

    void update_state()
    {
        graph_->set_text(u16(print()));
        boost::lock_guard<boost::mutex> lock(state_mutex_);
        state_["file/clip"] = {start().value_or(0) / format_desc_.fps, duration().value_or(0) / format_desc_.fps};
        state_["file/time"] = {time() / format_desc_.fps, file_duration().value_or(0) / format_desc_.fps};
        state_["loop"]      = loop_;
    }

    core::draw_frame prev_frame(const core::video_field field)
    {
        CASPAR_SCOPE_EXIT { update_state(); };

        // Don't start a new frame on the 2nd field
        if (field != core::video_field::b) {
            if (frame_flush_ || !frame_) {
                boost::lock_guard<boost::mutex> lock(buffer_mutex_);

                if (!buffer_.empty()) {
                    frame_          = buffer_[0].frame;
                    frame_time_     = buffer_[0].pts;
                    frame_duration_ = buffer_[0].duration;
                    frame_flush_    = false;
                }
            }
        }

        return core::draw_frame::still(frame_);
    }

    bool is_ready()
    {
        boost::lock_guard<boost::mutex> lock(buffer_mutex_);
        return !buffer_.empty() || frame_;
    }

    core::draw_frame next_frame(const core::video_field field)
    {
        CASPAR_SCOPE_EXIT { update_state(); };

        boost::lock_guard<boost::mutex> lock(buffer_mutex_);

        if (buffer_.empty() || (frame_flush_ && buffer_.size() < 4)) {
            auto start    = start_.load();
            auto duration = duration_.load();

            start    = start != AV_NOPTS_VALUE ? start : 0;
            auto end = duration != AV_NOPTS_VALUE ? start + duration : INT64_MAX;

            if (buffer_eof_ && !frame_flush_) {
                if (frame_time_ < end && frame_duration_ != AV_NOPTS_VALUE) {
                    frame_time_ += frame_duration_;
                } else if (frame_time_ < end) {
                    frame_time_ = input_duration_;
                }
                return core::draw_frame::still(frame_);
            }
            graph_->set_tag(diagnostics::tag_severity::WARNING, "underflow");
            latency_ += 1;
            return core::draw_frame{};
        }

        if (format_desc_.field_count == 2) {
            // Check if the next frame is the correct 'field'
            auto is_field_1 = (buffer_[0].frame_count % 2) == 0;
            if ((field == core::video_field::a && !is_field_1) || (field == core::video_field::b && is_field_1)) {
                graph_->set_tag(diagnostics::tag_severity::WARNING, "underflow");
                latency_ += 1;
                return core::draw_frame{};
            }
        }

        if (latency_ != -1) {
            CASPAR_LOG(warning) << print() << " Latency: " << latency_;
            latency_ = -1;
        }

        frame_          = buffer_[0].frame;
        frame_time_     = buffer_[0].pts;
        frame_duration_ = buffer_[0].duration;
        frame_flush_    = false;

        buffer_.pop_front();
        buffer_cond_.notify_all();

        graph_->set_value("buffer", static_cast<double>(buffer_.size()) / static_cast<double>(buffer_capacity_));

        return frame_;
    }

    void seek(int64_t time)
    {
        CASPAR_SCOPE_EXIT { update_state(); };

        seek_ = av_rescale_q(time, format_tb_, TIME_BASE_Q);

        {
            boost::lock_guard<boost::mutex> lock(buffer_mutex_);
            buffer_.clear();
            buffer_cond_.notify_all();
            graph_->set_value("buffer", static_cast<double>(buffer_.size()) / static_cast<double>(buffer_capacity_));
        }
    }

    int64_t time() const
    {
        if (frame_time_ == AV_NOPTS_VALUE) {
            // TODO (fix) How to handle NOPTS case?
            return 0;
        }

        return av_rescale_q(frame_time_, TIME_BASE_Q, format_tb_);
    }

    void loop(bool loop)
    {
        CASPAR_SCOPE_EXIT { update_state(); };

        loop_ = loop;
    }

    bool loop() const { return loop_; }

    void start(int64_t start)
    {
        CASPAR_SCOPE_EXIT { update_state(); };
        start_ = av_rescale_q(start, format_tb_, TIME_BASE_Q);
    }

    std::optional<int64_t> start() const
    {
        auto start = start_.load();
        return start != AV_NOPTS_VALUE ? av_rescale_q(start, TIME_BASE_Q, format_tb_) : std::optional<int64_t>();
    }

    void duration(int64_t duration)
    {
        CASPAR_SCOPE_EXIT { update_state(); };

        duration_ = av_rescale_q(duration, format_tb_, TIME_BASE_Q);
    }

    std::optional<int64_t> duration() const
    {
        const auto duration = duration_.load();
        if (duration == AV_NOPTS_VALUE) {
            return {};
        }
        return av_rescale_q(duration, TIME_BASE_Q, format_tb_);
    }

    std::optional<int64_t> file_duration() const
    {
        const auto input_duration = input_duration_.load();
        if (input_duration == AV_NOPTS_VALUE) {
            return {};
        }
        return av_rescale_q(input_duration, TIME_BASE_Q, format_tb_);
    }

  private:
    bool want_packet()
    {
        return std::any_of(decoders_.begin(), decoders_.end(), [](auto& p) { return p.second.want_packet(); });
    }

    bool schedule()
    {
        auto result = false;

        std::shared_ptr<AVPacket> packet;
        while (want_packet() && input_.try_pop(packet)) {
            result = true;

            if (!packet) {
                for (auto& p : decoders_) {
                    p.second.push(nullptr);
                }
            } else if (sources_.find(packet->stream_index) != sources_.end()) {
                auto it = decoders_.find(packet->stream_index);
                if (it != decoders_.end()) {
                    // TODO (fix): limit it->second.input.size()?
                    it->second.push(std::move(packet));
                }
            }
        }

        std::vector<int> eof;

        for (auto& p : sources_) {
            auto it = decoders_.find(p.first);
            if (it == decoders_.end()) {
                continue;
            }

            auto nb_requests = 0U;
            for (auto source : p.second) {
                nb_requests = std::max(nb_requests, av_buffersrc_get_nb_failed_requests(source));
            }

            if (nb_requests == 0) {
                continue;
            }

            auto frame = it->second.pop();
            if (!frame) {
                continue;
            }

            for (auto& source : p.second) {
                if (!has_content(*frame)) {
                    FF(av_buffersrc_close(source, frame->pts, 0));
                } else {
                    // TODO (fix) Guard against overflow?
                    FF(av_buffersrc_write_frame(source, frame.get()));
                }
                result = true;
            }

            // End Of File
            if (!has_content(*frame)) {
                eof.push_back(p.first);
            }
        }

        for (auto index : eof) {
            sources_.erase(index);
        }

        return result;
    }

    void seek_internal(int64_t time)
    {
        time = time != AV_NOPTS_VALUE ? time : 0;
        time = time + (input_->start_time != AV_NOPTS_VALUE ? input_->start_time : 0);

        // TODO (fix) Dont seek if time is close future.
        if (seekable_) {
            input_.seek(time);
        }
        frame_flush_ = true;
        frame_count_ = 0;
        buffer_eof_  = false;

        decoders_.clear();

        reset(time);
    }

    /// The video strategies this producer may use, best first. Hardware decoding is only a
    /// candidate when nothing about the request rules it out; everything else — no Vulkan
    /// accelerator on this channel, no Vulkan video on this GPU, no CUDA in this FFmpeg — is
    /// settled inside each strategy, which reports failure by not being created.
    ///
    /// Vulkan comes before CUDA because it decodes into the render device directly, while CUDA
    /// pays for a copy across the API boundary; CUDA earns its place on the codecs Vulkan video
    /// decode cannot do at all. On macOS neither exists — MoltenVK has no Vulkan video decode and
    /// Apple hardware has no CUDA — and VideoToolbox is the whole of hardware decoding there.
    /// The CPU strategy is always last and always present.
    std::vector<std::shared_ptr<video_strategy>> build_video_strategies() const
    {
        std::vector<std::shared_ptr<video_strategy>> candidates;

        const auto enabled = env::properties().get(L"configuration.ffmpeg.producer.hardware-decode", true);

        // A user-supplied video filter chain is a chain of software filters, so it pins the
        // producer to software decoding.
        if (enabled && vfilter_.empty()) {
#ifdef ENABLE_VULKAN
            if (auto vulkan = try_create_vulkan_video_strategy(frame_factory_)) {
                candidates.push_back(std::move(vulkan));
            }
            if (auto cuda = try_create_cuda_video_strategy(frame_factory_)) {
                candidates.push_back(std::move(cuda));
            }
#ifdef __APPLE__
            if (auto videotoolbox = try_create_videotoolbox_video_strategy(frame_factory_)) {
                candidates.push_back(std::move(videotoolbox));
            }
#endif
#endif
        }

        candidates.push_back(create_cpu_video_strategy(frame_factory_));
        return candidates;
    }

    /// Give up on the current strategy and move to the next. The CPU one is last, so this always
    /// lands somewhere.
    void next_video_strategy()
    {
        CASPAR_LOG(info) << print() << " " << u8(video()->name()) << " video decoding is not available for this file.";
        if (video_candidate_ + 1 < video_candidates_.size()) {
            ++video_candidate_;
            CASPAR_LOG(info) << print() << " Trying " << u8(video()->name()) << " video decoding instead.";
        }
    }

    /// The one video stream the graph will read, or null when the file has none or has several
    /// (which the Filter may combine with alphamerge — a software-only arrangement).
    AVStream* single_video_stream() const
    {
        AVStream* found = nullptr;
        for (auto n = 0U; n < input_->nb_streams; ++n) {
            auto st = input_->streams[n];
            if (st->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
                continue;
            }
            if (st->disposition && st->disposition != AV_DISPOSITION_DEFAULT) {
                continue;
            }
            if (found) {
                return nullptr;
            }
            found = st;
        }
        return found;
    }

    /// The video decoder for `stream`, opened with the strategy currently in hand.
    Decoder& open_video_decoder(AVStream& stream)
    {
        return decoders_
            .emplace(std::piecewise_construct,
                     std::forward_as_tuple(stream.index),
                     std::forward_as_tuple(&stream, video().get()))
            .first->second;
    }

    /// Point sources_ at whatever the filters currently expose, and drop the decoders nothing
    /// reads any more.
    ///
    /// A video stream whose graph is still pending is registered with no filter behind it: that
    /// is how its packets keep reaching its decoder while nothing yet consumes its frames.
    void rebuild_sources()
    {
        sources_.clear();
        for (auto& p : video_filter_.sources) {
            sources_[p.first].push_back(p.second);
        }
        for (auto& p : audio_filter_.sources) {
            sources_[p.first].push_back(p.second);
        }
        if (video_filter_.pending) {
            if (auto* stream = single_video_stream()) {
                sources_[stream->index];
            }
        }

        std::vector<int> keys;
        // Flush unused inputs.
        for (auto& p : decoders_) {
            if (sources_.find(p.first) == sources_.end()) {
                keys.push_back(p.first);
            }
        }

        for (auto& key : keys) {
            decoders_.erase(key);
        }
    }

    /// Build the video graph, now that the decoder has produced the frame that describes it.
    ///
    /// This is the one place the strategy is judged against the file. It cannot happen sooner:
    /// a hardware decoder does not know its own output format until FFmpeg calls get_format(),
    /// and FFmpeg only calls that once the first packet has been decoded — on the Decoder's own
    /// thread. avcodec_get_hw_frames_parameters() cannot be asked ahead of time either; it needs
    /// a codec context that is already open, which is why FFmpeg itself only ever calls it from
    /// inside get_format(). So the frame is the earliest honest answer, and waiting for it costs
    /// nothing: the producer is already willing to wait for a video frame.
    ///
    /// Returns whether it did anything, for the caller's progress tracking.
    bool configure_video_filter()
    {
        auto* stream = single_video_stream();
        CASPAR_VERIFY(stream);

        auto it = decoders_.find(stream->index);
        if (it == decoders_.end()) {
            return false;
        }

        auto frame = it->second.pop();
        if (!frame) {
            return false; // nothing decoded yet; the producer waits as it does for any frame
        }

        // A frame with no data is end of file before a single picture — there is nothing to
        // describe the source with, and nothing to show either. Build the graph from the
        // container so the shape is right, then close the source immediately below.
        const auto* description = has_content(*frame) ? frame.get() : nullptr;

        if (description && !video()->accepts(*description)) {
            return restart_with_next_video_strategy();
        }

        try {
            video_filter_ = Filter(
                vfilter_, input_, decoders_, reset_time_, AVMEDIA_TYPE_VIDEO, format_desc_, video().get(), description);
        } catch (...) {
            if (!video()->hw_device_context()) {
                throw; // the software graph failing to build is a real error, as it always was
            }
            CASPAR_LOG_CURRENT_EXCEPTION();
            return restart_with_next_video_strategy();
        }

        rebuild_sources();

        CASPAR_LOG(info) << print() << " Using " << u8(video()->name()) << " video decoding.";

        // The frame that described the graph is also the graph's first input.
        for (auto source : sources_[stream->index]) {
            if (has_content(*frame)) {
                FF(av_buffersrc_write_frame(source, frame.get()));
            } else {
                FF(av_buffersrc_close(source, frame->pts, 0));
            }
        }

        return true;
    }

    /// Hand the file to the next strategy: the frames this one produced are of no use to the
    /// next, and it has already eaten the packets the next one needs, so everything built for it
    /// goes and the input rewinds to where this attempt started.
    bool restart_with_next_video_strategy()
    {
        next_video_strategy();
        decoders_.clear();
        if (seekable_) {
            input_.seek(reset_time_);
        }
        reset(reset_time_);
        return true;
    }

    void reset(int64_t start_time)
    {
        reset_time_ = start_time;

        auto* stream = single_video_stream();

        // Hardware decoding needs the one video stream it can wait on. Without it — no video at
        // all, or several streams feeding one graph — only the CPU strategy applies, and its
        // graph can be built straight away from the container.
        if (!stream) {
            video_candidate_ = video_candidates_.size() - 1;
        } else {
            // Skip any strategy that will not even open a decoder for this codec. Opening one
            // reads no packets, so declining here costs nothing and saves a rewind later.
            for (;;) {
                auto& decoder = open_video_decoder(*stream);
                if (!video()->hw_device_context() || decoder.claimed_by_strategy()) {
                    break;
                }
                decoders_.erase(stream->index);
                next_video_strategy();
            }
        }

        audio_filter_ = Filter(afilter_, input_, decoders_, start_time, AVMEDIA_TYPE_AUDIO, format_desc_, nullptr);

        if (stream) {
            video_filter_         = Filter();
            video_filter_.pending = true;
        } else {
            video_filter_ =
                Filter(vfilter_, input_, decoders_, start_time, AVMEDIA_TYPE_VIDEO, format_desc_, video().get());
        }

        rebuild_sources();
    }

    std::string print() const
    {
        const int          position = std::max(static_cast<int>(time() - start().value_or(0)), 0);
        std::ostringstream str;
        str << std::fixed << std::setprecision(4) << "ffmpeg[" << name_ << "|"
            << av_q2d({position * format_tb_.num, format_tb_.den}) << "/"
            << av_q2d({static_cast<int>(duration().value_or(0LL)) * format_tb_.num, format_tb_.den}) << "]";
        return str.str();
    }
};

AVProducer::AVProducer(std::shared_ptr<core::frame_factory> frame_factory,
                       core::video_format_desc              format_desc,
                       std::string                          name,
                       std::string                          path,
                       std::optional<std::string>           vfilter,
                       std::optional<std::string>           afilter,
                       std::optional<int64_t>               start,
                       std::optional<int64_t>               seek,
                       std::optional<int64_t>               duration,
                       std::optional<bool>                  loop,
                       int                                  seekable,
                       core::frame_geometry::scale_mode     scale_mode)
    : impl_(new Impl(std::move(frame_factory),
                     std::move(format_desc),
                     std::move(name),
                     std::move(path),
                     std::move(vfilter.value_or("")),
                     std::move(afilter.value_or("")),
                     std::move(start),
                     std::move(seek),
                     std::move(duration),
                     std::move(loop.value_or(false)),
                     seekable,
                     scale_mode))
{
}

core::draw_frame AVProducer::next_frame(const core::video_field field) { return impl_->next_frame(field); }

core::draw_frame AVProducer::prev_frame(const core::video_field field) { return impl_->prev_frame(field); }

bool AVProducer::is_ready() { return impl_->is_ready(); }

AVProducer& AVProducer::seek(int64_t time)
{
    impl_->seek(time);
    return *this;
}

AVProducer& AVProducer::loop(bool loop)
{
    impl_->loop(loop);
    return *this;
}

bool AVProducer::loop() const { return impl_->loop(); }

AVProducer& AVProducer::start(int64_t start)
{
    impl_->start(start);
    return *this;
}

int64_t AVProducer::time() const { return impl_->time(); }

int64_t AVProducer::start() const { return impl_->start().value_or(0); }

AVProducer& AVProducer::duration(int64_t duration)
{
    impl_->duration(duration);
    return *this;
}

int64_t AVProducer::duration() const { return impl_->duration().value_or(std::numeric_limits<int64_t>::max()); }

core::monitor::state AVProducer::state() const
{
    boost::lock_guard<boost::mutex> lock(impl_->state_mutex_);
    return impl_->state_;
}

}} // namespace caspar::ffmpeg
