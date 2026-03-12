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

#include <core/consumer/frame_consumer.h>
#include <core/consumer/channel_info.h>
#include <core/frame/frame.h>
#include <core/video_format.h>

#include <modules/ffmpeg/defines.h>

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

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <queue>
#include <thread>
#include <vector>

#include <AL/al.h>
#include <AL/alc.h>

namespace caspar::oal {

using namespace std::chrono_literals;

using clock = std::chrono::high_resolution_clock;
using msec = std::chrono::milliseconds;

struct audio_packet
{
    clock::time_point time_point;
    std::vector<std::int16_t> samples;
};

class device
{
    ALCdevice*         device_  = nullptr;
    ALCcontext*        context_ = nullptr;
    const std::wstring device_name_;

  public:
    explicit device(std::wstring device_name) : device_name_(std::move(device_name))
    {
        const char* deviceName = nullptr;

        if (!device_name_.empty()) {
            if (alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT")) {

                auto s = alcGetString(nullptr, alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT")
                    ? ALC_ALL_DEVICES_SPECIFIER
                    : ALC_DEVICE_SPECIFIER
                );
                if (s) deviceName = iterate_and_find_device(s, device_name_);

                if (deviceName)
                    CASPAR_LOG(info) << "Using OpenAL device: " << deviceName;
                else
                    CASPAR_LOG(warning) << "Specified OpenAL device not found - using default";
            } else {
                CASPAR_LOG(info) << "Unable to enumerate OpenAL devices - using default";
            }
        } else {
            CASPAR_LOG(info) << "Using default OpenAL device";
        }

        device_ = alcOpenDevice(static_cast<const ALchar*>(deviceName));
        if (!device_) CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("Failed to initialize audio device."));

        context_ = alcCreateContext(device_, nullptr);
        if (!context_) CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("Failed to create audio context."));

        if (!alcMakeContextCurrent(context_))
            CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("Failed to activate audio context."));
    }

    ~device()
    {
        alcMakeContextCurrent(nullptr);

        if (context_ != nullptr)
            alcDestroyContext(context_);

        if (device_ != nullptr)
            alcCloseDevice(device_);
    }

    ALCdevice* get() { return device_; }

  private:
    static const char* iterate_and_find_device(const char* names, const std::wstring& device_name)
    {
        const char* found = nullptr;

        // generate ascii string for comparison purposes vs what openAL provides
        auto name = u8(device_name);
        boost::algorithm::erase_all(name, " ");

        CASPAR_LOG(info) << "-------- OpenAL Devices -------";

        // `names` contains multiple null-terminated device names
        for (auto p = names; *p; p += strlen(p) + 1) {
            std::string tmp_name{p};
            CASPAR_LOG(info) << tmp_name;

            boost::algorithm::erase_all(tmp_name, " ");
            if (boost::iequals(name, tmp_name)) found = p;
        }

        CASPAR_LOG(info) << "-------- OpenAL Devices -------";

        return found;
    }
};

struct oal_consumer : public core::frame_consumer
{
    spl::shared_ptr<diagnostics::graph> graph_;
    caspar::timer                       perf_timer_;
    int                                 channel_index_ = -1;

    core::video_format_desc format_desc_;

    tbb::concurrent_queue<audio_packet> sched_queue_;
    std::thread         sched_thread_;
    std::atomic<bool>   stop_sched_thread_{false};
    int                 sample_rate_; // used inside the thread

    clock::time_point   start_time_;
    std::atomic<std::int64_t> frame_num_{0};
    clock::duration     delay_;

    device              device_;
    ALuint              source_ = 0;
    std::array<ALuint, 8> buffers_;
    std::queue<ALuint>  free_;

    struct swr_deleter { void operator()(SwrContext* p) { swr_free(&p); } };
    std::unique_ptr<SwrContext, swr_deleter> swr_;

    executor executor_{L"oal_consumer"};

    void start_sched_thread()
    {
        sched_thread_ = std::thread([this] {
            set_thread_realtime_priority();
            set_thread_name(L"oal-audio-sched");

            while (!stop_sched_thread_) {
                audio_packet packet;

                if (!sched_queue_.try_pop(packet)) {
                    std::this_thread::sleep_for(100us);
                    continue;
                }

                std::this_thread::sleep_until(packet.time_point);
                play_audio_packet(packet);
            }
        });
    }

