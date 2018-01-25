#include "av_producer.h"

#include <boost/exception/exception.hpp>
#include <boost/format.hpp>
#include <boost/rational.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/algorithm/rotate.hpp>

#include <common/scope_exit.h>
#include <common/except.h>
#include <common/diagnostics/graph.h>

#include <core/frame/draw_frame.h>
#include <core/frame/frame.h>
#include <core/frame/frame_factory.h>
#include <core/help/help_repository.h>
#include <core/help/help_sink.h>
#include <core/producer/media_info/media_info.h>

#include <tbb/concurrent_queue.h>

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timecode.h>
}
#ifdef _MSC_VER
#pragma warning (pop)
#endif

#include "av_assert.h"
#include "av_util.h"

#include <atomic>
#include <queue>
#include <deque>
#include <exception>
#include <mutex>
#include <memory>
#include <string>
#include <cinttypes>
#include <thread>
#include <condition_variable>
#include <set>

namespace caspar {
namespace ffmpeg2 {

using namespace std::chrono_literals;

const AVRational TIME_BASE_Q = { 1, AV_TIME_BASE };

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

struct Frame
{
    std::shared_ptr<AVFrame> video;
    std::shared_ptr<AVFrame> audio;
};

core::mutable_frame make_frame(void* tag, core::frame_factory& frame_factory, const Frame& in_frame)
{
    const auto video = in_frame.video;
    const auto audio = in_frame.audio;

    const auto pix_desc = video
        ? ffmpeg2::pixel_format_desc(static_cast<AVPixelFormat>(video->format), video->width, video->height)
        : core::pixel_format_desc(core::pixel_format::invalid);

    const auto channel_layout = audio
        ? ffmpeg2::get_audio_channel_layout(audio->channels, audio->channel_layout)
        : core::audio_channel_layout::invalid();

    auto frame = frame_factory.create_frame(tag, pix_desc, channel_layout);

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
        const auto beg = reinterpret_cast<uint32_t*>(audio->data[0]);
        const auto end = beg + audio->nb_samples * audio->channels;
        frame.audio_data() = core::mutable_audio_buffer(beg, end);
    }

    return frame;
}

// TODO AVFMT_TS_DISCONT
// TODO filter preset
// TODO AVDISCARD
// TODO forward options

struct Stream
{
    mutable std::mutex                        decoder_mutex_;
    std::shared_ptr<AVCodecContext>           decoder_;

    mutable std::mutex                        mutex_;
    std::condition_variable                   cond_;

    int                                       input_capacity_ = 256;
    std::queue<std::shared_ptr<AVPacket>>     input_;

    int                                       output_capacity_ = 8;
    std::queue<std::shared_ptr<AVFrame>>      output_;

    std::atomic<bool>                         abort_request_ = false;
    std::thread                               thread_;

    Stream(AVStream* stream)
    {
        const auto codec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!codec) {
            FF_RET(AVERROR_DECODER_NOT_FOUND, "avcodec_find_decoder");
        }

        decoder_ = std::shared_ptr<AVCodecContext>(avcodec_alloc_context3(codec), [](AVCodecContext* ptr) { avcodec_free_context(&ptr); });
        if (!decoder_) {
            FF_RET(AVERROR(ENOMEM), "avcodec_alloc_context3");
        }

        FF(avcodec_parameters_to_context(decoder_.get(), stream->codecpar));

        FF(av_opt_set_int(decoder_.get(), "refcounted_frames", 1, 0));

        decoder_->pkt_timebase = stream->time_base;

        if (decoder_->codec_type == AVMEDIA_TYPE_VIDEO) {
            decoder_->framerate = av_guess_frame_rate(nullptr, stream, nullptr);
            decoder_->sample_aspect_ratio = av_guess_sample_aspect_ratio(nullptr, stream, nullptr);
        } else if (decoder_->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (!decoder_->channel_layout && decoder_->channels) {
                decoder_->channel_layout = av_get_default_channel_layout(decoder_->channels);
            }
            if (!decoder_->channels && decoder_->channel_layout) {
                decoder_->channels = av_get_channel_layout_nb_channels(decoder_->channel_layout);
            }
        }

        FF(avcodec_open2(decoder_.get(), codec, nullptr));

