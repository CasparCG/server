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

#include "../StdAfx.h"

#include "decklink_producer.h"

#include "../util/util.h"

#include <common/diagnostics/graph.h>
#include <common/except.h>
#include <common/executor.h>
#include <common/log.h>
#include <common/param.h>
#include <common/scope_exit.h>
#include <common/timer.h>

#include <ffmpeg/util/av_assert.h>
#include <ffmpeg/util/av_util.h>

#include <core/diagnostics/call_context.h>
#include <core/frame/draw_frame.h>
#include <core/frame/frame_factory.h>
#include <core/monitor/monitor.h>
#include <core/producer/frame_producer.h>

#include <tbb/concurrent_queue.h>

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/range/adaptor/transformed.hpp>

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
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timecode.h>
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <boost/format.hpp>

#include "../decklink_api.h"

using namespace caspar::ffmpeg;

namespace caspar { namespace decklink {

struct Filter
{
    std::shared_ptr<AVFilterGraph> graph        = nullptr;
    AVFilterContext*               sink         = nullptr;
    AVFilterContext*               video_source = nullptr;
    AVFilterContext*               audio_source = nullptr;

    Filter() {}

    Filter(std::string                    filter_spec,
           AVMediaType                    type,
           const core::video_format_desc& format_desc,
           com_ptr<IDeckLinkDisplayMode>  dm)
    {
        BMDTimeScale timeScale;
        BMDTimeValue frameDuration;
        dm->GetFrameRate(&frameDuration, &timeScale);

        if (type == AVMEDIA_TYPE_VIDEO) {
            if (filter_spec.empty()) {
                filter_spec = "null";
            }

            bool                 doScale = dm->GetHeight() != format_desc.height || dm->GetWidth() != format_desc.width;
            boost::rational<int> bmdFramerate(
                timeScale / 1000 * (dm->GetFieldDominance() == bmdProgressiveFrame ? 1 : 2), frameDuration / 1000);
            bool doFps = bmdFramerate != format_desc.framerate;
            bool i2p   = (dm->GetFieldDominance() != bmdProgressiveFrame) && (1 == format_desc.field_count);

            std::string deintStr =
                (doScale || doFps || i2p) ? ",bwdif=mode=send_field" : ",yadif=mode=send_field_nospatial";
            switch (dm->GetFieldDominance()) {
                case bmdUpperFieldFirst:
                    filter_spec += deintStr + ":parity=tff:deint=all";
                    break;
                case bmdLowerFieldFirst:
                    filter_spec += deintStr + ":parity=bff:deint=all";
                    break;
                case bmdUnknownFieldDominance:
                    filter_spec += deintStr + ":parity=auto:deint=interlaced";
                    break;
            }

            if (doScale) {
                filter_spec += (boost::format(",scale=%dx%d") % format_desc.width % format_desc.height).str();
            }

            if (doFps) {
                filter_spec += (boost::format(",fps=%d/%d") % format_desc.framerate.numerator() %
                                format_desc.framerate.denominator())
                                   .str();
            }
        } else {
            if (filter_spec.empty()) {
                filter_spec = "anull";
            }
            filter_spec +=
                (boost::format(",aresample=sample_rate=%d:async=2000") % format_desc.audio_sample_rate).str();
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
                const auto filter_type = avfilter_pad_get_type(cur->filter_ctx->input_pads, cur->pad_idx);
                if (filter_type == AVMEDIA_TYPE_VIDEO) {
                    video_input_count += 1;
                } else if (filter_type == AVMEDIA_TYPE_AUDIO) {
                    audio_input_count += 1;
                }
            }
        }

        graph = std::shared_ptr<AVFilterGraph>(avfilter_graph_alloc(),
                                               [](AVFilterGraph* ptr) { avfilter_graph_free(&ptr); });

        if (!graph) {
            FF_RET(AVERROR(ENOMEM), "avfilter_graph_alloc");
        }

