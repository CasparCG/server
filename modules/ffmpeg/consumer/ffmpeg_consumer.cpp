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
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "ffmpeg_consumer.h"

#include "../util/av_util.h"
#include "../util/av_assert.h"

#include <common/memory.h>
#include <common/future.h>
#include <common/env.h>
#include <common/executor.h>
#include <common/timer.h>
#include <common/scope_exit.h>
#include <common/diagnostics/graph.h>

#include <core/video_format.h>
#include <core/frame/frame.h>

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/format.hpp>
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

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

#include <tbb/concurrent_queue.h>

#include <memory>
#include <thread>

namespace caspar {
namespace ffmpeg {

// TODO multiple output streams
// TODO multiple output files
// TODO run video filter, video encoder, audio filter, audio encoder in separate threads.

struct Stream
{
    std::shared_ptr<AVFilterGraph> graph = nullptr;
    AVFilterContext* sink = nullptr;
    AVFilterContext* source = nullptr;

    std::shared_ptr<AVCodecContext> enc = nullptr;
    AVStream* st  = nullptr;

    int64_t pts = 0;

    Stream(AVFormatContext* oc, std::string suffix, AVCodecID codec_id, const core::video_format_desc& format_desc, AVDictionary** options)
    {
        AVDictionary* stream_options = nullptr;
        CASPAR_SCOPE_EXIT
        {
            AVDictionaryEntry* t = nullptr;
            while (stream_options) {
                t = av_dict_get(stream_options, "", t, AV_DICT_IGNORE_SUFFIX);
                if (!t) {
                    break;
                }
                FF(av_dict_set(options, (t->key + suffix).c_str(), t->value, 0));
            }
        };

        {
            AVDictionaryEntry* t = nullptr;
            while (*options) {
                t = av_dict_get(*options, "", t, AV_DICT_IGNORE_SUFFIX);
                if (!t) {
                    break;
                }
                auto key = std::string(t->key);
                if (boost::algorithm::ends_with(key, suffix)) {
                    FF(av_dict_set(&stream_options, key.substr(0, key.size() - suffix.size()).c_str(), t->value, 0));
                    FF(av_dict_set(options, t->key, nullptr, 0));
                }
            }
        }

        std::string filter_spec = "";
        {
            const auto filter_opt = stream_options ? av_dict_get(stream_options, "filter", nullptr, 0) : nullptr;
            if (filter_opt) {
                filter_spec = filter_opt->value;
                FF(av_dict_set(&stream_options, "filter", nullptr, 0));
            }
        }

        auto codec = avcodec_find_encoder(codec_id);
        {
            const auto codec_opt = stream_options ? av_dict_get(stream_options, "codec", nullptr, 0) : nullptr;
            if (codec_opt) {
                codec = avcodec_find_encoder_by_name(codec_opt->value);
                FF(av_dict_set(&stream_options, "codec", nullptr, 0));
            }
        }

        if (!codec) {
            FF_RET(AVERROR(EINVAL), "avcodec_find_encoder");
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
                const auto sar =
                    boost::rational<int>(format_desc.square_width, format_desc.square_height) /
                    boost::rational<int>(format_desc.width, format_desc.height);

                auto args = (boost::format("video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:sar=%d/%d:frame_rate=%d/%d")
                    % format_desc.width % format_desc.height
                    % AV_PIX_FMT_BGRA
                    % format_desc.duration % format_desc.time_scale
                    % sar.numerator() % sar.denominator()
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
            // TODO codec->profiles
            // TODO FF(av_opt_set_int_list(sink, "framerates", codec->supported_framerates, { 0, 0 }, AV_OPT_SEARCH_CHILDREN));
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
            // TODO codec->profiles
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

        FF(avcodec_open2(enc.get(), codec, &stream_options));
        FF(avcodec_parameters_from_context(st->codecpar, enc.get()));

        if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
            enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
    }

    void send(boost::optional<core::const_frame> in_frame, const core::video_format_desc& format_desc, std::function<void(std::shared_ptr<AVPacket>)> cb)
    {
        int ret;
        std::shared_ptr<AVFrame> frame;
        std::shared_ptr<AVPacket> pkt;

        if (in_frame) {
            if (enc->codec_type == AVMEDIA_TYPE_VIDEO) {
                frame = make_av_video_frame(*in_frame, format_desc);
                frame->pts = pts;
                pts += 1;
            } else if (enc->codec_type == AVMEDIA_TYPE_AUDIO) {
                frame = make_av_audio_frame(*in_frame, format_desc);
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
                av_packet_rescale_ts(pkt.get(), enc->time_base, st->time_base);
                cb(std::move(pkt));
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
    std::string                                     args_;

    tbb::concurrent_bounded_queue<boost::optional<core::const_frame>>  frame_buffer_;
    std::thread                                                        frame_thread_;

public:
    ffmpeg_consumer(std::string path, std::string args)
        : path_(std::move(path))
        , args_(std::move(args))
        , channel_index_([&]
        {
            boost::crc_16_type result;
            result.process_bytes(path.data(), path.length());
            return result.checksum();
        }())
    {
        frame_buffer_.set_capacity(128);

        diagnostics::register_graph(graph_);
        graph_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
        graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
    }

    ~ffmpeg_consumer()
    {
        if (frame_thread_.joinable()) {
            frame_buffer_.try_push(boost::none);
            frame_thread_.join();
        }
    }

    // frame consumer

    void initialize(const core::video_format_desc& format_desc, int channel_index) override
    {
        if (frame_thread_.joinable()) {
            CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("Cannot reinitialize ffmpeg-consumer."));
        }

        format_desc_ = format_desc;
        channel_index_ = channel_index;

        graph_->set_text(print());

        frame_thread_ = std::thread([=]
        {
            AVDictionary* options = nullptr;
            {
                static boost::regex opt_exp("-(?<NAME>[^-\\s]+)(\\s+(?<VALUE>[^\\s]+))?");
                for (auto it =boost::sregex_iterator(args_.begin(), args_.end(), opt_exp); it != boost::sregex_iterator(); ++it) {
                    FF(av_dict_set(&options, (*it)["NAME"].str().c_str(), (*it)["VALUE"].matched ? (*it)["VALUE"].str().c_str() : "", 0));
                }
            }

            boost::filesystem::path full_path = path_;

            static boost::regex prot_exp("^.+:.*");
            if (!boost::regex_match(path_, prot_exp)) {
                if (!full_path.is_complete()) {
                    full_path = u8(env::media_folder()) + path_;
                }

                // TODO -y?
                if (boost::filesystem::exists(full_path)) {
                    boost::filesystem::remove(full_path);
                }

                boost::filesystem::create_directories(full_path.parent_path());
            }

            try {
                AVFormatContext* oc = nullptr;

                {
                    const auto format_opt = options ? av_dict_get(options, "format", nullptr, 0) : nullptr;
                    FF(avformat_alloc_output_context2(&oc, nullptr, format_opt ? format_opt->value : nullptr, path_.c_str()));
                    FF(av_dict_set(&options, "format", nullptr, 0));
                }

                CASPAR_SCOPE_EXIT
                {
                    avformat_free_context(oc);
                };

                boost::optional<Stream> video_stream;
                if (oc->oformat->video_codec != AV_CODEC_ID_NONE) {
                    video_stream.emplace(oc, ":v", oc->oformat->video_codec, format_desc, &options);
                }

                boost::optional<Stream> audio_stream;
                if (oc->oformat->audio_codec != AV_CODEC_ID_NONE) {
                    audio_stream.emplace(oc, ":a", oc->oformat->audio_codec, format_desc, &options);
                }

                if (!(oc->oformat->flags & AVFMT_NOFILE)) {
                    FF(avio_open2(&oc->pb, full_path.string().c_str(), AVIO_FLAG_WRITE, nullptr, &options));
                }

                FF(avformat_write_header(oc, &options))

                {
                    AVDictionaryEntry* t = nullptr;
                    while (options) {
                        t = av_dict_get(options, "", t, AV_DICT_IGNORE_SUFFIX);
                        if (!t) {
                            break;
                        }
                        CASPAR_LOG(warning) << print() << " Unused option " << t->key << "=" << t->value;
                    }
                }

                // TODO errors

                tbb::concurrent_bounded_queue<std::shared_ptr<AVPacket>> packet_buffer;
                packet_buffer.set_capacity(128);
                auto packet_thread = std::thread([&]
                {
                    try {
                        std::shared_ptr<AVPacket> pkt;
                        while (true) {
                            packet_buffer.pop(pkt);
                            if (!pkt) {
                                break;
                            }
                            FF(av_interleaved_write_frame(oc, pkt.get()));
                        }
                    } catch (...) {
                        // TODO
                    }
                });
                CASPAR_SCOPE_EXIT
                {
                    if (packet_thread.joinable()) {
                        packet_buffer.push(nullptr);
                        packet_thread.join();
                    }
                };

                auto packet_cb = [&](std::shared_ptr<AVPacket>&& pkt)
                {
                    packet_buffer.push(std::move(pkt));
                };

                while (true) {
                    boost::optional<core::const_frame> frame;
                    frame_buffer_.pop(frame);

                    caspar::timer frame_timer;
                    if (video_stream) {
                        video_stream->send(frame, format_desc, packet_cb);
                    }
                    if (audio_stream) {
                        audio_stream->send(frame, format_desc, packet_cb);
                    }
                    graph_->set_value("frame-time", frame_timer.elapsed() * format_desc.fps * 0.5);

                    if (!frame) {
                        packet_buffer.push(nullptr);
                        break;
                    }
                }

                packet_thread.join();

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
        if (!frame_buffer_.try_push(std::move(frame))) {
            graph_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
        } else {
            frame_buffer_.push(std::move(frame));
        }

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
     if (params.size() < 2 || (!boost::iequals(params.at(0), L"STREAM") && !boost::iequals(params.at(0), L"FILE")))
         return core::frame_consumer::empty();

     auto path = u8(params.size() > 1 ? params.at(1) : L"");
     std::vector<std::string> args;
     for (auto n = 2; n < params.size(); ++n) {
         args.emplace_back(u8(params[n]));
     }
     return spl::make_shared<ffmpeg_consumer>(path, boost::join(args, L" "));
}

spl::shared_ptr<core::frame_consumer> create_preconfigured_consumer(
    const boost::property_tree::wptree& ptree,
    core::interaction_sink*,
    std::vector<spl::shared_ptr<core::video_channel>> channels
) {
    return spl::make_shared<ffmpeg_consumer>(u8(ptree.get<std::wstring>(L"path", L"")), u8(ptree.get<std::wstring>(L"args", L"")));
}
}
}