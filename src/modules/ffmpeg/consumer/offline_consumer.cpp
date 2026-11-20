/*
 * Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
 *
 * This file is part of CasparCG (www.casparcg.com).
 *
 * CasparCG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CasparCG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ole Kristensen, Den Frie Vilje ApS, ole@kristensen.name
 */

#include "offline_consumer.h"

#include "../util/av_assert.h"
#include "../util/av_util.h"

#include <common/bit_depth.h>
#include <common/diagnostics/graph.h>
#include <common/env.h>
#include <common/future.h>
#include <common/log.h>
#include <common/memory.h>
#include <common/os/thread.h>
#include <common/scope_exit.h>
#include <common/timer.h>

#include <core/consumer/channel_info.h>
#include <core/frame/frame.h>
#include <core/video_format.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/regex.hpp>

#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4245)
#pragma warning(disable : 4701)
#include <boost/crc.hpp>
#pragma warning(pop)

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <tbb/concurrent_queue.h>
#include <tbb/parallel_invoke.h>

#include <memory>
#include <optional>
#include <thread>

namespace caspar { namespace ffmpeg {

// ---------------------------------------------------------------------------
// Stream — encoding pipeline (filter graph + codec + mux)
//
// This is a near-verbatim copy of the Stream struct in ffmpeg_consumer.cpp.
// It is duplicated here rather than extracted to a shared header to minimise
// the diff surface against the upstream CasparCG codebase.
// ---------------------------------------------------------------------------
struct OfflineStream
{
    std::shared_ptr<AVFilterGraph> graph  = nullptr;
    AVFilterContext*               sink   = nullptr;
    AVFilterContext*               source = nullptr;

    std::shared_ptr<AVCodecContext> enc = nullptr;
    AVStream*                       st  = nullptr;

    OfflineStream(AVFormatContext*                    oc,
                  std::string                         suffix,
                  AVCodecID                           codec_id,
                  const core::video_format_desc&      format_desc,
                  common::bit_depth                   depth,
                  std::map<std::string, std::string>& options)
    {
        std::map<std::string, std::string> stream_options;

        {
            auto tmp = std::move(options);
            for (auto& p : tmp) {
                if (boost::algorithm::ends_with(p.first, suffix)) {
                    const auto key = p.first.substr(0, p.first.size() - suffix.size());
                    stream_options.emplace(key, std::move(p.second));
                } else {
                    options.insert(std::move(p));
                }
            }
        }

        std::string filter_spec = "";
        {
            const auto it = stream_options.find("filter");
            if (it != stream_options.end()) {
                filter_spec = std::move(it->second);
                stream_options.erase(it);
            }
        }

        auto codec = avcodec_find_encoder(codec_id);
        {
            const auto it = stream_options.find("codec");
            if (it != stream_options.end()) {
                codec = avcodec_find_encoder_by_name(it->second.c_str());
                stream_options.erase(it);
            }
        }

        if (!codec) {
            FF_RET(AVERROR(EINVAL), "avcodec_find_encoder");
        }

        AVFilterInOut* outputs = nullptr;
        AVFilterInOut* inputs  = nullptr;

        CASPAR_SCOPE_EXIT
        {
            avfilter_inout_free(&inputs);
            avfilter_inout_free(&outputs);
        };

        graph = std::shared_ptr<AVFilterGraph>(avfilter_graph_alloc(),
                                               [](AVFilterGraph* ptr) { avfilter_graph_free(&ptr); });

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
                CASPAR_THROW_EXCEPTION(ffmpeg_error_t() << boost::errinfo_errno(EINVAL)
                                                        << msg_info_t("invalid filter graph input count"));
            }