    void play_audio_packet(const audio_packet& packet)
    {
        ALint proc = 0;
        alGetSourcei(source_, AL_BUFFERS_PROCESSED, &proc);

        while (proc--) {
            ALuint buf;
            alSourceUnqueueBuffers(source_, 1, &buf);
            free_.push(buf);
        }

        if (free_.empty()) {
            CASPAR_LOG(warning) << "Audio buffer pool exhausted - dropping packet";
            return;
        }

        ALuint buf = free_.front();
        free_.pop();

        alBufferData(buf, AL_FORMAT_STEREO16, packet.samples.data(),
             static_cast<ALsizei>(packet.samples.size() * sizeof(std::int16_t)),
             sample_rate_
        );
        alSourceQueueBuffers(source_, 1, &buf);

        ALenum state = 0;
        alGetSourcei(source_, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING) alSourcePlay(source_);
    }

  public:
    explicit oal_consumer(std::wstring device_name, clock::duration delay) :
        device_{std::move(device_name)}, delay_{delay}
    {
        graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
        graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
        graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.3f));
        diagnostics::register_graph(graph_);

        CASPAR_LOG(info) << print() << ": delay = " << std::chrono::duration_cast<msec>(delay).count() << "ms";
    }

    ~oal_consumer() override
    {
        executor_.invoke([=] {
            stop_sched_thread_ = true;
            if (sched_thread_.joinable()) sched_thread_.join();

            if (source_) {
                alSourceStop(source_);
                alDeleteSources(1, &source_);
                alDeleteBuffers(static_cast<ALsizei>(buffers_.size()), buffers_.data());
            }
        });
    }

    // frame consumer

    void initialize(const core::video_format_desc& format_desc, const core::channel_info& channel_info, int port_index) override
    {
        format_desc_   = format_desc;
        channel_index_ = channel_info.index;
        start_time_    = clock::now();
        sample_rate_   = format_desc.audio_sample_rate;

        graph_->set_text(print());

        executor_.begin_invoke([=] {
            AVChannelLayout from, to;
            av_channel_layout_default(&from, format_desc_.audio_channels);
            av_channel_layout_default(&to, 2); // stereo

            SwrContext* raw;
            swr_alloc_set_opts2(&raw,
                &to,   AV_SAMPLE_FMT_S16, sample_rate_,
                &from, AV_SAMPLE_FMT_S32, sample_rate_,
                0, nullptr
            );
            swr_.reset(raw);
            swr_init(raw);

            alGenBuffers(static_cast<ALsizei>(buffers_.size()), buffers_.data());
            for (auto buf : buffers_) free_.push(buf);

            alGenSources(1, &source_);
        });

        start_sched_thread();
    }

    std::future<bool> send(core::video_field field, core::const_frame frame) override
    {
        auto frame_num = frame_num_++;

        executor_.begin_invoke([=] {
            auto&& samples = frame.audio_data();
            auto num_samples = static_cast<int>(samples.size() / format_desc_.audio_channels);

            audio_packet packet;
            packet.samples.resize(num_samples * 2); // stereo

            auto from = reinterpret_cast<const std::uint8_t*>(samples.data());
            auto to = reinterpret_cast<std::uint8_t*>(packet.samples.data());
            swr_convert(swr_.get(), &to, num_samples, &from, num_samples);

            std::chrono::duration<double> time{frame_num / format_desc_.fps};
            packet.time_point = start_time_ + std::chrono::duration_cast<clock::duration>(time) + delay_;

            sched_queue_.push(std::move(packet));

            graph_->set_value("tick-time", perf_timer_.elapsed() * format_desc_.fps * 0.5);
            perf_timer_.restart();
        });

        return make_ready_future(true);
    }

    std::wstring print() const override
    {
        return L"oal[" + std::to_wstring(channel_index_) + L"|" + format_desc_.name + L"]";
    }

    std::wstring name() const override { return L"system-audio"; }

    bool has_synchronization_clock() const override { return false; }

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

    auto device_name = get_param(L"DEVICE_NAME", params, L"");
    msec delay{ get_param<int>(L"DELAY", params, 0) };
    return spl::make_shared<oal_consumer>(std::move(device_name), delay);
}

spl::shared_ptr<core::frame_consumer>
create_preconfigured_consumer(const boost::property_tree::wptree&                      ptree,
                              const core::video_format_repository&                     format_repository,
                              const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                              const core::channel_info&                                channel_info)
{
    auto device_name = ptree.get(L"device-name", L"");
    msec delay{ ptree.get<int>(L"delay", 0) };
    return spl::make_shared<oal_consumer>(std::move(device_name), delay);
}

} // namespace caspar::oal
