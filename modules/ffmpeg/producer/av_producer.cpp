#include "av_producer.h"

#include "av_decoder.h"
#include "av_input.h"

#include "../util/av_assert.h"
#include "../util/av_util.h"

#include <boost/exception/exception.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/algorithm/rotate.hpp>
#include <boost/rational.hpp>

#include <common/diagnostics/graph.h>
#include <common/except.h>
#include <common/os/thread.h>
#include <common/scope_exit.h>
#include <common/timer.h>

#include <core/frame/draw_frame.h>
#include <core/frame/frame.h>
#include <core/frame/frame_factory.h>

#include <tbb/concurrent_queue.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <algorithm>
#include <atomic>
#include <cinttypes>
#include <condition_variable>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>

namespace caspar { namespace ffmpeg {

using namespace std::chrono_literals;

const AVRational TIME_BASE_Q = {1, AV_TIME_BASE};

struct Frame
{
    core::draw_frame frame;
    int64_t          pts      = AV_NOPTS_VALUE;
    int64_t          duration = 0;
};

// TODO (fix) ts discontinuities
// TODO (feat) filter preset
// TODO (feat) forward options

struct Filter
{
    std::shared_ptr<AVFilterGraph>  graph;
    AVFilterContext*                sink = nullptr;
    std::map<int, AVFilterContext*> sources;
    std::shared_ptr<AVFrame>        frame;
    bool                            eof = false;

    Filter() = default;