            if (codec->type == AVMEDIA_TYPE_VIDEO) {
                const auto sar = boost::rational<int>(format_desc.square_width, format_desc.square_height) /
                                 boost::rational<int>(format_desc.width, format_desc.height);

                const auto pix_fmt = (depth == common::bit_depth::bit8) ? AV_PIX_FMT_BGRA : AV_PIX_FMT_BGRA64LE;

                auto args = (boost::format("video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:sar=%d/%d:frame_rate=%d/%d") %
                             format_desc.width % format_desc.height % pix_fmt % format_desc.duration %
                             (format_desc.time_scale * format_desc.field_count) % sar.numerator() % sar.denominator() %
                             (format_desc.framerate.numerator() * format_desc.field_count) %
                             format_desc.framerate.denominator())
                                .str();
                auto name = (boost::format("in_%d") % 0).str();

                FF(avfilter_graph_create_filter(
                    &source, avfilter_get_by_name("buffer"), name.c_str(), args.c_str(), nullptr, graph.get()));
                FF(avfilter_link(source, 0, cur->filter_ctx, cur->pad_idx));
            } else if (codec->type == AVMEDIA_TYPE_AUDIO) {
                auto args = (boost::format("time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%#x") % 1 %
                             format_desc.audio_sample_rate % format_desc.audio_sample_rate % AV_SAMPLE_FMT_S32 %
                             get_channel_layout_mask_for_channels(format_desc.audio_channels))
                                .str();
                auto name = (boost::format("in_%d") % 0).str();

                FF(avfilter_graph_create_filter(
                    &source, avfilter_get_by_name("abuffer"), name.c_str(), args.c_str(), nullptr, graph.get()));
                FF(avfilter_link(source, 0, cur->filter_ctx, cur->pad_idx));
            } else {
                CASPAR_THROW_EXCEPTION(ffmpeg_error_t() << boost::errinfo_errno(EINVAL)
                                                        << msg_info_t("invalid filter input media type"));
            }
        }

        if (codec->type == AVMEDIA_TYPE_VIDEO) {
            sink = FFMEM(avfilter_graph_alloc_filter(graph.get(), avfilter_get_by_name("buffersink"), "out"));

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4245)
#endif
#if LIBAVUTIL_VERSION_MAJOR >= 60 // FFmpeg 8
            const void* pix_fmts;
            int nb_pix_fmts = 0;
            FF(avcodec_get_supported_config(nullptr, codec, AV_CODEC_CONFIG_PIX_FORMAT, 0, &pix_fmts, &nb_pix_fmts));

            FF(av_opt_set_array(sink, "pixel_formats", AV_OPT_SEARCH_CHILDREN | AV_OPT_ARRAY_REPLACE,
                0, nb_pix_fmts, AV_OPT_TYPE_PIXEL_FMT, pix_fmts)
            );
#else
            FF(av_opt_set_int_list(sink, "pix_fmts", codec->pix_fmts, -1, AV_OPT_SEARCH_CHILDREN));
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif
        } else if (codec->type == AVMEDIA_TYPE_AUDIO) {
            sink = FFMEM(avfilter_graph_alloc_filter(graph.get(), avfilter_get_by_name("abuffersink"), "out"));
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4245)
#endif

#if LIBAVUTIL_VERSION_MAJOR >= 60 // FFmpeg 8
            const void* sample_fmts;
            int nb_sample_fmts = 0;
            FF(avcodec_get_supported_config(nullptr, codec, AV_CODEC_CONFIG_SAMPLE_FORMAT, 0, &sample_fmts, &nb_sample_fmts));

            FF(av_opt_set_array(sink, "sample_formats", AV_OPT_SEARCH_CHILDREN | AV_OPT_ARRAY_REPLACE,
                0, nb_sample_fmts, AV_OPT_TYPE_SAMPLE_FMT, sample_fmts)
            );

            const void* sample_rates;
            int nb_sample_rates = 0;
            FF(avcodec_get_supported_config(nullptr, codec, AV_CODEC_CONFIG_SAMPLE_RATE, 0, &sample_rates, &nb_sample_rates));

