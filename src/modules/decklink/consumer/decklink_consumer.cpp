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

#include "common/os/thread.h"
#include "config.h"
#include "decklink_consumer.h"
#include "frame.h"

#include "../decklink.h"
#include "../util/util.h"

#include <core/consumer/frame_consumer.h>
#include <core/diagnostics/call_context.h>
#include <core/frame/frame.h>
#include <core/mixer/audio/audio_mixer.h>
#include <core/video_format.h>

#include <common/diagnostics/graph.h>
#include <common/except.h>
#include <common/executor.h>
#include <common/param.h>
#include <common/ptree.h>
#include <common/timer.h>

#include <tbb/parallel_for.h>
#include <tbb/scalable_allocator.h>

#include <boost/circular_buffer.hpp>

#include <atomic>
#include <common/prec_timer.h>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

namespace caspar { namespace decklink {

template <typename Configuration>
void set_latency(const com_iface_ptr<Configuration>& config,
                 configuration::latency_t            latency,
                 const std::wstring&                 print)
{
    if (latency == configuration::latency_t::low_latency) {
        config->SetFlag(bmdDeckLinkConfigLowLatencyVideoOutput, true);
        CASPAR_LOG(info) << print << L" Enabled low-latency mode.";
    } else if (latency == configuration::latency_t::normal_latency) {
        config->SetFlag(bmdDeckLinkConfigLowLatencyVideoOutput, false);
        CASPAR_LOG(info) << print << L" Disabled low-latency mode.";
    }
}

com_ptr<IDeckLinkDisplayMode> get_display_mode(const com_iface_ptr<IDeckLinkOutput>& device,
                                               core::video_format                    fmt,
                                               BMDPixelFormat                        pix_fmt,
                                               BMDSupportedVideoModeFlags            flag)
{
    auto format = get_decklink_video_format(fmt);

    IDeckLinkDisplayMode*         m = nullptr;
    IDeckLinkDisplayModeIterator* iter;
    if (SUCCEEDED(device->GetDisplayModeIterator(&iter))) {
        auto iterator = wrap_raw<com_ptr>(iter, true);
        while (SUCCEEDED(iterator->Next(&m)) && m != nullptr && m->GetDisplayMode() != format) {
            m->Release();
        }
    }

    if (!m)
        CASPAR_THROW_EXCEPTION(user_error()
                               << msg_info("Device could not find requested video-format: " + std::to_string(format)));

    com_ptr<IDeckLinkDisplayMode> mode = wrap_raw<com_ptr>(m, true);

    BMDDisplayMode actualMode = bmdModeUnknown;
    BOOL           supported  = false;

    if (FAILED(device->DoesSupportVideoMode(
            bmdVideoConnectionUnspecified, mode->GetDisplayMode(), pix_fmt, flag, &actualMode, &supported)))
        CASPAR_THROW_EXCEPTION(caspar_exception()
                               << msg_info(L"Could not determine whether device supports requested video format: " +
                                           get_mode_name(mode)));
    else if (!supported)
        CASPAR_LOG(info) << L"Device may not support video-format: " << get_mode_name(mode);
    else if (actualMode != bmdModeUnknown)
        CASPAR_LOG(warning) << L"Device supports video-format with conversion: " << get_mode_name(mode);

    return mode;
}

void set_duplex(const com_iface_ptr<IDeckLinkAttributes_v10_11>&    attributes,
                const com_iface_ptr<IDeckLinkConfiguration_v10_11>& config,
                configuration::duplex_t                             duplex,
                const std::wstring&                                 print)
{
    BOOL supportsDuplexModeConfiguration;
    if (FAILED(
            attributes->GetFlag(static_cast<BMDDeckLinkAttributeID>(BMDDeckLinkSupportsDuplexModeConfiguration_v10_11),
                                &supportsDuplexModeConfiguration))) {
        CASPAR_LOG(error) << print
                          << L" Failed to set duplex mode, unable to check if card supports duplex mode setting.";
    };

    if (!supportsDuplexModeConfiguration) {
        CASPAR_LOG(warning) << print << L" This device does not support setting the duplex mode.";
        return;
    }

    std::map<configuration::duplex_t, BMDDuplexMode_v10_11> config_map{
        {configuration::duplex_t::full_duplex, bmdDuplexModeFull_v10_11},
        {configuration::duplex_t::half_duplex, bmdDuplexModeHalf_v10_11},
    };
    auto duplex_mode = config_map[duplex];

    if (FAILED(
            config->SetInt(static_cast<BMDDeckLinkConfigurationID>(bmdDeckLinkConfigDuplexMode_v10_11), duplex_mode))) {
        CASPAR_LOG(error) << print << L" Unable to set duplex mode.";
        return;
    }
    CASPAR_LOG(info) << print << L" Duplex mode set.";
}

void set_keyer(const com_iface_ptr<IDeckLinkProfileAttributes>& attributes,
               const com_iface_ptr<IDeckLinkKeyer>&             decklink_keyer,
               configuration::keyer_t                           keyer,
               const std::wstring&                              print)
{
    if (keyer == configuration::keyer_t::internal_keyer) {
        BOOL value = true;
        if (SUCCEEDED(attributes->GetFlag(BMDDeckLinkSupportsInternalKeying, &value)) && !value)
            CASPAR_LOG(error) << print << L" Failed to enable internal keyer.";
        else if (FAILED(decklink_keyer->Enable(FALSE)))
            CASPAR_LOG(error) << print << L" Failed to enable internal keyer.";
        else if (FAILED(decklink_keyer->SetLevel(255)))
            CASPAR_LOG(error) << print << L" Failed to set key-level to max.";
        else
            CASPAR_LOG(info) << print << L" Enabled internal keyer.";
    } else if (keyer == configuration::keyer_t::external_keyer) {
        BOOL value = true;
        if (SUCCEEDED(attributes->GetFlag(BMDDeckLinkSupportsExternalKeying, &value)) && !value)
            CASPAR_LOG(error) << print << L" Failed to enable external keyer.";
        else if (FAILED(decklink_keyer->Enable(TRUE)))
            CASPAR_LOG(error) << print << L" Failed to enable external keyer.";
        else if (FAILED(decklink_keyer->SetLevel(255)))
            CASPAR_LOG(error) << print << L" Failed to set key-level to max.";
        else
            CASPAR_LOG(info) << print << L" Enabled external keyer.";
    }
}

class decklink_frame : public IDeckLinkVideoFrame
{
    core::video_format_desc format_desc_;
    std::shared_ptr<void>   data_;
    std::atomic<int>        ref_count_{0};
    int                     nb_samples_;