        thread_ = std::thread([&]
        {
            try {
                while (!abort_request_) {
                    {
                        std::unique_lock<std::mutex> lock(mutex_);
                        cond_.wait(lock, [&] { return (!input_.empty() && output_.size() < output_capacity_) || abort_request_; });
                    }

                    if (abort_request_) {
                        break;
                    }

                    std::lock_guard<std::mutex> decoder_lock(decoder_mutex_);

                    std::shared_ptr<AVPacket> packet;
                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        packet = std::move(input_.front());
                        input_.pop();
                    }
                    cond_.notify_all();

                    FF(avcodec_send_packet(decoder_.get(), packet.get()));

                    while (true) {
                        auto frame = alloc_frame();
                        auto ret = avcodec_receive_frame(decoder_.get(), frame.get());

                        if (ret == AVERROR(EAGAIN)) {
                            break;
                        }

                        if (ret == AVERROR_EOF) {
                            avcodec_flush_buffers(decoder_.get());
                            frame = nullptr;
                        } else {
                            FF_RET(ret, "avcodec_receive_frame");

                            // TODO (fix) is this always best?
                            frame->pts = frame->best_effort_timestamp;
                        }

                        {
                            std::lock_guard<std::mutex> lock(mutex_);
                            output_.push(std::move(frame));
                        }
                        cond_.notify_all();
                    }
                }
            } catch (...) {
                CASPAR_LOG_CURRENT_EXCEPTION();
            }
        });
    }

    ~Stream()
    {
        abort_request_ = true;
        cond_.notify_all();
        thread_.join();
    }

    AVCodecContext* operator->()
    {
        return decoder_.get();
    }

    const AVCodecContext* operator->() const
    {
        return decoder_.get();
    }

    bool try_push(std::shared_ptr<AVPacket> packet)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (input_.size() > input_capacity_ && packet) {
                return false;
            }
            input_.push(std::move(packet));
        }
        cond_.notify_all();

        return true;
    }

    void operator()(std::function<bool(std::shared_ptr<AVFrame>&)> fn)
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            while (!output_.empty() && fn(output_.front())) {
                output_.pop();
            }
        }
        cond_.notify_all();
    }

    void flush()
    {
        std::lock_guard<std::mutex> decoder_lock(decoder_mutex_);

        avcodec_flush_buffers(decoder_.get());

        {
            std::lock_guard<std::mutex> lock(mutex_);
            while (!output_.empty()) {
                output_.pop();
            }
            while (!input_.empty()) {
                input_.pop();
            }
        }
        cond_.notify_all();
    }
};

struct Input
{
    std::shared_ptr<diagnostics::graph>             graph_;

    mutable std::mutex                              format_mutex_;
    std::shared_ptr<AVFormatContext>                format_;

    std::map<int, Stream>                           streams_;

    mutable std::mutex                              mutex_;
    std::condition_variable                         cond_;

    int                                             output_capacity_ = 64;
    std::queue<std::shared_ptr<AVPacket>>           output_;

    bool                                            paused_ = false;

    std::atomic<bool>                               abort_request_ = false;
    std::thread                                     thread_;