            FF(av_opt_set_array(sink, "samplerates", AV_OPT_SEARCH_CHILDREN | AV_OPT_ARRAY_REPLACE,
                0, nb_sample_rates, AV_OPT_TYPE_INT, sample_rates)
            );
#else
            FF(av_opt_set_int_list(sink, "sample_fmts", codec->sample_fmts, -1, AV_OPT_SEARCH_CHILDREN));
            FF(av_opt_set_int_list(sink, "sample_rates", codec->supported_samplerates, 0, AV_OPT_SEARCH_CHILDREN));
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif
        } else {
            CASPAR_THROW_EXCEPTION(ffmpeg_error_t()
                                   << boost::errinfo_errno(EINVAL) << msg_info_t("invalid output media type"));
        }

        FF(avfilter_init_str(sink, nullptr));

        {
            const auto cur = outputs;

            if (!cur || cur->next) {
                CASPAR_THROW_EXCEPTION(ffmpeg_error_t() << boost::errinfo_errno(EINVAL)
                                                        << msg_info_t("invalid filter graph output count"));
            }

            if (avfilter_pad_get_type(cur->filter_ctx->output_pads, cur->pad_idx) != codec->type) {
                CASPAR_THROW_EXCEPTION(ffmpeg_error_t() << boost::errinfo_errno(EINVAL)
                                                        << msg_info_t("invalid filter output media type"));
            }

            FF(avfilter_link(cur->filter_ctx, cur->pad_idx, sink, 0));
        }

        FF(avfilter_graph_config(graph.get(), nullptr));

        st = avformat_new_stream(oc, nullptr);
        if (!st) {
            FF_RET(AVERROR(ENOMEM), "avformat_new_stream");
        }

        enc = std::shared_ptr<AVCodecContext>(avcodec_alloc_context3(codec),
                                              [](AVCodecContext* ptr) { avcodec_free_context(&ptr); });

        if (!enc) {
            FF_RET(AVERROR(ENOMEM), "avcodec_alloc_context3")
        }

        if (codec->type == AVMEDIA_TYPE_VIDEO) {
            st->time_base = av_inv_q(av_buffersink_get_frame_rate(sink));

            st->avg_frame_rate = av_buffersink_get_frame_rate(sink);

            enc->width               = av_buffersink_get_w(sink);
            enc->height              = av_buffersink_get_h(sink);
            enc->framerate           = av_buffersink_get_frame_rate(sink);
            enc->sample_aspect_ratio = av_buffersink_get_sample_aspect_ratio(sink);
            enc->time_base           = st->time_base;
            enc->pix_fmt             = static_cast<AVPixelFormat>(av_buffersink_get_format(sink));
        } else if (codec->type == AVMEDIA_TYPE_AUDIO) {
            st->time_base = {1, av_buffersink_get_sample_rate(sink)};

            enc->sample_fmt  = static_cast<AVSampleFormat>(av_buffersink_get_format(sink));
            enc->sample_rate = av_buffersink_get_sample_rate(sink);
            enc->time_base   = st->time_base;

            FF(av_buffersink_get_ch_layout(sink, &enc->ch_layout));
        }

        // Offline mode: no realtime constraint, so allow full multithreaded encoding.
        if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
            enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        auto dict = to_dict(std::move(stream_options));
        CASPAR_SCOPE_EXIT { av_dict_free(&dict); };
        FF(avcodec_open2(enc.get(), codec, &dict));
        for (auto& p : to_map(&dict)) {
            options[p.first] = p.second + suffix;
        }

        FF(avcodec_parameters_from_context(st->codecpar, enc.get()));

        if (codec->type == AVMEDIA_TYPE_AUDIO && !(codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)) {
            av_buffersink_set_frame_size(sink, enc->frame_size);
        }
    }

    void send(std::tuple<core::const_frame, std::int64_t, std::int64_t>& data,
              const core::video_format_desc&                             format_desc,
              std::function<void(std::shared_ptr<AVPacket>)>             cb)
    {
        std::shared_ptr<AVFrame>  frame;
        std::shared_ptr<AVPacket> pkt;

        const auto [in_frame, video_pts, audio_pts] = data;

        if (in_frame) {
            if (enc->codec_type == AVMEDIA_TYPE_VIDEO) {
                frame      = make_av_video_frame(in_frame, format_desc);
                frame->pts = video_pts;
            } else if (enc->codec_type == AVMEDIA_TYPE_AUDIO) {
                frame      = make_av_audio_frame(in_frame, format_desc);
                frame->pts = audio_pts;
            }
            FF(av_buffersrc_write_frame(source, frame.get()));
        } else {
            FF(av_buffersrc_close(source, AV_NOPTS_VALUE, 0));
        }

        while (true) {
            pkt     = alloc_packet();
            int ret = avcodec_receive_packet(enc.get(), pkt.get());

            if (ret == AVERROR(EAGAIN)) {
                frame = alloc_frame();
                ret   = av_buffersink_get_frame(sink, frame.get());
                if (ret == AVERROR(EAGAIN)) {
                    return;
                }
                if (ret == AVERROR_EOF) {
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
                av_packet_rescale_ts(pkt.get(), enc->time_base, st->time_base);
                cb(std::move(pkt));
            }
        }
    }
};

