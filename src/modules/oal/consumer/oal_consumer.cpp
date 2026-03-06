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

/*
 * oal_consumer.cpp - Video-Scheduled Audio Implementation with Auto-Tuning
 * CasparCG System Audio Sync Fix
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
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <numeric>
#include <queue>
#include <thread>
#include <vector>

#include <AL/al.h>
#include <AL/alc.h>

namespace caspar { namespace oal {

// Audio packet for video-scheduled playback
struct audio_packet
{
    int64_t                                        video_frame_number;
    std::chrono::high_resolution_clock::time_point target_time;
    std::vector<int16_t>                           samples;
    int                                            sample_rate;
    int                                            channels;

    bool operator<(const audio_packet& other) const
    {
        return target_time > other.target_time; // For priority queue (earliest first)
    }
};

// Device management class (preserved from original)
class device
{
    ALCdevice*         device_  = nullptr;
    ALCcontext*        context_ = nullptr;
    const std::wstring device_name_;

  public:
    explicit device(std::wstring device_name)
        : device_name_(std::move(device_name))
    {
        ALCchar* deviceName = nullptr;

        if (!device_name_.empty()) {
            ALboolean enumeration = alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT");

            if (enumeration == AL_FALSE) {
                CASPAR_LOG(info) << L"Unable to enumerate OpenAL devices. Using system default device";
            } else {
                char* s;

                if (alcIsExtensionPresent(nullptr, "ALC_enumerate_all_EXT") == AL_FALSE)
                    s = (char*)alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
                else
                    s = (char*)alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);

                deviceName = iterate_and_find_device(s, device_name_);

                if (deviceName == nullptr) {
                    CASPAR_LOG(warning)
                        << L"Failed to find specified OpenAL output device. Using system default device";
                } else {
                    CASPAR_LOG(debug) << L"Found specified OpenAL output device";
                }
            }
        }

        device_ = alcOpenDevice(deviceName);

        if (device_ == nullptr)
            CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("Failed to initialize audio device."));

        context_ = alcCreateContext(device_, nullptr);

        if (context_ == nullptr)
            CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("Failed to create audio context."));

        if (alcMakeContextCurrent(context_) == ALC_FALSE)
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
    static ALCchar* iterate_and_find_device(const char* list, const std::wstring& device_name)
    {
        ALCchar* result = nullptr;

        std::string short_device_name = u8(device_name);
        boost::algorithm::erase_all(short_device_name, " ");

        CASPAR_LOG(info) << L"------- OpenAL Device List -----";

        if (!list) {
            CASPAR_LOG(info) << L"No device names found";
        } else {
            ALCchar* ptr = (ALCchar*)list;

            while (strlen(ptr) > 0) {
                CASPAR_LOG(info) << ptr;

                std::string tmpStr = ptr;
                boost::algorithm::erase_all(tmpStr, " ");

                if (boost::iequals(short_device_name, tmpStr)) {
                    result = ptr;
                }

                ptr += strlen(ptr) + 1;
            }
        }

        CASPAR_LOG(info) << L"------ OpenAL Devices List done -----";
        return result;
    }
};

void init_device()
{
    static std::unique_ptr<device> instance;
    static std::once_flag          f;

    std::call_once(f, [&] {
        std::wstring device_name =
            env::properties().get(L"configuration.system-audio.producer.default-device-name", L"");
        instance = std::make_unique<device>(device_name);
    });
}

// Video-scheduled audio consumer
class oal_consumer : public core::frame_consumer
{
  private:
    // Diagnostics and basic info
    spl::shared_ptr<diagnostics::graph> graph_;
    caspar::timer                       perf_timer_;
    int                                 channel_index_ = -1;
    core::video_format_desc             format_desc_;

    // Video-scheduled audio system
    std::priority_queue<audio_packet> audio_schedule_;
    std::mutex                        schedule_mutex_;
    std::condition_variable           schedule_cv_;
    std::thread                       audio_dispatch_thread_;
    std::atomic<bool>                 stop_audio_dispatch_{false};

    // Timing and synchronization
    std::chrono::high_resolution_clock::time_point channel_start_time_;
    std::atomic<int64_t>                           frame_counter_{0};
    double                                         frame_duration_seconds_ = 0.0;
    std::atomic<int>                               audio_latency_compensation_ms_{40};

    // OpenAL resources
    ALuint               source_ = 0;
    std::vector<ALuint>  stream_buffers_;
    std::atomic<int>     current_buffer_index_{0};
    static constexpr int NUM_STREAM_BUFFERS = 4;

    // Audio processing
    std::shared_ptr<SwrContext> swr_;
    int                         duration_ = 1920;
    bool                        started_  = false;

    // Auto-tuning members
    std::vector<int64_t>                  timing_samples_;
    bool                                  auto_tune_enabled_ = false;
    std::chrono::steady_clock::time_point last_auto_tune_;
    int                                   auto_tune_sample_count_ = 0;

    executor executor_{L"oal_consumer"};

    void perform_auto_tune()
    {
        if (timing_samples_.empty())
            return;

        // Calculate average timing error
        int64_t sum          = std::accumulate(timing_samples_.begin(), timing_samples_.end(), 0LL);
        int64_t avg_error_us = sum / static_cast<int64_t>(timing_samples_.size());
        int     avg_error_ms = static_cast<int>(avg_error_us / 1000);

        // Calculate standard deviation to check consistency
        int64_t variance_sum = 0;
        for (auto sample : timing_samples_) {
            int64_t diff = sample - avg_error_us;
            variance_sum += diff * diff;
        }
        int stddev_ms = static_cast<int>(std::sqrt(variance_sum / timing_samples_.size()) / 1000);

        CASPAR_LOG(info) << L"Auto-tune analysis: Average=" << avg_error_ms << L"ms, StdDev=" << stddev_ms << L"ms";

        // Only adjust if error is significant and timing is consistent
        if (std::abs(avg_error_ms) >= 3 && stddev_ms < 20) {
            int old_compensation = audio_latency_compensation_ms_.load();

            // Adjust by 75% of the measured error (conservative approach)
            int adjustment       = static_cast<int>(avg_error_ms * 0.75);
            int new_compensation = old_compensation - adjustment; // SUBTRACT to fix positive feedback loop;

                                   // Clamp to reasonable range
                                   if (new_compensation < 0) new_compensation = 0;
            if (new_compensation > 1000)
                new_compensation = 1000;

            audio_latency_compensation_ms_.store(new_compensation);

            CASPAR_LOG(info) << L"Auto-tuned latency compensation: " << old_compensation << L"ms | "
                             << new_compensation << L"ms (adjustment: " << adjustment << L"ms)";
        } else if (stddev_ms >= 20) {
            CASPAR_LOG(warning) << L"Auto-tune skipped: timing too inconsistent (StdDev=" << stddev_ms << L"ms)";
        } else {
            CASPAR_LOG(debug) << L"Auto-tune: timing already optimal (" << avg_error_ms << L"ms)";
        }

        // Clear samples for next cycle
        timing_samples_.clear();
    }

    void start_audio_dispatch_thread()
    {
        audio_dispatch_thread_ = std::thread([this] {
            try {
                set_thread_realtime_priority();
                set_thread_name(L"oal-audio-dispatch");

                CASPAR_LOG(info) << L"Audio dispatch thread started";

                while (!stop_audio_dispatch_) {
                    audio_packet packet;

                    // Wait for next scheduled audio packet
                    {
                        std::unique_lock<std::mutex> lock(schedule_mutex_);
                        schedule_cv_.wait(lock, [this] { return !audio_schedule_.empty() || stop_audio_dispatch_; });

                        if (stop_audio_dispatch_)
                            break;

                        packet = audio_schedule_.top();
                        audio_schedule_.pop();
                    }

                    // Wait until the precise moment to dispatch audio
                    auto now = std::chrono::high_resolution_clock::now();
                    if (packet.target_time > now) {
                        std::this_thread::sleep_until(packet.target_time);
                    }

                    // Dispatch audio at the precise moment
                    dispatch_audio_packet(packet);
                }

            } catch (...) {
                CASPAR_LOG_CURRENT_EXCEPTION();
            }
        });
    }

    void dispatch_audio_packet(const audio_packet& packet)
    {
        if (packet.samples.empty())
            return;

        try {
            // CRITICAL: Check source state first and handle properly
            ALenum state;
            alGetSourcei(source_, AL_SOURCE_STATE, &state);
            check_openal_error("get source state");

            // Handle processed buffers BEFORE trying to queue new ones
            ALint processed = 0;
            alGetSourcei(source_, AL_BUFFERS_PROCESSED, &processed);
            check_openal_error("get processed buffers");

            if (processed > 0) {
                std::vector<ALuint> temp_buffers(static_cast<size_t>(processed));
                alSourceUnqueueBuffers(source_, processed, temp_buffers.data());
                check_openal_error("unqueue processed buffers");

                // CRITICAL: Clear any buffer data to avoid AL_INVALID_OPERATION
                for (ALuint buffer : temp_buffers) {
                    alBufferData(buffer, AL_FORMAT_STEREO16, nullptr, 0, 44100);
                }
            }

            // Get queued buffer count to avoid over-queuing
            ALint queued = 0;
            alGetSourcei(source_, AL_BUFFERS_QUEUED, &queued);
            check_openal_error("get queued buffers");

            // Don't queue more than a reasonable number of buffers
            if (queued >= NUM_STREAM_BUFFERS) {
                CASPAR_LOG(warning) << L"Audio buffer queue full (" << queued << L"), skipping frame";
                return;
            }

            // FIXED: Use a simple round-robin for buffer selection
            static std::atomic<int> buffer_selector{0};
            int                     buffer_index = buffer_selector.fetch_add(1) % NUM_STREAM_BUFFERS;
            ALuint                  buffer       = stream_buffers_[static_cast<size_t>(buffer_index)];

            // Validate sample count and ensure it's even for stereo
            size_t sample_count = packet.samples.size();
            if (sample_count == 0) {
                return;
            }

            // Ensure even number of samples for stereo
            if (sample_count % 2 != 0) {
                sample_count--;
            }

            // CRITICAL: Validate buffer before using it
            ALint buffer_size = 0;
            alGetBufferi(buffer, AL_SIZE, &buffer_size);
            if (alGetError() != AL_NO_ERROR) {
                CASPAR_LOG(warning) << L"Invalid buffer detected, skipping audio packet";
                return;
            }

            // Upload audio data with error checking
            alBufferData(buffer,
                         AL_FORMAT_STEREO16,
                         packet.samples.data(),
                         static_cast<ALsizei>(sample_count * sizeof(int16_t)),
                         static_cast<ALsizei>(packet.sample_rate));

            if (alGetError() != AL_NO_ERROR) {
                CASPAR_LOG(warning) << L"Failed to upload audio data, skipping";
                return;
            }

            // Queue the buffer
            alSourceQueueBuffers(source_, 1, &buffer);
            if (alGetError() != AL_NO_ERROR) {
                CASPAR_LOG(warning) << L"Failed to queue audio buffer";
                return;
            }

            // Start playback if stopped (but not if paused)
            if (state == AL_STOPPED || state == AL_INITIAL) {
                alSourcePlay(source_);
                check_openal_error("source play");

                if (!started_) {
                    started_ = true;
                    CASPAR_LOG(info) << L"Started audio playback";
                }
            }

            // Rest of timing/auto-tune logic remains the same...
            if (auto_tune_enabled_) {
                timing_samples_.push_back(std::chrono::duration_cast<std::chrono::microseconds>(
                                              std::chrono::high_resolution_clock::now() - packet.target_time)
                                              .count());
                auto_tune_sample_count_++;

                auto now = std::chrono::steady_clock::now();
                auto time_since_last_tune =
                    std::chrono::duration_cast<std::chrono::seconds>(now - last_auto_tune_).count();

                if (time_since_last_tune >= 20 && timing_samples_.size() >= 500) {
                    perform_auto_tune();
                    last_auto_tune_ = now;
                }
            }

            // Simplified timing logging
            static int timing_log_counter = 0;
            if (++timing_log_counter % 250 == 0) {
                auto latency = std::chrono::duration_cast<std::chrono::microseconds>(
                                   std::chrono::high_resolution_clock::now() - packet.target_time)
                                   .count();
                CASPAR_LOG(debug) << L"Audio timing: " << (latency / 1000) << L"ms";
            }

        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
        }
    }

    void check_openal_error(const std::string& operation)
    {
        ALenum error = alGetError();
        if (error != AL_NO_ERROR) {
            std::string error_str;
            switch (error) {
                case AL_INVALID_NAME:
                    error_str = "AL_INVALID_NAME";
                    break;
                case AL_INVALID_ENUM:
                    error_str = "AL_INVALID_ENUM";
                    break;
                case AL_INVALID_VALUE:
                    error_str = "AL_INVALID_VALUE";
                    break;
                case AL_INVALID_OPERATION:
                    error_str = "AL_INVALID_OPERATION";
                    break;
                case AL_OUT_OF_MEMORY:
                    error_str = "AL_OUT_OF_MEMORY";
                    break;
                default:
                    error_str = "Unknown error " + std::to_string(error);
                    break;
            }
            CASPAR_LOG(warning) << L"OpenAL error in " << operation.c_str() << L": " << error_str.c_str();
        }
    }

    void set_thread_realtime_priority()
    {
#ifdef _WIN32
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#else
        struct sched_param sp;
        sp.sched_priority = 50;
        pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
#endif
    }

    void set_thread_name(const std::wstring& name)
    {
#ifdef _WIN32
        SetThreadDescription(GetCurrentThread(), name.c_str());
#else
        std::string narrow_name = u8(name);
        pthread_setname_np(pthread_self(), narrow_name.c_str());
#endif
    }

  public:
    explicit oal_consumer()
    {
        graph_ = spl::make_shared<diagnostics::graph>();
        init_device();

        graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
        graph_->set_color("audio-latency", diagnostics::color(0.9f, 0.6f, 0.0f));
        graph_->set_color("audio-drift", diagnostics::color(0.6f, 0.3f, 0.3f));
        graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
        graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.3f));
        diagnostics::register_graph(graph_);

        // Get audio latency compensation from config
        audio_latency_compensation_ms_.store(
            static_cast<int>(env::properties().get(L"configuration.system-audio.latency-compensation-ms", 3)));

        // Get auto-tuning setting from config
        auto_tune_enabled_ = env::properties().get(L"configuration.system-audio.auto-tune-latency", false);

        if (auto_tune_enabled_) {
            CASPAR_LOG(info) << L"OAL Consumer: Auto-tuning enabled for latency compensation";
            timing_samples_.reserve(1000); // Pre-allocate for ~20 seconds of samples
            last_auto_tune_ = std::chrono::steady_clock::now();
        }

        CASPAR_LOG(info) << L"OAL Consumer: Using video-scheduled audio with " << audio_latency_compensation_ms_.load()
                         << L"ms latency compensation";
    }

    ~oal_consumer() override
    {
        // Stop audio dispatch thread
        stop_audio_dispatch_ = true;
        schedule_cv_.notify_all();

        if (audio_dispatch_thread_.joinable()) {
            audio_dispatch_thread_.join();
        }

        // Clean up OpenAL resources
        executor_.invoke([=] {
            if (source_ != 0u) {
                alSourceStop(source_);
                alDeleteSources(1, &source_);
            }

            for (auto& buffer : stream_buffers_) {
                if (buffer != 0u)
                    alDeleteBuffers(1, &buffer);
            }
        });
    }

    // Interface for video-scheduled audio (without inheritance)
    void
    schedule_audio_for_frame(int64_t frame_number, const std::vector<int32_t>& samples, int sample_rate, int channels)
    {
        if (!samples.empty()) {
            audio_packet packet;
            packet.video_frame_number = frame_number;
            packet.sample_rate        = sample_rate;
            packet.channels           = channels;

            // Convert int32_t to int16_t with proper scaling and channel handling
            packet.samples.reserve(samples.size());

            if (channels == 1) {
                // Mono to stereo - duplicate each sample
                for (size_t i = 0; i < samples.size(); ++i) {
                    // Better conversion with clamping to avoid distortion
                    int32_t sample_32 = samples[i];
                    // Clamp to valid 16-bit range before conversion
                    if (sample_32 > INT16_MAX * 65536)
                        sample_32 = INT16_MAX * 65536;
                    if (sample_32 < INT16_MIN * 65536)
                        sample_32 = INT16_MIN * 65536;

                    int16_t sample = static_cast<int16_t>(sample_32 >> 16);
                    packet.samples.push_back(sample); // Left
                    packet.samples.push_back(sample); // Right (duplicate)
                }
            } else if (channels >= 2) {
                // Multi-channel to stereo - take first two channels
                for (size_t i = 0; i < samples.size(); i += static_cast<size_t>(channels)) {
                    // Left channel with clamping
                    int32_t left_32 = samples[i];
                    if (left_32 > INT16_MAX * 65536)
                        left_32 = INT16_MAX * 65536;
                    if (left_32 < INT16_MIN * 65536)
                        left_32 = INT16_MIN * 65536;
                    int16_t left = static_cast<int16_t>(left_32 >> 16);
                    packet.samples.push_back(left);

                    // Right channel (or duplicate left if only one channel available)
                    if (i + 1 < samples.size()) {
                        int32_t right_32 = samples[i + 1];
                        if (right_32 > INT16_MAX * 65536)
                            right_32 = INT16_MAX * 65536;
                        if (right_32 < INT16_MIN * 65536)
                            right_32 = INT16_MIN * 65536;
                        int16_t right = static_cast<int16_t>(right_32 >> 16);
                        packet.samples.push_back(right);
                    } else {
                        packet.samples.push_back(left); // Duplicate left if no right channel
                    }
                }
            }

            // Calculate target time
            auto frame_time_offset =
                std::chrono::duration<double>(static_cast<double>(frame_number) * frame_duration_seconds_);
            auto latency_compensation = std::chrono::milliseconds(audio_latency_compensation_ms_.load());
            packet.target_time        = channel_start_time_ +
                                 std::chrono::duration_cast<std::chrono::nanoseconds>(frame_time_offset) -
                                 latency_compensation;

            // Schedule the packet
            {
                std::lock_guard<std::mutex> lock(schedule_mutex_);
                audio_schedule_.push(packet);
            }
            schedule_cv_.notify_one();
        }
    }

    // frame consumer interface
    void initialize(const core::video_format_desc& format_desc,
                    const core::channel_info&      channel_info,
                    int                            port_index) override
    {
        format_desc_            = format_desc;
        channel_index_          = channel_info.index;
        frame_duration_seconds_ = 1.0 / format_desc.fps;
        channel_start_time_     = std::chrono::high_resolution_clock::now();

        graph_->set_text(print());

        executor_.begin_invoke([=] {
            duration_ = *std::min_element(format_desc_.audio_cadence.begin(), format_desc_.audio_cadence.end());

            // IMPROVED: Initialize OpenAL resources with better error handling
            stream_buffers_.resize(NUM_STREAM_BUFFERS);
            alGenBuffers(static_cast<ALsizei>(stream_buffers_.size()), stream_buffers_.data());
            check_openal_error("generate buffers");

            // Pre-initialize all buffers with empty data to avoid AL_INVALID_OPERATION
            for (ALuint buffer : stream_buffers_) {
                alBufferData(buffer, AL_FORMAT_STEREO16, nullptr, 0, format_desc_.audio_sample_rate);
            }
            check_openal_error("initialize empty buffers");

            alGenSources(1, &source_);
            check_openal_error("generate source");

            // Configure source properties
            alSourcei(source_, AL_LOOPING, AL_FALSE);
            alSourcef(source_, AL_PITCH, 1.0f);
            alSourcef(source_, AL_GAIN, 1.0f);
            alSource3f(source_, AL_POSITION, 0.0f, 0.0f, 0.0f);
            alSource3f(source_, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
            check_openal_error("configure source");
        });

        // Start audio dispatch thread
        start_audio_dispatch_thread();
    }

    std::future<bool> send(core::video_field field, core::const_frame frame) override
    {
        auto current_frame = frame_counter_.fetch_add(1) + 1;

        // Calculate when this frame should be presented
        auto target_time =
            channel_start_time_ + std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(
                                      static_cast<double>(current_frame) * frame_duration_seconds_));

        // Sleep until the right time to maintain proper frame rate
        auto now = std::chrono::high_resolution_clock::now();
        if (target_time > now) {
            std::this_thread::sleep_until(target_time);
        }

        executor_.begin_invoke([=] {
            try {
                // Process audio if present using our video-scheduled approach
                if (frame.audio_data() && frame.audio_data().size() > 0) {
                    // Extract audio data directly from the frame
                    std::vector<int32_t> audio_data;
                    auto                 audio_array = frame.audio_data();
                    audio_data.assign(audio_array.begin(), audio_array.end());

                    // Call our scheduling method directly
                    schedule_audio_for_frame(
                        current_frame, audio_data, format_desc_.audio_sample_rate, format_desc_.audio_channels);
                }

                graph_->set_value("tick-time", perf_timer_.elapsed() * format_desc_.fps * 0.5);
                perf_timer_.restart();

            } catch (...) {
                CASPAR_LOG_CURRENT_EXCEPTION();
            }
        });

        return make_ready_future(true);
    }

    std::wstring print() const override
    {
        return L"oal[" + std::to_wstring(channel_index_) + L"|" + format_desc_.name + L"|video-sync]";
    }

    std::wstring name() const override { return L"system-audio-sync"; }

    bool has_synchronization_clock() const override { return true; }

    int index() const override { return 500; }

    core::monitor::state state() const override
    {
        core::monitor::state state;
        state["sync-mode"]     = std::string("video-scheduled");
        state["frame-counter"] = static_cast<int>(frame_counter_.load());
        {
            std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(schedule_mutex_));
            state["queued-packets"] = static_cast<int>(audio_schedule_.size());
        }
        state["latency-compensation-ms"] = audio_latency_compensation_ms_.load();
        return state;
    }
};

// Factory functions
spl::shared_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>&     params,
                                                      const core::video_format_repository& format_repository,
                                                      const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                                                      const core::channel_info& channel_info)
{
    if (params.empty() || (!boost::iequals(params.at(0), L"AUDIO") && !boost::iequals(params.at(0), L"SYSTEM-AUDIO")))
        return core::frame_consumer::empty();

    return spl::make_shared<oal_consumer>();
}

spl::shared_ptr<core::frame_consumer>
create_preconfigured_consumer(const boost::property_tree::wptree&                      ptree,
                              const core::video_format_repository&                     format_repository,
                              const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                              const core::channel_info&                                channel_info)
{
    return spl::make_shared<oal_consumer>();
}

}} // namespace caspar::oal