        graph->nb_threads = 16;
        graph->execute    = graph_execute;

        FF(avfilter_graph_parse2(graph.get(), filter_spec.c_str(), &inputs, &outputs));

        for (auto cur = inputs; cur; cur = cur->next) {
            const auto filter_type = avfilter_pad_get_type(cur->filter_ctx->input_pads, cur->pad_idx);

            if (filter_type == AVMEDIA_TYPE_VIDEO) {
                if (video_source) {
                    CASPAR_THROW_EXCEPTION(ffmpeg_error_t() << boost::errinfo_errno(EINVAL)
                                                            << msg_info_t("only single video input supported"));
                }
                const auto sar = boost::rational<int>(format_desc.square_width, format_desc.square_height) /
                                 boost::rational<int>(format_desc.width, format_desc.height);

                auto args =
                    (boost::format("video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:sar=%d/%d:frame_rate=%d/%d") %
                     dm->GetWidth() % dm->GetHeight() % AV_PIX_FMT_UYVY422 % 1 % AV_TIME_BASE % sar.numerator() %
                     sar.denominator() % (timeScale / 1000 * (dm->GetFieldDominance() == bmdProgressiveFrame ? 1 : 2)) %
                     (frameDuration / 1000))
                        .str();
                auto name = (boost::format("in_%d") % 0).str();

                FF(avfilter_graph_create_filter(
                    &video_source, avfilter_get_by_name("buffer"), name.c_str(), args.c_str(), nullptr, graph.get()));
                FF(avfilter_link(video_source, 0, cur->filter_ctx, cur->pad_idx));
            } else if (filter_type == AVMEDIA_TYPE_AUDIO) {
                if (audio_source) {
                    CASPAR_THROW_EXCEPTION(ffmpeg_error_t() << boost::errinfo_errno(EINVAL)
                                                            << msg_info_t("only single audio input supported"));
                }

                auto args = (boost::format("time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%#x") % 1 %
                             format_desc.audio_sample_rate % format_desc.audio_sample_rate % AV_SAMPLE_FMT_S32 %
                             av_get_default_channel_layout(format_desc.audio_channels))
                                .str();
                auto name = (boost::format("in_%d") % 0).str();

                FF(avfilter_graph_create_filter(
                    &audio_source, avfilter_get_by_name("abuffer"), name.c_str(), args.c_str(), nullptr, graph.get()));
                FF(avfilter_link(audio_source, 0, cur->filter_ctx, cur->pad_idx));
            } else {
                CASPAR_THROW_EXCEPTION(ffmpeg_error_t() << boost::errinfo_errno(EINVAL)
                                                        << msg_info_t("only video and audio filters supported"));
            }
        }

        if (type == AVMEDIA_TYPE_VIDEO) {
            FF(avfilter_graph_create_filter(
                &sink, avfilter_get_by_name("buffersink"), "out", nullptr, nullptr, graph.get()));

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4245)
#endif
            AVPixelFormat pix_fmts[] = {AV_PIX_FMT_UYVY422, AV_PIX_FMT_NONE};
            FF(av_opt_set_int_list(sink, "pix_fmts", pix_fmts, -1, AV_OPT_SEARCH_CHILDREN));
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        } else if (type == AVMEDIA_TYPE_AUDIO) {
            FF(avfilter_graph_create_filter(
                &sink, avfilter_get_by_name("abuffersink"), "out", nullptr, nullptr, graph.get()));
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4245)
#endif
            AVSampleFormat sample_fmts[]     = {AV_SAMPLE_FMT_S32, AV_SAMPLE_FMT_NONE};
            int64_t        channel_layouts[] = {av_get_default_channel_layout(format_desc.audio_channels), 0};
            int            sample_rates[]    = {format_desc.audio_sample_rate, 0};
            FF(av_opt_set_int_list(sink, "sample_fmts", sample_fmts, -1, AV_OPT_SEARCH_CHILDREN));
            FF(av_opt_set_int_list(sink, "channel_layouts", channel_layouts, 0, AV_OPT_SEARCH_CHILDREN));
            FF(av_opt_set_int_list(sink, "sample_rates", sample_rates, 0, AV_OPT_SEARCH_CHILDREN));
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        } else {
            CASPAR_THROW_EXCEPTION(ffmpeg_error_t()
                                   << boost::errinfo_errno(EINVAL) << msg_info_t("invalid output media type"));
        }

