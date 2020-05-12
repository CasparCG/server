/*
 * Copyright 2018
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
 * Author: Krzysztof Zegzula, zegzulakrzysztof@gmail.com
 * based on work of Robert Nagy, ronag89@gmail.com and Jerzy Jaśkiewicz, jurek@tvp.pl
 */

#include "../StdAfx.h"

#include "newtek_ndi_producer.h"

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
#include <boost/regex.hpp>
#include <boost/thread.hpp>

#include <tbb/parallel_for.h>

#include <ffmpeg/util/av_util.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
extern "C" {
#include <libavformat/avformat.h>
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "../util/ndi.h"

namespace caspar { namespace newtek {

struct newtek_ndi_producer : public core::frame_producer
{
    static std::atomic<int> instances_;
    const int               instance_no_;
    const std::wstring      name_;
    const bool              low_bandwidth_;

    spl::shared_ptr<core::frame_factory> frame_factory_;
    core::video_format_desc              format_desc_;
    NDIlib_v3*                           ndi_lib_;
    NDIlib_framesync_instance_t          ndi_framesync_;
    NDIlib_recv_instance_t               ndi_recv_instance_;
    spl::shared_ptr<diagnostics::graph>  graph_;
    timer                                tick_timer_;
    timer                                frame_timer_;

    std::queue<core::draw_frame> frames_;
    mutable std::mutex           frames_mutex_;
    core::draw_frame             last_frame_;
    executor                     executor_;

    int cadence_counter_;
    int cadence_length_;

  public:
    explicit newtek_ndi_producer(spl::shared_ptr<core::frame_factory> frame_factory,
                                 core::video_format_desc              format_desc,
                                 std::wstring                         name,
                                 bool                                 low_bandwidth)
        : format_desc_(format_desc)
        , frame_factory_(frame_factory)
        , name_(name)
        , low_bandwidth_(low_bandwidth)
        , instance_no_(instances_++)
        , executor_(print())
        , cadence_counter_(0)
    {
        ndi_lib_ = ndi::load_library();
        graph_->set_text(print());
        graph_->set_color("frame-time", diagnostics::color(0.5f, 1.0f, 0.2f));
        graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
        graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
        diagnostics::register_graph(graph_);
        executor_.set_capacity(2);
        cadence_length_ = static_cast<int>(format_desc_.audio_cadence.size());
        initialize();
    }

    ~newtek_ndi_producer()
    {
        executor_.stop();
        ndi_lib_->NDIlib_framesync_destroy(ndi_framesync_);
        ndi_lib_->NDIlib_recv_destroy(ndi_recv_instance_);
    }

    std::wstring print() const override
    {
        return L"ndi[" + boost::lexical_cast<std::wstring>(instance_no_) + L"|" + name_ + L"]";
    }

    std::wstring name() const override { return L"ndi"; }

    core::draw_frame receive_impl(int hints) override
    {
        graph_->set_value("tick-time", tick_timer_.elapsed() * format_desc_.fps * 0.5);
        tick_timer_.restart();
        if (executor_.size() < 2) {
            executor_.begin_invoke([this]() { prepare_next_frame(); });
        }
        {
            std::lock_guard<std::mutex> lock(frames_mutex_);
            if (frames_.size() > 0) {
                last_frame_ = frames_.front();
                frames_.pop();
            }
            return last_frame_;
        }
    }
    bool prepare_next_frame()
    {
        try {
            frame_timer_.restart();
            NDIlib_video_frame_v2_t video_frame;
            NDIlib_audio_frame_v2_t audio_frame;
            ndi_lib_->NDIlib_framesync_capture_video(
                ndi_framesync_, &video_frame, NDIlib_frame_format_type_progressive);
            ndi_lib_->NDIlib_framesync_capture_audio(ndi_framesync_,
                                                     &audio_frame,
                                                     format_desc_.audio_sample_rate,
                                                     format_desc_.audio_channels,
                                                     format_desc_.audio_cadence[++cadence_counter_ %= cadence_length_]);

            CASPAR_SCOPE_EXIT
            {
                if (video_frame.p_data != nullptr)
                    ndi_lib_->NDIlib_framesync_free_video(ndi_framesync_, &video_frame);
                if (audio_frame.p_data != nullptr)
                    ndi_lib_->NDIlib_framesync_free_audio(ndi_framesync_, &audio_frame);
            };

            if (video_frame.p_data != nullptr) {
                std::shared_ptr<AVFrame> av_frame(av_frame_alloc(), [](AVFrame* frame) { av_frame_free(&frame); });
                std::shared_ptr<AVFrame> a_frame(av_frame_alloc(), [](AVFrame* frame) { av_frame_free(&frame); });
                av_frame->data[0]     = video_frame.p_data;
                av_frame->linesize[0] = video_frame.line_stride_in_bytes;
                switch (video_frame.FourCC) {
                    case NDIlib_FourCC_type_BGRA:
                        av_frame->format = AV_PIX_FMT_BGRA;
                        break;
                    case NDIlib_FourCC_type_BGRX:
                        av_frame->format = AV_PIX_FMT_BGRA;
                        break;
                    case NDIlib_FourCC_type_RGBA:
                        av_frame->format = AV_PIX_FMT_RGBA;
                        break;
                    case NDIlib_FourCC_type_RGBX:
                        av_frame->format = AV_PIX_FMT_RGBA;
                        break;
                    case NDIlib_FourCC_type_UYVY:
                        av_frame->format = AV_PIX_FMT_UYVY422;
                        break;
                    default: // should never happen because library handles the conversion for us
                        av_frame->format = AV_PIX_FMT_BGRA;
                        break;
                }
                av_frame->width  = video_frame.xres;
                av_frame->height = video_frame.yres;
                NDIlib_audio_frame_interleaved_32s_t audio_frame_32s;
                audio_frame_32s.p_data = new int32_t[audio_frame.no_samples * audio_frame.no_channels];
                if (audio_frame.p_data != nullptr) {
                    audio_frame_32s.reference_level = 0;
                    ndi_lib_->NDIlib_util_audio_to_interleaved_32s_v2(&audio_frame, &audio_frame_32s);
                    a_frame->channels    = audio_frame_32s.no_channels;
                    a_frame->sample_rate = audio_frame_32s.sample_rate;
                    a_frame->nb_samples  = audio_frame_32s.no_samples;
                    a_frame->data[0]     = reinterpret_cast<uint8_t*>(audio_frame_32s.p_data);
                }
                auto mframe =
                    ffmpeg::make_frame(this, *(frame_factory_.get()), std::move(av_frame), std::move(a_frame));
                delete[] audio_frame_32s.p_data;
                auto dframe = core::draw_frame(std::move(mframe));
                {
                    std::lock_guard<std::mutex> lock(frames_mutex_);
                    frames_.push(dframe);
                    while (frames_.size() > 2) { // should never happen because frame sync takes care of it
                        frames_.pop();
                        graph_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
                    }
                }
            }

            graph_->set_value("frame-time", frame_timer_.elapsed() * format_desc_.fps * 0.5);
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            return false;
        }
        return true;
    }

    // frame_producer

    void initialize()
    {
        auto sources = ndi::get_current_sources();

        NDIlib_recv_create_v3_t NDI_recv_create_desc;
        NDI_recv_create_desc.allow_video_fields = false;
        NDI_recv_create_desc.bandwidth = low_bandwidth_ ? NDIlib_recv_bandwidth_lowest : NDIlib_recv_bandwidth_highest;
        NDI_recv_create_desc.color_format = NDIlib_recv_color_format_UYVY_BGRA;
        std::string src_name              = u8(name_);

        auto found_source = sources.find(src_name);
        if (found_source != sources.end()) {
            NDI_recv_create_desc.source_to_connect_to = found_source->second;
        } else {
            CASPAR_LOG(info) << print() << " Source currently not available.";
            NDI_recv_create_desc.source_to_connect_to.p_ndi_name = src_name.c_str();
        }
        std::string receiver_name = "CasparCG " + u8(env::version()) + " NDI Producer " + std::to_string(instance_no_);
        NDI_recv_create_desc.p_ndi_recv_name = receiver_name.c_str();
        ndi_recv_instance_                   = ndi_lib_->NDIlib_recv_create_v3(&NDI_recv_create_desc);
        ndi_framesync_                       = ndi_lib_->NDIlib_framesync_create(ndi_recv_instance_);
        CASPAR_VERIFY(ndi_recv_instance_);
    }

    core::draw_frame last_frame() override
    {
        if (!last_frame_) {
            last_frame_ = receive_impl(0);
        }
        return core::draw_frame::still(last_frame_);
    }

}; // namespace newtek

std::atomic<int> newtek_ndi_producer::instances_(0);

spl::shared_ptr<core::frame_producer> create_ndi_producer(const core::frame_producer_dependencies& dependencies,
                                                          const std::vector<std::wstring>&         params)
{
    const bool ndi_prefix  = boost::iequals(params.at(0), "[NDI]");
    auto       name_or_url = ndi_prefix ? params.at(1) : params.at(0);
    const bool ndi_url     = boost::algorithm::istarts_with(name_or_url, L"ndi:");
    if (!ndi_prefix && !ndi_url)
        return core::frame_producer::empty();

    if (ndi_url) {
        boost::replace_all(name_or_url, L"+", L" ");
        boost::wregex expression(L"^ndi://([^/]+)/([^/]+)");

        boost::match_results<std::wstring::const_iterator> match;
        if (regex_search(name_or_url, match, expression)) {
            std::wstring device(match[1].first, match[1].second);
            std::wstring source(match[2].first, match[2].second);
            name_or_url = device + L" (" + source + L")";
        } else {
            return core::frame_producer::empty();
        }
    }
    const bool low_bandwidth = contains_param(L"LOW_BANDWIDTH", params);

    auto producer = spl::make_shared<newtek_ndi_producer>(
        dependencies.frame_factory, dependencies.format_desc, name_or_url, low_bandwidth);
    return core::create_destroy_proxy(std::move(producer));
}
}} // namespace caspar::newtek