  public:
    decklink_frame(std::shared_ptr<void> data, core::video_format_desc format_desc, int nb_samples)
        : format_desc_(std::move(format_desc))
        , data_(std::move(data))
        , nb_samples_(nb_samples)
    {
    }

    // IUnknown

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID*) override { return E_NOINTERFACE; }

    ULONG STDMETHODCALLTYPE AddRef() override { return ++ref_count_; }

    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--ref_count_ == 0) {
            delete this;

            return 0;
        }

        return ref_count_;
    }

    // IDecklinkVideoFrame

    long STDMETHODCALLTYPE           GetWidth() override { return static_cast<long>(format_desc_.width); }
    long STDMETHODCALLTYPE           GetHeight() override { return static_cast<long>(format_desc_.height); }
    long STDMETHODCALLTYPE           GetRowBytes() override { return static_cast<long>(format_desc_.width) * 4; }
    BMDPixelFormat STDMETHODCALLTYPE GetPixelFormat() override { return bmdFormat8BitBGRA; }
    BMDFrameFlags STDMETHODCALLTYPE  GetFlags() override { return bmdFrameFlagDefault; }

    HRESULT STDMETHODCALLTYPE GetBytes(void** buffer) override
    {
        *buffer = data_.get();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetTimecode(BMDTimecodeFormat format, IDeckLinkTimecode** timecode) override
    {
        return S_FALSE;
    }

    HRESULT STDMETHODCALLTYPE GetAncillaryData(IDeckLinkVideoFrameAncillary** ancillary) override { return S_FALSE; }

    [[nodiscard]] int nb_samples() const { return nb_samples_; }
};

struct decklink_base
{
  public:
    [[nodiscard]] virtual std::wstring print() const = 0;

    [[nodiscard]] core::video_format_desc get_decklink_format(const port_configuration&      config,
                                                              const core::video_format_desc& fallback_format_desc) const
    {
        if (config.format.format != core::video_format::invalid &&
            config.format.format != fallback_format_desc.format) {
            if (config.format.format != core::video_format::invalid &&
                config.format.format != core::video_format::custom &&
                config.format.framerate * config.format.field_count ==
                    fallback_format_desc.framerate * fallback_format_desc.field_count &&
                config.format.duration == fallback_format_desc.duration) {
                return config.format;
            } else {
                CASPAR_LOG(warning) << print() << L"Ignoring specified format for decklink";
            }
        }

        if (fallback_format_desc.format == core::video_format::invalid ||
            fallback_format_desc.format == core::video_format::custom)
            CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Decklink does not support the channel format"));

        return fallback_format_desc;
    }
};