        {
            const auto cur = outputs;

            if (!cur || cur->next) {
                CASPAR_THROW_EXCEPTION(ffmpeg_error_t() << boost::errinfo_errno(EINVAL)
                                                        << msg_info_t("invalid filter graph output count"));
            }

            if (avfilter_pad_get_type(cur->filter_ctx->output_pads, cur->pad_idx) != type) {
                CASPAR_THROW_EXCEPTION(ffmpeg_error_t() << boost::errinfo_errno(EINVAL)
                                                        << msg_info_t("invalid filter output media type"));
            }

            FF(avfilter_link(cur->filter_ctx, cur->pad_idx, sink, 0));
        }

        FF(avfilter_graph_config(graph.get(), nullptr));

        CASPAR_LOG(debug) << avfilter_graph_dump(graph.get(), nullptr);
    }
};

class decklink_producer : public IDeckLinkInputCallback
{
    const int                           device_index_;
    core::monitor::state                state_;
    mutable std::mutex                  state_mutex_;
    spl::shared_ptr<diagnostics::graph> graph_;
    caspar::timer                       tick_timer_;

    com_ptr<IDeckLink>                 decklink_   = get_device(device_index_);
    com_iface_ptr<IDeckLinkInput>      input_      = iface_cast<IDeckLinkInput>(decklink_);
    com_iface_ptr<IDeckLinkAttributes> attributes_ = iface_cast<IDeckLinkAttributes>(decklink_);

    const std::wstring model_name_ = get_model_name(decklink_);

    core::video_format_desc              format_desc_;
    std::vector<int>                     audio_cadence_ = format_desc_.audio_cadence;
    spl::shared_ptr<core::frame_factory> frame_factory_;

    double in_sync_  = 0.0;
    double out_sync_ = 0.0;

    bool freeze_on_lost_;
    bool has_signal_;

    tbb::concurrent_bounded_queue<core::draw_frame> frame_buffer_;
    core::draw_frame                                last_frame_;

    std::exception_ptr exception_;

    com_ptr<IDeckLinkDisplayMode> mode_;

    core::video_format_desc input_format;

    std::string vfilter_;
    std::string afilter_;

    Filter video_filter_;
    Filter audio_filter_;