    Input(const std::string& filename, std::shared_ptr<diagnostics::graph> graph)
        : graph_(graph)
    {
        graph_->set_color("seek", diagnostics::color(1.0f, 0.5f, 0.0f));
        graph_->set_color("input", diagnostics::color(0.7f, 0.4f, 0.4f));

        AVDictionary* options = nullptr;
        CASPAR_SCOPE_EXIT{ av_dict_free(&options); };

        // TODO (fix) check if filename is http
        FF(av_dict_set(&options, "reconnect", "1", 0)); // HTTP reconnect
        // TODO (fix) timeout?
        FF(av_dict_set(&options, "rw_timeout", "5000000", 0)); // 5 second IO timeout

        AVFormatContext* ic = nullptr;
        FF(avformat_open_input(&ic, filename.c_str(), nullptr, &options));
        format_ = std::shared_ptr<AVFormatContext>(ic, [](AVFormatContext* ctx) { avformat_close_input(&ctx); });

        format_->interrupt_callback.callback = Input::interrupt_cb;
        format_->interrupt_callback.opaque = this;

        FF(avformat_find_stream_info(format_.get(), nullptr));

        for (auto n = 0ULL; n < format_->nb_streams; ++n) {
            // TODO lazy load decoders?
            try {
                streams_.try_emplace(static_cast<int>(n), format_->streams[n]);
            } catch (...) {
                // TODO
            }
        }

        thread_ = std::thread([&]
        {
            try {
                while (!abort_request_) {
                    {
                        std::unique_lock<std::mutex> lock(mutex_);
                        cond_.wait(lock, [&] { return (!paused_ && output_.size() < output_capacity_) || abort_request_; });
                    }

                    if (abort_request_) {
                        break;
                    }

                    std::lock_guard<std::mutex> format_lock(format_mutex_);

                    auto packet = alloc_packet();
                    auto ret = av_read_frame(format_.get(), packet.get());

                    if (ret == AVERROR_EXIT) {
                        break;
                    } else if (ret == AVERROR_EOF) {
                        packet = nullptr;
                    } else {
                        FF_RET(ret, "av_read_frame");
                    }

                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        paused_ = packet == nullptr;
                        output_.push(packet);

                        graph_->set_value("input", (static_cast<double>(output_.size() + 0.001) / output_capacity_));
                    }
                    cond_.notify_all();
                }
            } catch (...) {
                CASPAR_LOG_CURRENT_EXCEPTION();
            }
        });
    }

    ~Input()
    {
        abort_request_ = true;
        cond_.notify_all();
        thread_.join();
    }

    bool paused() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return paused_;
    }

    void operator()(std::function<bool(std::shared_ptr<AVPacket>&)> fn)
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            while (!output_.empty() && fn(output_.front())) {
                output_.pop();
                graph_->set_value("input", (static_cast<double>(output_.size() + 0.001) / output_capacity_));
            }
        }
        cond_.notify_all();
    }

    static int interrupt_cb(void* ctx)
    {
        auto input = reinterpret_cast<Input*>(ctx);
        return input->abort_request_ ? 1 : 0;
    }

    AVFormatContext* operator->()
    {
        return format_.get();
    }

    AVFormatContext* const operator->() const
    {
        return format_.get();
    }

    auto find(int n)
    {
        return streams_.find(n);
    }

    auto begin()
    {
        return streams_.begin();
    }

    auto end()
    {
        return streams_.end();
    }

    auto begin() const
    {
        return streams_.begin();
    }

    auto end() const
    {
        return streams_.end();
    }

    int64_t start_time() const
    {
        return format_->start_time != AV_NOPTS_VALUE ? format_->start_time : 0;
    }

    void seek(int64_t ts, bool flush = true)
    {
        // TODO (perf) non blocking av_read_frame

        std::lock_guard<std::mutex> lock(format_mutex_);

        FF(avformat_seek_file(format_.get(), -1, INT64_MIN, ts, ts, 0));

        if (flush)
        {
            {
                std::lock_guard<std::mutex> output_lock(mutex_);
                while (!output_.empty()) {
                    output_.pop();
                }
                paused_ = false;
            }

            for (auto& p : streams_) {
                p.second.flush();
            }
        } else {
            std::lock_guard<std::mutex> output_lock(mutex_);
            paused_ = false;
        }

        cond_.notify_all();

        graph_->set_tag(diagnostics::tag_severity::INFO, "seek");
    }
};

struct Filter
{
    std::shared_ptr<AVFilterGraph>  graph;
    AVFilterContext*                sink = nullptr;
    std::map<int, AVFilterContext*> sources;
    std::shared_ptr<AVFrame>        frame;
    bool                            eof = false;

    Filter()
    {

    }

