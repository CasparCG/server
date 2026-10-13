/*
 * Copyright (c) 2026 Sveriges Television AB <info@casparcg.com>
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
 * Author: Mint de Wit mint@araminta.dev
 */

#include "../StdAfx.h"

#include "dmf_mxl_producer.h"

#include <mxl/flow.h>
#include <mxl/flowinfo.h>
#include <mxl/mxl.h>
#include <mxl/time.h>

#include <core/frame/draw_frame.h>
#include <core/frame/frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/geometry.h>
#include <core/frame/pixel_format.h>
#include <core/monitor/monitor.h>
#include <core/producer/frame_producer.h>

#include <common/assert.h>
#include <common/diagnostics/graph.h>
#include <common/env.h>
#include <common/executor.h>
#include <common/future.h>
#include <common/param.h>
#include <common/scope_exit.h>
#include <common/timer.h>
#include <common/utf.h>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>
#include <nlohmann/json.hpp>
#include <regex>

#include <ffmpeg/util/av_assert.h>
#include <ffmpeg/util/av_util.h>

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
#include <libavutil/timecode.h>
}

#include <tbb/parallel_for.h>
#include <tbb/parallel_invoke.h>

using namespace caspar::ffmpeg;

namespace caspar { namespace dmf_mxl {

struct Decoder
{
    Decoder(const Decoder&) = delete;

  public:
    std::shared_ptr<AVCodecContext> ctx;

    Decoder() = default;

    Decoder(int width, int height)
    {
        const auto codec = avcodec_find_decoder(AV_CODEC_ID_V210);
        if (!codec) {
            FF_RET(AVERROR_DECODER_NOT_FOUND, "avcodec_find_decoder");
        }

        ctx = std::shared_ptr<AVCodecContext>(avcodec_alloc_context3(codec),
                                              [](AVCodecContext* ptr) { avcodec_free_context(&ptr); });
        if (!ctx) {
            FF_RET(AVERROR(ENOMEM), "avcodec_alloc_context3");
        }

        auto params = std::shared_ptr<AVCodecParameters>(avcodec_parameters_alloc(),
                                                         [](AVCodecParameters* ptr) { avcodec_parameters_free(&ptr); });
        if (!params) {
            FF_RET(AVERROR(ENOMEM), "avcodec_parameters_alloc");
        }
        params->width      = width;
        params->height     = height;
        params->codec_type = AVMEDIA_TYPE_VIDEO;
        params->codec_id   = AV_CODEC_ID_V210;
        params->format     = AV_PIX_FMT_YUV422P10;

        FF(avcodec_parameters_to_context(ctx.get(), params.get()));

        // int thread_count = env::properties().get(L"configuration.ffmpeg.producer.threads", 0);
        FF(av_opt_set_image_size(ctx.get(), "video_size", width, height, 0));
        FF(avcodec_open2(ctx.get(), codec, nullptr));
    }

    std::shared_ptr<AVFrame> decode(uint8_t* video, int size)
    {
        auto frame = std::shared_ptr<AVFrame>(av_frame_alloc(), [video](AVFrame* ptr) { av_frame_free(&ptr); });
        if (!frame)
            FF_RET(AVERROR(ENOMEM), "av_frame_alloc");

        AVPacket packet;
        av_init_packet(&packet);
        packet.data = video;
        packet.size = size;
        FF(avcodec_send_packet(ctx.get(), &packet));
        FF(avcodec_receive_frame(ctx.get(), frame.get()));

#if LIBAVCODEC_VERSION_MAJOR < 61
        frame->interlaced_frame = false;
        frame->top_field_first  = 0;
#else
        // frame->flags |= mode->GetFieldDominance() != bmdProgressiveFrame ? AV_FRAME_FLAG_INTERLACED : 0;
        // frame->flags |= mode->GetFieldDominance() == bmdUpperFieldFirst ? AV_FRAME_FLAG_TOP_FIELD_FIRST : 0;
#endif

        return frame;
    }
};

std::string format_hex(uint8_t* bytes, size_t length)
{
    std::ostringstream convert;
    for (size_t i = 0; i < length; ++i) {
        convert << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]) << " ";
    }
    return convert.str();
}

