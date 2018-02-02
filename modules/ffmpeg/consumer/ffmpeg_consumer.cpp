#include "ffmpeg_consumer.h"

#include "../util/av_util.h"
#include "../util/av_assert.h"

#include <common/memory.h>
#include <common/future.h>
#include <common/env.h>
#include <common/executor.h>
#include <common/scope_exit.h>
#include <common/diagnostics/graph.h>

#include <core/video_format.h>
#include <core/frame/frame.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/format.hpp>
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>

#pragma warning(push)
#pragma warning(disable: 4244)
#pragma warning(disable: 4245)
#include <boost/crc.hpp>
#pragma warning(pop)

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

#include <queue>
#include <memory>
#include <regex>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace caspar {
namespace ffmpeg {

// TODO multiple output streams
// TODO multiple output files

struct Stream
{
    std::shared_ptr<AVFilterGraph> graph = nullptr;
    AVFilterContext* sink = nullptr;
    AVFilterContext* source = nullptr;

    std::shared_ptr<AVCodecContext> enc = nullptr;
    AVStream* st  = nullptr;

    int64_t pts = 0;

    Stream(AVFormatContext* oc, std::string filter_spec, AVCodecID codec_id, const core::video_format_desc& format_desc)
    {
        auto codec = avcodec_find_encoder(codec_id);
        if (!codec) {
            FF_RET(AVERROR(ENOMEM), "avcodec_find_encoder");
        }

        AVFilterInOut* outputs = nullptr;
        AVFilterInOut* inputs = nullptr;

        CASPAR_SCOPE_EXIT
        {
            avfilter_inout_free(&inputs);
            avfilter_inout_free(&outputs);
        };

        graph = std::shared_ptr<AVFilterGraph>(avfilter_graph_alloc(), [](AVFilterGraph* ptr)
        {
            avfilter_graph_free(&ptr);
        });

        if (!graph) {
            FF_RET(AVERROR(ENOMEM), "avfilter_graph_alloc");
        }

        if (codec->type == AVMEDIA_TYPE_VIDEO) {
            if (filter_spec.empty()) {
                filter_spec = "null";
            }
        } else {
            if (filter_spec.empty()) {
                filter_spec = "anull";
            }
        }

        FF(avfilter_graph_parse2(graph.get(), filter_spec.c_str(), &inputs, &outputs));

        {
            auto cur = inputs;

            if (!cur || cur->next) {
                CASPAR_THROW_EXCEPTION(ffmpeg_error_t()
                    << boost::errinfo_errno(EINVAL)
                    << msg_info_t("invalid filter graph input count")
                );
            }

            if (codec->type == AVMEDIA_TYPE_VIDEO) {
                const auto sample_aspect_ratio =
                    boost::rational<int>(format_desc.square_width, format_desc.square_height) /
                    boost::rational<int>(format_desc.width, format_desc.height);

                auto args = (boost::format("video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:sar=%d/%d:frame_rate=%d/%d")
                    % format_desc.width % format_desc.height
                    % AV_PIX_FMT_BGRA
                    % format_desc.duration % format_desc.time_scale
                    % sample_aspect_ratio.numerator() % sample_aspect_ratio.denominator()
                    % format_desc.framerate.numerator() % format_desc.framerate.denominator()
                    ).str();
                auto name = (boost::format("in_%d") % 0).str();

                FF(avfilter_graph_create_filter(&source, avfilter_get_by_name("buffer"), name.c_str(), args.c_str(), nullptr, graph.get()));
                FF(avfilter_link(source, 0, cur->filter_ctx, cur->pad_idx));
            } else if (codec->type == AVMEDIA_TYPE_AUDIO) {
                auto args = (boost::format("time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%#x")
                    % 1 % format_desc.audio_sample_rate
                    % format_desc.audio_sample_rate
                    % AV_SAMPLE_FMT_S32
                    % av_get_default_channel_layout(format_desc.audio_channels)
                    ).str();
                auto name = (boost::format("in_%d") % 0).str();

                FF(avfilter_graph_create_filter(&source, avfilter_get_by_name("abuffer"), name.c_str(), args.c_str(), nullptr, graph.get()));
                FF(avfilter_link(source, 0, cur->filter_ctx, cur->pad_idx));
            } else {
                CASPAR_THROW_EXCEPTION(ffmpeg_error_t()
                    << boost::errinfo_errno(EINVAL)
                    << msg_info_t("invalid filter input media type")
                );
            }
        }

        if (codec->type == AVMEDIA_TYPE_VIDEO) {
            FF(avfilter_graph_create_filter(&sink, avfilter_get_by_name("buffersink"), "out", nullptr, nullptr, graph.get()));

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4245)
#endif
            // codec->profiles
            //FF(av_opt_set_int_list(sink, "framerates", codec->supported_framerates, { 0, 0 }, AV_OPT_SEARCH_CHILDREN));
            FF(av_opt_set_int_list(sink, "pix_fmts", codec->pix_fmts, -1, AV_OPT_SEARCH_CHILDREN));
#ifdef _MSC_VER
#pragma warning (pop)
#endif
        } else if (codec->type == AVMEDIA_TYPE_AUDIO) {
            FF(avfilter_graph_create_filter(&sink, avfilter_get_by_name("abuffersink"), "out", nullptr, nullptr, graph.get()));
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4245)
#endif
            // codec->profiles
            FF(av_opt_set_int_list(sink, "sample_fmts", codec->sample_fmts, -1, AV_OPT_SEARCH_CHILDREN));
            FF(av_opt_set_int_list(sink, "channel_layouts", codec->channel_layouts, 0, AV_OPT_SEARCH_CHILDREN));
            FF(av_opt_set_int_list(sink, "sample_rates", codec->supported_samplerates, 0, AV_OPT_SEARCH_CHILDREN));
#ifdef _MSC_VER
#pragma warning (pop)
#endif
        } else {
            CASPAR_THROW_EXCEPTION(ffmpeg_error_t()
                << boost::errinfo_errno(EINVAL)
                << msg_info_t("invalid output media type")
            );
        }