    Filter(std::string filter_spec, const Input& input, int64_t start_time, AVMediaType media_type, const core::video_format_desc& format_desc)
    {
        if (media_type == AVMEDIA_TYPE_VIDEO) {
            if (filter_spec.empty()) {
                filter_spec = "null";
            }

            filter_spec += (boost::format(",bwdif=mode=send_field:parity=auto:deint=all")
                ).str();

            filter_spec += (boost::format(",fps=fps=%d/%d:start_time=%f")
                % (format_desc.framerate.numerator() * format_desc.field_count) % format_desc.framerate.denominator()
                % (static_cast<double>(start_time) / AV_TIME_BASE)
                ).str();
        } else if (media_type == AVMEDIA_TYPE_AUDIO) {
            if (filter_spec.empty()) {
                filter_spec = "anull";
            }

            filter_spec += (boost::format(",aresample=sample_rate=%d:async=2000:first_pts=%d")
                % format_desc.audio_sample_rate
                % av_rescale_q(start_time, TIME_BASE_Q, { 1, format_desc.audio_sample_rate })
                ).str();
        }

        AVFilterInOut* outputs = nullptr;
        AVFilterInOut* inputs = nullptr;

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

        graph = std::shared_ptr<AVFilterGraph>(avfilter_graph_alloc(), [](AVFilterGraph* ptr) { avfilter_graph_free(&ptr); });

        if (!graph) {
            FF_RET(AVERROR(ENOMEM), "avfilter_graph_alloc");
        }

        if (audio_input_count == 1) {
            int count = 0;
            for (auto& p : input) {
                if (p.second->codec_type == AVMEDIA_TYPE_AUDIO) {
                    count += 1;
                }
            }
            if (count > 1) {
                filter_spec = (boost::format("amerge=inputs=%d,") % count).str() + filter_spec;
            }
        } else if (video_input_count == 1) {
            int count = 0;
            for (auto& p : input) {
                if (p.second->codec_type == AVMEDIA_TYPE_VIDEO) {
                    count += 1;
                }
            }
            if (count > 1) {
                filter_spec = "alphamerge," + filter_spec;
            }
        }

        FF(avfilter_graph_parse2(graph.get(), filter_spec.c_str(), &inputs, &outputs));

        // inputs
        {
            for (auto cur = inputs; cur; cur = cur->next) {
                const auto type = avfilter_pad_get_type(cur->filter_ctx->input_pads, cur->pad_idx);
                if (type != AVMEDIA_TYPE_VIDEO && type != AVMEDIA_TYPE_AUDIO) {
                    CASPAR_THROW_EXCEPTION(ffmpeg_error_t()
                        << boost::errinfo_errno(EINVAL)
                        << msg_info_t("only video and audio filters supported")
                    );
                }

                // TODO find stream based on link name
                // TODO share stream decoders between graphs
                const auto it = std::find_if(input.begin(), input.end(), [&](const auto& p)
                {
                    return p.second->codec_type == type && sources.find(static_cast<int>(p.first)) == sources.end();
                });

                if (it == input.end()) {
                    graph = nullptr;
                    // TODO warn?
                    return;
                }

                auto& st = it->second;
                auto index = it->first;

                if (st->codec_type == AVMEDIA_TYPE_VIDEO) {
                    auto args = (boost::format("video_size=%dx%d:pix_fmt=%d:time_base=%d/%d")
                        % st->width % st->height
                        % st->pix_fmt
                        % st->pkt_timebase.num % st->pkt_timebase.den
                        ).str();
                    auto name = (boost::format("in_%d") % index).str();

                    if (st->sample_aspect_ratio.num > 0 && st->sample_aspect_ratio.den > 0) {
                        args += (boost::format(":sar=%d/%d")
                            % st->sample_aspect_ratio.num % st->sample_aspect_ratio.den
                            ).str();
                    }

                    if (st->framerate.num > 0 && st->framerate.den > 0) {
                        args += (boost::format(":frame_rate=%d/%d")
                            % st->framerate.num % st->framerate.den
                            ).str();
                    }

                    AVFilterContext* source = nullptr;
                    FF(avfilter_graph_create_filter(&source, avfilter_get_by_name("buffer"), name.c_str(), args.c_str(), nullptr, graph.get()));
                    FF(avfilter_link(source, 0, cur->filter_ctx, cur->pad_idx));
                    sources.emplace(index, source);
                } else if (st->codec_type == AVMEDIA_TYPE_AUDIO) {
                    auto args = (boost::format("time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%#x")
                        % st->pkt_timebase.num % st->pkt_timebase.den
                        % st->sample_rate
                        % av_get_sample_fmt_name(st->sample_fmt)
                        % st->channel_layout
                        ).str();
                    auto name = (boost::format("in_%d") % index).str();

                    AVFilterContext* source = nullptr;
                    FF(avfilter_graph_create_filter(&source, avfilter_get_by_name("abuffer"), name.c_str(), args.c_str(), nullptr, graph.get()));
                    FF(avfilter_link(source, 0, cur->filter_ctx, cur->pad_idx));
                    sources.emplace(index, source);
                } else {
                    CASPAR_THROW_EXCEPTION(ffmpeg_error_t()
                        << boost::errinfo_errno(EINVAL)
                        << msg_info_t("invalid filter input media type")
                    );
                }
            }
        }

        if (media_type == AVMEDIA_TYPE_VIDEO) {
            FF(avfilter_graph_create_filter(&sink, avfilter_get_by_name("buffersink"), "out", nullptr, nullptr, graph.get()));

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4245)
#endif
            const AVPixelFormat pix_fmts[] = {
                AV_PIX_FMT_GRAY8,
                AV_PIX_FMT_RGB24,
                AV_PIX_FMT_BGR24,
                AV_PIX_FMT_BGRA,
                AV_PIX_FMT_ARGB,
                AV_PIX_FMT_RGBA,
                AV_PIX_FMT_ABGR,
                AV_PIX_FMT_YUV444P,
                AV_PIX_FMT_YUV422P,
                AV_PIX_FMT_YUVA444P,
                AV_PIX_FMT_YUVA422P,
                // NOTE CasparCG does not properly handle interlaced vertical chrome subsampling.
                // Use only YUV444 and YUV422 formats.
                AV_PIX_FMT_NONE
            };
            FF(av_opt_set_int_list(sink, "pix_fmts", pix_fmts, -1, AV_OPT_SEARCH_CHILDREN));
#ifdef _MSC_VER
#pragma warning (pop)
#endif
        } else if (media_type == AVMEDIA_TYPE_AUDIO) {
            FF(avfilter_graph_create_filter(&sink, avfilter_get_by_name("abuffersink"), "out", nullptr, nullptr, graph.get()));
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4245)
#endif
            const AVSampleFormat sample_fmts[] = {
                AV_SAMPLE_FMT_S32,
                AV_SAMPLE_FMT_NONE
            };
            FF(av_opt_set_int_list(sink, "sample_fmts", sample_fmts, -1, AV_OPT_SEARCH_CHILDREN));