struct dmf_mxl_producer : public core::frame_producer
{
    static std::atomic<int> instances_;
    const int               instance_no_;
    const std::wstring      name_;
    const std::wstring      authority_;
    const std::wstring      domain_;
    const std::wstring      flow1_;
    const std::wstring      flow2_;

    spl::shared_ptr<core::frame_factory> frame_factory_;
    core::video_format_desc              format_desc_;
    spl::shared_ptr<diagnostics::graph>  graph_;
    timer                                tick_timer_;
    timer                                frame_timer_;

    mxlInstance       instance_;
    uint64_t          vFlowIndex_;
    mxlFlowReader     vFlow_;
    mxlFlowConfigInfo vFlowConf_;

    std::string             vFlowLabel;
    int                     width;
    int                     height;
    bool                    alpha;
    core::color_space       colorspace;
    core::pixel_format_desc pix_fmt_desc;

    mxlFlowReader     aFlow_;
    mxlFlowConfigInfo aFlowConf_;
    uint64_t          aFlowIndex_;

    std::string aFlowLabel;
    uint        aFlowChannelCount;

    Decoder video_decoder_;

    std::queue<core::draw_frame> frames_;
    mutable std::mutex           frames_mutex_;
    core::draw_frame             last_frame_;
    executor                     executor_;

    int cadence_counter_;
    int cadence_length_;

    uint64_t last_frame_ts_;
    uint64_t frame_dur_ns;
    uint64_t ts_;
    uint64_t latency_;

    uint64_t flow_ts_;

  public:
    explicit dmf_mxl_producer(spl::shared_ptr<core::frame_factory> frame_factory,
                              core::video_format_desc              format_desc,
                              std::wstring                         name,
                              std::wstring                         authority,
                              std::wstring                         domain,
                              std::wstring                         flow1,
                              std::wstring                         flow2)
        : format_desc_(format_desc)
        , frame_factory_(frame_factory)
        , name_(name)
        , authority_(authority)
        , domain_(domain)
        , flow1_(flow1)
        , flow2_(flow2)
        , instance_no_(instances_++)
        , executor_(print())
        , cadence_counter_(0)
    {
        instance_ = mxlCreateInstance(u8(domain_).c_str(), NULL);
        if (instance_ == nullptr) {
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("failed to create mxl instance"));
        }

        vFlow_   = nullptr;
        aFlow_   = nullptr;
        flow_ts_ = 0;
        latency_ = 0;

        frame_dur_ns = (static_cast<uint64_t>(format_desc.duration) * 1000000000) /
                       (static_cast<uint64_t>(format_desc.time_scale));

        CASPAR_LOG(info) << "[mxl] dur_ns=" << frame_dur_ns << " dur=" << format_desc.duration
                         << " time_scale=" << format_desc.time_scale;

        graph_->set_text(print());
        graph_->set_color("frame-time", diagnostics::color(0.5f, 1.0f, 0.2f));
        graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
        graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
        diagnostics::register_graph(graph_);
        executor_.set_capacity(2);
        cadence_length_ = static_cast<int>(format_desc_.audio_cadence.size());
        initialize();