  public:
    decklink_producer(const core::video_format_desc&              format_desc,
                      int                                         device_index,
                      const spl::shared_ptr<core::frame_factory>& frame_factory,
                      const std::string&                          vfilter,
                      const std::string&                          afilter,
                      const std::wstring&                         format,
                      bool                                        freeze_on_lost)
        : device_index_(device_index)
        , format_desc_(format_desc)
        , frame_factory_(frame_factory)
        , freeze_on_lost_(freeze_on_lost)
        , input_format(format_desc_)
        , vfilter_(vfilter)
        , afilter_(afilter)
    {
        // use user-provided format if available, or choose the channel's output format
        if (!format.empty()) {
            input_format = core::video_format_desc(format);
        }

        mode_         = get_display_mode(input_, input_format.format, bmdFormat8BitYUV, bmdVideoOutputFlagDefault);
        video_filter_ = Filter(vfilter_, AVMEDIA_TYPE_VIDEO, format_desc_, mode_);
        audio_filter_ = Filter(afilter_, AVMEDIA_TYPE_AUDIO, format_desc_, mode_);

        boost::range::rotate(audio_cadence_, std::end(audio_cadence_) - 1);

        frame_buffer_.set_capacity(3);

        graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
        graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.3f));
        graph_->set_color("frame-time", diagnostics::color(1.0f, 0.0f, 0.0f));
        graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
        graph_->set_color("output-buffer", diagnostics::color(0.0f, 1.0f, 0.0f));
        graph_->set_color("in-sync", diagnostics::color(1.0f, 0.2f, 0.0f));
        graph_->set_color("out-sync", diagnostics::color(0.0f, 0.2f, 1.0f));
        graph_->set_text(print());
        diagnostics::register_graph(graph_);

        BOOL status = FALSE;
        int  flags  = bmdVideoInputEnableFormatDetection;

        if (!format.empty()) {
            flags = 0;
        } else if (FAILED(attributes_->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &status)) || !status) {
            CASPAR_LOG(warning) << L"Decklink producer does not support auto detect input, you can explicitly choose a "
                                   L"format by appending FORMAT";
            flags = 0;
        }

        if (FAILED(input_->EnableVideoInput(mode_->GetDisplayMode(), bmdFormat8BitYUV, flags))) {
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Could not enable video input.")
                                                      << boost::errinfo_api_function("EnableVideoInput"));
        }

        if (FAILED(input_->EnableAudioInput(bmdAudioSampleRate48kHz,
                                            bmdAudioSampleType32bitInteger,
                                            static_cast<int>(format_desc_.audio_channels)))) {
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Could not enable audio input.")
                                                      << boost::errinfo_api_function("EnableAudioInput"));
        }

        if (FAILED(input_->SetCallback(this)) != S_OK) {
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Unable to set input callback.")
                                                      << boost::errinfo_api_function("SetCallback"));
        }

        if (FAILED(input_->StartStreams())) {
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Unable to start input stream.")
                                                      << boost::errinfo_api_function("StartStreams"));
        }

        CASPAR_LOG(info) << print() << L" Initialized";
    }

    ~decklink_producer()
    {
        if (input_ != nullptr) {
            input_->StopStreams();
            input_->DisableVideoInput();
        }
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID*) override { return E_NOINTERFACE; }
    ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }

    HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents notificationEvents,
                                                      IDeckLinkDisplayMode*            newDisplayMode,
                                                      BMDDetectedVideoInputFormatFlags /*detectedSignalFlags*/) override
    {
        try {
            auto newMode = newDisplayMode->GetDisplayMode();
            auto fmt     = get_caspar_video_format(newMode);

            auto new_fmt = core::video_format_desc(fmt);

            CASPAR_LOG(info) << print() << L" Input format changed from " << input_format.name << L" to "
                             << new_fmt.name;

            input_->PauseStreams();

            // reinitializing filters because not all filters can handle on-the-fly format changes
            input_format = new_fmt;
            mode_        = get_display_mode(input_, newMode, bmdFormat8BitYUV, bmdVideoOutputFlagDefault);

            graph_->set_text(print());

            video_filter_ = Filter(vfilter_, AVMEDIA_TYPE_VIDEO, format_desc_, mode_);
            audio_filter_ = Filter(afilter_, AVMEDIA_TYPE_AUDIO, format_desc_, mode_);

            // reinitializing video input with the new display mode
            if (FAILED(input_->EnableVideoInput(newMode, bmdFormat8BitYUV, bmdVideoInputEnableFormatDetection))) {
                CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Unable to enable video input.")
                                                          << boost::errinfo_api_function("EnableVideoInput"));
            }

            if (FAILED(input_->FlushStreams())) {
                CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Unable to flush input stream.")
                                                          << boost::errinfo_api_function("FlushStreams"));
            }

            if (FAILED(input_->StartStreams())) {
                CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Unable to start input stream.")
                                                          << boost::errinfo_api_function("StartStreams"));
            }
            return S_OK;
        } catch (...) {
            exception_ = std::current_exception();
            return E_FAIL;
        }
    }

    HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame*  video,
                                                     IDeckLinkAudioInputPacket* audio) override
    {
        caspar::timer frame_timer;

        CASPAR_SCOPE_EXIT
        {
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                state_["file/name"]              = model_name_;
                state_["file/path"]              = device_index_;
                state_["file/audio/sample-rate"] = format_desc_.audio_sample_rate;
                state_["file/audio/channels"]    = format_desc_.audio_channels;
                state_["file/fps"]               = format_desc_.fps;
                state_["profiler/time"]          = {frame_timer.elapsed(), format_desc_.fps};
                state_["buffer"]                 = {frame_buffer_.size(), frame_buffer_.capacity()};
                state_["has_signal"]             = has_signal_;

                if (video) {
                    state_["file/video/width"]  = video->GetWidth();
                    state_["file/video/height"] = video->GetHeight();
                }
            }

            graph_->set_value("frame-time", frame_timer.elapsed() * format_desc_.fps * 0.5);
            graph_->set_value("output-buffer",
                              static_cast<float>(frame_buffer_.size()) / static_cast<float>(frame_buffer_.capacity()));
        };

        try {
            graph_->set_value("tick-time", tick_timer_.elapsed() * format_desc_.fps * 0.5);
            tick_timer_.restart();

            BMDTimeValue in_video_pts = 0LL;
            BMDTimeValue in_audio_pts = 0LL;

            // If the video is delayed too much, audio only will be delivered
            // we don't want audio only since the buffer is small and we keep avcodec
            // very busy processing all the (unnecessary) packets. Also, because there is
            // no audio, the sync values will be incorrect.
            if (!audio) {
                return S_OK;
            }

            if (video) {
                const auto flags = video->GetFlags();
                has_signal_      = !(flags & bmdFrameHasNoInputSource);
                if (freeze_on_lost_ && !has_signal_) {
                    return S_OK;
                }

                auto src    = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* ptr) { av_frame_free(&ptr); });
                src->format = AV_PIX_FMT_UYVY422;
                src->width  = video->GetWidth();
                src->height = video->GetHeight();
                src->interlaced_frame = mode_->GetFieldDominance() != bmdProgressiveFrame;
                src->top_field_first  = mode_->GetFieldDominance() == bmdUpperFieldFirst ? 1 : 0;
                src->key_frame        = 1;

                void* video_bytes = nullptr;
                if (SUCCEEDED(video->GetBytes(&video_bytes)) && video_bytes) {
                    video->AddRef();
                    src = std::shared_ptr<AVFrame>(src.get(), [src, video](AVFrame* ptr) { video->Release(); });

                    src->data[0]     = reinterpret_cast<uint8_t*>(video_bytes);
                    src->linesize[0] = video->GetRowBytes();

                    BMDTimeValue duration;
                    if (SUCCEEDED(video->GetStreamTime(&in_video_pts, &duration, AV_TIME_BASE))) {
                        src->pts = in_video_pts;
                    }

                    if (video_filter_.video_source) {
                        FF(av_buffersrc_write_frame(video_filter_.video_source, src.get()));
                    }
                    if (audio_filter_.video_source) {
                        FF(av_buffersrc_write_frame(audio_filter_.video_source, src.get()));
                    }
                }
            }

            if (audio) {
                auto src      = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* ptr) { av_frame_free(&ptr); });
                src->format   = AV_SAMPLE_FMT_S32;
                src->channels = format_desc_.audio_channels;
                src->sample_rate = format_desc_.audio_sample_rate;

                void* audio_bytes = nullptr;
                if (SUCCEEDED(audio->GetBytes(&audio_bytes)) && audio_bytes) {
                    audio->AddRef();
                    src = std::shared_ptr<AVFrame>(src.get(), [src, audio](AVFrame* ptr) { audio->Release(); });
                    src->nb_samples  = audio->GetSampleFrameCount();
                    src->data[0]     = reinterpret_cast<uint8_t*>(audio_bytes);
                    src->linesize[0] = src->nb_samples * src->channels *
                                       av_get_bytes_per_sample(static_cast<AVSampleFormat>(src->format));

                    if (SUCCEEDED(audio->GetPacketTime(&in_audio_pts, format_desc_.audio_sample_rate))) {
                        src->pts = in_audio_pts;
                    }

                    if (video_filter_.audio_source) {
                        FF(av_buffersrc_write_frame(video_filter_.audio_source, src.get()));
                    }
                    if (audio_filter_.audio_source) {
                        FF(av_buffersrc_write_frame(audio_filter_.audio_source, src.get()));
                    }
                }
            }

            while (true) {
                {
                    auto av_video = alloc_frame();
                    auto av_audio = alloc_frame();

                    // TODO (fix) this may get stuck if the decklink sends a frame of video or audio

                    if (av_buffersink_get_frame_flags(video_filter_.sink, av_video.get(), AV_BUFFERSINK_FLAG_PEEK) <
                        0) {
                        return S_OK;
                    }

                    audio_filter_.sink->inputs[0]->min_samples = audio_cadence_[0];
                    if (av_buffersink_get_frame_flags(audio_filter_.sink, av_audio.get(), AV_BUFFERSINK_FLAG_PEEK) <
                        0) {
                        return S_OK;
                    }
                }
                auto av_video = alloc_frame();
                auto av_audio = alloc_frame();

                // TODO (fix) auto V/A sync even if decklink is wrong.

                av_buffersink_get_frame(video_filter_.sink, av_video.get());
                av_buffersink_get_samples(audio_filter_.sink, av_audio.get(), audio_cadence_[0]);

                auto video_tb = av_buffersink_get_time_base(video_filter_.sink);
                auto audio_tb = av_buffersink_get_time_base(audio_filter_.sink);

                // CASPAR_LOG(trace) << "decklink a/v pts:" << av_video->pts << " " << av_audio->pts;

                auto in_sync = static_cast<double>(in_video_pts) / AV_TIME_BASE -
                               static_cast<double>(in_audio_pts) / format_desc_.audio_sample_rate;
                auto out_sync = static_cast<double>(av_video->pts * video_tb.num) / video_tb.den -
                                static_cast<double>(av_audio->pts * video_tb.num) / audio_tb.den;

                if (std::abs(in_sync - in_sync_) > 0.01) {
                    CASPAR_LOG(warning) << print() << " in-sync changed: " << in_sync;
                }
                in_sync_ = in_sync;

                if (std::abs(out_sync - out_sync_) > 0.01) {
                    CASPAR_LOG(warning) << print() << " out-sync changed: " << out_sync;
                }
                out_sync_ = out_sync;

                graph_->set_value("in-sync", in_sync * 2.0 + 0.5);
                graph_->set_value("out-sync", out_sync * 2.0 + 0.5);

                auto frame = core::draw_frame(make_frame(this, *frame_factory_, av_video, av_audio));
                if (!frame_buffer_.try_push(frame)) {
                    core::draw_frame dummy;
                    frame_buffer_.try_pop(dummy);
                    frame_buffer_.try_push(frame);
                    graph_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
                }

                boost::range::rotate(audio_cadence_, std::end(audio_cadence_) - 1);
            }
        } catch (...) {
            exception_ = std::current_exception();
            return E_FAIL;
        }

        return S_OK;
    }

    core::draw_frame get_frame(bool use_last_frame)
    {
        if (exception_ != nullptr) {
            std::rethrow_exception(exception_);
        }

        core::draw_frame frame;

        if (!frame_buffer_.try_pop(frame)) {
            if (freeze_on_lost_ || use_last_frame)
                frame = last_frame_;
            graph_->set_tag(diagnostics::tag_severity::WARNING, "late-frame");
        } else {
            last_frame_ = frame;
        }

        graph_->set_value("output-buffer",
                          static_cast<float>(frame_buffer_.size()) / static_cast<float>(frame_buffer_.capacity()));

        return frame;
    }

    std::wstring print() const
    {
        return model_name_ + L" [" + std::to_wstring(device_index_) + L"|" + input_format.name + L"]";
    }

    boost::rational<int> get_out_framerate() const { return format_desc_.framerate; }

    core::monitor::state state() const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return state_;
    }
};