            const int sample_rates[] = {
                format_desc.audio_sample_rate,
                -1
            };
            FF(av_opt_set_int_list(sink, "sample_rates", sample_rates, -1, AV_OPT_SEARCH_CHILDREN));
#ifdef _MSC_VER
#pragma warning (pop)
#endif
        } else {
            CASPAR_THROW_EXCEPTION(ffmpeg_error_t()
                << boost::errinfo_errno(EINVAL)
                << msg_info_t("invalid output media type")
            );
        }

        // output
        {
            const auto cur = outputs;

            if (!cur || cur->next) {
                CASPAR_THROW_EXCEPTION(ffmpeg_error_t()
                    << boost::errinfo_errno(EINVAL)
                    << msg_info_t("invalid filter graph output count")
                );
            }

            if (avfilter_pad_get_type(cur->filter_ctx->output_pads, cur->pad_idx) != media_type) {
                CASPAR_THROW_EXCEPTION(ffmpeg_error_t()
                    << boost::errinfo_errno(EINVAL)
                    << msg_info_t("invalid filter output media type")
                );
            }

            FF(avfilter_link(cur->filter_ctx, cur->pad_idx, sink, 0));
        }

        FF(avfilter_graph_config(graph.get(), nullptr));
    }
};

struct AVProducer::Impl
{
    spl::shared_ptr<diagnostics::graph>             graph_;

    const std::shared_ptr<core::frame_factory>      frame_factory_;
    const core::video_format_desc                   format_desc_;
    const std::string                               filename_;

    std::vector<int>                                audio_cadence_;

    Input                                           input_;
    Filter                                          video_filter_;
    Filter                                          audio_filter_;

    std::map<int, std::vector<AVFilterContext*>>    sources_;

    int64_t                                         start_ = AV_NOPTS_VALUE;
    int64_t                                         duration_ = AV_NOPTS_VALUE;
    bool                                            loop_ = false;

    std::string                                     afilter_;
    std::string                                     vfilter_;

    mutable std::mutex                              mutex_;
    std::condition_variable                         cond_;
    int64_t                                         time_ = AV_NOPTS_VALUE;

    mutable std::mutex                              frame_mutex_;
    boost::optional<int64_t>                        frame_time_;
    core::draw_frame                                frame_ = core::draw_frame::late();

    std::mutex                                      buffer_mutex_;
    std::condition_variable                         buffer_cond_;
    std::deque<Frame>                               buffer_;
    int                                             buffer_capacity_;

