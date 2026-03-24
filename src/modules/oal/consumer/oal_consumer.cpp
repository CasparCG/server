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

#include "oal_consumer.h"

#include <common/diagnostics/graph.h>
#include <common/env.h>
#include <common/except.h>
#include <common/executor.h>
#include <common/future.h>
#include <common/log.h>
#include <common/param.h>
#include <common/timer.h>
#include <common/utf.h>

#include <core/consumer/channel_info.h>
#include <core/consumer/frame_consumer.h>
#include <core/frame/frame.h>
#include <core/video_format.h>

#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/property_tree/ptree.hpp>

#include <tbb/concurrent_queue.h>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
extern "C" {
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include <AL/al.h>
#include <AL/alc.h>

namespace caspar { namespace oal {

using namespace std::chrono_literals;

// -------------------------------------------------------------------------
// Shared OpenAL infrastructure
//
// Two fundamental OpenAL constraints on Windows:
//
//   1. Only one ALCcontext can be current per process at a time.
//      Multiple consumers each calling alcMakeContextCurrent() races
//      and corrupts each other's AL calls.
//
//   2. AL calls must be made from the thread that has the context current.
//      If two executor threads both call alcMakeContextCurrent() the last
//      one wins and the first thread's subsequent AL calls use the wrong
//      context.
//
// Solution: one shared ALCdevice, one ALCcontext, one shared executor thread.
// All AL calls from all consumers funnel through the single executor so the
// context is always current on exactly one thread. Each consumer gets its
// own ALuint source on the shared context -- OpenAL supports multiple
// independent sources on one context.
//
// Both the device and the executor are reference-counted via shared_ptr.
// They are created on the first consumer and destroyed when the last
// consumer is removed.
// -------------------------------------------------------------------------

struct oal_shared
{
    ALCdevice*  device  = nullptr;
    ALCcontext* context = nullptr;
    executor    exec{L"oal_shared"};   // single thread for all AL calls

    explicit oal_shared(const std::wstring& device_name)
    {
        ALCchar* deviceName = nullptr;

        if (!device_name.empty()) {
            if (alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT")) {
                auto s = alcGetString(nullptr,
                    alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT")
                        ? ALC_ALL_DEVICES_SPECIFIER
                        : ALC_DEVICE_SPECIFIER);

                deviceName = find_device(s, device_name);
                if (deviceName)
                    CASPAR_LOG(debug) << L"[oal] Found specified OpenAL output device";
                else
                    CASPAR_LOG(warning) << L"[oal] Specified device not found, using default";
            }
        }

        // All device/context creation and the make_current call happen
        // on the shared executor thread so the context is current there
        // from the very start.
        exec.invoke([&] {
            device = alcOpenDevice(deviceName);
            if (!device)
                CASPAR_THROW_EXCEPTION(invalid_operation()
                    << msg_info("Failed to open OpenAL device."));

            context = alcCreateContext(device, nullptr);
            if (!context)
                CASPAR_THROW_EXCEPTION(invalid_operation()
                    << msg_info("Failed to create OpenAL context."));

            if (!alcMakeContextCurrent(context))
                CASPAR_THROW_EXCEPTION(invalid_operation()
                    << msg_info("Failed to activate OpenAL context."));

            CASPAR_LOG(info) << L"[oal] Shared OpenAL device initialised on dedicated thread";
        });
    }

    ~oal_shared()
    {
        exec.invoke([&] {
            alcMakeContextCurrent(nullptr);
            if (context) alcDestroyContext(context);
            if (device)  alcCloseDevice(device);
            CASPAR_LOG(info) << L"[oal] Shared OpenAL device destroyed";
        });
    }

    oal_shared(const oal_shared&)            = delete;
    oal_shared& operator=(const oal_shared&) = delete;

  private:
    static ALCchar* find_device(const char* list, const std::wstring& name)
    {
        if (!list) return nullptr;

        std::string short_name = u8(name);
        boost::algorithm::erase_all(short_name, " ");

        CASPAR_LOG(info) << L"[oal] Available OpenAL devices:";
        ALCchar* ptr    = (ALCchar*)list;
        ALCchar* result = nullptr;
        while (strlen(ptr) > 0) {
            CASPAR_LOG(info) << L"  " << ptr;
            std::string tmp = ptr;
            boost::algorithm::erase_all(tmp, " ");
            if (boost::iequals(short_name, tmp))
                result = ptr;
            ptr += strlen(ptr) + 1;
        }
        return result;
    }
};

// Process-wide singleton
static std::weak_ptr<oal_shared> g_shared;
static std::mutex                g_shared_mutex;

static std::shared_ptr<oal_shared> get_or_create_shared(const std::wstring& device_name)
{
    std::lock_guard<std::mutex> lock(g_shared_mutex);
    auto shared = g_shared.lock();
    if (!shared) {
        shared   = std::make_shared<oal_shared>(device_name);
        g_shared = shared;
    }
    return shared;
}

// -------------------------------------------------------------------------
// oal_consumer
// -------------------------------------------------------------------------

struct oal_consumer : public core::frame_consumer
{
    spl::shared_ptr<diagnostics::graph> graph_;
    caspar::timer                       perf_timer_;
    int                                 channel_index_ = -1;

    core::video_format_desc format_desc_;

    std::shared_ptr<oal_shared> shared_;
    std::wstring                device_name_;

    ALuint              source_ = 0;
    std::vector<ALuint> buffers_;
    std::queue<ALuint>  free_;

    std::atomic<bool> stop_{false};

    struct swr_deleter { void operator()(SwrContext* p) { swr_free(&p); } };
    std::unique_ptr<SwrContext, swr_deleter> swr_;

  public:
    explicit oal_consumer(std::wstring device_name)
        : device_name_(std::move(device_name))
    {
        graph_->set_color("tick-time",     diagnostics::color(0.0f, 0.6f, 0.9f));
        graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
        graph_->set_color("late-frame",    diagnostics::color(0.6f, 0.3f, 0.3f));
        diagnostics::register_graph(graph_);
    }

    ~oal_consumer() override
    {
        stop_ = true;

        if (shared_) {
            shared_->exec.invoke([this] {
                if (source_) {
                    alSourceStop(source_);
                    alDeleteSources(1, &source_);
                    alDeleteBuffers(static_cast<ALsizei>(buffers_.size()), buffers_.data());
                    source_ = 0;
                }
            });
            // Release our reference. When the last consumer releases,
            // the shared destructor runs and tears down device + context.
            shared_.reset();
        }
    }

    void initialize(const core::video_format_desc& format_desc,
                    const core::channel_info&      channel_info,
                    int                            port_index) override
    {
        format_desc_   = format_desc;
        channel_index_ = channel_info.index;
        graph_->set_text(print());

        shared_ = get_or_create_shared(device_name_);

        // All init happens on the shared executor thread where the
        // OpenAL context is current.
        shared_->exec.invoke([this, format_desc] {
            // Resampler
            AVChannelLayout from, to;
            av_channel_layout_default(&from, format_desc.audio_channels);
            av_channel_layout_default(&to, 2);

            SwrContext* raw;
            swr_alloc_set_opts2(&raw,
                &to,   AV_SAMPLE_FMT_S16, format_desc.audio_sample_rate,
                &from, AV_SAMPLE_FMT_S32, format_desc.audio_sample_rate,
                0, nullptr
            );
            swr_.reset(raw);
            swr_init(raw);

            // Buffers and source
            buffers_.resize(4);
            alGenBuffers(static_cast<ALsizei>(buffers_.size()), buffers_.data());
            for (auto buf : buffers_) free_.push(buf);

            alGenSources(1, &source_);

            // Pre-queue one frame of silence
            std::vector<int16_t> silence(format_desc.audio_cadence[0] * 2, 0);
            auto buf = free_.front();
            free_.pop();
            alBufferData(buf, AL_FORMAT_STEREO16, silence.data(),
                static_cast<ALsizei>(silence.size() * sizeof(std::int16_t)),
                format_desc.audio_sample_rate
            );
            alSourceQueueBuffers(source_, 1, &buf);
        });
    }

    std::future<bool> send(core::video_field field, core::const_frame frame) override
    {
        // Deferred future runs on the output thread when output.cpp calls
        // .get() -- after all consumers' send() calls have fired, so screen
        // consumers are already doing GPU work concurrently.
        //
        // All AL calls inside are dispatched to the shared executor via
        // invoke(), guaranteeing they run on the single thread that has
        // the OpenAL context current. No context switching, no races.
        return std::async(std::launch::deferred, [this, frame = std::move(frame)] {

            return shared_->exec.invoke([this, &frame]() -> bool {

                // Reclaim buffers OpenAL has finished playing
                {
                    ALint done = 0;
                    alGetSourcei(source_, AL_BUFFERS_PROCESSED, &done);
                    while (done-- > 0) {
                        ALuint buf;
                        alSourceUnqueueBuffers(source_, 1, &buf);
                        free_.push(buf);
                    }
                }

                // Resample to stereo int16
                auto&&    audio_in    = frame.audio_data();
                auto      num_samples = static_cast<int>(audio_in.size() / format_desc_.audio_channels);
                std::vector<std::int16_t> audio_out(num_samples * 2);
                auto from_ptr = reinterpret_cast<const std::uint8_t*>(audio_in.data());
                auto to_ptr   = reinterpret_cast<std::uint8_t*>(audio_out.data());
                swr_convert(swr_.get(), &to_ptr, num_samples, &from_ptr, num_samples);

                // Wait for a free buffer
                while (free_.empty() && !stop_) {
                    std::this_thread::sleep_for(100us);
                    ALint done = 0;
                    alGetSourcei(source_, AL_BUFFERS_PROCESSED, &done);
                    while (done-- > 0) {
                        ALuint buf;
                        alSourceUnqueueBuffers(source_, 1, &buf);
                        free_.push(buf);
                    }
                }
                if (stop_) return false;

                // Queue the buffer
                auto buf = free_.front();
                free_.pop();
                alBufferData(buf, AL_FORMAT_STEREO16, audio_out.data(),
                    static_cast<ALsizei>(audio_out.size() * sizeof(std::int16_t)),
                    format_desc_.audio_sample_rate
                );
                alSourceQueueBuffers(source_, 1, &buf);

                ALenum state = 0;
                alGetSourcei(source_, AL_SOURCE_STATE, &state);
                if (state != AL_PLAYING) alSourcePlay(source_);

                // Gate channel tick to OpenAL playback position.
                // First frame returns immediately to let playback start.
                ALint queued = 0;
                alGetSourcei(source_, AL_BUFFERS_QUEUED, &queued);
                if (queued <= 1) {
                    graph_->set_value("tick-time", perf_timer_.elapsed() * format_desc_.fps * 0.5);
                    perf_timer_.restart();
                    return true;
                }

                while (!stop_) {
                    ALint done = 0;
                    alGetSourcei(source_, AL_BUFFERS_PROCESSED, &done);
                    if (done > 0) {
                        graph_->set_value("tick-time", perf_timer_.elapsed() * format_desc_.fps * 0.5);
                        perf_timer_.restart();
                        return true;
                    }
                    std::this_thread::sleep_for(100us);
                }
                return false;
            });
        });
    }

    std::wstring print() const override
    {
        return L"oal[" + std::to_wstring(channel_index_) + L"|" + format_desc_.name + L"]";
    }

    std::wstring name() const override { return L"system-audio"; }

    bool has_synchronization_clock() const override { return true; }

    int index() const override { return 500; }

    core::monitor::state state() const override
    {
        static const core::monitor::state empty;
        return empty;
    }
};

spl::shared_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>&     params,
                                                      const core::video_format_repository& format_repository,
                                                      const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                                                      const core::channel_info& channel_info)
{
    if (params.empty() || !(boost::iequals(params.at(0), L"AUDIO") || boost::iequals(params.at(0), L"SYSTEM-AUDIO")))
        return core::frame_consumer::empty();

    return spl::make_shared<oal_consumer>(get_param(L"DEVICE_NAME", params, L""));
}

spl::shared_ptr<core::frame_consumer>
create_preconfigured_consumer(const boost::property_tree::wptree&                      ptree,
                              const core::video_format_repository&                     format_repository,
                              const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                              const core::channel_info&                                channel_info)
{
    return spl::make_shared<oal_consumer>(ptree.get(L"device-name", L""));
}

}} // namespace caspar::oal
