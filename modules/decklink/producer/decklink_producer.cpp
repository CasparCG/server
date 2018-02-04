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
#include <common/timer.h>

#include <core/diagnostics/call_context.h>
#include <core/frame/draw_frame.h>
#include <core/frame/frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/frame_transform.h>
#include <core/frame/pixel_format.h>
#include <core/mixer/audio/audio_mixer.h>
#include <core/monitor/monitor.h>
#include <core/producer/frame_producer.h>

#include <tbb/concurrent_queue.h>

#include <boost/algorithm/string.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/range/adaptor/transformed.hpp>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
extern "C"
{
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "../decklink_api.h"

#include <functional>

namespace caspar { namespace decklink {

template <typename T>
std::wstring to_string(const T& cadence)
{
    return boost::join(cadence | boost::adaptors::transformed([](size_t i) { return boost::lexical_cast<std::wstring>(i); }), L", ");
}

class decklink_producer
    : boost::noncopyable
    , public IDeckLinkInputCallback
{
    const int                           device_index_;
    core::monitor::subject              monitor_subject_;
    spl::shared_ptr<diagnostics::graph> graph_;
    caspar::timer                       tick_timer_;

    com_ptr<IDeckLink>                 decklink_   = get_device(device_index_);
    com_iface_ptr<IDeckLinkInput>      input_      = iface_cast<IDeckLinkInput>(decklink_);
    com_iface_ptr<IDeckLinkAttributes> attributes_ = iface_cast<IDeckLinkAttributes>(decklink_);

    const std::wstring model_name_ = get_model_name(decklink_);

    core::video_format_desc              format_desc_;
    std::vector<int>                     audio_cadence_ = format_desc_.audio_cadence;
    boost::circular_buffer<size_t>       sync_buffer_{audio_cadence_.size()};
    spl::shared_ptr<core::frame_factory> frame_factory_;

    tbb::concurrent_bounded_queue<core::draw_frame> frame_buffer_;
    core::draw_frame                                last_frame_;

    std::shared_ptr<SwsContext> sws_;

    std::exception_ptr exception_;

  public:
    decklink_producer(const core::video_format_desc&              format_desc,
                      int                                         device_index,
                      const spl::shared_ptr<core::frame_factory>& frame_factory,
                      const std::wstring&                         filter)
        : device_index_(device_index)
        , format_desc_(format_desc)
        , frame_factory_(frame_factory)
    {
        frame_buffer_.set_capacity(4);

        graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
        graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.3f));
        graph_->set_color("frame-time", diagnostics::color(1.0f, 0.0f, 0.0f));
        graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
        graph_->set_color("output-buffer", diagnostics::color(0.0f, 1.0f, 0.0f));
        graph_->set_text(print());
        diagnostics::register_graph(graph_);

        auto display_mode = get_display_mode(input_, format_desc.format, bmdFormat8BitYUV, bmdVideoInputFlagDefault);

        // NOTE: bmdFormat8BitARGB is currently not supported by any decklink card. (2011-05-08)
        if (FAILED(input_->EnableVideoInput(display_mode, bmdFormat8BitYUV, 0)))
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Could not enable video input.")
                                                      << boost::errinfo_api_function("EnableVideoInput"));

        if (FAILED(input_->EnableAudioInput(bmdAudioSampleRate48kHz, bmdAudioSampleType32bitInteger, static_cast<int>(format_desc_.audio_channels))))
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Could not enable audio input.")
                                                      << boost::errinfo_api_function("EnableAudioInput"));

        if (FAILED(input_->SetCallback(this)) != S_OK)
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to set input callback.") << boost::errinfo_api_function("SetCallback"));

        if (FAILED(input_->StartStreams()))
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to start input stream.") << boost::errinfo_api_function("StartStreams"));

        // Wait for first frame until returning or give up after 2 seconds.
        caspar::timer timeout_timer;

        while (frame_buffer_.size() < 1 && timeout_timer.elapsed() < 2.0)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        CASPAR_LOG(info) << print() << L" Initialized";
    }

    ~decklink_producer()
    {
        if (input_ != nullptr) {
            input_->StopStreams();
            input_->DisableVideoInput();
        }
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID*) { return E_NOINTERFACE; }
    virtual ULONG STDMETHODCALLTYPE AddRef() { return 1; }
    virtual ULONG STDMETHODCALLTYPE Release() { return 1; }

    virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents /*notificationEvents*/,
                                                              IDeckLinkDisplayMode* newDisplayMode,
                                                              BMDDetectedVideoInputFormatFlags /*detectedSignalFlags*/)
    {
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame* video, IDeckLinkAudioInputPacket* audio)
    {
        if (!video)
            return S_OK;

        try {
            graph_->set_value("tick-time", tick_timer_.elapsed() * format_desc_.fps * 0.5);
            tick_timer_.restart();

            caspar::timer frame_timer;

            // Video

            void* video_bytes = nullptr;
            if (FAILED(video->GetBytes(&video_bytes)) || !video_bytes)
                return S_OK;

            monitor_subject_ << core::monitor::message("/file/name") % model_name_ << core::monitor::message("/file/path") % device_index_
                             << core::monitor::message("/file/video/width") % video->GetWidth()
                             << core::monitor::message("/file/video/height") % video->GetHeight()
                             << core::monitor::message("/file/video/field") % u8(format_desc_.field_mode != core::field_mode::progressive
                                                                                     ? "progressive"
                                                                                     : (format_desc_.field_mode == core::field_mode::upper ? "upper" : "lower"))
                             << core::monitor::message("/file/audio/sample-rate") % 48000 << core::monitor::message("/file/audio/channels") % 2
                             << core::monitor::message("/file/audio/format") % u8(av_get_sample_fmt_name(AV_SAMPLE_FMT_S32))
                             << core::monitor::message("/file/fps") % format_desc_.fps;

            // Audio

            std::shared_ptr<std::vector<int32_t>> audio_buffer;
            void*                                 audio_bytes = nullptr;

            // It is assumed that audio is always equal or ahead of video.
            if (audio && SUCCEEDED(audio->GetBytes(&audio_bytes)) && audio_bytes) {
                auto sample_frame_count = audio->GetSampleFrameCount();
                auto audio_data         = reinterpret_cast<int32_t*>(audio_bytes);

                audio_buffer = std::make_shared<std::vector<int32_t>>(audio_data, audio_data + sample_frame_count * format_desc_.audio_channels);
            } else {
                audio_buffer = std::make_shared<std::vector<int32_t>>(audio_cadence_.front() * format_desc_.audio_channels, 0);
            }

            // Note: Uses 1 step rotated cadence for 1001 modes (1602, 1602, 1601, 1602, 1601)
            // This cadence fills the audio mixer most optimally.

            sync_buffer_.push_back(audio_buffer->size() / format_desc_.audio_channels);
            if (!boost::range::equal(sync_buffer_, audio_cadence_)) {
                CASPAR_LOG(trace) << print() << L" Syncing audio. Expected cadence: " << to_string(audio_cadence_) << L" Got cadence: "
                                  << to_string(sync_buffer_);
                return S_OK;
            }
            boost::range::rotate(audio_cadence_, std::begin(audio_cadence_) + 1);

            auto frame = [&] {
                auto src = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* ptr) { av_frame_free(&ptr); });

                src->data[0]          = reinterpret_cast<uint8_t*>(video_bytes);
                src->linesize[0]      = video->GetRowBytes();
                src->format           = AV_PIX_FMT_UYVY422;
                src->width            = video->GetWidth();
                src->height           = video->GetHeight();
                src->interlaced_frame = format_desc_.field_mode != core::field_mode::progressive;
                src->top_field_first  = format_desc_.field_mode == core::field_mode::upper ? 1 : 0;
                src->key_frame        = 1;

                core::pixel_format_desc desc;
                auto                    frame = frame_factory_->create_frame(this, desc);
                desc.planes.push_back(core::pixel_format_desc::plane(src->width, src->height, 1));
                desc.planes.push_back(core::pixel_format_desc::plane(src->width / 2, src->height, 1));
                desc.planes.push_back(core::pixel_format_desc::plane(src->width / 2, src->height, 1));

                auto dst              = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* ptr) { av_frame_free(&ptr); });
                dst->data[0]          = reinterpret_cast<uint8_t*>(frame.image_data(0).data());
                dst->data[1]          = reinterpret_cast<uint8_t*>(frame.image_data(1).data());
                dst->data[2]          = reinterpret_cast<uint8_t*>(frame.image_data(2).data());
                dst->linesize[0]      = desc.planes[0].linesize;
                dst->linesize[1]      = desc.planes[1].linesize;
                dst->linesize[2]      = desc.planes[2].linesize;
                dst->format           = AV_PIX_FMT_YUV422P;
                dst->width            = format_desc_.width;
                dst->height           = format_desc_.height;
                dst->interlaced_frame = format_desc_.field_mode != core::field_mode::progressive;
                dst->top_field_first  = format_desc_.field_mode == core::field_mode::upper ? 1 : 0;
                dst->key_frame        = 1;

                auto sws = sws_getCachedContext(sws_.get(),
                                                src->width,
                                                src->height,
                                                static_cast<AVPixelFormat>(src->format),
                                                dst->width,
                                                dst->height,
                                                static_cast<AVPixelFormat>(dst->format),
                                                SWS_BILINEAR,
                                                nullptr,
                                                nullptr,
                                                nullptr);

                if (sws != sws_.get()) {
                    sws_.reset(sws, sws_freeContext);
                }

                sws_scale(sws_.get(), src->data, src->linesize, 0, src->height, dst->data, dst->linesize);

                return core::draw_frame(std::move(frame));
            }();

            if (!frame_buffer_.try_push(frame)) {
                auto dummy = core::draw_frame{};
                frame_buffer_.try_pop(dummy);

                frame_buffer_.try_push(frame);

                graph_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
            }

            graph_->set_value("frame-time", frame_timer.elapsed() * format_desc_.fps * 0.5);
            monitor_subject_ << core::monitor::message("/profiler/time") % frame_timer.elapsed() % format_desc_.fps;

            graph_->set_value("output-buffer", static_cast<float>(frame_buffer_.size()) / static_cast<float>(frame_buffer_.capacity()));
            monitor_subject_ << core::monitor::message("/buffer") % frame_buffer_.size() % frame_buffer_.capacity();
        } catch (...) {
            exception_ = std::current_exception();
            return E_FAIL;
        }

        return S_OK;
    }

    core::draw_frame get_frame()
    {
        if (exception_ != nullptr)
            std::rethrow_exception(exception_);

        core::draw_frame frame = last_frame_;

        if (!frame_buffer_.try_pop(frame))
            graph_->set_tag(diagnostics::tag_severity::WARNING, "late-frame");
        else
            last_frame_ = frame;

        graph_->set_value("output-buffer", static_cast<float>(frame_buffer_.size()) / static_cast<float>(frame_buffer_.capacity()));

        return frame;
    }

    std::wstring print() const { return model_name_ + L" [" + boost::lexical_cast<std::wstring>(device_index_) + L"|" + format_desc_.name + L"]"; }

    boost::rational<int> get_out_framerate() const { return format_desc_.framerate; }

    core::monitor::subject& monitor_output() { return monitor_subject_; }
};