// ---------------------------------------------------------------------------
// offline_consumer — renders to file as fast as the encoder allows
// ---------------------------------------------------------------------------
struct offline_consumer : public core::frame_consumer
{
    core::monitor::state    state_;
    mutable std::mutex      state_mutex_;
    int                     channel_index_ = -1;
    core::video_format_desc format_desc_;
    std::int64_t            video_pts_ = 0;
    std::int64_t            audio_pts_ = 0;

    spl::shared_ptr<diagnostics::graph> graph_;

    std::string path_;
    std::string args_;
    int         queue_depth_;

    std::exception_ptr exception_;
    std::mutex         exception_mutex_;

    // Bounded queue: pipeline pushes here, encoder thread pops.
    // When full, push() blocks -> back-pressure propagates upstream.
    tbb::concurrent_bounded_queue<std::tuple<core::const_frame, std::int64_t, std::int64_t>> frame_buffer_;
    std::thread                                                                              frame_thread_;

    common::bit_depth depth_;

  public:
    offline_consumer(std::string path, std::string args, int queue_depth, common::bit_depth depth)
        : channel_index_([&] {
            boost::crc_16_type result;
            result.process_bytes(path.data(), path.length());
            return result.checksum();
        }())
        , path_(std::move(path))
        , args_(std::move(args))
        , queue_depth_(queue_depth)
        , depth_(depth)
    {
        state_["offline/path"] = u8(path_);

        // The queue depth is the only tuning knob. Deeper = more decode/GPU
        // latency hidden, more memory used. Shallower = tighter back-pressure.
        frame_buffer_.set_capacity(queue_depth_);

        diagnostics::register_graph(graph_);
        graph_->set_color("frame-time",  diagnostics::color(0.1f, 1.0f, 0.1f));
        graph_->set_color("queue-depth", diagnostics::color(1.0f, 0.6f, 0.0f));
        graph_->set_color("enc-time",    diagnostics::color(0.8f, 0.1f, 0.1f));
    }

    ~offline_consumer()
    {
        if (frame_thread_.joinable()) {
            // Push EOS sentinel — the encoder thread will drain remaining frames,
            // flush the codec, write the trailer, and exit cleanly.
            frame_buffer_.push({core::const_frame{}, -1, -1});
            frame_thread_.join();
        }
    }

    // -- frame_consumer interface -------------------------------------------

    void initialize(const core::video_format_desc& format_desc,
                    const core::channel_info&      channel_info,
                    int                            port_index) override
    {
        if (frame_thread_.joinable()) {
            CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("Cannot reinitialize offline-consumer."));
        }

        format_desc_   = format_desc;
        channel_index_ = channel_info.index;

        graph_->set_text(print());

