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

#include "dmf_mxl_consumer.h"

#include "../StdAfx.h"
#include "./v210.h"

#include <mxl/flow.h>
#include <mxl/flowinfo.h>
#include <mxl/mxl.h>
#include <mxl/time.h>

#include <common/diagnostics/graph.h>
#include <common/executor.h>
#include <common/param.h>
#include <common/timer.h>
#include <core/consumer/channel_info.h>
#include <core/consumer/frame_consumer.h>
#include <core/frame/frame.h>

#include <boost/algorithm/string.hpp>
#include <boost/json.hpp>
#include <boost/property_tree/ptree.hpp>
#include <tbb/parallel_for.h>

namespace caspar { namespace dmf_mxl {

std::vector<int32_t> create_int_matrix(const std::vector<float>& matrix)
{
    static const float LumaRangeWidth   = 876.f * (1024.f / 1023.f);
    static const float ChromaRangeWidth = 896.f * (1024.f / 1023.f);

    std::vector<float> color_matrix_f(matrix);

    color_matrix_f[0] *= LumaRangeWidth;
    color_matrix_f[1] *= LumaRangeWidth;
    color_matrix_f[2] *= LumaRangeWidth;

    color_matrix_f[3] *= ChromaRangeWidth;
    color_matrix_f[4] *= ChromaRangeWidth;
    color_matrix_f[5] *= ChromaRangeWidth;
    color_matrix_f[6] *= ChromaRangeWidth;
    color_matrix_f[7] *= ChromaRangeWidth;
    color_matrix_f[8] *= ChromaRangeWidth;

    std::vector<int32_t> int_matrix(color_matrix_f.size());

    transform(color_matrix_f.cbegin(), color_matrix_f.cend(), int_matrix.begin(), [](const float& f) {
        return (int32_t)round(f * 1024.f);
    });

    return int_matrix;
};

struct mxl_consumer : public core::frame_consumer
{
    std::vector<float> bt709{0.212639005871510,
                             0.715168678767756,
                             0.072192315360734,
                             -0.114592177555732,
                             -0.385407822444268,
                             0.5,
                             0.5,
                             -0.454155517037873,
                             -0.045844482962127};
    std::vector<float> bt2020{0.262700212011267,
                              0.677998071518871,
                              0.059301716469862,
                              -0.139630430187157,
                              -0.360369569812843,
                              0.5,
                              0.5,
                              -0.459784529009814,
                              -0.040215470990186};

    std::vector<int32_t> color_matrix;

    spl::shared_ptr<diagnostics::graph> graph_;
    caspar::timer                       frame_timer_;
    core::video_format_desc             format_desc_;
    executor                            executor_;

    std::thread frame_thread_;

    int  channel_index_ = -1;
    bool hdr_           = false;

    mxlInstance   instance_;
    std::wstring  vId_;
    mxlFlowWriter vWriter_;
    std::wstring  aId_;
    mxlFlowWriter aWriter_;
    uint64_t      last_index_ = MXL_UNDEFINED_INDEX;

  public:
    explicit mxl_consumer(std::wstring video, std::wstring audio)
        : color_matrix(create_int_matrix(bt709))
        , vId_(video)
        , aId_(audio)
        , executor_(L"mxl_consumer")
    {
        instance_ = mxlCreateInstance(u8(L"/dev/shm/mxl").c_str(), NULL);
        if (instance_ == nullptr) {
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("failed to create mxl instance"));
        }

        diagnostics::register_graph(graph_);
        graph_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
        graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
        graph_->set_color("input", diagnostics::color(0.7f, 0.4f, 0.4f));
    }

    ~mxl_consumer()
    {
        mxlReleaseFlowWriter(instance_, vWriter_);
        mxlReleaseFlowWriter(instance_, aWriter_);
        mxlDestroyInstance(instance_);

        if (frame_thread_.joinable()) {
            frame_thread_.join();
        }
    }

    void initialize(const core::video_format_desc& format_desc,
                    const core::channel_info&      channel_info,
                    int                            port_index) override
    {
        if (frame_thread_.joinable()) {
            CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("Cannot reinitialize mxl consumer."));
        }

        format_desc_   = format_desc;
        channel_index_ = channel_info.index;
        hdr_           = channel_info.depth != common::bit_depth::bit8;

        if (channel_info.default_color_space == core::color_space::bt2020) {
            color_matrix = create_int_matrix(bt2020);
        }