struct decklink_secondary_port
    : public IDeckLinkVideoOutputCallback
    , public decklink_base
{
    const configuration                       config_;
    const port_configuration                  output_config_;
    com_ptr<IDeckLink>                        decklink_      = get_device(output_config_.device_index);
    com_iface_ptr<IDeckLinkOutput>            output_        = iface_cast<IDeckLinkOutput>(decklink_);
    com_iface_ptr<IDeckLinkKeyer>             keyer_         = iface_cast<IDeckLinkKeyer>(decklink_, true);
    com_iface_ptr<IDeckLinkProfileAttributes> attributes_    = iface_cast<IDeckLinkProfileAttributes>(decklink_);
    com_iface_ptr<IDeckLinkConfiguration>     configuration_ = iface_cast<IDeckLinkConfiguration>(decklink_);
    int                                       device_sync_group_;
    boost::optional<core::const_frame>        first_field_;

    const std::wstring model_name_ = get_model_name(decklink_);

    // long long video_scheduled_ = 0;

    const core::video_format_desc channel_format_desc_;
    const core::video_format_desc decklink_format_desc_;
    com_ptr<IDeckLinkDisplayMode> mode_ =
        get_display_mode(output_, decklink_format_desc_.format, bmdFormat8BitBGRA, bmdSupportedVideoModeDefault);

    decklink_secondary_port(const configuration&           config,
                            port_configuration             output_config,
                            core::video_format_desc        channel_format_desc,
                            const core::video_format_desc& main_decklink_format_desc,
                            const std::wstring&            print,
                            int                            device_sync_group)
        : config_(config)
        , output_config_(std::move(output_config))
        , device_sync_group_(device_sync_group)
        , channel_format_desc_(std::move(channel_format_desc))
        , decklink_format_desc_(get_decklink_format(output_config_, main_decklink_format_desc))
    {
        if (config.duplex != configuration::duplex_t::default_duplex) {
            set_duplex(iface_cast<IDeckLinkAttributes_v10_11>(decklink_),
                       iface_cast<IDeckLinkConfiguration_v10_11>(decklink_),
                       config.duplex,
                       print);
        }

        set_latency(configuration_, config.latency, print);
        set_keyer(attributes_, keyer_, config.keyer, print);

        if (device_sync_group_ > 0 &&
            FAILED(configuration_->SetInt(bmdDeckLinkConfigPlaybackGroup, device_sync_group_))) {
            CASPAR_LOG(error) << print << L" Failed to enable sync group.";
            device_sync_group_ = 0;
        } else {
            CASPAR_LOG(trace) << print << L" Joined sync group " << device_sync_group;
        }

        if (FAILED(output_->SetScheduledFrameCompletionCallback(this)))
            CASPAR_THROW_EXCEPTION(caspar_exception()
                                   << msg_info(print + L" Failed to set key playback completion callback.")
                                   << boost::errinfo_api_function("SetScheduledFrameCompletionCallback"));
    }

    [[nodiscard]] std::wstring print_single() const
    {
        return model_name_ + L" [" + std::to_wstring(output_config_.device_index) + L"|" + decklink_format_desc_.name +
               L"]";
    }

    [[nodiscard]] std::wstring print() const override { return L"TODO"; }

    template <typename Print>
    void enable_video(const Print& print)
    {
        if (FAILED(output_->EnableVideoOutput(mode_->GetDisplayMode(),
                                              device_sync_group_ > 0 ? bmdVideoOutputSynchronizeToPlaybackGroup
                                                                     : bmdVideoOutputFlagDefault)))
            CASPAR_THROW_EXCEPTION(caspar_exception()
                                   << msg_info(print() + L" Could not enable secondary video output."));
    }

    template <typename Print>
    void start_playback(const Print& print)
    {
        if (device_sync_group_ == 0) {
            if (FAILED(output_->StartScheduledPlayback(0, decklink_format_desc_.time_scale, 1.0))) {
                CASPAR_THROW_EXCEPTION(caspar_exception()
                                       << msg_info(print() + L" Failed to schedule secondary playback."));
            }
        }
    }

    void schedule_frame(core::const_frame frame, BMDTimeValue display_time)
    {
        bool isInterlaced = decklink_format_desc_.field_count != 1;
        if (isInterlaced && !first_field_.is_initialized()) {
            // If this is interlaced it needs a pair of frames at a time
            first_field_ = frame;
            return;
        }

        // Figure out which frame is which
        core::const_frame frame1;
        core::const_frame frame2;
        if (isInterlaced) {
            frame1       = *first_field_;
            first_field_ = boost::none;
            frame2       = frame;
        } else {
            frame1 = frame;
        }

        auto image_data = convert_frame_pair(
            channel_format_desc_, decklink_format_desc_, output_config_, frame1, frame2, mode_->GetFieldDominance());

        schedule_next_video(image_data, 0, display_time);
    }

    void schedule_next_video(std::shared_ptr<void> image_data, int nb_samples, BMDTimeValue display_time)
    {
        auto packed_frame = wrap_raw<com_ptr, IDeckLinkVideoFrame>(
            new decklink_frame(std::move(image_data), decklink_format_desc_, nb_samples));
        if (FAILED(output_->ScheduleVideoFrame(get_raw(packed_frame),
                                               display_time,
                                               decklink_format_desc_.duration,
                                               decklink_format_desc_.time_scale))) {
            CASPAR_LOG(error) << print() << L" Failed to schedule primary video.";
        }

        // video_scheduled_ += decklink_format_desc_.duration;
    }

    ~decklink_secondary_port() override
    {
        if (output_) {
            if (device_sync_group_ == 0) {
                output_->StopScheduledPlayback(0, nullptr, 0);
            }

            output_->DisableVideoOutput();
        }
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID*) override { return E_NOINTERFACE; }
    ULONG STDMETHODCALLTYPE   AddRef() override { return 1; }
    ULONG STDMETHODCALLTYPE   Release() override { return 1; }

    HRESULT STDMETHODCALLTYPE ScheduledPlaybackHasStopped() override { return S_OK; }

    HRESULT STDMETHODCALLTYPE ScheduledFrameCompleted(IDeckLinkVideoFrame*           completed_frame,
                                                      BMDOutputFrameCompletionResult result) override
    {
        // Let the primary callback keep the pace, so no scheduling here.

        return S_OK;
    }
};