        // The encoding thread is identical in structure to ffmpeg_consumer's
        // thread, but is fed via blocking push rather than try_push.
        frame_thread_ = std::thread([=] {
            set_thread_name(L"[offline_consumer] encoder");
            try {
                std::map<std::string, std::string> options;
                {
                    static boost::regex opt_exp("-(?<NAME>[^\\s]+)(\\s+(?<VALUE>[^\\s]+))?");
                    for (auto it = boost::sregex_iterator(args_.begin(), args_.end(), opt_exp);
                         it != boost::sregex_iterator();
                         ++it) {
                        options[(*it)["NAME"].str().c_str()] =
                            (*it)["VALUE"].matched ? (*it)["VALUE"].str().c_str() : "";
                    }
                }

                boost::filesystem::path full_path = path_;

                static boost::regex prot_exp("^.+:.*");
                if (!boost::regex_match(path_, prot_exp)) {
                    if (!full_path.is_absolute()) {
                        full_path = u8(env::media_folder()) + path_;
                    }

                    if (boost::filesystem::exists(full_path)) {
                        boost::filesystem::remove(full_path);
                    }

                    boost::filesystem::create_directories(full_path.parent_path());
                }

                AVFormatContext* oc = nullptr;

                {
                    std::string format;
                    {
                        const auto format_it = options.find("format");
                        if (format_it != options.end()) {
                            format = std::move(format_it->second);
                            options.erase(format_it);
                        }
                    }

                    FF(avformat_alloc_output_context2(
                        &oc, nullptr, !format.empty() ? format.c_str() : nullptr, path_.c_str()));
                }

                CASPAR_SCOPE_EXIT { avformat_free_context(oc); };

                std::optional<OfflineStream> video_stream;
                if (oc->oformat->video_codec != AV_CODEC_ID_NONE) {
                    if (oc->oformat->video_codec == AV_CODEC_ID_H264 && options.find("preset:v") == options.end()) {
                        options["preset:v"] = "veryfast";
                    }
                    video_stream.emplace(oc, ":v", oc->oformat->video_codec, format_desc, depth_, options);
                }

                std::optional<OfflineStream> audio_stream;
                if (oc->oformat->audio_codec != AV_CODEC_ID_NONE) {
                    audio_stream.emplace(oc, ":a", oc->oformat->audio_codec, format_desc, depth_, options);
                }

                if (!(oc->oformat->flags & AVFMT_NOFILE)) {
                    auto dict = to_dict(std::move(options));
                    CASPAR_SCOPE_EXIT { av_dict_free(&dict); };
                    FF(avio_open2(&oc->pb, full_path.string().c_str(), AVIO_FLAG_WRITE, nullptr, &dict));
                    options = to_map(&dict);
                }

                {
                    auto dict = to_dict(std::move(options));
                    CASPAR_SCOPE_EXIT { av_dict_free(&dict); };
                    FF(avformat_write_header(oc, &dict));
                    options = to_map(&dict);
                }

                {
                    for (auto& p : options) {
                        CASPAR_LOG(warning) << print() << " Unused option " << p.first << "=" << p.second;
                    }
                }

                // Packet writer thread — same pattern as ffmpeg_consumer.
                tbb::concurrent_bounded_queue<std::shared_ptr<AVPacket>> packet_buffer;
                packet_buffer.set_capacity(128);
                auto packet_thread = std::thread([&] {
                    try {
                        CASPAR_SCOPE_EXIT
                        {
                            if (!(oc->oformat->flags & AVFMT_NOFILE)) {
                                FF(avio_closep(&oc->pb));
                            }
                        };

                        std::map<int, int64_t> count;

                        std::shared_ptr<AVPacket> pkt;
                        while (true) {
                            packet_buffer.pop(pkt);
                            if (!pkt) {
                                break;
                            }
                            count[pkt->stream_index] += 1;
                            FF(av_interleaved_write_frame(oc, pkt.get()));
                        }

                        auto video_st = video_stream ? video_stream->st : nullptr;
                        auto audio_st = audio_stream ? audio_stream->st : nullptr;

                        if ((!video_st || count[video_st->index]) && (!audio_st || count[audio_st->index])) {
                            FF(av_write_trailer(oc));
                        }

                    } catch (...) {
                        CASPAR_LOG_CURRENT_EXCEPTION();
                        packet_buffer.abort();
                    }
                });
                CASPAR_SCOPE_EXIT
                {
                    if (packet_thread.joinable()) {
                        packet_buffer.push(nullptr);
                        packet_buffer.abort();
                        packet_thread.join();
                    }
                };

                auto packet_cb = [&](std::shared_ptr<AVPacket>&& pkt) { packet_buffer.push(std::move(pkt)); };

                std::int64_t frame_number = 0;
                while (true) {
                    {
                        std::lock_guard<std::mutex> lock(state_mutex_);
                        state_["offline/frame"] = frame_number++;
                    }

                    std::tuple<core::const_frame, std::int64_t, std::int64_t> data;
                    try {
                        frame_buffer_.pop(data);
                    } catch (const tbb::user_abort&) {
                        // Queue was aborted during shutdown — treat as EOS.
                        data = {core::const_frame{}, -1, -1};
                    }

                    graph_->set_value("queue-depth",
                                      static_cast<double>(frame_buffer_.size() + 0.001) / frame_buffer_.capacity());

                    caspar::timer enc_timer;
                    tbb::parallel_invoke(
                        [&] {
                            if (video_stream) {
                                video_stream->send(data, format_desc, packet_cb);
                            }
                        },
                        [&] {
                            if (audio_stream) {
                                audio_stream->send(data, format_desc, packet_cb);
                            }
                        });
                    graph_->set_value("enc-time", enc_timer.elapsed() * format_desc.fps * 0.5);

                    if (!std::get<0>(data)) {
                        packet_buffer.push(nullptr);
                        break;
                    }
                }

                packet_thread.join();

                CASPAR_LOG(info) << print() << L" Offline render complete. Frames: " << frame_number - 1;

            } catch (...) {
                std::lock_guard<std::mutex> lock(exception_mutex_);
                exception_ = std::current_exception();
            }
        });