        {
            const auto cur = outputs;

            if (!cur || cur->next) {
                CASPAR_THROW_EXCEPTION(ffmpeg_error_t()
                    << boost::errinfo_errno(EINVAL)
                    << msg_info_t("invalid filter graph output count")
                );
            }

            if (avfilter_pad_get_type(cur->filter_ctx->output_pads, cur->pad_idx) != codec->type) {
                CASPAR_THROW_EXCEPTION(ffmpeg_error_t()
                    << boost::errinfo_errno(EINVAL)
                    << msg_info_t("invalid filter output media type")
                );
            }

            FF(avfilter_link(cur->filter_ctx, cur->pad_idx, sink, 0));
        }

        FF(avfilter_graph_config(graph.get(), nullptr));

        st = avformat_new_stream(oc, nullptr);
        if (!st) {
            FF_RET(AVERROR(ENOMEM), "avformat_new_stream");
        }

        enc = std::shared_ptr<AVCodecContext>(avcodec_alloc_context3(codec), [](AVCodecContext* ptr)
        {
            avcodec_free_context(&ptr);
        });

        if (!enc) {
            FF_RET(AVERROR(ENOMEM), "avcodec_alloc_context3")
        }

        if (codec->type == AVMEDIA_TYPE_VIDEO) {
            st->time_base = av_inv_q(av_buffersink_get_frame_rate(sink));

            enc->codec_id = codec_id;
            enc->width = av_buffersink_get_w(sink);
            enc->height = av_buffersink_get_h(sink);
            enc->framerate = av_buffersink_get_frame_rate(sink);
            enc->sample_aspect_ratio = av_buffersink_get_sample_aspect_ratio(sink);
            enc->time_base = st->time_base;
            enc->pix_fmt = static_cast<AVPixelFormat>(av_buffersink_get_format(sink));

            // TODO tick_per_frame
        } else if (codec->type == AVMEDIA_TYPE_AUDIO) {
            st->time_base = { 1, av_buffersink_get_sample_rate(sink) };

            enc->sample_fmt = static_cast<AVSampleFormat>(av_buffersink_get_format(sink));
            enc->sample_rate = av_buffersink_get_sample_rate(sink);
            enc->channels = av_buffersink_get_channels(sink);
            enc->channel_layout = av_buffersink_get_channel_layout(sink);
            enc->time_base = st->time_base;

            // TODO check channels & channel_layout
        } else {
            // TODO
        }