struct decklink_consumer
    : public IDeckLinkVideoOutputCallback
    , public decklink_base
{
    const int           channel_index_;
    const configuration config_;

    com_ptr<IDeckLink>                        decklink_      = get_device(config_.primary.device_index);
    com_iface_ptr<IDeckLinkOutput>            output_        = iface_cast<IDeckLinkOutput>(decklink_);
    com_iface_ptr<IDeckLinkConfiguration>     configuration_ = iface_cast<IDeckLinkConfiguration>(decklink_);
    com_iface_ptr<IDeckLinkKeyer>             keyer_         = iface_cast<IDeckLinkKeyer>(decklink_, true);
    com_iface_ptr<IDeckLinkProfileAttributes> attributes_    = iface_cast<IDeckLinkProfileAttributes>(decklink_);

    std::mutex         exception_mutex_;
    std::exception_ptr exception_;

    const std::wstring            model_name_ = get_model_name(decklink_);
    const core::video_format_desc channel_format_desc_;
    const core::video_format_desc decklink_format_desc_;

    std::mutex                    buffer_mutex_;
    std::condition_variable       buffer_cond_;
    std::queue<core::const_frame> buffer_;
    int                           buffer_capacity_ = channel_format_desc_.field_count;

    const int buffer_size_ = config_.buffer_depth(); // Minimum buffer-size 3.

    long long video_scheduled_ = 0;
    long long audio_scheduled_ = 0;

    boost::circular_buffer<std::vector<int32_t>> audio_container_{static_cast<unsigned long>(buffer_size_ + 1)};

    spl::shared_ptr<diagnostics::graph> graph_;
    caspar::timer                       tick_timer_;
    reference_signal_detector           reference_signal_detector_{output_};
    // std::atomic<int64_t>                                  scheduled_frames_completed_{0};
    std::vector<std::unique_ptr<decklink_secondary_port>> secondary_port_contexts_;
    int                                                   device_sync_group_;

    com_ptr<IDeckLinkDisplayMode> mode_ =
        get_display_mode(output_, decklink_format_desc_.format, bmdFormat8BitBGRA, bmdSupportedVideoModeDefault);

    std::atomic<bool> abort_request_{false};

  public:
    decklink_consumer(const configuration& config, core::video_format_desc channel_format_desc, int channel_index)
        : channel_index_(channel_index)
        , config_(config)
        , channel_format_desc_(std::move(channel_format_desc))
        , decklink_format_desc_(get_decklink_format(config.primary, channel_format_desc_))
    {
        graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
        graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.3f));
        graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
        graph_->set_color("flushed-frame", diagnostics::color(0.4f, 0.3f, 0.8f));
        graph_->set_color("buffered-audio", diagnostics::color(0.9f, 0.9f, 0.5f));
        graph_->set_color("buffered-video", diagnostics::color(0.2f, 0.9f, 0.9f));

        if (config.duplex != configuration::duplex_t::default_duplex) {
            set_duplex(iface_cast<IDeckLinkAttributes_v10_11>(decklink_),
                       iface_cast<IDeckLinkConfiguration_v10_11>(decklink_),
                       config.duplex,
                       print());
        }

        /*
        if (key_context_) {
            graph_->set_color("key-offset", diagnostics::color(1.0f, 0.0f, 0.0f));
        }
        */

        graph_->set_text(print());
        diagnostics::register_graph(graph_);

        // If there are additional ports devices, then enable the sync group
        if (!config.secondaries.empty() || config.keyer == configuration::keyer_t::external_separate_device_keyer) {
            // A unique id is needed for this group, this is simpler than a random number
            device_sync_group_ = config.primary.device_index;

            if (FAILED(configuration_->SetInt(bmdDeckLinkConfigPlaybackGroup, device_sync_group_))) {
                device_sync_group_ = 0;
                CASPAR_LOG(error) << print() << L" Failed to enable sync group.";
            } else {
                CASPAR_LOG(debug) << print() << L" Enabled sync group: " << device_sync_group_;
            }
        }

        // create the secondary ports
        for (auto& secondary_port_config : config_.secondaries) {
            secondary_port_contexts_.push_back(std::make_unique<decklink_secondary_port>(config,
                                                                                         secondary_port_config,
                                                                                         channel_format_desc_,
                                                                                         decklink_format_desc_,
                                                                                         print(),
                                                                                         device_sync_group_));
        }
        if (config.keyer == configuration::keyer_t::external_separate_device_keyer) {
            // TODO - need to generate a config with a mangled id
            // child_device_contexts_.push_back(
            // std::make_unique<child_device_context>(config, child_config, print(), device_sync_group_));
        }

        enable_video();

        if (config.embedded_audio) {
            enable_audio();
        }

        set_latency(configuration_, config.latency, print());
        set_keyer(attributes_, keyer_, config.keyer, print());

        if (config.embedded_audio) {
            output_->BeginAudioPreroll();
        }

        for (int n = 0; n < buffer_size_; ++n) {
            auto nb_samples = decklink_format_desc_.audio_cadence[n % decklink_format_desc_.audio_cadence.size()] *
                              decklink_format_desc_.field_count;
            if (config.embedded_audio) {
                schedule_next_audio(std::vector<int32_t>(nb_samples * decklink_format_desc_.audio_channels),
                                    nb_samples);
            }

            std::shared_ptr<void> image_data(scalable_aligned_malloc(decklink_format_desc_.size, 64),
                                             scalable_aligned_free);

            schedule_next_video(image_data, nb_samples, video_scheduled_);
            for (auto& context : secondary_port_contexts_) {
                context->schedule_next_video(image_data, 0, video_scheduled_);
            }

            video_scheduled_ += decklink_format_desc_.duration;
        }

        if (config.embedded_audio) {
            output_->EndAudioPreroll();
        }

        wait_for_reference_lock();

        start_playback();
    }

    ~decklink_consumer() override
    {
        abort_request_ = true;
        buffer_cond_.notify_all();

        if (output_ != nullptr) {
            output_->StopScheduledPlayback(0, nullptr, 0);
            if (config_.embedded_audio) {
                output_->DisableAudioOutput();
            }
            output_->DisableVideoOutput();
        }

        secondary_port_contexts_.clear();
    }

    void wait_for_reference_lock()
    {
        if (config_.wait_for_reference_duration == 0 ||
            config_.wait_for_reference == configuration::wait_for_reference_t::disabled) {
            // Wait disabled
            return;
        }
        if (config_.wait_for_reference == configuration::wait_for_reference_t::automatic && device_sync_group_ == 0) {
            // Wait is not necessary
            return;
        }

        CASPAR_LOG(info) << print() << L" Reference signal: waiting for lock";

        // When using the sync group we need a reference lock before starting playback, otherwise the outputs will not
        // be locked correctly and will be out of sync
        auto wait_end = std::chrono::system_clock::now() + std::chrono::seconds(config_.wait_for_reference_duration);
        while (std::chrono::system_clock::now() < wait_end) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            BMDReferenceStatus reference_status;
            if (output_->GetReferenceStatus(&reference_status) != S_OK) {
                CASPAR_LOG(error) << print() << L" Reference signal: failed while querying status";
                break;
            }

            if (reference_status & bmdReferenceLocked) {
                CASPAR_LOG(info) << print() << L" Reference signal: locked";

                // TODO - is this necessary? This is to give it a chance to stabilise before continuing
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                return;
            }
        }

        CASPAR_LOG(warning) << print() << L" Reference signal: unable to acquire lock";
    }

    void enable_audio()
    {
        if (FAILED(output_->EnableAudioOutput(bmdAudioSampleRate48kHz,
                                              bmdAudioSampleType32bitInteger,
                                              decklink_format_desc_.audio_channels,
                                              bmdAudioOutputStreamTimestamped))) {
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Could not enable audio output."));
        }

        CASPAR_LOG(info) << print() << L" Enabled embedded-audio.";
    }

    void enable_video()
    {
        if (FAILED(output_->EnableVideoOutput(mode_->GetDisplayMode(),
                                              device_sync_group_ > 0 ? bmdVideoOutputSynchronizeToPlaybackGroup
                                                                     : bmdVideoOutputFlagDefault))) {
            CASPAR_THROW_EXCEPTION(caspar_exception()
                                   << msg_info(print() + L" Could not enable primary video output."));
        }

        if (FAILED(output_->SetScheduledFrameCompletionCallback(this))) {
            CASPAR_THROW_EXCEPTION(caspar_exception()
                                   << msg_info(print() + L" Failed to set primary playback completion callback.")
                                   << boost::errinfo_api_function("SetScheduledFrameCompletionCallback"));
        }

        for (auto& context : secondary_port_contexts_) {
            context->enable_video([this]() { return print(); });
        }
    }

    void start_playback()
    {
        if (FAILED(output_->StartScheduledPlayback(0, decklink_format_desc_.time_scale, 1.0))) {
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to schedule primary playback."));
        }

        for (auto& context : secondary_port_contexts_) {
            context->start_playback([this]() { return print(); });
        }
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID*) override { return E_NOINTERFACE; }
    ULONG STDMETHODCALLTYPE   AddRef() override { return 1; }
    ULONG STDMETHODCALLTYPE   Release() override { return 1; }

    HRESULT STDMETHODCALLTYPE ScheduledPlaybackHasStopped() override
    {
        CASPAR_LOG(info) << print() << L" Scheduled playback has stopped.";
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ScheduledFrameCompleted(IDeckLinkVideoFrame*           completed_frame,
                                                      BMDOutputFrameCompletionResult result) override
    {
        thread_local auto priority_set = false;
        if (!priority_set) {
            priority_set = true;
            // set_thread_realtime_priority();
        }
        try {
            auto tick_time = tick_timer_.elapsed() * decklink_format_desc_.hz * 0.5;
            graph_->set_value("tick-time", tick_time);
            tick_timer_.restart();

            reference_signal_detector_.detect_change([this]() { return print(); });

            auto dframe = reinterpret_cast<decklink_frame*>(completed_frame);
            // ++scheduled_frames_completed_;

            /*
            if (key_context_) {
                graph_->set_value(
                    "key-offset",
                    static_cast<double>(scheduled_frames_completed_ - key_context_->scheduled_frames_completed_) * 0.1 +
                        0.5);
            }
            */

            /**
             * TODO - track how the secondaries are doing by comparing IDeckLinkOutput::GetScheduledStreamTime
             */

            if (result == bmdOutputFrameDisplayedLate) {
                graph_->set_tag(diagnostics::tag_severity::WARNING, "late-frame");
                video_scheduled_ += decklink_format_desc_.duration;
                audio_scheduled_ += dframe->nb_samples();
            } else if (result == bmdOutputFrameDropped) {
                graph_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
            } else if (result == bmdOutputFrameFlushed) {
                graph_->set_tag(diagnostics::tag_severity::WARNING, "flushed-frame");
            }

            {
                UINT32 buffered;
                output_->GetBufferedVideoFrameCount(&buffered);
                graph_->set_value("buffered-video", static_cast<double>(buffered) / config_.buffer_depth());

                if (config_.embedded_audio) {
                    output_->GetBufferedAudioSampleFrameCount(&buffered);
                    graph_->set_value("buffered-audio",
                                      static_cast<double>(buffered) /
                                          (decklink_format_desc_.audio_cadence[0] * decklink_format_desc_.field_count *
                                           config_.buffer_depth()));
                }
            }

            core::const_frame frame1 = pop();
            core::const_frame frame2;

            bool isInterlaced = mode_->GetFieldDominance() != bmdProgressiveFrame;
            if (mode_->GetFieldDominance() != bmdProgressiveFrame) {
                // If the main is not progressive, then pop the second frame
                frame2 = pop();
            }

            if (abort_request_)
                return E_FAIL;

            BMDTimeValue video_display_time = video_scheduled_;
            video_scheduled_ += decklink_format_desc_.duration;

            std::vector<std::int32_t> audio_data;
            if (config_.embedded_audio) {
                audio_data.insert(audio_data.end(), frame1.audio_data().begin(), frame1.audio_data().end());
                if (isInterlaced) {
                    audio_data.insert(audio_data.end(), frame2.audio_data().begin(), frame2.audio_data().end());
                }
            }
            // TODO: is this reliable?
            const int nb_samples = static_cast<int>(audio_data.size()) / decklink_format_desc_.audio_channels;

            // Schedule video
            tbb::parallel_for(-1, static_cast<int>(secondary_port_contexts_.size()), [&](int i) {
                if (i == -1) {
                    // Primary port
                    std::shared_ptr<void> image_data = convert_frame_pair(channel_format_desc_,
                                                                          decklink_format_desc_,
                                                                          config_.primary,
                                                                          frame1,
                                                                          frame2,
                                                                          mode_->GetFieldDominance());

                    schedule_next_video(image_data, nb_samples, video_display_time);

                    if (config_.embedded_audio) {
                        schedule_next_audio(std::move(audio_data), nb_samples);
                    }
                } else {
                    // Send frame to secondary ports
                    auto& context = secondary_port_contexts_[i];
                    context->schedule_frame(frame1, video_display_time);
                    if (isInterlaced) {
                        context->schedule_frame(frame2, video_display_time);
                    }

                    if (config_.embedded_audio) {
                        // TODO - audio for secondaries?
                    }
                }
            });

        } catch (...) {
            std::lock_guard<std::mutex> lock(exception_mutex_);
            exception_ = std::current_exception();
            return E_FAIL;
        }

        return S_OK;
    }

    core::const_frame pop()
    {
        core::const_frame frame;
        {
            std::unique_lock<std::mutex> lock(buffer_mutex_);
            buffer_cond_.wait(lock, [&] { return !buffer_.empty() || abort_request_; });
            if (!abort_request_) {
                frame = buffer_.front();
                buffer_.pop();
            }
        }
        buffer_cond_.notify_all();
        return frame;
    }

    void schedule_next_audio(std::vector<std::int32_t> audio, int nb_samples)
    {
        // TODO (refactor) does ScheduleAudioSamples copy data?

        audio_container_.push_back(std::move(audio));

        if (FAILED(output_->ScheduleAudioSamples(audio_container_.back().data(),
                                                 nb_samples,
                                                 audio_scheduled_,
                                                 decklink_format_desc_.audio_sample_rate,
                                                 nullptr))) {
            CASPAR_LOG(error) << print() << L" Failed to schedule audio.";
        }

        // TODO - audio to secondary ports?

        audio_scheduled_ += nb_samples; // TODO - what if there are too many/few samples in this frame?
    }

    void schedule_next_video(std::shared_ptr<void> image_data, int nb_samples, BMDTimeValue display_time)
    {
        auto fill_frame = wrap_raw<com_ptr, IDeckLinkVideoFrame>(
            new decklink_frame(std::move(image_data), decklink_format_desc_, nb_samples));
        if (FAILED(output_->ScheduleVideoFrame(
                get_raw(fill_frame), display_time, decklink_format_desc_.duration, decklink_format_desc_.time_scale))) {
            CASPAR_LOG(error) << print() << L" Failed to schedule primary video.";
        }
    }

    bool send(core::video_field field, core::const_frame frame)
    {
        {
            std::lock_guard<std::mutex> lock(exception_mutex_);
            if (exception_ != nullptr) {
                std::rethrow_exception(exception_);
            }
        }

        if (frame) {
            std::unique_lock<std::mutex> lock(buffer_mutex_);
            if (field != core::video_field::b) {
                // Always push a field2, as we have supplied field1
                buffer_cond_.wait(lock, [&] { return buffer_.size() < buffer_capacity_ || abort_request_; });
            }
            buffer_.push(std::move(frame));
        }
        buffer_cond_.notify_all();

        return !abort_request_;
    }

    [[nodiscard]] std::wstring print() const final
    {
        std::wstringstream buffer;

        buffer << model_name_ << L" [" + std::to_wstring(channel_index_) << L"-"
               << std::to_wstring(config_.primary.device_index) << L"|" << decklink_format_desc_.name << L"]";

        for (auto& context : secondary_port_contexts_) {
            buffer << L" && " + context->print_single();
        }

        return buffer.str();
    }
};