class decklink_producer_proxy : public core::frame_producer
{
    std::unique_ptr<decklink_producer> producer_;
    const uint32_t                     length_;
    executor                           executor_;

  public:
    explicit decklink_producer_proxy(const core::video_format_desc&              format_desc,
                                     const spl::shared_ptr<core::frame_factory>& frame_factory,
                                     int                                         device_index,
                                     const std::string&                          vfilter,
                                     const std::string&                          afilter,
                                     uint32_t                                    length,
                                     const std::wstring&                         format,
                                     bool                                        freeze_on_lost)
        : length_(length)
        , executor_(L"decklink_producer[" + std::to_wstring(device_index) + L"]")
    {
        auto ctx = core::diagnostics::call_context::for_thread();
        executor_.invoke([=] {
            core::diagnostics::call_context::for_thread() = ctx;
            com_initialize();
            producer_.reset(new decklink_producer(
                format_desc, device_index, frame_factory, vfilter, afilter, format, freeze_on_lost));
        });
    }

    ~decklink_producer_proxy()
    {
        executor_.invoke([=] {
            producer_.reset();
            com_uninitialize();
        });
    }

    core::monitor::state state() const override { return producer_->state(); }

    // frame_producer

    core::draw_frame receive_impl(int nb_samples) override { return producer_->get_frame(false); }