        CASPAR_LOG(info) << print() << L" Initialized. Queue depth: " << queue_depth_ << L". Path: " << u16(path_);
    }

    // -- send() — the back-pressure mechanism ------------------------------
    //
    // Unlike ffmpeg_consumer which uses try_push (dropping frames on overflow),
    // the offline consumer uses a BLOCKING push. This is the core of the
    // offline rendering design: when the encoder can't keep up, the push
    // blocks, which stalls the channel output loop, which stalls the mixer
    // and stage. The pipeline self-throttles to the encoder's sustained
    // throughput. No wall-clock, no sleep, no multiplier.
    std::future<bool> send(core::video_field field, core::const_frame frame) override
    {
        {
            std::lock_guard<std::mutex> lock(exception_mutex_);
            if (exception_ != nullptr) {
                std::rethrow_exception(exception_);
            }
        }

        caspar::timer frame_timer;

        // Blocking push: the queue's capacity is the only throttle.
        // When full, this blocks the channel output loop -> back-pressure.
        frame_buffer_.push({frame, video_pts_, audio_pts_});

        video_pts_ += 1;
        audio_pts_ += frame.audio_data().size() / format_desc_.audio_channels;

        graph_->set_value("queue-depth",
                          static_cast<double>(frame_buffer_.size() + 0.001) / frame_buffer_.capacity());
        graph_->set_value("frame-time", frame_timer.elapsed() * format_desc_.fps * 0.5);

        // Resolve immediately — do NOT wait for encoding to complete.
        return make_ready_future(true);
    }

    std::wstring print() const override { return L"offline[" + u16(path_) + L"]"; }

    std::wstring name() const override { return L"offline"; }

    // KEY PROPERTY: returning true tells output.cpp to skip its wall-clock
    // sleep (which enforces real-time when no consumer provides timing).
    // The offline consumer's blocking queue push is the actual throttle —
    // the pipeline runs as fast as the encoder drains the queue.
    // Returning false here would cause output.cpp to sleep at real-time
    // rate, defeating the purpose of offline rendering.
    bool has_synchronization_clock() const override { return true; }

    int index() const override { return 200000 + channel_index_; }

    core::monitor::state state() const override
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return state_;
    }
};

// ---------------------------------------------------------------------------
// Factory functions
// ---------------------------------------------------------------------------

spl::shared_ptr<core::frame_consumer>
create_offline_consumer(const std::vector<std::wstring>&                         params,
                        const core::video_format_repository&                     format_repository,
                        const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                        const core::channel_info&                                channel_info)
{
    if (params.size() < 2 || !boost::iequals(params.at(0), L"OFFLINE"))
        return core::frame_consumer::empty();

    auto                     path = u8(params.at(1));
    std::vector<std::string> args;
    for (auto n = 2u; n < params.size(); ++n) {
        args.emplace_back(u8(params[n]));
    }

    // Default queue depth; can be overridden via -queue-depth N in args.
    int queue_depth = 4;
    for (std::size_t i = 0; i + 1 < args.size(); ++i) {
        if (args[i] == "-queue-depth") {
            try {
                queue_depth = std::stoi(args[i + 1]);
                if (queue_depth < 1) queue_depth = 1;
                if (queue_depth > 128) queue_depth = 128;
            } catch (...) {
                CASPAR_LOG(warning) << L"[offline] Invalid -queue-depth value, using default 4.";
                queue_depth = 4;
            }
            args.erase(args.begin() + i, args.begin() + i + 2);
            break;
        }
    }

    return spl::make_shared<offline_consumer>(path, boost::join(args, " "), queue_depth, channel_info.depth);
}

spl::shared_ptr<core::frame_consumer>
create_preconfigured_offline_consumer(const boost::property_tree::wptree&                      ptree,
                                      const core::video_format_repository&                     format_repository,
                                      const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                                      const core::channel_info&                                channel_info)
{
    return spl::make_shared<offline_consumer>(u8(ptree.get<std::wstring>(L"path", L"")),
                                              u8(ptree.get<std::wstring>(L"args", L"")),
                                              ptree.get<int>(L"queue-depth", 4),
                                              channel_info.depth);
}

}} // namespace caspar::ffmpeg