        video_decoder_ = Decoder(width, height);
    }

    ~dmf_mxl_producer()
    {
        executor_.stop();

        if (vFlow_ != nullptr) {
            mxlReleaseFlowReader(instance_, vFlow_);
        }

        if (aFlow_ != nullptr) {
            mxlReleaseFlowReader(instance_, aFlow_);
        }

        mxlDestroyInstance(instance_);
    }

    std::wstring print() const override
    {
        return L"mxl[" + boost::lexical_cast<std::wstring>(instance_no_) + L"|" + name_ + L"]";
    }

    std::wstring name() const override { return L"mxl"; }

  private:
    void update_latency(uint64_t latency)
    {
        if (latency > latency_) {
            // adjust latency in frames
            latency_ = (1 + latency / frame_dur_ns) * frame_dur_ns;
        }
    }

    core::draw_frame receive_impl(const core::video_field field, int nb_samples) override
    {
        try {
            frame_timer_.restart();

            graph_->set_value("tick-time", tick_timer_.elapsed() * format_desc_.fps * 0.5);
            tick_timer_.restart();

            auto const frame_dur_ =
                aFlow_ != nullptr ? mxlIndexToTimestamp(&aFlowConf_.common.grainRate, nb_samples) : frame_dur_ns;
            if (mxlGetTime() - latency_ > flow_ts_ + 3 * frame_dur_) {
                CASPAR_LOG(info) << "[mxl] [time] skip " << mxlGetTime() - latency_ - (flow_ts_ + 3 * frame_dur_);
                // drop frame.
                graph_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
                if (vFlow_ != nullptr) {
                    // sync to start of a frame
                    mxlFlowRuntimeInfo info;
                    if (auto const ret = mxlFlowReaderGetRuntimeInfo(vFlow_, &info); ret != MXL_STATUS_OK) {
                        CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("failed to get vflow runtime info"));
                    }
                    auto const latency_in_grains = mxlTimestampToIndex(&vFlowConf_.common.grainRate, latency_);
                    flow_ts_ = mxlIndexToTimestamp(&vFlowConf_.common.grainRate, info.headIndex - latency_in_grains);
                } else if (aFlow_ != nullptr) {
                    mxlFlowRuntimeInfo info;
                    if (auto const ret = mxlFlowReaderGetRuntimeInfo(aFlow_, &info); ret != MXL_STATUS_OK) {
                        CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("failed to get vflow runtime info"));
                    }
                    flow_ts_ = mxlIndexToTimestamp(&aFlowConf_.common.grainRate, info.headIndex - nb_samples);
                }
            }

            // CASPAR_LOG(info) << "[mxl] [time] flow timestamp " << flow_ts_;

            flow_ts_ += frame_dur_;

            // start with empty frame
            auto frame2 = frame_factory_->create_frame(this, pix_fmt_desc, common::bit_depth::bit10);

            tbb::parallel_invoke(
                [&]() {
                    if (vFlow_ != nullptr) {
                        // add video to frame
                        mxlFlowRuntimeInfo info;
                        if (auto const ret = mxlFlowReaderGetRuntimeInfo(vFlow_, &info); ret != MXL_STATUS_OK) {
                            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("failed to get vflow runtime info"));
                        }

                        mxlGrainInfo grainInfo;
                        uint8_t*     payload;

                        auto latency = mxlGetTime() - mxlIndexToTimestamp(&vFlowConf_.common.grainRate, info.headIndex);
                        update_latency(latency);

                        vFlowIndex_ = mxlTimestampToIndex(&vFlowConf_.common.grainRate, flow_ts_);

                        auto const ret = mxlFlowReaderGetGrain(vFlow_, vFlowIndex_, 10000000, &grainInfo, &payload);
                        if (ret == MXL_STATUS_OK) {
                            if (grainInfo.validSlices >= grainInfo.totalSlices) {
                                uint32_t const key_line_size = alpha ? ((width + 2) / 3) * 4 : 0;
                                uint32_t const v_size        = grainInfo.grainSize - key_line_size * height;
                                auto           src           = video_decoder_.decode(payload, v_size);

                                if (src) {
                                    // for yuv planes:
                                    for (int n = 0; n < 3; ++n) {
                                        tbb::parallel_for(0, pix_fmt_desc.planes[n].height, [&](int y) {
                                            std::memcpy(frame2.image_data(n).begin() +
                                                            y * pix_fmt_desc.planes[n].linesize,
                                                        src->data[n] + y * src->linesize[n],
                                                        pix_fmt_desc.planes[n].linesize);
                                        });
                                    }

                                    if (alpha) {
                                        // unpack the alpha plane
                                        tbb::parallel_for(0, height, [&](int y) {
                                            auto src_line_start = v_size + key_line_size * y;
                                            auto src_line       = reinterpret_cast<uint32_t*>(payload + src_line_start);
                                            auto dest_line_start = y * pix_fmt_desc.planes[3].linesize;
                                            auto dest_line = reinterpret_cast<uint16_t*>(frame2.image_data(3).begin() +
                                                                                         dest_line_start);

                                            auto shift = 0;
                                            for (int x = 0; x < width; x++) {
                                                auto pixel   = (src_line[0] >> shift) & 0x3ff;
                                                dest_line[x] = static_cast<uint16_t>(pixel);

                                                shift += 10;
                                                if (shift > 20) {
                                                    shift = 0;
                                                    src_line++;
                                                }
                                            }
                                        });
                                    }
                                } else {
                                    CASPAR_LOG(info) << L"[mxl] could not make frame, no src";
                                }
                            }
                        } else if (ret == MXL_ERR_OUT_OF_RANGE_TOO_LATE || ret == MXL_ERR_OUT_OF_RANGE_TOO_EARLY) {
                            CASPAR_LOG(warning) << "[mxl] video status=" << ret << ", headIndex=" << info.headIndex;
                        } else {
                            CASPAR_LOG(warning) << L"[mxl] read status " + std::to_wstring(ret) + L", vFlowIndex=" +
                                                       std::to_wstring(vFlowIndex_) + L", info.headIndex=" +
                                                       std::to_wstring(info.headIndex);
                            // return last_frame_;
                        }
                    }
                },
                [&]() {
                    if (aFlow_ != nullptr) {
                        // add audio to the frame
                        mxlFlowRuntimeInfo info;
                        if (auto const ret = mxlFlowReaderGetRuntimeInfo(aFlow_, &info); ret != MXL_STATUS_OK) {
                            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("failed to get aflow runtime info"));
                        }

                        auto latency = mxlGetTime() -
                                       mxlIndexToTimestamp(&aFlowConf_.common.grainRate, info.headIndex) + frame_dur_ns;
                        update_latency(latency);

                        // aFlowIndex_ marks the end of the frame...
                        aFlowIndex_ = mxlTimestampToIndex(&aFlowConf_.common.grainRate, flow_ts_) + nb_samples - 1;

                        mxlWrappedMultiBufferSlice payload;
                        auto const                 ret =
                            mxlFlowReaderGetSamples(aFlow_, aFlowIndex_, nb_samples, frame_dur_ns / 2, &payload);
                        if (ret == MXL_STATUS_OK) {
                            // aFlowIndex_ += nb_samples;
                            frame2.audio_data() = std::vector<int32_t>(nb_samples * format_desc_.audio_channels, 0);
                            auto dest           = frame2.audio_data().data();

                            size_t const channels         = payload.count;
                            size_t const strideBytes      = payload.stride;
                            size_t const fragment0Samples = payload.base.fragments[0].size / sizeof(float);
                            size_t const fragment1Samples = payload.base.fragments[1].size / sizeof(float);

                            for (size_t channel = 0;
                                 channel < std::min(static_cast<int>(channels), format_desc_.audio_channels);
                                 ++channel) {
                                size_t const         channelOffset = channel * strideBytes;
                                uint8_t const* const channelBase0 =
                                    (uint8_t const*)(payload.base.fragments[0].pointer) + channelOffset;
                                uint8_t const* const channelBase1 =
                                    (uint8_t const*)(payload.base.fragments[1].pointer) + channelOffset;

                                float const* const firstSlice  = (float const*)(channelBase0);
                                float const* const secondSlice = (float const*)(channelBase1);

                                // Process the first fragment (may already contain the entire window).
                                for (size_t i = 0; i < fragment0Samples; ++i) {
                                    dest[i * format_desc_.audio_channels + channel] =
                                        static_cast<int32_t>(firstSlice[i] * std::numeric_limits<int32_t>::max());
                                }

                                // Only needed when the window wrapped around the ring buffer.
                                for (size_t i = 0; i < fragment1Samples; ++i) {
                                    dest[(fragment0Samples + i) * format_desc_.audio_channels + channel] =
                                        static_cast<int32_t>(secondSlice[i] * std::numeric_limits<int32_t>::max());
                                }
                            }
                        } else if (ret == MXL_ERR_OUT_OF_RANGE_TOO_LATE || ret == MXL_ERR_OUT_OF_RANGE_TOO_EARLY) {
                            CASPAR_LOG(warning) << "[mxl] audio status=" << ret << ", headIndex=" << info.headIndex
                                                << ", latency=" << latency_;
                        } else {
                            CASPAR_LOG(info) << L"[mxl] failed to read audio " + std::to_wstring(ret);
                        }
                    }
                });

            auto frame  = core::draw_frame(std::move(frame2));
            last_frame_ = frame;

            graph_->set_value("frame-time", frame_timer_.elapsed() * format_desc_.fps * 0.5);

            return frame;
        } catch (...) {
            return last_frame_;
        }
    }

    bool is_ready() override
    {
        bool active;
        if (mxlIsFlowActive(instance_, u8(flow1_).c_str(), &active) == MXL_STATUS_OK) {
            return active;
        }

        return false;
    }

    // frame_producer

    void initialize()
    {
        initialize_flow(flow1_);
        if (!flow2_.empty()) {
            initialize_flow(flow2_);
        }
    }

  private:
    void initialize_flow(std::wstring flowId)
    {
        auto flowDef   = read_flow_desc(flowId);
        auto mediaType = flowDef.at("media_type").get<std::string>();

        if (boost::equals(mediaType, "video/v210") || boost::equals(mediaType, "video/v210a")) {
            if (vFlow_ != nullptr) {
                return;
            }

            vFlowLabel = flowDef.at("label").get<std::string>();
            width      = flowDef.at("frame_width").get<int>();
            height     = flowDef.at("frame_height").get<int>();
            alpha      = boost::equals(mediaType, "video/v210a");

            auto const col_space = flowDef.at("colorspace").get<std::string>();
            if (boost::iequals(col_space, "BT601")) {
                colorspace = core::color_space::bt601;
            } else if (boost::iequals(col_space, "BT2020")) {
                colorspace = core::color_space::bt2020;
            } else {
                colorspace = core::color_space::bt709;
            }

            pix_fmt_desc =
                core::pixel_format_desc(alpha ? core::pixel_format::ycbcra : core::pixel_format::ycbcr, colorspace);
            pix_fmt_desc.is_straight_alpha = alpha ? true : false;
            pix_fmt_desc.planes.push_back(core::pixel_format_desc::plane(width, height, 1, common::bit_depth::bit10));
            pix_fmt_desc.planes.push_back(
                core::pixel_format_desc::plane(width / 2, height, 1, common::bit_depth::bit10));
            pix_fmt_desc.planes.push_back(
                core::pixel_format_desc::plane(width / 2, height, 1, common::bit_depth::bit10));
            if (alpha) {
                pix_fmt_desc.planes.push_back(
                    core::pixel_format_desc::plane(width, height, 1, common::bit_depth::bit10));
            }

            // @todo - interlaced sources

            CASPAR_LOG(info) << L"[mxl] [video] opening flow " + flowId;

            if (mxlCreateFlowReader(instance_, u8(flowId).c_str(), NULL, &vFlow_) != MXL_STATUS_OK) {
                CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("failed to create mxl video flow reader"));
            }

            if (mxlFlowReaderGetConfigInfo(vFlow_, &vFlowConf_) != MXL_STATUS_OK) {
                CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("failed to create mxl video flow config info"));
            }

            CASPAR_LOG(info) << L"[mxl] [video] flow grainRate " +
                                    std::to_wstring(vFlowConf_.common.grainRate.numerator) + L"/" +
                                    std::to_wstring(vFlowConf_.common.grainRate.denominator);

            mxlFlowRuntimeInfo info;
            if (auto const ret = mxlFlowReaderGetRuntimeInfo(vFlow_, &info); ret != MXL_STATUS_OK) {
                CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("failed to get vflow runtime info"));
            }

            auto const ts_now  = mxlGetTime();
            auto const ts_flow = mxlIndexToTimestamp(&vFlowConf_.common.grainRate, info.headIndex);
            auto const latency = ts_now - ts_flow;
            CASPAR_LOG(info) << "[mxl] [video] latency " << latency;

            update_latency(latency);
        } else if (boost::equals(mediaType, "audio/float32")) {
            if (aFlow_ != nullptr) {
                return;
            }

            aFlowLabel        = flowDef.at("label").get<std::string>();
            aFlowChannelCount = flowDef.at("channel_count").get<uint>();

            CASPAR_LOG(info) << L"[mxl] [audio] opening flow " + flowId;

            if (mxlCreateFlowReader(instance_, u8(flowId).c_str(), NULL, &aFlow_) != MXL_STATUS_OK) {
                CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("failed to create mxl audio flow reader"));
            }

            if (mxlFlowReaderGetConfigInfo(aFlow_, &aFlowConf_) != MXL_STATUS_OK) {
                CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("failed to create mxl video flow config info"));
            }

            CASPAR_LOG(info) << L"[mxl] [audio] flow grainRate " +
                                    std::to_wstring(aFlowConf_.common.grainRate.numerator) + L"/" +
                                    std::to_wstring(aFlowConf_.common.grainRate.denominator);

            mxlFlowRuntimeInfo info;
            if (auto const ret = mxlFlowReaderGetRuntimeInfo(aFlow_, &info); ret != MXL_STATUS_OK) {
                CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("failed to get vflow runtime info"));
            }

            auto const ts_now  = mxlGetTime();
            auto const ts_flow = mxlIndexToTimestamp(&aFlowConf_.common.grainRate, info.headIndex);

            auto const latency = ts_now - ts_flow + frame_dur_ns;
            CASPAR_LOG(info) << "[mxl] [audio] latency " << latency;

            update_latency(latency);

            CASPAR_LOG(info) << "[mxl] [audio] flow ch count" << aFlowConf_.continuous.channelCount;
        }
    }

  private:
    nlohmann::json read_flow_desc(std::wstring flowId)
    {
        char fourKBuffer[4096];
        auto fourKBufferSize    = sizeof(fourKBuffer);
        auto requiredBufferSize = fourKBufferSize;

        if (mxlGetFlowDef(instance_, u8(flowId).c_str(), fourKBuffer, &requiredBufferSize) != MXL_STATUS_OK) {
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(L"failed to get mxl flow definition for " + flowId));
        }

        return nlohmann::json::parse(fourKBuffer);
    }

    core::draw_frame last_frame(const core::video_field field) override
    {
        if (!last_frame_) {
            last_frame_ = receive_impl(field, 0);
        }
        return core::draw_frame::still(last_frame_);
    }

    core::monitor::state state() const override
    {
        core::monitor::state state;
        state["mxl/name"]    = u8(name_);
        state["mxl/latency"] = latency_;

        if (vFlow_ != nullptr) {
            state["mxl/video/label"]     = vFlowLabel;
            state["mxl/video/width"]     = width;
            state["mxl/video/height"]    = height;
            state["mxl/video/headIndex"] = vFlowIndex_;
        }

        if (aFlow_ != nullptr) {
            state["mxl/audio/label"]        = aFlowLabel;
            state["mxl/audio/channelCount"] = aFlowChannelCount;
            state["mxl/audio/headIndex"]    = aFlowIndex_;
        }

        return state;
    }

}; // namespace dmf_mxl

std::atomic<int> dmf_mxl_producer::instances_(0);

spl::shared_ptr<core::frame_producer> create_mxl_producer(const core::frame_producer_dependencies& dependencies,
                                                          const std::vector<std::wstring>&         params)
{
    const bool mxl_prefix  = boost::iequals(params.at(0), "[MXL]");
    auto       name_or_url = mxl_prefix ? params.at(1) : params.at(0);
    const bool mxl_url     = boost::algorithm::istarts_with(name_or_url, L"mxl:");

    if (mxl_url) {
        std::wregex expression(L"^mxl://([^/]+)?(/[^?]+)\\?id=([^&]+)(?:&id=([^&]+))?");

        std::wsmatch match;
        if (std::regex_search(name_or_url, match, expression)) {
            std::wstring authority(match[1].first, match[1].second);
            std::wstring domain(match[2].first, match[2].second);
            std::wstring id1(match[3].first, match[3].second);
            std::wstring id2(match[4].first, match[4].second);

            return spl::make_shared<dmf_mxl_producer>(
                dependencies.frame_factory, dependencies.format_desc, name_or_url, authority, domain, id1, id2);
        }
    }

    return core::frame_producer::empty();
}
}} // namespace caspar::dmf_mxl