    Filter(std::string                    filter_spec,
           const Input&                   input,
           std::map<int, Decoder>&        streams,
           int64_t                        start_time,
           AVMediaType                    media_type,
           const core::video_format_desc& format_desc)
    {
        if (media_type == AVMEDIA_TYPE_VIDEO) {
            if (filter_spec.empty()) {
                filter_spec = "null";
            }

            filter_spec += (boost::format(",bwdif=mode=send_field:parity=auto:deint=all")).str();

            filter_spec += (boost::format(",fps=fps=%d/%d:start_time=%f") %
                            (format_desc.framerate.numerator() * format_desc.field_count) %
                            format_desc.framerate.denominator() % (static_cast<double>(start_time) / AV_TIME_BASE))
                               .str();
        } else if (media_type == AVMEDIA_TYPE_AUDIO) {
            if (filter_spec.empty()) {
                filter_spec = "anull";
            }

            filter_spec +=
                (boost::format(",aresample=sample_rate=%d:async=2000:first_pts=%d") % format_desc.audio_sample_rate %
                 av_rescale_q(start_time, TIME_BASE_Q, {1, format_desc.audio_sample_rate}))
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

        if (audio_input_count == 1) {
            int count = 0;
            for (unsigned n = 0; n < input->nb_streams; ++n) {
                if (input->streams[n]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                    count += 1;
                }
            }
            if (count > 1) {
                filter_spec = (boost::format("amerge=inputs=%d,") % count).str() + filter_spec;
            }
        } else if (video_input_count == 1) {
            int count = 0;
            for (unsigned n = 0; n < input->nb_streams; ++n) {
                if (input->streams[n]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                    count += 1;
                }
            }
            if (count > 1) {
                filter_spec = "alphamerge," + filter_spec;
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
                if (type != AVMEDIA_TYPE_VIDEO && type != AVMEDIA_TYPE_AUDIO) {
                    CASPAR_THROW_EXCEPTION(ffmpeg_error_t() << boost::errinfo_errno(EINVAL)
                                                            << msg_info_t("only video and audio filters supported"));
                }

                unsigned index = 0;

                // TODO find stream based on link name
                // TODO share stream decoders between graphs
                while (true) {
                    if (index == input->nb_streams) {
                        CASPAR_THROW_EXCEPTION(
                            ffmpeg_error_t()
                            << boost::errinfo_errno(EINVAL)
                            << msg_info_t((boost::format("could not find input for: %s") % cur->name).str()));
                    }
                    if (input->streams[index]->codecpar->codec_type == type &&
                        sources.find(static_cast<int>(index)) == sources.end()) {
                        break;
                    }
                    index++;
                }

                auto it = streams.find(index);
                if (it == streams.end()) {
                    it = streams.emplace(index, input->streams[index]).first;
                }

                auto& st = it->second;

                if (st->codec_type == AVMEDIA_TYPE_VIDEO) {
                    auto args = (boost::format("video_size=%dx%d:pix_fmt=%d:time_base=%d/%d") % st->width % st->height %
                                 st->pix_fmt % st->pkt_timebase.num % st->pkt_timebase.den)
                                    .str();
                    auto name = (boost::format("in_%d") % index).str();

                    if (st->sample_aspect_ratio.num > 0 && st->sample_aspect_ratio.den > 0) {
                        args +=
                            (boost::format(":sar=%d/%d") % st->sample_aspect_ratio.num % st->sample_aspect_ratio.den)
                                .str();
                    }

                    if (st->framerate.num > 0 && st->framerate.den > 0) {
                        args += (boost::format(":frame_rate=%d/%d") % st->framerate.num % st->framerate.den).str();
                    }

                    AVFilterContext* source = nullptr;
                    FF(avfilter_graph_create_filter(
                        &source, avfilter_get_by_name("buffer"), name.c_str(), args.c_str(), nullptr, graph.get()));
                    FF(avfilter_link(source, 0, cur->filter_ctx, cur->pad_idx));
                    sources.emplace(index, source);
                } else if (st->codec_type == AVMEDIA_TYPE_AUDIO) {
                    auto args = (boost::format("time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%#x") %
                                 st->pkt_timebase.num % st->pkt_timebase.den % st->sample_rate %
                                 av_get_sample_fmt_name(st->sample_fmt) % st->channel_layout)
                                    .str();
                    auto name = (boost::format("in_%d") % index).str();

                    AVFilterContext* source = nullptr;
                    FF(avfilter_graph_create_filter(
                        &source, avfilter_get_by_name("abuffer"), name.c_str(), args.c_str(), nullptr, graph.get()));
                    FF(avfilter_link(source, 0, cur->filter_ctx, cur->pad_idx));
                    sources.emplace(index, source);
                } else {
                    CASPAR_THROW_EXCEPTION(ffmpeg_error_t() << boost::errinfo_errno(EINVAL)
                                                            << msg_info_t("invalid filter input media type"));
                }
            }
        }

        if (media_type == AVMEDIA_TYPE_VIDEO) {
            FF(avfilter_graph_create_filter(
                &sink, avfilter_get_by_name("buffersink"), "out", nullptr, nullptr, graph.get()));

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4245)
#endif
            const AVPixelFormat pix_fmts[] = {AV_PIX_FMT_RGB24,
                                              AV_PIX_FMT_BGR24,
                                              AV_PIX_FMT_BGRA,
                                              AV_PIX_FMT_ARGB,
                                              AV_PIX_FMT_RGBA,
                                              AV_PIX_FMT_ABGR,
                                              AV_PIX_FMT_YUV444P,
                                              AV_PIX_FMT_YUV422P,
                                              AV_PIX_FMT_YUV420P,
                                              AV_PIX_FMT_YUV410P,
                                              AV_PIX_FMT_YUVA444P,
                                              AV_PIX_FMT_YUVA422P,
                                              AV_PIX_FMT_YUVA420P,
                                              AV_PIX_FMT_NONE};
            FF(av_opt_set_int_list(sink, "pix_fmts", pix_fmts, -1, AV_OPT_SEARCH_CHILDREN));
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        } else if (media_type == AVMEDIA_TYPE_AUDIO) {
            FF(avfilter_graph_create_filter(
                &sink, avfilter_get_by_name("abuffersink"), "out", nullptr, nullptr, graph.get()));
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4245)
#endif
            const AVSampleFormat sample_fmts[] = {AV_SAMPLE_FMT_S32, AV_SAMPLE_FMT_NONE};
            FF(av_opt_set_int_list(sink, "sample_fmts", sample_fmts, -1, AV_OPT_SEARCH_CHILDREN));

            // TODO Always output 8 channels and remove hack in make_frame.

            const int sample_rates[] = {format_desc.audio_sample_rate, -1};
            FF(av_opt_set_int_list(sink, "sample_rates", sample_rates, -1, AV_OPT_SEARCH_CHILDREN));
#ifdef _MSC_VER
#pragma warning(pop)
#endif
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
    }
};

struct AVProducer::Impl
{
    spl::shared_ptr<diagnostics::graph> graph_;

    const std::shared_ptr<core::frame_factory> frame_factory_;
    const core::video_format_desc              format_desc_;
    const AVRational                           format_tb_;
    const std::string                          filename_;

    std::vector<int> audio_cadence_;

    Input                  input_;
    std::map<int, Decoder> decoders_;
    Filter                 video_filter_;
    Filter                 audio_filter_;

    std::map<int, std::vector<AVFilterContext*>> sources_;

    int64_t start_    = AV_NOPTS_VALUE;
    int64_t duration_ = AV_NOPTS_VALUE;
    bool    loop_     = false;

    std::string afilter_;
    std::string vfilter_;