struct decklink_consumer_proxy : public core::frame_consumer
{
    const configuration                config_;
    std::unique_ptr<decklink_consumer> consumer_;
    core::video_format_desc            format_desc_;
    executor                           executor_;

  public:
    explicit decklink_consumer_proxy(const configuration& config)
        : config_(config)
        , executor_(L"decklink_consumer[" + std::to_wstring(config.primary.device_index) + L"]")
    {
        executor_.begin_invoke([=] { com_initialize(); });
    }

    ~decklink_consumer_proxy() override
    {
        executor_.invoke([=] {
            set_thread_realtime_priority();
            consumer_.reset();
            com_uninitialize();
        });
    }

    void initialize(const core::video_format_desc& format_desc, int channel_index) override
    {
        format_desc_ = format_desc;
        executor_.invoke([=] {
            consumer_.reset();
            consumer_ = std::make_unique<decklink_consumer>(config_, format_desc, channel_index);
        });
    }

    std::future<bool> send(core::video_field field, core::const_frame frame) override
    {
        return executor_.begin_invoke([=] { return consumer_->send(field, frame); });
    }

    [[nodiscard]] std::wstring print() const override
    {
        return consumer_ ? consumer_->print() : L"[decklink_consumer]";
    }

    [[nodiscard]] std::wstring name() const override { return L"decklink"; }