class decklink_producer_proxy : public core::frame_producer_base
{
    std::unique_ptr<decklink_producer> producer_;
    const uint32_t                     length_;
    executor                           executor_;

  public:
    explicit decklink_producer_proxy(const core::video_format_desc&              format_desc,
                                     const spl::shared_ptr<core::frame_factory>& frame_factory,
                                     int                                         device_index,
                                     const std::wstring&                         filter_str,
                                     uint32_t                                    length)
        : executor_(L"decklink_producer[" + boost::lexical_cast<std::wstring>(device_index) + L"]")
        , length_(length)
    {
        auto ctx = core::diagnostics::call_context::for_thread();
        executor_.invoke([=] {
            core::diagnostics::call_context::for_thread() = ctx;
            com_initialize();
            producer_.reset(new decklink_producer(format_desc, device_index, frame_factory, filter_str));
        });
    }

    ~decklink_producer_proxy()
    {
        executor_.invoke([=] {
            producer_.reset();
            com_uninitialize();
        });
    }

    core::monitor::subject& monitor_output() { return producer_->monitor_output(); }

    // frame_producer

    core::draw_frame receive_impl() override { return producer_->get_frame(); }

    uint32_t nb_frames() const override { return length_; }

    std::wstring print() const override { return producer_->print(); }

    std::wstring name() const override { return L"decklink"; }

    boost::rational<int> get_out_framerate() const { return producer_->get_out_framerate(); }
};

spl::shared_ptr<core::frame_producer> create_producer(const core::frame_producer_dependencies& dependencies, const std::vector<std::wstring>& params)
{
    if (params.empty() || !boost::iequals(params.at(0), "decklink"))
        return core::frame_producer::empty();

    auto device_index = get_param(L"DEVICE", params, -1);
    if (device_index == -1)
        device_index = boost::lexical_cast<int>(params.at(1));

    auto filter_str = get_param(L"FILTER", params);
    auto length     = get_param(L"LENGTH", params, std::numeric_limits<uint32_t>::max());

    boost::ireplace_all(filter_str, L"DEINTERLACE_BOB", L"YADIF=1:-1");
    boost::ireplace_all(filter_str, L"DEINTERLACE_LQ", L"SEPARATEFIELDS");
    boost::ireplace_all(filter_str, L"DEINTERLACE", L"YADIF=0:-1");

    auto producer = spl::make_shared<decklink_producer_proxy>(dependencies.format_desc, dependencies.frame_factory, device_index, filter_str, length);
    return core::create_destroy_proxy(producer);
}
}} // namespace caspar::decklink