    mutable std::mutex      mutex_;
    std::condition_variable cond_;

    int64_t          frame_time_ = 0;
    core::draw_frame frame_;

    caspar::timer tick_timer_;

    std::mutex              buffer_mutex_;
    std::condition_variable buffer_cond_;
    std::deque<Frame>       buffer_;
    int                     buffer_capacity_ = 2;
    bool                    buffer_eof_      = true;
    bool                    buffer_flush_    = true;

    std::atomic<bool> abort_request_ = false;
    std::thread       thread_;

    Impl(std::shared_ptr<core::frame_factory> frame_factory,
         core::video_format_desc              format_desc,
         std::string                          filename,
         std::string                          vfilter,
         std::string                          afilter,
         boost::optional<int64_t>             start,
         boost::optional<int64_t>             duration,
         bool                                 loop)
        : frame_factory_(frame_factory)
        , format_desc_(format_desc)
        , format_tb_({format_desc.duration, format_desc.time_scale})
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
        graph_->set_color("tick-time", caspar::diagnostics::color(0.0f, 0.6f, 0.9f));
        graph_->set_text(u16(print()));

        if (start_ != AV_NOPTS_VALUE) {
            seek(start_);
        } else {
            reset_filters(0);
        }

        thread_ = std::thread([=] {
            try {
                set_thread_name(L"[ffmpeg::av_producer]");

                while (!abort_request_) {
                    // Use 1 step rotated cadence for 1001 modes (1602, 1602, 1601, 1602, 1601), (801, 801, 800, 801,
                    // 800)
                    boost::range::rotate(audio_cadence_, std::end(audio_cadence_) - 1);

                    {
                        std::unique_lock<std::mutex> lock(buffer_mutex_);
                        buffer_cond_.wait(lock, [&] { return buffer_.size() < buffer_capacity_ || abort_request_; });
                    }

                    std::unique_lock<std::mutex> lock(mutex_);

                    if (abort_request_) {
                        break;
                    }

                    if (loop_ && input_.eof()) {
                        auto time = start_ != AV_NOPTS_VALUE ? start_ : 0;
                        input_.seek(time + input_.start_time().value_or(0), false);
                        input_.paused(false);

                        // TODO (fix) Interlaced looping.
                        continue;
                    }

                    if (!video_filter_.frame && !filter_frame(video_filter_)) {
                        // TODO (perf) Avoid polling.
                        cond_.wait_for(lock, 10ms, [&] { return abort_request_.load(); });
                        continue;
                    }

                    if (!audio_filter_.frame &&
                        !filter_frame(audio_filter_, audio_cadence_[0] / format_desc_.field_count)) {
                        // TODO (perf) Avoid polling.
                        cond_.wait_for(lock, 10ms, [&] { return abort_request_.load(); });
                        continue;
                    }

                    // End of file. Reset filters.
                    if (!video_filter_.frame && !audio_filter_.frame) {
                        auto start = start_ != AV_NOPTS_VALUE ? start_ : 0;
                        reset_filters(start + input_.start_time().value_or(0));
                        {
                            std::lock_guard<std::mutex> buffer_lock(buffer_mutex_);
                            buffer_eof_ = true;
                        }
                        // TODO (fix) Set duration_?
                        continue;
                    }

                    // Drop extra audio.
                    if (video_filter_.sink && !video_filter_.frame) {
                        audio_filter_.frame = nullptr;
                        video_filter_.frame = nullptr;
                        continue;
                    }

                    auto video = std::move(video_filter_.frame);
                    auto audio = std::move(audio_filter_.frame);

                    Frame frame;

                    const auto start_time = input_->start_time != AV_NOPTS_VALUE ? input_->start_time : 0;

                    // TODO (fix) check video->pts vs audio->pts
                    // TODO (fix) ensure pts != AV_NOPTS_VALUE

                    if (video) {
                        frame.pts =
                            av_rescale_q(video->pts, av_buffersink_get_time_base(video_filter_.sink), TIME_BASE_Q) -
                            start_time;
                        frame.duration =
                            av_rescale_q(1, av_inv_q(av_buffersink_get_frame_rate(video_filter_.sink)), TIME_BASE_Q);
                    } else if (audio) {
                        frame.pts =
                            av_rescale_q(audio->pts, av_buffersink_get_time_base(audio_filter_.sink), TIME_BASE_Q) -
                            start_time;
                        frame.duration = av_rescale_q(
                            audio->nb_samples, {1, av_buffersink_get_sample_rate(audio_filter_.sink)}, TIME_BASE_Q);
                    }

                    // TODO (perf) Seek input as soon as possible.
                    if (loop_ && duration_ != AV_NOPTS_VALUE && frame.pts >= duration_) {
                        auto start = start_ != AV_NOPTS_VALUE ? start_ : 0;
                        reset_filters(start + input_.start_time().value_or(0));
                        input_.seek(start, true);
                        input_.paused(false);
                        continue;
                    }

                    // TODO (fix) Don't lose frame if duration_ is increased.
                    if (duration_ != AV_NOPTS_VALUE && frame.pts >= duration_) {
                        input_.paused(true);
                        continue;
                    }

                    frame.frame = core::draw_frame(make_frame(this, *frame_factory_, video, audio));

                    {
                        std::lock_guard<std::mutex> buffer_lock(buffer_mutex_);
                        buffer_.push_back(std::move(frame));
                        buffer_eof_ = false;
                    }
                }
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
        str << std::fixed << std::setprecision(4) << "ffmpeg[" << filename_ << "|"
            << av_q2d({static_cast<int>(time()) * format_tb_.num, format_tb_.den}) << "/"
            << av_q2d({static_cast<int>(duration().value_or(0LL)) * format_tb_.num, format_tb_.den}) << "]";
        return str.str();
    }

    bool schedule_inputs()
    {
        auto result = false;

        input_([&](std::shared_ptr<AVPacket>& packet) {
            if (!packet) {
                for (auto& p : decoders_) {
                    // NOTE: Should never fail.
                    p.second.try_push(nullptr);
                }
            } else if (sources_.find(packet->stream_index) != sources_.end()) {
                auto it = decoders_.find(packet->stream_index);
                if (it != decoders_.end() && !it->second.try_push(packet)) {
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
        // TODO (perf) avoid copy?
        auto sources = sources_;

        for (auto& p : sources) {
            auto it = decoders_.find(p.first);
            if (it == decoders_.end()) {
                continue;
            }

            int nb_requests = 0;
            for (auto source : p.second) {
                nb_requests = std::max<int>(nb_requests, av_buffersrc_get_nb_failed_requests(source));
            }

            if (nb_requests == 0) {
                continue;
            }

            it->second([&](std::shared_ptr<AVFrame>& frame) {
                if (nb_requests == 0) {
                    return false;
                }

                for (auto& source : p.second) {
                    if (frame && !frame->data[0]) {
                        FF(av_buffersrc_close(source, frame->pts, 0));
                    } else {
                        // TODO (fix) Guard against overflow?
                        FF(av_buffersrc_write_frame(source, frame.get()));
                    }
                    result = true;
                }

                // End Of File
                if (frame && !frame->data[0]) {
                    sources_.erase(p.first);
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
            auto ret   = nb_samples >= 0 ? av_buffersink_get_samples(filter.sink, frame.get(), nb_samples)
                                       : av_buffersink_get_frame(filter.sink, frame.get());

            if (ret == AVERROR(EAGAIN)) {
                if (!schedule_filters()) {
                    return false;
                }
            } else if (ret == AVERROR_EOF) {
                filter.eof   = true;
                filter.frame = nullptr;
                return true;
            } else {
                FF_RET(ret, "av_buffersink_get_frame");
                filter.frame = frame;
                return true;
            }
        }
    }

    void reset_filters(int64_t ts)
    {
        video_filter_ = Filter(vfilter_, input_, decoders_, ts, AVMEDIA_TYPE_VIDEO, format_desc_);
        audio_filter_ = Filter(afilter_, input_, decoders_, ts, AVMEDIA_TYPE_AUDIO, format_desc_);

        sources_.clear();
        for (auto& p : video_filter_.sources) {
            sources_[p.first].push_back(p.second);
        }
        for (auto& p : audio_filter_.sources) {
            sources_[p.first].push_back(p.second);
        }

        // Flush unused inputs.
        for (auto& p : decoders_) {
            if (sources_.find(p.first) == sources_.end()) {
                p.second.flush();
            }
        }
    }

  public:
    core::draw_frame prev_frame()
    {
        {
            std::lock_guard<std::mutex> frame_lock(buffer_mutex_);

            if (!buffer_.empty() && (buffer_flush_ || !frame_)) {
                frame_        = core::draw_frame::still(buffer_[0].frame);
                frame_time_   = buffer_[0].pts + buffer_[0].duration;
                buffer_flush_ = false;
            }
        }

        graph_->set_text(u16(print()));

        return frame_;
    }

    core::draw_frame next_frame()
    {
        core::draw_frame result;
        {
            std::lock_guard<std::mutex> buffer_lock(buffer_mutex_);

            if (!buffer_eof_ && buffer_.size() < format_desc_.field_count) {
                graph_->set_tag(diagnostics::tag_severity::WARNING, "underflow");
                return core::draw_frame{};
            }

            if (format_desc_.field_count == 2 && buffer_.size() >= 2) {
                result      = core::draw_frame::interlace(buffer_[0].frame, buffer_[1].frame, format_desc_.field_mode);
                frame_      = core::draw_frame::still(buffer_[1].frame);
                frame_time_ = buffer_[0].pts + buffer_[0].duration + buffer_[1].duration;
                buffer_.pop_front();
                buffer_.pop_front();
            } else if (buffer_.size() >= 1) {
                result      = buffer_[0].frame;
                frame_      = core::draw_frame::still(buffer_[0].frame);
                frame_time_ = buffer_[0].pts + buffer_[0].duration;
                buffer_.pop_front();
            } else {
                result = frame_;
            }

            buffer_flush_ = false;
        }
        buffer_cond_.notify_all();

        auto tick_time = tick_timer_.elapsed() * format_desc_.fps * 0.5;
        graph_->set_value("tick-time", tick_time);
        tick_timer_.restart();

        graph_->set_text(u16(print()));

        return result;
    }

    void seek(int64_t time)
    {
        // TODO (fix) Validate input.

        {
            std::lock_guard<std::mutex> lock(mutex_);

            {
                std::lock_guard<std::mutex> buffer_lock(buffer_mutex_);
                buffer_.clear();
                buffer_eof_   = false;
                buffer_flush_ = true;
            }
            buffer_cond_.notify_all();

            time = av_rescale_q(time, format_tb_, TIME_BASE_Q) + input_.start_time().value_or(0);

            // TODO (fix) Dont seek if time is close future.
            input_.seek(time);
            input_.paused(false);

            for (auto& p : decoders_) {
                p.second.flush();
            }

            reset_filters(time);
        }

        cond_.notify_all();
    }

    int64_t time() const
    {
        // TODO (fix) How to handle NOPTS case?
        return frame_time_ != AV_NOPTS_VALUE ? av_rescale_q(frame_time_, TIME_BASE_Q, format_tb_) : 0;
    }

    void loop(bool loop)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            loop_ = loop;
        }
        cond_.notify_all();
    }

    bool loop() const { return loop_; }

    void start(int64_t start)
    {
        // TODO (fix) Validate input.

        {
            std::lock_guard<std::mutex> lock(mutex_);
            start_ = av_rescale_q(start, format_tb_, TIME_BASE_Q);

            // TODO (fix) Flush.
        }
        cond_.notify_all();
    }

    boost::optional<int64_t> start() const
    {
        return start_ != AV_NOPTS_VALUE ? av_rescale_q(start_, TIME_BASE_Q, format_tb_) : boost::optional<int64_t>();
    }

    void duration(int64_t duration)
    {
        // TODO (fix) Validate input.
        {
            std::lock_guard<std::mutex> lock(mutex_);
            duration_ = av_rescale_q(duration, format_tb_, TIME_BASE_Q);

            // TODO (fix) Flush.

            input_.paused(false);
        }
        cond_.notify_all();
    }

    boost::optional<int64_t> duration() const
    {
        auto start    = start_ != AV_NOPTS_VALUE ? start_ : 0;
        auto duration = duration_;

        if (duration == AV_NOPTS_VALUE && input_->duration != AV_NOPTS_VALUE) {
            duration = input_->duration - start;
        }

        if (duration == AV_NOPTS_VALUE || duration < 0) {
            return boost::none;
        }

        return av_rescale_q(duration, TIME_BASE_Q, format_tb_);
    }
};

AVProducer::AVProducer(std::shared_ptr<core::frame_factory> frame_factory,
                       core::video_format_desc              format_desc,
                       std::string                          filename,
                       boost::optional<std::string>         vfilter,
                       boost::optional<std::string>         afilter,
                       boost::optional<int64_t>             start,
                       boost::optional<int64_t>             duration,
                       boost::optional<bool>                loop)
    : impl_(new Impl(std::move(frame_factory),
                     std::move(format_desc),
                     std::move(filename),
                     std::move(vfilter.get_value_or("")),
                     std::move(afilter.get_value_or("")),
                     std::move(start),
                     std::move(duration),
                     std::move(loop.get_value_or(false))))
{
}

core::draw_frame AVProducer::next_frame() { return impl_->next_frame(); }

core::draw_frame AVProducer::prev_frame() { return impl_->prev_frame(); }

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

}} // namespace caspar::ffmpeg