    core::draw_frame first_frame() override { return receive_impl(0); }

    core::draw_frame last_frame() override { return core::draw_frame::still(producer_->get_frame(true)); }

    uint32_t nb_frames() const override { return length_; }

    std::wstring print() const override { return producer_->print(); }

    std::wstring name() const override { return L"decklink"; }

    boost::rational<int> get_out_framerate() const { return producer_->get_out_framerate(); }
};

spl::shared_ptr<core::frame_producer> create_producer(const core::frame_producer_dependencies& dependencies,
                                                      const std::vector<std::wstring>&         params)
{
    if (params.empty() || !boost::iequals(params.at(0), "decklink"))
        return core::frame_producer::empty();

    auto device_index = get_param(L"DEVICE", params, -1);
    if (device_index == -1)
        device_index = std::stoi(params.at(1));

    auto freeze_on_lost = contains_param(L"FREEZE_ON_LOST", params);

    auto format_str = get_param(L"FORMAT", params);

    auto filter_str = get_param(L"FILTER", params);
    auto length     = get_param(L"LENGTH", params, std::numeric_limits<uint32_t>::max());

    boost::ireplace_all(filter_str, L"DEINTERLACE_BOB", L"YADIF=1:-1");
    boost::ireplace_all(filter_str, L"DEINTERLACE_LQ", L"SEPARATEFIELDS");
    boost::ireplace_all(filter_str, L"DEINTERLACE", L"YADIF=0:-1");

    auto vfilter = boost::to_lower_copy(get_param(L"VF", params, filter_str));
    auto afilter = boost::to_lower_copy(get_param(L"AF", params, get_param(L"FILTER", params, L"")));

    auto producer = spl::make_shared<decklink_producer_proxy>(dependencies.format_desc,
                                                              dependencies.frame_factory,
                                                              device_index,
                                                              u8(vfilter),
                                                              u8(afilter),
                                                              length,
                                                              format_str,
                                                              freeze_on_lost);
    return core::create_destroy_proxy(producer);
}
}} // namespace caspar::decklink