    std::atomic<bool>                               abort_request_ = false;
    std::thread                                     thread_;

    Impl(
        std::shared_ptr<core::frame_factory> frame_factory,
        core::video_format_desc format_desc,
        std::string filename,
        std::string vfilter,
        std::string afilter,
        boost::optional<int64_t> start,
        boost::optional<int64_t> duration,
        bool loop)
        : frame_factory_(frame_factory)
        , format_desc_(format_desc)
        , filename_(filename)
        , input_(filename, graph_)
        , start_(start.value_or(AV_NOPTS_VALUE))
        , duration_(duration.value_or(AV_NOPTS_VALUE))
        , loop_(loop)
        , vfilter_(vfilter)
        , afilter_(afilter)
        , audio_cadence_(format_desc_.audio_cadence)
        , buffer_capacity_(boost::rational_cast<int>(format_desc_.framerate))
    {
        diagnostics::register_graph(graph_);
        graph_->set_color("underflow", diagnostics::color(0.6f, 0.3f, 0.9f));
        graph_->set_text(u16(print()));

        if (start_ != AV_NOPTS_VALUE) {
            seek(start_);
        } else {
            reset(0);
        }

        thread_ = std::thread([&]
        {
            try {
                auto eof = [&]
                {
                    return time_ != AV_NOPTS_VALUE && duration_ != AV_NOPTS_VALUE && time_ >= duration_;
                };

                while (!abort_request_) {
                    // Note: Use 1 step rotated cadence for 1001 modes (1602, 1602, 1601, 1602, 1601), (801, 801, 800, 801, 800)
                    // This cadence fills the audio mixer most optimally.
                    boost::range::rotate(audio_cadence_, std::end(audio_cadence_) - 1);

                    {
                        std::unique_lock<std::mutex> lock(buffer_mutex_);
                        buffer_cond_.wait(lock, [&] { return buffer_.size() < buffer_capacity_ || abort_request_; });
                    }

                    std::unique_lock<std::mutex> lock(mutex_);
                    // TODO input_.paused?
                    cond_.wait(lock, [&] { return loop_ || !eof() || abort_request_; });

                    if (abort_request_) {
                        break;
                    }

                    // TODO (perf) loop as soon as input pts >= duration
                    if (loop_ && eof()) {
                        time_ = start_ != AV_NOPTS_VALUE ? start_ : 0;
                        input_.seek(time_ + input_.start_time(), true);
                        reset(time_);
                        continue;
                    }

                    if (loop_ && input_.paused()) {
                        auto time = start_ != AV_NOPTS_VALUE ? start_ : 0;
                        input_.seek(time + input_.start_time(), false);
                        continue;
                    }

                    if (!video_filter_.frame && !filter_frame(video_filter_)) {
                        // TODO (perf) avoid polling
                        cond_.wait_for(lock, 10ms, [&] { return abort_request_.load(); });
                        continue;
                    }

                    if (!audio_filter_.frame && !filter_frame(audio_filter_, audio_cadence_[0] / format_desc_.field_count)) {
                        // TODO (perf) avoid polling
                        cond_.wait_for(lock, 10ms, [&] { return abort_request_.load(); });
                        continue;
                    }

                    // eof
                    if (!video_filter_.frame && !audio_filter_.frame) {
                        auto start = start_ != AV_NOPTS_VALUE ? start_ : 0;
                        reset(start + input_.start_time());
                        // TODO set duration_?
                        continue;
                    }

                    // drop extra audio
                    if (video_filter_.sink && !video_filter_.frame) {
                        audio_filter_.frame = nullptr;
                        video_filter_.frame = nullptr;
                        continue;
                    }

                    Frame frame;
                    frame.video = std::move(video_filter_.frame);
                    frame.audio = std::move(audio_filter_.frame);

                    const auto start_time = input_->start_time != AV_NOPTS_VALUE ? input_->start_time : 0;

                    // TODO (fix) check video->pts vs audio->pts

                    if (frame.video) {
                        frame.video->pts = av_rescale_q(frame.video->pts, av_buffersink_get_time_base(video_filter_.sink), TIME_BASE_Q) - start_time;
                        time_ = time_ != AV_NOPTS_VALUE ? std::max(time_, frame.video->pts) : frame.video->pts;
                    }

                    if (frame.audio) {
                        frame.audio->pts = av_rescale_q(frame.audio->pts, av_buffersink_get_time_base(audio_filter_.sink), TIME_BASE_Q) - start_time;
                        time_ = time_ != AV_NOPTS_VALUE ? std::max(time_, frame.audio->pts) : frame.audio->pts;
                    }

                    {
                         std::lock_guard<std::mutex> buffer_lock(buffer_mutex_);
                         buffer_.push_back(frame);
                    }
                }
            } catch (tbb::user_abort&) {
                return;
            } catch (...) {
                CASPAR_LOG_CURRENT_EXCEPTION();
            }
        });
    }