        if (!vId_.empty()) {
            const boost::json::value vFlowDefValue = {
                {"id", u8(vId_)},
                {"label", "CasparCG Ch " + std::to_string(channel_index_) + " (V)"},
                {"description", "CasparCG Channel " + std::to_string(channel_index_) + " Video Flow"},
                {"tags", {{"urn:x-nmos:tag:grouphint/v1.0", {"casparcg:Video"}}}},

                {"format", "urn:x-nmos:format:video"},
                {"media_type", "video/v210"},
                {"grain_rate",
                 {{"numerator", format_desc_.framerate.numerator()},
                  {"denominator", format_desc_.framerate.denominator()}}},
                {"frame_width", format_desc_.width},
                {"frame_height", format_desc_.height},
                // @todo - add interlaced support
                {"interlace_mode", "progressive"},
                {"colorspace", channel_info.default_color_space == core::color_space::bt2020 ? "BT2020" : "BT709"},

                {"components",
                 {
                     {{"name", "Y"}, {"width", format_desc_.width}, {"height", format_desc_.height}, {"bit_depth", 10}},
                     {{"name", "Cb"},
                      {"width", format_desc_.width / 2},
                      {"height", format_desc_.height},
                      {"bit_depth", 10}},
                     {{"name", "Cr"},
                      {"width", format_desc_.width / 2},
                      {"height", format_desc_.height},
                      {"bit_depth", 10}},
                 }},

                {"parents", boost::json::array{}}};

            const auto vFlowDef = boost::json::serialize(vFlowDefValue);
            CASPAR_LOG(debug) << "[mxl] [video] " << vFlowDef;

            bool wasCreated;

            if (auto const ret = mxlCreateFlowWriter(instance_, vFlowDef.c_str(), NULL, &vWriter_, NULL, &wasCreated);
                ret != MXL_STATUS_OK) {
                CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("failed to create mxl video writer code=" + ret));
            }
        }