    [[nodiscard]] int index() const override { return 300 + config_.primary.device_index; }

    [[nodiscard]] bool has_synchronization_clock() const override { return true; }

    [[nodiscard]] core::monitor::state state() const override
    {
        core::monitor::state state;

        state["decklink/index"]          = config_.primary.device_index;
        state["decklink/embedded-audio"] = config_.embedded_audio;
        state["decklink/key-only"]       = config_.primary.key_only;
        state["decklink/buffer-depth"]   = config_.base_buffer_depth;
        if (config_.primary.format.format == core::video_format::invalid) {
            state["decklink/video-mode"] = format_desc_.name;
        } else {
            state["decklink/video-mode"] = config_.primary.format.name;
        }

        if (config_.keyer == configuration::keyer_t::external_keyer) {
            state["decklink/keyer"] = std::wstring(L"external");
        } else if (config_.keyer == configuration::keyer_t::internal_keyer) {
            state["decklink/keyer"] = std::wstring(L"internal");
        } else if (config_.keyer == configuration::keyer_t::external_separate_device_keyer) {
            state["decklink/keyer"] = std::wstring(L"external_separate_device");
        }

        if (config_.latency == configuration::latency_t::low_latency) {
            state["decklink/latency"] = std::wstring(L"low");
        } else if (config_.latency == configuration::latency_t::normal_latency) {
            state["decklink/latency"] = std::wstring(L"normal");
        }

        if (config_.primary.has_subregion_geometry()) {
            state["decklink/subregion/src-x"]  = config_.primary.src_x;
            state["decklink/subregion/src-y"]  = config_.primary.src_y;
            state["decklink/subregion/src-x"]  = config_.primary.dest_x;
            state["decklink/subregion/dest-y"] = config_.primary.dest_y;
            state["decklink/subregion/width"]  = config_.primary.region_w;
            state["decklink/subregion/height"] = config_.primary.region_h;
        }

        return state;
    }
};