    ~Impl()
    {
        abort_request_ = true;
        cond_.notify_all();
        buffer_cond_.notify_all();
        thread_.join();
    }

    std::string print() const
    {
        std::ostringstream str;
        str << std::fixed << std::setprecision(4)
            << "ffmpeg[" << filename_ << "|"
            << (static_cast<double>(frame_time_.value_or(0LL)) / AV_TIME_BASE) << "/"
            << (static_cast<double>(duration().value_or(0LL)) / AV_TIME_BASE)
            << "]";
        return str.str();
    }

    bool schedule_inputs()
    {
        auto result = false;

        // TODO (fix) check streams vs sources

        input_([&](std::shared_ptr<AVPacket>& packet)
        {
            if (!packet) {
                for (auto& p : input_) {
                    // NOTE: This should never fail
                    // TODO (fix) overflow?
                    p.second.try_push(nullptr);
                }
            } else {
                auto it = input_.find(packet->stream_index);
                if (it != input_.end() && !it->second.try_push(packet)) {
                    return false;
                }
            }

            result = true;

            return true;
        });

        return result;
    }

    bool schedule_filters()
    {
        auto result = schedule_inputs();

        for (auto& p : sources_) {
            int nb_requests = 0;
            for (auto source : p.second) {
                nb_requests = std::max<int>(nb_requests, av_buffersrc_get_nb_failed_requests(source));
            }

            auto it = input_.find(p.first);
            if (it == input_.end()) {
                continue;
            }

            it->second([&](std::shared_ptr<AVFrame>& frame)
            {
                if (nb_requests == 0) {
                    return false;
                }

                result = true;

                for (auto source : p.second) {
                    // TODO (fix) overflow?
                    FF(av_buffersrc_write_frame(source, frame.get()));
                }

                nb_requests -= 1;

                return true;
            });
        }

        return result || schedule_inputs();
    }

    bool filter_frame(Filter& filter, int nb_samples = -1)
    {
        if (!filter.sink || filter.sources.empty() || filter.eof) {
            filter.frame = nullptr;
            return true;
        }

        while (true) {
            auto frame = alloc_frame();
            auto ret = nb_samples >= 0
                ? av_buffersink_get_samples(filter.sink, frame.get(), nb_samples)
                : av_buffersink_get_frame(filter.sink, frame.get());

            if (ret == AVERROR(EAGAIN)) {
                if (!schedule_filters()) {
                    return false;
                }
            } else if (ret == AVERROR_EOF) {
                filter.eof = true;
                filter.frame = nullptr;
                return true;
            } else {
                FF_RET(ret, "av_buffersink_get_frame");
                filter.frame = frame;
                return true;
            }
        }
    }

    void reset(int64_t ts)
    {
        video_filter_ = Filter(vfilter_, input_, ts, AVMEDIA_TYPE_VIDEO, format_desc_);
        audio_filter_ = Filter(afilter_, input_, ts, AVMEDIA_TYPE_AUDIO, format_desc_);

        sources_.clear();

        for (auto& p : video_filter_.sources) {
            sources_[p.first].push_back(p.second);
        }
        for (auto& p : audio_filter_.sources) {
            sources_[p.first].push_back(p.second);
        }
    }

public:
    // TODO first frame
    // TODO interlaced step
    // TODO make_frame buffered

    core::draw_frame prev_frame()
    {
        return frame_;
    }