        if (!aId_.empty()) {
            const boost::json::value aFlowDefValue = {
                {"id", u8(aId_)},
                {"description", "CasparCG Channel " + std::to_string(channel_index_) + " Audio Flow"},
                {"label", "CasparCG Ch " + std::to_string(channel_index_) + " (A)"},
                {"tags", {{"urn:x-nmos:tag:grouphint/v1.0", {"casparcg:Audio"}}}},

                {"format", "urn:x-nmos:format:audio"},
                {"media_type", "audio/float32"},
                {"sample_rate", {{"numerator", 48000}, {"denominator", 1}}},
                {"channel_count", format_desc_.audio_channels},
                {"bit_depth", 32},

                {"parents", boost::json::array{}}};

            const auto aFlowDef = boost::json::serialize(aFlowDefValue);
            CASPAR_LOG(debug) << "[mxl] [audio] " << aFlowDef;

            bool wasCreated;
            if (auto const ret = mxlCreateFlowWriter(instance_, aFlowDef.c_str(), NULL, &aWriter_, NULL, &wasCreated);
                ret != MXL_STATUS_OK) {
                CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("failed to create mxl audio writer code=" + ret));
            }
        }
    }

    // @todo - add avx2 like decklink has
    std::future<bool> send(core::video_field field, core::const_frame frame) override
    {
        return executor_.begin_invoke([=, this] {
            frame_timer_.restart();

            graph_->set_text(print());

            auto const vrate = mxlRational{format_desc_.framerate.numerator(), format_desc_.framerate.denominator()};
            auto const arate = mxlRational{48000, 1};
            auto const grainIndex     = mxlGetCurrentIndex(&vrate);
            uint64_t   skippedGrains  = 0;
            uint64_t   skippedSamples = 0;

            if (last_index_ == MXL_UNDEFINED_INDEX) {
                last_index_ = grainIndex;
            } else if (++last_index_ < grainIndex - 2) {
                graph_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");

                skippedGrains  = grainIndex - last_index_;
                skippedSamples = mxlTimestampToIndex(&arate, mxlIndexToTimestamp(&vrate, skippedGrains));

                CASPAR_LOG(warning) << "[mxl_consumer] unexpected index, dropping " << skippedGrains << " frames";
                last_index_ = grainIndex;
            } else if (last_index_ > grainIndex) {
                CASPAR_LOG(warning) << "[mxl_consumer] unexpected index, time went backwards. expected=" << grainIndex
                                    << " writing=" << last_index_;
                last_index_ = grainIndex;
            }

            if (!vId_.empty()) {
                // mark skipped grains as invalid
                while (skippedGrains > 0) {
                    auto          gInfo            = mxlGrainInfo{};
                    std::uint8_t* mxlBuffer        = nullptr;
                    auto const    actualGrainIndex = grainIndex - skippedGrains;

                    /// Open the grain for writing.
                    if (::mxlFlowWriterOpenGrain(vWriter_, actualGrainIndex, &gInfo, &mxlBuffer) != MXL_STATUS_OK) {
                        CASPAR_LOG(info) << "[mxl_consumer] Failed to open grain at index " << actualGrainIndex;
                        break;
                    }

                    gInfo.flags = MXL_GRAIN_FLAG_INVALID;
                    if (::mxlFlowWriterCommitGrain(vWriter_, &gInfo) != MXL_STATUS_OK) {
                        CASPAR_LOG(info) << "[mxl_consumer] Failed to commit invalid grain at index "
                                         << actualGrainIndex;
                        break;
                    }

                    skippedGrains--;
                }

                // write the next grain
                auto          gInfo     = mxlGrainInfo{};
                std::uint8_t* mxlBuffer = nullptr;

                if (mxlFlowWriterOpenGrain(vWriter_, last_index_, &gInfo, &mxlBuffer) != MXL_STATUS_OK) {
                    CASPAR_LOG(warning) << "[mxl_consumer] Failed to open grain at index " << last_index_;
                    return false;
                }

                try {
                    // write v210 into the mxl grain...
                    size_t    dest_line_bytes = ((format_desc_.width + 47) / 48) * 128;
                    const int NUM_THREADS     = 6;
                    auto      rows_per_thread = format_desc_.height / NUM_THREADS;

                    tbb::parallel_for(0, NUM_THREADS, [&](int thread_index) {
                        auto start_y = thread_index * rows_per_thread;
                        auto end_y   = (thread_index + 1) * rows_per_thread;

                        for (uint64_t y = start_y; y < end_y; y++) {
                            auto     dest_row  = mxlBuffer + y * dest_line_bytes;
                            __m128i* v210_dest = reinterpret_cast<__m128i*>(dest_row);

                            if (hdr_) {
                                auto src = (reinterpret_cast<const ARGBPixel<uint16_t>*>(frame.image_data(0).data()) +
                                            (y * format_desc_.width));

                                do_row_to_v210(src, format_desc_.width, color_matrix, v210_dest);
                            } else {
                                auto src = (reinterpret_cast<const ARGBPixel<uint8_t>*>(frame.image_data(0).data()) +
                                            (y * format_desc_.width));

                                do_row_to_v210(src, format_desc_.width, color_matrix, v210_dest);
                            }
                        }
                    });

                    gInfo.validSlices = format_desc_.height;

                    if (mxlFlowWriterCommitGrain(vWriter_, &gInfo) != MXL_STATUS_OK) {
                        CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("failed to commit mxl grain"));
                    }

                    graph_->set_value("frame-time", frame_timer_.elapsed() * format_desc_.fps * 0.5);

                } catch (...) {
                    mxlFlowWriterCancelGrain(vWriter_);

                    graph_->set_value("frame-time", frame_timer_.elapsed() * format_desc_.fps * 0.5);

                    CASPAR_LOG(warning) << "[mxl_consumer] Failed to write grain at index " << last_index_;
                    return false;
                }
            }

            if (!aId_.empty()) {
                try {
                    auto const samples = frame.audio_data().size() / format_desc_.audio_channels;
                    auto const ts      = mxlIndexToTimestamp(&vrate, last_index_);
                    auto const aIndex  = mxlTimestampToIndex(&arate, ts);

                    // generate silence for dropped frames
                    while (skippedSamples > 0) {
                        auto const batchSize         = std::min<uint64_t>(skippedSamples, samples);
                        auto const actualSampleIndex = aIndex - skippedSamples;

                        mxlMutableWrappedMultiBufferSlice payloadBuffersSlices;
                        if (::mxlFlowWriterOpenSamples(aWriter_, actualSampleIndex, samples, &payloadBuffersSlices) !=
                            MXL_STATUS_OK) {
                            CASPAR_LOG(info) << "[mxl_consumer] Failed to open samples at index " << actualSampleIndex;
                            break;
                        }

                        for (auto chan = std::size_t{0}; chan < payloadBuffersSlices.count; ++chan) {
                            for (auto& fragment : payloadBuffersSlices.base.fragments) {
                                if (fragment.size != 0) {
                                    auto const dst = static_cast<std::uint8_t*>(fragment.pointer) +
                                                     (chan * payloadBuffersSlices.stride);
                                    std::memset(dst, 0, fragment.size); // fill with silence
                                }
                            }
                        }

                        if (::mxlFlowWriterCommitSamples(aWriter_) != MXL_STATUS_OK) {
                            CASPAR_LOG(info)
                                << "[mxl_consumer] Failed to commit silence samples at index " << actualSampleIndex;
                            break;
                        }

                        skippedSamples -= batchSize;
                    }

                    // write some audio samples here...
                    mxlMutableWrappedMultiBufferSlice payloadBuffersSlices;

                    if (mxlFlowWriterOpenSamples(aWriter_, aIndex, samples, &payloadBuffersSlices) != MXL_STATUS_OK) {
                        CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("failed to open mxl samples"));
                    }

                    auto const aSrc = frame.audio_data().data();

                    for (size_t channel = 0;
                         channel < std::min(static_cast<int>(payloadBuffersSlices.count), format_desc_.audio_channels);
                         channel++) {
                        // for each channel
                        size_t const channelOffset = channel * payloadBuffersSlices.stride;
                        size_t       src_i         = 0;

                        for (auto& fragment : payloadBuffersSlices.base.fragments) {
                            // for each fragment
                            if (fragment.size != 0) {
                                uint8_t const* const channelBase = (uint8_t const*)(fragment.pointer) + channelOffset;
                                float* const         slice       = (float*)(channelBase);

                                // write x samples
                                for (size_t i = 0; i < fragment.size / sizeof(float); ++i) {
                                    slice[i] = static_cast<float>(aSrc[src_i * format_desc_.audio_channels + channel]) /
                                               std::numeric_limits<int32_t>::max();
                                    src_i++;
                                }
                            }
                        }
                    }

                    if (mxlFlowWriterCommitSamples(aWriter_) != MXL_STATUS_OK) {
                        CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("failed to commit mxl samples"));
                    }

                    graph_->set_value("frame-time", frame_timer_.elapsed() * format_desc_.fps * 0.5);
                } catch (...) {
                    mxlFlowWriterCancelSamples(aWriter_);

                    graph_->set_value("frame-time", frame_timer_.elapsed() * format_desc_.fps * 0.5);

                    CASPAR_LOG(warning) << "[mxl_consumer] Failed to write samples at index " << last_index_;
                    return false;
                }
            }

            return true;
        });
    }

    [[nodiscard]] std::wstring name() const override { return L"mxl"; }

    int index() const override { return 1000; }

    std::wstring print() const override { return L"[mxl_consumer]"; }

    [[nodiscard]] core::monitor::state state() const override
    {
        core::monitor::state state;
        state["mxl/last_index"] = last_index_;

        if (!vId_.empty()) {
            state["mxl/video/flow"] = vId_;
        }

        if (!aId_.empty()) {
            state["mxl/audio/flow"] = aId_;
        }

        return state;
    }
};

spl::shared_ptr<core::frame_consumer>
create_mxl_consumer(const std::vector<std::wstring>&                         params,
                    const core::video_format_repository&                     format_repository,
                    const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                    const core::channel_info&                                channel_info)
{
    if (params.size() < 1 || !boost::iequals(params.at(0), "MXL")) {
        return core::frame_consumer::empty();
    }

    std::wstring video = get_param(L"VIDEO", params, L"");
    std::wstring audio = get_param(L"AUDIO", params, L"");

    if (video.empty() && audio.empty()) {
        CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("MXL Consumer must have at least one flow"));
    }

    return spl::make_shared<mxl_consumer>(video, audio);
}

spl::shared_ptr<core::frame_consumer>
create_preconfigured_mxl_consumer(const boost::property_tree::wptree&                      ptree,
                                  const core::video_format_repository&                     format_repository,
                                  const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                                  const core::channel_info&                                channel_info)
{
    auto video = ptree.get(L"video", L"");
    auto audio = ptree.get(L"audio", L"");

    return spl::make_shared<mxl_consumer>(video, audio);
}

}} // namespace caspar::dmf_mxl