spl::shared_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>&     params,
                                                      const core::video_format_repository& format_repository,
                                                      std::vector<spl::shared_ptr<core::video_channel>> channels)
{
    if (params.empty() || !boost::iequals(params.at(0), L"DECKLINK")) {
        return core::frame_consumer::empty();
    }

    configuration config;

    if (params.size() > 1)
        config.primary.device_index = std::stoi(params.at(1));

    if (contains_param(L"INTERNAL_KEY", params)) {
        config.keyer = configuration::keyer_t::internal_keyer;
    } else if (contains_param(L"EXTERNAL_KEY", params)) {
        config.keyer = configuration::keyer_t::external_keyer;
    } else if (contains_param(L"EXTERNAL_SEPARATE_DEVICE_KEY", params)) {
        config.keyer = configuration::keyer_t::external_separate_device_keyer;
    } else {
        config.keyer = configuration::keyer_t::default_keyer;
    }

    if (contains_param(L"FULL_DUPLEX", params)) {
        config.duplex = configuration::duplex_t::full_duplex;
    } else if (contains_param(L"HALF_DUPLEX", params)) {
        config.duplex = configuration::duplex_t::half_duplex;
    }

    if (contains_param(L"LOW_LATENCY", params)) {
        config.latency = configuration::latency_t::low_latency;
    }

    config.embedded_audio   = contains_param(L"EMBEDDED_AUDIO", params);
    config.primary.key_only = contains_param(L"KEY_ONLY", params);

    return spl::make_shared<decklink_consumer_proxy>(config);
}