        // TODO options
        FF(avcodec_open2(enc.get(), codec, nullptr));
        FF(avcodec_parameters_from_context(st->codecpar, enc.get()));

        if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
            enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
    }

    // TODO concurrency
    void send(AVFormatContext* oc, boost::optional<core::const_frame> in_frame)
    {
        int ret;
        std::shared_ptr<AVFrame> frame;
        std::shared_ptr<AVPacket> pkt;

        if (in_frame) {
            if (enc->codec_type == AVMEDIA_TYPE_VIDEO) {
                frame = make_av_video_frame(*in_frame);
                frame->pts = pts;
                pts += 1;
            } else if (enc->codec_type == AVMEDIA_TYPE_AUDIO) {
                frame = make_av_audio_frame(*in_frame);
                frame->pts = pts;
                pts += frame->nb_samples;
            } else {
                // TODO
            }
            FF(av_buffersrc_write_frame(source, frame.get()));
        } else {
            FF(av_buffersrc_close(source, pts, 0));
        }

        while (true) {
            pkt = alloc_packet();
            ret = avcodec_receive_packet(enc.get(), pkt.get());

            if (ret == AVERROR(EAGAIN)) {
                frame = alloc_frame();
                ret = av_buffersink_get_frame(sink, frame.get());
                if (ret == AVERROR(EAGAIN)) {
                    return;
                } else if (ret == AVERROR_EOF) {
                    FF(avcodec_send_frame(enc.get(), nullptr));
                } else {
                    FF_RET(ret, "av_buffersink_get_frame");
                    FF(avcodec_send_frame(enc.get(), frame.get()));
                }
            } else if (ret == AVERROR_EOF) {
                return;
            } else {
                FF_RET(ret, "avcodec_receive_packet");
                pkt->stream_index = st->index;
                FF(av_interleaved_write_frame(oc, pkt.get()));
            }
        }
    }
};

struct ffmpeg_consumer : public core::frame_consumer
{
    core::monitor::subject							monitor_subject_;
    int                                             channel_index_ = -1;
    core::video_format_desc                         format_desc_;

    spl::shared_ptr<diagnostics::graph>				graph_;

    std::string                                     path_;

    std::mutex                                      buffer_mutex_;
    std::condition_variable                         buffer_cond_;
    std::queue<boost::optional<core::const_frame>>  buffer_;
    std::thread                                     thread_;

public:
    ffmpeg_consumer(std::string path)
        : path_(std::move(path))
        , channel_index_([&]
        {
            boost::crc_16_type result;
            result.process_bytes(path.data(), path.length());
            return result.checksum();
        }())
    {
        diagnostics::register_graph(graph_);
        // TODO
        graph_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
        graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
    }

    ~ffmpeg_consumer()
    {
        if (thread_.joinable()) {
            {
                std::lock_guard<std::mutex> lock(buffer_mutex_);
                buffer_.push(boost::none);
            }
            buffer_cond_.notify_all();
            thread_.join();
        }
    }

    // frame consumer

    void initialize(const core::video_format_desc& format_desc, int channel_index) override
    {
        if (thread_.joinable()) {
            CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("Cannot reinitialize ffmpeg-consumer."));
        }

        format_desc_ = format_desc;
        channel_index_ = channel_index;

        graph_->set_text(print());

        // TODO reinit