    core::draw_frame next_frame()
    {
        std::lock_guard<std::mutex> lock(frame_mutex_);

        core::draw_frame result;
        {
            std::lock_guard<std::mutex> buffer_lock(buffer_mutex_);
            if (buffer_.size() < format_desc_.field_count) {
                graph_->set_tag(diagnostics::tag_severity::WARNING, "underflow");
                return core::draw_frame::late();
            }

            frame_time_ = buffer_[0].video ? buffer_[0].video->pts : buffer_[0].audio->pts;
            auto frame1 = core::draw_frame(make_frame(this, *frame_factory_, buffer_[0]));
            auto frame2 = frame1;

            if (format_desc_.field_count == 2) {
                frame2 = core::draw_frame(make_frame(this, *frame_factory_, buffer_[1]));
                result = core::draw_frame::interlace(frame1, frame2, format_desc_.field_mode);
            } else {
                result = frame2;
            }

            frame_ = core::draw_frame::still(frame2);

            graph_->set_text(u16(print()));

            for (auto n = 0; n < format_desc_.field_count; ++n) {
                buffer_.pop_front();
            }
        }
        buffer_cond_.notify_all();

        return result;
    }

    boost::optional<int64_t> time() const
    {
        std::lock_guard<std::mutex> lock(frame_mutex_);

        return frame_time_;
    }

    void seek(int64_t time)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);

            {
                std::lock_guard<std::mutex> buffer_lock(buffer_mutex_);
                buffer_.clear();
            }

            time_ = time + input_.start_time();
            input_.seek(time_);
            reset(time_);
        }
        cond_.notify_all();

        {
            std::lock_guard<std::mutex> lock(frame_mutex_);

            frame_time_ = boost::none;
            frame_ = core::draw_frame::late();
            graph_->set_text(u16(print()));
        }
    }

    void loop(bool loop)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            loop_ = loop;
        }
        cond_.notify_all();
    }

    bool loop() const
    {
        return loop_;
    }

    void start(int64_t start)
    {
        // TODO (fix) What if input has already looped?
        {
            std::lock_guard<std::mutex> lock(mutex_);
            start_ = start;
        }
        cond_.notify_all();
    }

    boost::optional<int64_t> start() const
    {
        return start_;
    }

    void duration(int64_t duration)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            duration_ = duration;
        }
        cond_.notify_all();
    }

    boost::optional<int64_t> duration() const
    {
        auto start = start_ != AV_NOPTS_VALUE ? start_ : 0;
        auto duration = duration_;

        if (duration == AV_NOPTS_VALUE && input_->duration != AV_NOPTS_VALUE) {
            duration = input_->duration - start;
        }

        if (duration == AV_NOPTS_VALUE || duration < 0) {
            return boost::none;
        }

        return duration;
    }

    int width() const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        return video_filter_.sink ? av_buffersink_get_w(video_filter_.sink) : 0;
    }

    int height() const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        return audio_filter_.sink ? av_buffersink_get_h(audio_filter_.sink) : 0;
    }
};

AVProducer::AVProducer(
    std::shared_ptr<core::frame_factory> frame_factory,
    core::video_format_desc format_desc,
    std::string filename,
    boost::optional<std::string> vfilter,
    boost::optional<std::string> afilter,
    boost::optional<int64_t> start,
    boost::optional<int64_t> duration,
    boost::optional<bool> loop
)
    : impl_(new Impl(
        std::move(frame_factory),
        std::move(format_desc),
        std::move(filename),
        std::move(vfilter.get_value_or("")),
        std::move(afilter.get_value_or("")),
        std::move(start),
        std::move(duration),
        std::move(loop.get_value_or(false))
    ))
{
}

core::draw_frame AVProducer::next_frame()
{
    return impl_->next_frame();
}

core::draw_frame AVProducer::prev_frame()
{
    return impl_->prev_frame();
}

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

bool AVProducer::loop() const
{
    return impl_->loop();
}

AVProducer& AVProducer::start(int64_t start)
{
    impl_->start(start);
    return *this;
}

int64_t AVProducer::time() const
{
    return impl_->time().value_or(0);
}

int64_t AVProducer::start() const
{
    return impl_->start().value_or(0);
}

AVProducer& AVProducer::duration(int64_t duration)
{
    impl_->duration(duration);
    return *this;
}

int64_t AVProducer::duration() const
{
    return impl_->duration().value_or(std::numeric_limits<int64_t>::max());
}

int AVProducer::width() const
{
    return impl_->width();
}

int AVProducer::height() const
{
    return impl_->height();
}

}  // namespace ffmpeg
}  // namespace caspar