spl::shared_ptr<core::frame_consumer>
create_preconfigured_consumer(const boost::property_tree::wptree&               ptree,
                              const core::video_format_repository&              format_repository,
                              std::vector<spl::shared_ptr<core::video_channel>> channels)
{
    configuration config;

    auto keyer = ptree.get(L"keyer", L"default");
    if (keyer == L"external") {
        config.keyer = configuration::keyer_t::external_keyer;
    } else if (keyer == L"internal") {
        config.keyer = configuration::keyer_t::internal_keyer;
    } else if (keyer == L"external_separate_device") {
        config.keyer = configuration::keyer_t::external_separate_device_keyer;
    }

    auto duplex = ptree.get(L"duplex", L"default");
    if (duplex == L"full") {
        config.duplex = configuration::duplex_t::full_duplex;
    } else if (duplex == L"half") {
        config.duplex = configuration::duplex_t::half_duplex;
    }

    auto latency = ptree.get(L"latency", L"default");
    if (latency == L"low") {
        config.latency = configuration::latency_t::low_latency;
    } else if (latency == L"normal") {
        config.latency = configuration::latency_t::normal_latency;
    }

    auto wait_for_reference = ptree.get(L"wait-for-reference", L"auto");
    if (wait_for_reference == L"disable" || wait_for_reference == L"disabled") {
        config.wait_for_reference = configuration::wait_for_reference_t::disabled;
    } else if (wait_for_reference == L"enable" || wait_for_reference == L"enabled") {
        config.wait_for_reference = configuration::wait_for_reference_t::enabled;
    } else {
        config.wait_for_reference = configuration::wait_for_reference_t::automatic;
    }
    config.wait_for_reference_duration = ptree.get(L"wait-for-reference-duration", config.wait_for_reference_duration);

    config.primary = parse_output_config(ptree, format_repository);
    if (config.primary.device_index == -1)
        config.primary.device_index = 1;

    config.embedded_audio    = ptree.get(L"embedded-audio", config.embedded_audio);
    config.base_buffer_depth = ptree.get(L"buffer-depth", config.base_buffer_depth);

    if (ptree.get_child_optional(L"ports")) {
        for (auto& xml_port : ptree | witerate_children(L"ports") | welement_context_iteration) {
            ptree_verify_element_name(xml_port, L"port");

            port_configuration port_config = parse_output_config(xml_port.second, format_repository);

            config.secondaries.push_back(port_config);
        }
    }

    return spl::make_shared<decklink_consumer_proxy>(config);
}

}} // namespace caspar::decklink

/*
##############################################################################
Pre-rolling

Mail: 2011-05-09

Yoshan
BMD Developer Support
developer@blackmagic-design.com

-----------------------------------------------------------------------------

Thanks for your inquiry. The minimum number of frames that you can preroll
for scheduled playback is three frames for video and four frames for audio.
As you mentioned if you preroll less frames then playback will not start or
playback will be very sporadic. From our experience with Media Express, we
recommended that at least seven frames are prerolled for smooth playback.

Regarding the bmdDeckLinkConfigLowLatencyVideoOutput flag:
There can be around 3 frames worth of latency on scheduled output.
When the bmdDeckLinkConfigLowLatencyVideoOutput flag is used this latency is
reduced  or removed for scheduled playback. If the DisplayVideoFrameSync()
method is used, the bmdDeckLinkConfigLowLatencyVideoOutput setting will
guarantee that the provided frame will be output as soon the previous
frame output has been completed.
################################################################################
*/

/*
##############################################################################
Async DMA Transfer without redundant copying

Mail: 2011-05-10

Yoshan
BMD Developer Support
developer@blackmagic-design.com

-----------------------------------------------------------------------------

Thanks for your inquiry. You could try subclassing IDeckLinkMutableVideoFrame
and providing a pointer to your video buffer when GetBytes() is called.
This may help to keep copying to a minimum. Please ensure that the pixel
format is in bmdFormat10BitYUV, otherwise the DeckLink API / driver will
have to colourspace convert which may result in additional copying.
################################################################################
*/