        thread_ = std::thread([=]
        {
            static std::regex prot_exp("^.+:.*");

            boost::filesystem::path full_path = path_;

            if (!std::regex_match(path_, prot_exp)) {
                if (!full_path.is_complete()) {
                    full_path = u8(env::media_folder()) + path_;
                }

                if (boost::filesystem::exists(full_path)) {
                    boost::filesystem::remove(full_path);
                }

                boost::filesystem::create_directories(full_path.parent_path());
            }

            try {
                AVFormatContext* oc = nullptr;
                FF(avformat_alloc_output_context2(&oc, nullptr, nullptr, path_.c_str()));
                CASPAR_SCOPE_EXIT
                {
                    avformat_free_context(oc);
                };

                boost::optional<Stream> video_stream;
                if (oc->oformat->video_codec != AV_CODEC_ID_NONE) {
                    video_stream.emplace(oc, "", oc->oformat->video_codec, format_desc);
                }

                boost::optional<Stream> audio_stream;
                if (oc->oformat->audio_codec != AV_CODEC_ID_NONE) {
                    audio_stream.emplace(oc, "", oc->oformat->audio_codec, format_desc);
                }

                if (!(oc->oformat->flags & AVFMT_NOFILE)) {
                    // TODO options
                    FF(avio_open2(&oc->pb, full_path.string().c_str(), AVIO_FLAG_WRITE, nullptr, nullptr));
                }

                // TODO options
                FF(avformat_write_header(oc, nullptr))

                while (true) {
                    boost::optional<core::const_frame> frame;
                    {
                        std::unique_lock<std::mutex> lock(buffer_mutex_);
                        buffer_cond_.wait(lock, [&] { return !buffer_.empty(); });
                        frame = std::move(buffer_.front());
                        buffer_.pop();
                    }
                    if (video_stream) {
                        video_stream->send(oc, frame);
                    }
                    if (audio_stream) {
                        audio_stream->send(oc, frame);
                    }
                    if (!frame) {
                        break;
                    }
                }

                FF(av_write_trailer(oc));

                if (!(oc->oformat->flags & AVFMT_NOFILE)) {
                    FF(avio_closep(&oc->pb));
                }
            } catch (...) {
                // TODO
                CASPAR_LOG_CURRENT_EXCEPTION();
            }
        });
    }

    std::future<bool> send(core::const_frame frame) override
    {
        {
            std::lock_guard<std::mutex> lock(buffer_mutex_);
            // TODO overflow
            buffer_.push(std::move(frame));
        }
        buffer_cond_.notify_all();

        return make_ready_future(true);
    }

    std::wstring print() const override
    {
        return L"ffmpeg";
    }

    std::wstring name() const override
    {
        return L"ffmpeg";
    }

    bool has_synchronization_clock() const override
    {
        return false;
    }

    int buffer_depth() const override
    {
        // TODO is this safe?
        return -1;
    }

    int index() const override
    {
        return 100000 + channel_index_;
    }

    core::monitor::subject& monitor_output()
    {
        return monitor_subject_;
    }
};

 spl::shared_ptr<core::frame_consumer> create_consumer(
     const std::vector<std::wstring>& params,
     core::interaction_sink*,
     std::vector<spl::shared_ptr<core::video_channel>> channels
 ) {
     if (params.size() < 1 || (!boost::iequals(params.at(0), L"STREAM") && !boost::iequals(params.at(0), L"FILE")))
         return core::frame_consumer::empty();

     auto path = u8(params.size() > 1 ? params.at(1) : L"");

     return spl::make_shared<ffmpeg_consumer>(path);
}

spl::shared_ptr<core::frame_consumer> create_preconfigured_consumer(
    const boost::property_tree::wptree& ptree,
    core::interaction_sink*,
    std::vector<spl::shared_ptr<core::video_channel>> channels
) {
    return spl::make_shared<ffmpeg_consumer>(u8(ptree.get<std::wstring>(L"path", L"")));
}
}
}